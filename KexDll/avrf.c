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
	ANSI_STRING VerifierStopMessageName;
	PVOID VerifierDllBase;
	PVOID VerifierStopMessage;

	Status = KexHkInstallBasicHook(RtlApplicationVerifierStop, KexpApplicationVerifierStopHook, NULL);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// If verifier.dll is loaded, also patch out its VerifierStopMessage function.
	//
	RtlInitConstantUnicodeString(&VerifierDllName, L"verifier.dll");
	Status = LdrGetDllHandleByName(&VerifierDllName, NULL, &VerifierDllBase);
	if (NT_SUCCESS(Status)) {
		RtlInitConstantAnsiString(&VerifierStopMessageName, "VerifierStopMessage");
		Status = LdrGetProcedureAddress(VerifierDllBase, &VerifierStopMessageName, 0, &VerifierStopMessage);

		if (NT_SUCCESS(Status)) {
			Status = KexHkInstallBasicHook(VerifierStopMessage, KexpApplicationVerifierStopHook, NULL);

			if (!NT_SUCCESS(Status)) {
				return Status;
			}
		}
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