///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     module.c
//
// Abstract:
//
//     Thjs file contains functions related to DLL loading.
//     The main purpose of the files in here is to rewrite the names of DLLs
//     which the application requests to dynamically load.
//
// Author:
//
//     Author (10-Feb-2024)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              10-Feb-2024    Initial creation.
//     vxiiduu              02-Mar-2024    Fix GetModuleHandleExW logging when
//										   GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
//										   flag is passed.
//     vxiiduu              13-Mar-2024    Move most of the code here to kexldr.
//     vxiiduu              13-May-2025    Make Chromium compat code respect the
//                                         IfeoParameters->DisableAppSpecific value.
//     vxiiduu              02-May-2026    The KexLdrShouldRewriteDll flag is now
//                                         cleared inside KexLdrLoadDll. Remove code
//                                         from KxBase which clears the flag.
//     vxiiduu              02-May-2026    Move Ext_GetProcAddress to here.
//                                         Hide VirtualAlloc2 from apps by default.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>
#include <Shlwapi.h>

//
// The TebExtension->KexLdrShouldRewriteDll flag tells KexDll that an Ext_* module
// function was called. It causes KexDll to rewrite the DLL names which the Ext_*
// module functions have received.
//
// KexDll functions are responsible for clearing the flag.
//

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return GetModuleHandleA(ModuleName);
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName)
{
	//
	// APPSPECIFICHACK: Chromium-based software uses a bootleg knockoff version of
	// GetProcAddress that fails miserably and crashes the whole app when we rewrite
	// NTDLL, because their shitty implementation doesn't work properly with
	// the export forwarders in KxNt. Neither does it properly work with stubs,
	// because they actually scan the instruction code of system calls.
	//

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM) &&
			ModuleName != NULL &&
			StringEqual(ModuleName, L"ntdll.dll")) {

			KexLogDebugEvent(L"Not rewriting NTDLL for Chromium compatibility");
			return (HMODULE) KexData->SystemDllBase;
		}
	}

	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return GetModuleHandleW(ModuleName);
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return GetModuleHandleExA(Flags, ModuleName, ModuleHandleOut);
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return GetModuleHandleExW(Flags, ModuleName, ModuleHandleOut);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryA(
	IN	PCSTR	FileName)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return LoadLibraryA(FileName);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return LoadLibraryW(FileName);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return LoadLibraryExA(FileName, FileHandle, Flags);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	KexCurrentTebExtension()->KexLdrShouldRewriteDll = TRUE;
	return LoadLibraryExW(FileName, FileHandle, Flags);
}

// Note: A stubbed or extended GetProcAddress is required for Themida to function.
// Do not remove this.
KXBASEAPI FARPROC WINAPI Ext_GetProcAddress(
	IN	HMODULE	ModuleHandle,
	IN	PCSTR	ProcedureName)
{
	//
	// Hide VirtualAlloc2 from code which uses dynamic linking.
	// Most apps which want VirtualAlloc2 want to use the placeholder API
	// and if they find both VirtualAlloc and MapViewOfFile3 then they will
	// attempt to use placeholders which are not supported in VxKex.
	//
	// Examples of apps that do this: Chromium, .NET runtime.
	//

	if ((ULONG_PTR) ProcedureName > 0xFFFF &&
		StringEqualIA(ProcedureName, "VirtualAlloc2")) {

		KexLogInformationEvent(L"VirtualAlloc2 hidden from application");
		return NULL;
	}

	return GetProcAddress(ModuleHandle, ProcedureName);
}

KXBASEAPI HMODULE WINAPI LoadPackagedLibrary(
	IN	PCWSTR	LibFileName,
	IN	ULONG	Reserved)
{
	RTL_PATH_TYPE PathType;
	ULONG Index;

	if (Reserved) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	PathType = RtlDetermineDosPathNameType_U(LibFileName);

	if (PathType != RtlPathTypeRelative) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	for (Index = 0; LibFileName[Index] != '\0'; ++Index) {
		if (LibFileName[Index] == '.' && LibFileName[Index+1] == '.' &&
			(LibFileName[Index+2] == '\\' || LibFileName[Index+2] == '/')) {

			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return NULL;
		}

		do {
			++Index;
		} until (LibFileName[Index] == '\0' ||
				 LibFileName[Index] == '\\' ||
				 LibFileName[Index] == '/');
	}

	// On Windows 8 this would be the point where this function would call
	// LoadLibraryExW with the undocumented flag 0x04. However, this flag and
	// its underlying implementation inside LdrLoadDll is not present on Windows
	// 7, so we will just return an error straight away (as documented).
	RtlSetLastWin32Error(APPMODEL_ERROR_NO_PACKAGE);
	return NULL;
}

STATIC NTSTATUS BasepGetDllDirectoryProcedure(
	IN		PCSTR	ProcedureName,
	IN OUT	PPVOID	ProcedureAddress)
{
	NTSTATUS Status;

	Status = STATUS_SUCCESS;

	ASSERT (ProcedureName != NULL);
	ASSERT (ProcedureAddress != NULL);

	if (!*ProcedureAddress) {
		ANSI_STRING ProcedureNameAS;

		Status = RtlInitAnsiStringEx(&ProcedureNameAS, ProcedureName);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		ASSUME (KexData->BaseDllBase != NULL);

		Status = LdrGetProcedureAddress(
			KexData->BaseDllBase,
			&ProcedureNameAS,
			0,
			ProcedureAddress);

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"%hs is not available on this computer\r\n\r\n"
				L"This function is only available on Windows 7 with the KB2533623 "
				L"security update.", ProcedureName);

			BaseSetLastNTError(Status);
		}
	}

	if (NT_SUCCESS(Status)) {
		ASSUME (*ProcedureAddress != NULL);
	}

	return Status;
}

KXBASEAPI DLL_DIRECTORY_COOKIE WINAPI Ext_AddDllDirectory(
	IN	PCWSTR	NewDirectory)
{
	STATIC DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory) (PCWSTR) = NULL;

	BasepGetDllDirectoryProcedure("AddDllDirectory", (PPVOID) &AddDllDirectory);

	if (AddDllDirectory) {
		return AddDllDirectory(NewDirectory);
	} else {
		return NULL;
	}
}

KXBASEAPI BOOL WINAPI Ext_RemoveDllDirectory(
	IN	DLL_DIRECTORY_COOKIE	Cookie)
{
	STATIC BOOL (WINAPI *RemoveDllDirectory) (DLL_DIRECTORY_COOKIE) = NULL;

	BasepGetDllDirectoryProcedure("RemoveDllDirectory", (PPVOID) &RemoveDllDirectory);

	if (RemoveDllDirectory) {
		return RemoveDllDirectory(Cookie);
	} else {
		return FALSE;
	}
}

KXBASEAPI BOOL WINAPI Ext_SetDefaultDllDirectories(
	IN	ULONG	DirectoryFlags)
{
	STATIC BOOL (WINAPI *SetDefaultDllDirectories) (ULONG) = NULL;

	BasepGetDllDirectoryProcedure("SetDefaultDllDirectories", (PPVOID) &SetDefaultDllDirectories);

	if (SetDefaultDllDirectories) {
		DirectoryFlags |= LOAD_LIBRARY_SEARCH_USER_DIRS;
		return SetDefaultDllDirectories(DirectoryFlags);
	} else {
		return FALSE;
	}
}