///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     advlog.c
//
// Abstract:
//
//     Contains support for advanced logging.
//     Advanced logging allows DbgPrint and related messages to be shown in
//     the VxKex log without having a debugger attached.
//
// Author:
//
//     vxiiduu (06-Nov-2022)
//
// Revision History:
//
//     vxiiduu              06-Nov-2022  Initial creation.
//     vxiiduu              07-Nov-2022  Add special parsing for loader.
//     vxiiduu              10-Nov-2022  Change search range to 64 bytes.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC NTSTATUS FindAddressOfDbgPrintInternal(
	OUT	PPVOID	DbgPrintInternalAddress);

STATIC NTSTATUS NTAPI KexpvDbgPrintExWithPrefixInternalHook(
	IN	PSTR	Prefix,
	IN	ULONG	ComponentId,
	IN	ULONG	Level,
	IN	PSTR	Format,
	IN	ARGLIST	ArgList,
	IN	BOOLEAN	BreakPoint);

NTSTATUS KexInitializeAdvancedLogging(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PVOID vDbgPrintExWithPrefixInternal;
	PPEB Peb;

	Peb = NtCurrentPeb();

	//
	// Advanced logging is enabled with NtGlobalFlag & FLG_SHOW_LDR_SNAPS.
	//

	unless (Peb->NtGlobalFlag & FLG_SHOW_LDR_SNAPS) {
		KexLogDebugEvent(L"Not enabling advanced logging due to user preferences.");
		return STATUS_USER_DISABLED;
	}

	Status = FindAddressOfDbgPrintInternal(&vDbgPrintExWithPrefixInternal);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = KexHkInstallBasicHook(
		vDbgPrintExWithPrefixInternal, 
		KexpvDbgPrintExWithPrefixInternalHook, 
		NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	KexLogInformationEvent(L"Successfully initialized advanced logging.");
	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

STATIC NTSTATUS FindAddressOfDbgPrintInternal(
	OUT	PPVOID	DbgPrintInternalAddress) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING NtdllName;
	ANSI_STRING ProcedureName;
	PVOID NtdllBase;
	PVOID vDbgPrintExWithPrefix;
	PVOID vDbgPrintExWithPrefixInternal;
	PBYTE Search;
	PBYTE SearchEnd;
	LONG RelativeOffset;

	if (!DbgPrintInternalAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	*DbgPrintInternalAddress = NULL;

	//
	// Get NTDLL base address and then get the address of the exported
	// function "vDbgPrintExWithPrefix".
	//

	RtlInitConstantUnicodeString(&NtdllName, L"ntdll.dll");
	Status = LdrGetDllHandleByName(&NtdllName, NULL, &NtdllBase);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	RtlInitConstantAnsiString(&ProcedureName, "vDbgPrintExWithPrefix");
	Status = LdrGetProcedureAddress(NtdllBase, &ProcedureName, 0, &vDbgPrintExWithPrefix);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Scan for a byte pattern (which varies depending on bitness).
	// Limit the start of the search to 64 bytes starting from the beginning
	// of vDbgPrintExWithPrefix.
	//

	Search = (PBYTE) vDbgPrintExWithPrefix;
	SearchEnd = Search + 64;

	if (KexRtlCurrentProcessBitness() == 64) {
		// look for E8 followed by a 4-byte signed relative displacement,
		// and then 48 83 C4 38 C3.

		while (Search++ < SearchEnd) {
			if (Search[0] == 0xE8 &&
				Search[5] == 0x48 &&
				Search[6] == 0x83 &&
				Search[7] == 0xC4 &&
				Search[8] == 0x38 &&
				Search[9] == 0xC3) {

				goto Found;
			}
		}
	} else {
		// look for E8 followed by a 4-byte signed relative displacement,
		// and then 5D C2 10 00.

		while (Search++ < SearchEnd) {
			if (Search[0] == 0xE8 &&
				Search[5] == 0x5D &&
				Search[6] == 0xC2 &&
				Search[7] == 0x10 &&
				Search[8] == 0x00) {

				goto Found;
			}
		}
	}

	return STATUS_PROCEDURE_NOT_FOUND;

Found:
	RelativeOffset = *(PLONG) &Search[1];
	vDbgPrintExWithPrefixInternal = (PVOID) (&Search[5] + RelativeOffset);
	*DbgPrintInternalAddress = vDbgPrintExWithPrefixInternal;

	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

STATIC PCWSTR KexAdvlComponentIdToTextLookup(
	IN	ULONG	ComponentId)
{
	// _DPFLTR_TYPE, ntddk.h
	// only a small few are relevant to user mode, hence the
	// switch-case instead of a lookup table
	switch (ComponentId) {
	case 0x33:	return L"Side-by-Side";
	case 0x54:	return L"Threadpool";
	case 0x55:	return L"Loader";
	case 0x5D:	return L"Application Verifier";
	case 0x65:	return L"General/RTL";
	default:	return L"Unknown";
	}
}

STATIC VOID KexAdvlMapLdrFunctionToSourceFile(
	OUT	PUNICODE_STRING		SourceFile,
	IN	PCUNICODE_STRING	SourceFunction) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	STATIC PKEX_RTL_STRING_MAPPER StringMapper = NULL;
	STATIC CONST KEX_RTL_STRING_MAPPER_ENTRY StringMapperEntries[] = {
		{ RTL_CONSTANT_STRING(L"LdrAddRefDll"),								RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpUpdateLoadCount2"),						RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpUnloadDll"),							RTL_CONSTANT_STRING(L"ldrapi.c") },
		{ RTL_CONSTANT_STRING(L"LdrpFindKnownDll"),							RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpProcessStaticImports"),					RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializeProcess"),					RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpRunInitializeRoutines"),				RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpCallTlsInitializers"),					RTL_CONSTANT_STRING(L"ldrtls.c") },
		{ RTL_CONSTANT_STRING(L"LdrpRelocateImage"),						RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpMapViewOfSection"),						RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpGetKnownDllSectionHandle"),				RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpProtectAndRelocateImage"),				RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpAllocateTls"),							RTL_CONSTANT_STRING(L"ldrtls.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializeTls"),						RTL_CONSTANT_STRING(L"ldrtls.c") },
		{ RTL_CONSTANT_STRING(L"LdrpResolveDllName"),						RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpSnapIAT"),								RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrGetProcedureAddressEx"),					RTL_CONSTANT_STRING(L"ldrapi.c") },
		{ RTL_CONSTANT_STRING(L"LdrGetDllHandleEx"),						RTL_CONSTANT_STRING(L"ldrapi.c") },
		{ RTL_CONSTANT_STRING(L"LdrShutdownProcess"),						RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpLoadImportModule"),						RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpHandleOneOldFormatImportDescriptor"),	RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpHandleOneNewFormatImportDescriptor"),	RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpProcessStaticImports"),					RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"_LdrpInitialize"),							RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpSnapThunk"),							RTL_CONSTANT_STRING(L"ldrsnap.c") },
		{ RTL_CONSTANT_STRING(L"LdrpGenericExceptionFilter"),				RTL_CONSTANT_STRING(L"ldrutil.c") },
		{ RTL_CONSTANT_STRING(L"LdrpRunShimEngineInitRoutine"),				RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializationFailure"),				RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializeProcessWrapperFilter"),		RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpGetShimEngineInterface"),				RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrLoadDll"),								RTL_CONSTANT_STRING(L"ldrapi.c") },
		{ RTL_CONSTANT_STRING(L"LdrpResolveFileName"),						RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpSearchPath"),							RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpFindLoadedDll"),						RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializeApplicationVerifierPackage"),	RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpInitializeExecutionOptions"),			RTL_CONSTANT_STRING(L"ldrinit.c") },
		{ RTL_CONSTANT_STRING(L"LdrpFindOrMapDll"),							RTL_CONSTANT_STRING(L"ldrfind.c") },
		{ RTL_CONSTANT_STRING(L"LdrpLoadDll"),								RTL_CONSTANT_STRING(L"ldrapi.c") },
		{ RTL_CONSTANT_STRING(L"LdrpLoadShimEngine"),						RTL_CONSTANT_STRING(L"ldrinit.c") }
	};

	if (!StringMapper) {
		Status = KexRtlCreateStringMapper(&StringMapper, 0);
		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		KexRtlInsertMultipleEntriesStringMapper(
			StringMapper, 
			StringMapperEntries, 
			ARRAYSIZE(StringMapperEntries));
	}

	Status = KexRtlLookupEntryStringMapper(StringMapper, SourceFunction, SourceFile);
	if (!NT_SUCCESS(Status)) {
		goto BailOut;
	}

	return;

