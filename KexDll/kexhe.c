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
//     TODO: long term, perhaps add some more user friendly UI like in
//     0.0.0.3? however, in the absence of kexsrv this might be difficult.
//
// Author:
//
//     vxiiduu (29-Oct-2022)
//
// Environment:
//
//     During static import resolution. The hard error handler in this file
//     is specifically tailored for the subset of NTSTATUS values that can be
//     passed by NTDLL while loading static imports.
//
// Revision History:
//
//     vxiiduu              29-Oct-2022  Initial creation.
//     vxiiduu              05-Nov-2022  Remove ability to remove the HE hook.
//     vxiiduu              06-Nov-2022  KEXDLL init failure message now works
//                                       even if KexSrv is not running.
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Log hard errors.
//
NTSTATUS NTAPI Ext_NtRaiseHardError(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	ULONG UlongParameter;
	PCUNICODE_STRING StringParameter1;
	PCUNICODE_STRING StringParameter2;
	CONST UNICODE_STRING BlankString = RTL_CONSTANT_STRING(L"");

	StringParameter1 = &BlankString;
	StringParameter2 = &BlankString;

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

	KexLogErrorEvent(
		L"Hard Error handler has been called.\r\n\r\n"
		L"ErrorStatus:                  %s (0x%08lx)\r\n"
		L"NumberOfParameters:           0x%08lx\r\n"
		L"UnicodeStringParameterMask:   0x%08lx\r\n"
		L"Parameters:                   0x%p\r\n"
		L"ValidResponseOptions:         %lu\r\n"
		L"Response:                     0x%p\r\n\r\n"
		L"UlongParameter:               0x%08lx\r\n"
		L"StringParameter1:             \"%wZ\"\r\n"
		L"StringParameter2:             \"%wZ\"\r\n",
		KexRtlNtStatusToString(ErrorStatus),
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
	// call original NtRaiseHardError
	//

BailOut:
	KexDebugCheckpoint();

	Status = KexNtRaiseHardError(
		ErrorStatus,
		NumberOfParameters,
		UnicodeStringParameterMask,
		Parameters,
		ValidResponseOptions,
		Response);

	return Status;
} PROTECTED_FUNCTION_END

VOID KexMessageBox(
	IN	ULONG	Flags,
	IN	PCWSTR	Caption OPTIONAL,
	IN	PCWSTR	Message OPTIONAL)
{
	UNICODE_STRING MessageUS;
	UNICODE_STRING CaptionUS;
	ULONG_PTR Parameters[4];
	ULONG Response;

	if (!Message) {
		Message = L"";
	}

	if (!Caption) {
		Caption = L"VxKex";
	}

	RtlInitUnicodeString(&CaptionUS, Caption);
	RtlInitUnicodeString(&MessageUS, Message);

	Parameters[0] = (ULONG_PTR) &MessageUS;
	Parameters[1] = (ULONG_PTR) &CaptionUS;
	Parameters[2] = Flags;						// MB_* from kexdllp.h
	Parameters[3] = INFINITE;					// Timeout in milliseconds

	KexNtRaiseHardError(
		STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE,
		ARRAYSIZE(Parameters),
		3,
		Parameters,
		0,
		&Response);
}

VOID KexMessageBoxF(
	IN	ULONG	Flags,
	IN	PCWSTR	Caption OPTIONAL,
	IN	PCWSTR	Message OPTIONAL,
	IN	...)
{
	HRESULT Result;
	WCHAR Buffer[512];
	ARGLIST ArgList;

	va_start(ArgList, Message);

	Result = StringCchVPrintf(
		Buffer,
		ARRAYSIZE(Buffer),
		Message,
		ArgList);

	va_end(ArgList);

	ASSERT (SUCCEEDED(Result));

	if (SUCCEEDED(Result)) {
		KexMessageBox(Flags, Caption, Buffer);
	}
}

NORETURN VOID KexHeErrorBox(
	IN	PCWSTR	ErrorMessage)
{
	KexMessageBox(MB_ICONERROR, L"Application Error (VxKex)", ErrorMessage);
	NtTerminateProcess(NtCurrentProcess(), STATUS_KEXDLL_INITIALIZATION_FAILURE);
}