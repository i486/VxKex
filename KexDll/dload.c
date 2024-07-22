///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dload.c
//
// Abstract:
//
//     Implements the LdrResolveDelayLoadedAPI function, which is found on
//     Windows 8 and above. This function is required by many Windows 8 DLLs.
//
// Author:
//
//     vxiiduu (14-Feb-2024)
//
// Environment:
//
//     At any time when other DLLs are present.
//
// Revision History:
//
//     vxiiduu              14-Feb-2024   Initial creation.
//     vxiiduu              23-Feb-2024   Rework DLL load part of the function.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This function is essentially a combination of LoadLibrary and GetProcAddress.
// Its job is to resolve a singular DLL API function and write it into a
// particular location.
//
// If we fail to do so, and there are failure hooks provided, we need to call
// those in an attempt to remedy the missing API functions. (But in practice
// those failure hooks are either not provided or don't do anything useful.)
//
PVOID NTAPI KexLdrResolveDelayLoadedAPI(
	IN	PVOID								ParentModuleBase,
	IN	PCIMAGE_DELAYLOAD_DESCRIPTOR		DelayloadDescriptor,
	IN	PDELAYLOAD_FAILURE_DLL_CALLBACK		FailureDllHook OPTIONAL,
	IN	PDELAYLOAD_FAILURE_SYSTEM_ROUTINE	FailureSystemHook OPTIONAL,
	OUT	PIMAGE_THUNK_DATA					ThunkAddress,
	IN	ULONG								Flags)
{
	NTSTATUS Status;
	PPVOID DelayLoadedDllHandle;
	PIMAGE_THUNK_DATA ImportAddressTable;
	PIMAGE_THUNK_DATA ImportNameTable;
	PCSTR NameOfDllToLoad;
	PCSTR NameOfDelayLoadedAPI;
	USHORT OrdinalOfDelayLoadedAPI;
	PVOID ProcedureAddress;
	ULONG Index;

	ProcedureAddress = NULL;

	//
	// Validate parameters.
	//

	ASSERT (ParentModuleBase != NULL);
	ASSERT (DelayloadDescriptor != NULL);
	ASSERT (ThunkAddress != NULL);

	if (Flags) {
		// The Flags parameter is reserved.
		return NULL;
	}

	if (DelayloadDescriptor->Attributes.RvaBased == 0) {
		// This function does not handle v1 delay load descriptors.
		return NULL;
	}

	//
	// Decode some RVAs.
	//

	NameOfDllToLoad = (PCSTR) RVA_TO_VA(ParentModuleBase, DelayloadDescriptor->DllNameRVA);
	DelayLoadedDllHandle = (PPVOID) RVA_TO_VA(ParentModuleBase, DelayloadDescriptor->ModuleHandleRVA);
	ImportAddressTable = (PIMAGE_THUNK_DATA) RVA_TO_VA(ParentModuleBase, DelayloadDescriptor->ImportAddressTableRVA);
	ImportNameTable = (PIMAGE_THUNK_DATA) RVA_TO_VA(ParentModuleBase, DelayloadDescriptor->ImportNameTableRVA);
	Index = (ULONG) (ThunkAddress - ImportAddressTable);

	if (IMAGE_SNAP_BY_ORDINAL(ImportNameTable[Index].u1.Ordinal)) {
		NameOfDelayLoadedAPI = NULL;
		OrdinalOfDelayLoadedAPI = (USHORT) ImportNameTable[Index].u1.Ordinal;
	} else {
		PIMAGE_IMPORT_BY_NAME ImportByName;

		ImportByName = (PIMAGE_IMPORT_BY_NAME) RVA_TO_VA(ParentModuleBase, ImportNameTable[Index].u1.AddressOfData);
		NameOfDelayLoadedAPI = (PCSTR) ImportByName->Name;
		OrdinalOfDelayLoadedAPI = 0;
	}

	if (*DelayLoadedDllHandle == NULL) {
		ANSI_STRING NameOfDllToLoadAS;
		UNICODE_STRING NameOfDllToLoadUS;
		UNICODE_STRING RewrittenDllName;

		//
		// DLL not loaded, we need to load it.
		//

		RtlInitEmptyUnicodeStringFromTeb(&NameOfDllToLoadUS);

		Status = RtlInitAnsiStringEx(&NameOfDllToLoadAS, NameOfDllToLoad);
		ASSERT (NT_SUCCESS(Status));
		
		Status = RtlAnsiStringToUnicodeString(&NameOfDllToLoadUS, &NameOfDllToLoadAS, FALSE);
		ASSERT (NT_SUCCESS(Status));

		//
		// Rewrite DLL name.
		//

		RewrittenDllName = NameOfDllToLoadUS;

		Status = KexRewriteDllPath(
			&NameOfDllToLoadUS,
			&RewrittenDllName);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

		if (NT_SUCCESS(Status)) {
			NameOfDllToLoadUS = RewrittenDllName;

			//
			// The DLL name was rewritten.
			// Allocate a temporary stack buffer, and convert the DLL name back
			// to ANSI and put that in the global variable NameOfDllToLoad for the
			// rest of this function to use.
			//

			NameOfDllToLoadAS.Length = RtlUnicodeStringToAnsiSize(&NameOfDllToLoadUS);
			NameOfDllToLoadAS.MaximumLength = NameOfDllToLoadAS.Length;
			NameOfDllToLoadAS.Buffer = StackAlloc(CHAR, NameOfDllToLoadAS.MaximumLength);

			Status = RtlUnicodeStringToAnsiString(&NameOfDllToLoadAS, &NameOfDllToLoadUS, FALSE);
			ASSERT (NT_SUCCESS(Status));

			NameOfDllToLoad = NameOfDllToLoadAS.Buffer;
		}

		Status = LdrLoadDll(
			NULL,
			NULL,
			&NameOfDllToLoadUS,
			DelayLoadedDllHandle);

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"The DLL %wZ could not be delay-loaded.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				&NameOfDllToLoadUS,
				KexRtlNtStatusToString(Status), Status);

			goto Failure;
		}
	}

	ASSERT (*DelayLoadedDllHandle != NULL);

	//
	// Now that the DLL is loaded, we need to find the procedure address that
	// we're looking for.
	//

	if (NameOfDelayLoadedAPI) {
		ANSI_STRING NameOfDelayLoadedAPIAS;

		//
		// Import by procedure name.
		//

		Status = RtlInitAnsiStringEx(&NameOfDelayLoadedAPIAS, NameOfDelayLoadedAPI);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			goto Failure;
		}

		Status = LdrGetProcedureAddress(
			*DelayLoadedDllHandle,
			&NameOfDelayLoadedAPIAS,
			0,
			&ProcedureAddress);
	} else {
		//
		// Import by procedure ordinal number.
		//

		Status = LdrGetProcedureAddress(
			*DelayLoadedDllHandle,
			NULL,
			OrdinalOfDelayLoadedAPI,
			&ProcedureAddress);
	}

	if (!NT_SUCCESS(Status)) {
		if (NameOfDelayLoadedAPI) {
			KexLogErrorEvent(
				L"The procedure %hs was not found in %hs.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				NameOfDelayLoadedAPI,
				NameOfDllToLoad,
				KexRtlNtStatusToString(Status), Status);
		} else {
			KexLogErrorEvent(
				L"The ordinal #%hu was not found in %hs.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				OrdinalOfDelayLoadedAPI,
				NameOfDllToLoad,
				KexRtlNtStatusToString(Status), Status);
		}

		goto Failure;
	}