BailOut:
	RtlInitConstantUnicodeString(SourceFile, L"(unknown)");
} PROTECTED_FUNCTION_END_VOID

STATIC NTSTATUS KexAdvlParseAndDispatchMessage(
	IN	PSTR	Prefix,
	IN	ULONG	ComponentId,
	IN	ULONG	Level,
	IN	PSTR	Text) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (ComponentId == 0x55) {
		ANSI_STRING PrefixAnsi;
		UNICODE_STRING PrefixUnicode;
		UNICODE_STRING File;
		UNICODE_STRING Function;
		VXLSEVERITY Severity;

		//
		// Parse Loader messages.
		// The Prefix for loader messages has the following fixed format:
		// "[PID%04x]:[TID%04x] @ [TIME%08d] - [FUNCTION%s] - [SEVERITY%s]: "
		// (See LdrpLogDebugPrint for more info.)
		// Due to the fixed length fields it is very easy to parse.
		//
		
		RtlInitAnsiString(&PrefixAnsi, Prefix);
		RtlInitEmptyUnicodeStringFromTeb(&PrefixUnicode);
		Status = RtlAnsiStringToUnicodeString(&PrefixUnicode, &PrefixAnsi, FALSE);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		// speed past all the irrelevant information
		KexRtlAdvanceUnicodeString(&PrefixUnicode, 25 * sizeof(WCHAR));

		// parse function string
		Function = PrefixUnicode;
		Function.Length = 0;

		until (*KexRtlEndOfUnicodeString(&Function) == L' ' || Function.Length >= Function.MaximumLength) {
			Function.Length += 2;
		}

		// make sure null terminated
		*KexRtlEndOfUnicodeString(&Function) = '\0';

		// get past function and the 3 chars following it
		KexRtlAdvanceUnicodeString(&PrefixUnicode, Function.Length + (3 * sizeof(WCHAR)));

		//
		// parse severity string, which can be one of the following:
		//
		//   ERROR, WARNING, INFO, ENTER, RETURN
		//
		// We will look at the second character:
		//
		//   R -> LogSeverityError
		//   A -> LogSeverityDebug (or, if release build, no print)
		//   Anything else -> No print.
		//
		// We won't bother formatting anything other than errors or warnings
		// because they are basically useless information and there are many
		// of them generated by the loader.
		//   

		if (PrefixUnicode.Buffer[1] == L'R') {
			Severity = LogSeverityError;
		} else if (KexIsDebugBuild && PrefixUnicode.Buffer[1] == L'A') {
			Severity = LogSeverityDebug;
		} else {
			return STATUS_SUCCESS;
		}

		//
		// Based on the function, figure out which file this message came from.
		// LdrpLogDebugPrint actually takes a file as an input argument, but
		// it doesn't actually get passed down to us.
		//

		KexAdvlMapLdrFunctionToSourceFile(&File, &Function);

		return VxlWriteLogEx(
			KexData->LogHandle,
			L"Loader",
			File.Buffer,
			0,
			Function.Buffer,
			Severity,
			L"%hs",
			Text);
	}

	return STATUS_NOT_IMPLEMENTED;
} PROTECTED_FUNCTION_END

