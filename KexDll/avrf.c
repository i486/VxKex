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
//     vxiiduu              16-Mar-2024  Add more assertions.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Disable as many application verifier functionality as we can.
// This reduces the intrusiveness of VxKex, and also does stuff like stopping
// app verifier from intercepting some heap calls etc.
// Doesn't totally get rid of it though. We still can't totally unload verifier.dll
// without causing crashes.
//
NTSTATUS KexDisableAVrf(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING VerifierDllName;
	PVOID VerifierDllBase;
	PDLL_INIT_ROUTINE VerifierDllMain;

	RtlInitConstantUnicodeString(&VerifierDllName, L"verifier.dll");

	Status = LdrGetDllHandleByName(&VerifierDllName, NULL, &VerifierDllBase);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		// This function was probably already called.
		return STATUS_UNSUCCESSFUL;
	}

	Status = KexLdrFindDllInitRoutine(
		VerifierDllBase,
		(PPVOID) &VerifierDllMain);

	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status) && VerifierDllMain != NULL) {
		if (!VerifierDllMain(VerifierDllBase, DLL_PROCESS_DETACH, NULL)) {
			KexLogWarningEvent(L"Verifier.dll failed to de-initialize.");
			ASSERT (FALSE);
		}
	}

	NtCurrentPeb()->NtGlobalFlag &= ~(FLG_APPLICATION_VERIFIER | FLG_HEAP_PAGE_ALLOCS);

	Status = NtSetInformationProcess(
		NtCurrentProcess(),
		ProcessHandleTracing,
		NULL,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to disable process handle tracing.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
	}

	return STATUS_SUCCESS;
}