Failure:
	if (!ProcedureAddress) {
		if (FailureDllHook) {
			DELAYLOAD_INFO DelayLoadInfo;

			//
			// The application that has delay loaded some APIs has an opportunity
			// to recover from the situation where he tries to call some delay loaded
			// API but it can't be found.
			//
			// In most applications, the pointer to this hook is named
			// _pfnDefaultDliFailureHook2 and is provided by the C runtime, but its value
			// is often NULL because devs don't care about providing a fallback when the
			// precious API they depend on isn't available.
			//

			RtlZeroMemory(&DelayLoadInfo, sizeof(DelayLoadInfo));

			DelayLoadInfo.Size					= sizeof(DelayLoadInfo);
			DelayLoadInfo.DelayloadDescriptor	= DelayloadDescriptor;
			DelayLoadInfo.ThunkAddress			= ThunkAddress;
			DelayLoadInfo.TargetDllName			= NameOfDllToLoad;
			DelayLoadInfo.TargetModuleBase		= *DelayLoadedDllHandle;
			DelayLoadInfo.LastError				= RtlNtStatusToDosErrorNoTeb(Status);

			if (NameOfDelayLoadedAPI) {
				DelayLoadInfo.TargetApiDescriptor.ImportDescribedByName	= TRUE;
				DelayLoadInfo.TargetApiDescriptor.Description.Name		= NameOfDelayLoadedAPI;
			} else {
				DelayLoadInfo.TargetApiDescriptor.ImportDescribedByName	= FALSE;
				DelayLoadInfo.TargetApiDescriptor.Description.Ordinal	= OrdinalOfDelayLoadedAPI;
			}

			KexLogDebugEvent(L"Calling failure DLL hook");

			ProcedureAddress = FailureDllHook(
				DELAYLOAD_GPA_FAILURE,
				&DelayLoadInfo);
		}

		if (!ProcedureAddress) {
			//
			// If there was no application-provided failure hook, or the failure hook
			// itself failed to solve the problem, there is a system failure hook pointer
			// provided as well.
			//
			// Most likely, this is a pointer to the function DelayLoadFailureHook located
			// inside kernel32.dll. This function is available on Windows 2000 and above,
			// despite the Microsoft documentation saying it is only available on Windows 8
			// and above. (On Windows 8 it is physically located inside kernelbase instead.)
			//
			// The Windows 7 version of DelayLoadFailureHook at least seems quite similar
			// to the version from the leaked Windows XP source code, so have a look at that
			// if you want to know what it does. (I haven't bothered.)
			//

			KexLogDebugEvent(L"Calling failure system hook");

			if (FailureSystemHook) {
				ProcedureAddress = FailureSystemHook(
					NameOfDllToLoad,
					NameOfDelayLoadedAPI);
			}
		}
	}

	if (ProcedureAddress) {
		ImportAddressTable[Index].u1.Function = (ULONG_PTR) ProcedureAddress;

		if (NameOfDelayLoadedAPI) {
			KexLogDebugEvent(
				L"The procedure %hs was successfully delay-loaded from %hs",
				NameOfDelayLoadedAPI,
				NameOfDllToLoad);
		} else {
			KexLogDebugEvent(
				L"The ordinal #%hu was successfully delay-loaded from %hs",
				OrdinalOfDelayLoadedAPI,
				NameOfDllToLoad);
		}
	}

	return ProcedureAddress;
}

//
// This is the delay-load helper function used by KexDll itself.
//

EXTERN PVOID __ImageBase;

STATIC PVOID WINAPI __delayLoadHelper2(
	IN	PCIMAGE_DELAYLOAD_DESCRIPTOR	DelayloadDescriptor,
	OUT	PIMAGE_THUNK_DATA				ThunkData)
{
	return KexLdrResolveDelayLoadedAPI(
		&__ImageBase,
		DelayloadDescriptor,
		NULL,
		NULL,
		ThunkData,
		0);
}