STATIC NTSTATUS NTAPI KexpvDbgPrintExWithPrefixInternalHook(
	IN	PSTR	Prefix,
	IN	ULONG	ComponentId,
	IN	ULONG	Level,
	IN	PSTR	Format,
	IN	ARGLIST	ArgList,
	IN	BOOLEAN	BreakPoint) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PCWSTR Component;
	CHAR Buffer[1024];
	HRESULT Result;

	Result = StringCchVPrintfA(
		Buffer,
		ARRAYSIZE(Buffer),
		Format,
		ArgList);

	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	Status = KexAdvlParseAndDispatchMessage(
		Prefix,
		ComponentId,
		Level,
		Buffer);

	// if the advanced dispatch could parse the message, that's good -
	// we don't need to fall back to the generic case
	if (NT_SUCCESS(Status)) {
		return Status;
	}

	Component = KexAdvlComponentIdToTextLookup(ComponentId);

	// don't use KexLogDebugEvent because that will be disabled
	// in release builds
	return KexLogEvent(
		LogSeverityDebug,
		L"DbgPrintEx from component ID 0x%04lx (%s)\r\n\r\n"
		L"%hs\r\n"
		L"%hs",
		ComponentId,
		Component,
		Prefix,
		Buffer);
} PROTECTED_FUNCTION_END_BOOLEAN