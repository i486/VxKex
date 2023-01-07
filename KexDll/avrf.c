///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     avrf.c
//
// Abstract:
//
//     Functions for dealing with Application Verifier.
//
// Author:
//
//     vxiiduu (03-Nov-2022)
//
// Revision History:
//
//     vxiiduu              03-Nov-2022  Initial creation.
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

VOID NTAPI KexpApplicationVerifierStopHook (
    IN	ULONG_PTR		Code,
    IN	PCHAR			Message,
    IN	ULONG_PTR		Param1,
	IN	PCHAR			Description1,
	IN	ULONG_PTR		Param2,
	IN	PCHAR			Description2,
    IN	ULONG_PTR		Param3,
	IN	PCHAR			Description3,
    IN	ULONG_PTR		Param4,
	IN	PCHAR			Description4);

//
// Disable as many application verifier functionality as we can.
//
NTSTATUS KexDisableAVrf(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING VerifierDllName;
	PVOID VerifierDllBase;
	PDLL_INIT_ROUTINE VerifierDllMain;

	RtlInitConstantUnicodeString(&VerifierDllName, L"verifier.dll");

	Status = LdrGetDllHandleByName(&VerifierDllName, NULL, &VerifierDllBase);
	if (!NT_SUCCESS(Status)) {
		// This function was probably already called.
		return STATUS_UNSUCCESSFUL;
	}

	Status = KexLdrFindDllInitRoutine(
		VerifierDllBase,
		(PPVOID) &VerifierDllMain);

	if (NT_SUCCESS(Status) && VerifierDllMain != NULL) {
		if (!VerifierDllMain(VerifierDllBase, DLL_PROCESS_DETACH, NULL)) {
			KexLogWarningEvent(L"Verifier.dll failed to de-initialize.");
		}
	}

	NtCurrentPeb()->NtGlobalFlag &= ~(FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS);

	Status = NtSetInformationProcess(
		NtCurrentProcess(),
		ProcessHandleTracing,
		NULL,
		0);

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to disable process handle tracing.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
	}

	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

VOID NTAPI KexpApplicationVerifierStopHook (
    IN	ULONG_PTR		Code,
    IN	PCHAR			Message,
    IN	ULONG_PTR		Param1,
	IN	PCHAR			Description1,
	IN	ULONG_PTR		Param2,
	IN	PCHAR			Description2,
    IN	ULONG_PTR		Param3,
	IN	PCHAR			Description3,
    IN	ULONG_PTR		Param4,
	IN	PCHAR			Description4)
{
	return;
}