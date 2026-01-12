///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     etw.c
//
// Abstract:
//
//     Contains implementations of some ETW (Event Tracing for Windows)
//     functions.
//
// Author:
//
//     vxiiduu (16-Mar-2024)
//
// Environment:
//
//     Native mode.
//
// Revision History:
//
//     vxiiduu              16-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"
#include <evntprov.h>

typedef ULONG EVENT_INFO_CLASS;

//
// EtwEventSetInformation (which is forwarded from advapi32!EventSetInformation)
// is available only on later builds of Windows 7. This function is here as a
// proxy, so that earlier builds of Windows 7 do not fail to launch applications
// due to the lack of this non-essential function.
//

KEXAPI ULONG NTAPI KexEtwEventSetInformation(
	IN	REGHANDLE			Handle,
	IN	EVENT_INFO_CLASS	InformationClass,
	IN	PVOID				EventInformation,
	IN	ULONG				InformationLength)
{
	STATIC ULONG (NTAPI *EtwEventSetInformation) (REGHANDLE, EVENT_INFO_CLASS, PVOID, ULONG) = NULL;

	if (!EtwEventSetInformation) {
		NTSTATUS Status;
		ANSI_STRING ProcedureName;

		RtlInitConstantAnsiString(&ProcedureName, "EtwEventSetInformation");

		Status = LdrGetProcedureAddress(
			KexData->SystemDllBase,
			&ProcedureName,
			0,
			(PPVOID) &EtwEventSetInformation);

		if (!NT_SUCCESS(Status)) {
			ASSUME (EtwEventSetInformation == NULL);

			KexLogInformationEvent(
				L"EtwEventSetInformation is not available on this computer\r\n\r\n"
				L"This function was made available in later updates to Windows 7. "
				L"VxKex will return ERROR_NOT_SUPPORTED to the calling application.");
		}

		if (NT_SUCCESS(Status)) {
			ASSUME (EtwEventSetInformation != NULL);
		}
	}

	if (!EtwEventSetInformation) {
		return ERROR_NOT_SUPPORTED;
	}

	return EtwEventSetInformation(Handle, InformationClass, EventInformation, InformationLength);
}