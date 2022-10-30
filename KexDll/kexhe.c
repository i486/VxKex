///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexhe.c
//
// Abstract:
//
//     Hard Error handler.
//
// Author:
//
//     vxiiduu (29-Oct-2022)
//
// Environment:
//
//     Early process initialization only. The hard error handler in this file
//     is specifically tailored for the subset of NTSTATUS values that can be
//     passed by NTDLL during early process initialization.
//
// Revision History:
//
//     vxiiduu              29-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"
#include <KexLog.h>

STATIC KEX_BASIC_HOOK_CONTEXT KexHeHookContext;

//
// Send a message to KexSrv that contains the error and its parameters.
//
STATIC NTSTATUS NTAPI KexHepHardErrorHandler(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PKEX_IPC_MESSAGE Message;
	ULONG UlongParameter;
	PCUNICODE_STRING StringParameter1;
	PCUNICODE_STRING StringParameter2;
	CONST UNICODE_STRING BlankString = RTL_CONSTANT_STRING(L"");

	StringParameter1 = &BlankString;
	StringParameter2 = &BlankString;

	if (!KexData->SrvChannel) {
		goto BailOut;
	}

	//
	// Check the parameters. If anything seems strange, just bail out and
	// let the original code deal with it.
	//

	if (NumberOfParameters == 0 || NumberOfParameters > 2) {
		// All valid error codes here have 1 or 2 params.
		goto BailOut;
	}

	if (NumberOfParameters == 2 && UnicodeStringParameterMask == 0) {
		// Can't have two ULONG parameters
		goto BailOut;
	}

	//
	// Check which members of the Parameters array are strings.
	//

	if (UnicodeStringParameterMask & 1) {
		StringParameter1 = (PCUNICODE_STRING) Parameters[0];
	} else {
		UlongParameter = (ULONG) Parameters[0];
	}

	if (NumberOfParameters >= 2) {
		if (UnicodeStringParameterMask & 2) {
			StringParameter2 = (PCUNICODE_STRING) Parameters[1];
		} else {
			UlongParameter = (ULONG) Parameters[1];
		}
	}

	KexSrvLogDebugEvent(
		L"Hard Error handler has been called.\r\n\r\n"
		L"ErrorStatus:                  0x%08lx\r\n"
		L"NumberOfParameters:           0x%08lx\r\n"
		L"UnicodeStringParameterMask:   0x%08lx\r\n"
		L"Parameters:                   0x%p\r\n"
		L"ValidResponseOptions:         %lu\r\n"
		L"Response:                     0x%p\r\n\r\n"
		L"UlongParameter:               0x%08lx\r\n"
		L"StringParameter1:             \"%wZ\"\r\n"
		L"StringParameter2:             \"%wZ\"",
		ErrorStatus,
		NumberOfParameters,
		UnicodeStringParameterMask,
		Parameters,
		ValidResponseOptions,
		Response,
		UlongParameter,
		StringParameter1,
		StringParameter2);

	//
	// Allocate, fill out, and send the message to the server.
	//

	Message = (PKEX_IPC_MESSAGE) StackAlloc(
		BYTE, 
		sizeof(KEX_IPC_MESSAGE) + StringParameter1->Length + StringParameter2->Length);

	Message->MessageId = KexIpcHardError;
	Message->AuxiliaryDataBlockSize = StringParameter1->Length + StringParameter2->Length;
	Message->HardErrorInformation.Status = ErrorStatus;
	Message->HardErrorInformation.UlongParameter = UlongParameter;
	Message->HardErrorInformation.StringParameter1Length = StringParameter1->Length / sizeof(WCHAR);
	Message->HardErrorInformation.StringParameter2Length = StringParameter2->Length / sizeof(WCHAR);
	RtlCopyMemory(Message->AuxiliaryDataBlock, StringParameter1->Buffer, StringParameter1->Length);
	RtlCopyMemory(Message->AuxiliaryDataBlock + StringParameter1->Length, StringParameter2->Buffer, StringParameter2->Length);

	return KexSrvSendMessage(KexData->SrvChannel, Message);

BailOut:
	KexSrvLogWarningEvent(L"Bailing out to original function.");

	//
	// call original NtRaiseHardError
	//

	KexHeRemoveHandler();
	Status = NtRaiseHardError(
		ErrorStatus,
		NumberOfParameters,
		UnicodeStringParameterMask,
		Parameters,
		ValidResponseOptions,
		Response);
	KexHeInstallHandler();

	return Status;
} PROTECTED_FUNCTION_END

NTSTATUS KexHeInstallHandler(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (!KexData->SrvChannel) {
		return STATUS_PORT_DISCONNECTED;
	}

	Status = KexHkInstallBasicHook(NtRaiseHardError, KexHepHardErrorHandler, &KexHeHookContext);

	if (NT_SUCCESS(Status)) {
		KexSrvLogInformationEvent(L"Successfully installed hard error handler.");
	} else {
		KexSrvLogErrorEvent(
			L"Failed to install hard error handler\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
	}

	return Status;
} PROTECTED_FUNCTION_END

NTSTATUS KexHeRemoveHandler(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (!KexData->SrvChannel) {
		return STATUS_PORT_DISCONNECTED;
	}

	Status = KexHkRemoveBasicHook(&KexHeHookContext);

	if (NT_SUCCESS(Status)) {
		KexSrvLogInformationEvent(L"Successfully removed hard error handler.");
	} else {
		KexSrvLogErrorEvent(
			L"Failed to remove hard error handler\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
	}

	return Status;
} PROTECTED_FUNCTION_END

NORETURN VOID KexHeErrorBox(
	IN	PCWSTR	ErrorMessage)
{
	UNICODE_STRING ErrorString;
	PUNICODE_STRING Parameter;

	if (KexData->SrvChannel != NULL) {
		RtlInitUnicodeString(&ErrorString, ErrorMessage);
		Parameter = &ErrorString;

		KexHepHardErrorHandler(
			STATUS_KEXDLL_INITIALIZATION_FAILURE,
			1,
			1,
			(PULONG_PTR) &Parameter,
			0,
			NULL);
	}

	NtTerminateProcess(NtCurrentProcess(), STATUS_KEXDLL_INITIALIZATION_FAILURE);
}