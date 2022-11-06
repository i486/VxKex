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
		KexSrvLogDebugEvent(L"Not enabling advanced logging due to user preferences.");
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

	KexSrvLogInformationEvent(L"Successfully initialized advanced logging.");
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
	// Limit the start of the search to 32 bytes starting from the beginning
	// of vDbgPrintExWithPrefix.
	//

	Search = (PBYTE) vDbgPrintExWithPrefix;
	SearchEnd = Search + 32;

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

STATIC NTSTATUS NTAPI KexpvDbgPrintExWithPrefixInternalHook(
	IN	PSTR	Prefix,
	IN	ULONG	ComponentId,
	IN	ULONG	Level,
	IN	PSTR	Format,
	IN	ARGLIST	ArgList,
	IN	BOOLEAN	BreakPoint) PROTECTED_FUNCTION
{
	PCWSTR Component;
	CHAR Buffer[1024];
	HRESULT Result;

	// _DPFLTR_TYPE, ntddk.h
	// only a small few are relevant to user mode, hence the
	// switch-case instead of a lookup table
	switch (ComponentId) {
	case 0x33:
		Component = L"Side-by-Side";
		break;
	case 0x54:
		Component = L"Threadpool";
		break;
	case 0x55:
		Component = L"Loader";
		break;
	case 0x5D:
		Component = L"Application Verifier";
		break;
	case 0x65:
		Component = L"General/RTL";
		break;
	default:
		Component = L"Unknown";
		break;
	}

	Result = StringCchVPrintfA(
		Buffer,
		ARRAYSIZE(Buffer),
		Format,
		ArgList);

	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	// don't use KexSrvLogDebugEvent because that will be disabled
	// in release builds
	return KexSrvLogEvent(
		LogSeverityDebug,
		L"DbgPrintEx from component ID 0x%04lx (%s)\r\n\r\n"
		L"%hs\r\n"
		L"%hs",
		ComponentId,
		Component,
		Prefix,
		Buffer);
} PROTECTED_FUNCTION_END_BOOLEAN