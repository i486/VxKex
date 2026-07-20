#include "buildcfg.h"
#include "kxcomp.h"
#include <KexW32ML.h>

STATIC ULONG WinRTErrorFlags = RO_ERROR_REPORTING_NONE;

STATIC BOOLEAN ShouldRaiseExceptionOnError(
	VOID)
{
	if (WinRTErrorFlags & RO_ERROR_REPORTING_SUPPRESSEXCEPTIONS) {
		return FALSE;
	}

	if (WinRTErrorFlags & RO_ERROR_REPORTING_FORCEEXCEPTIONS) {
		return TRUE;
	}

	return IsDebuggerPresent();
}

STATIC VOID RaiseTransformExceptionIfAppropriate(
	IN	HRESULT	OldError,
	IN	HRESULT	NewError,
	IN	PCWSTR	Message OPTIONAL)
{
	ULONG_PTR ExceptionInformation[4];

	if (!ShouldRaiseExceptionOnError()) {
		return;
	}

	ExceptionInformation[0] = OldError;
	ExceptionInformation[1] = NewError;
	ExceptionInformation[2] = Message ? wcslen(Message) : 0;
	ExceptionInformation[3] = (ULONG_PTR) Message;

	RaiseException(
		EXCEPTION_RO_TRANSFORMERROR,
		0,
		ARRAYSIZE(ExceptionInformation),
		ExceptionInformation);
}

STATIC VOID RaiseOriginateExceptionIfAppropriate(
	IN	HRESULT	Error,
	IN	PCWSTR	Message OPTIONAL)
{
	ULONG_PTR ExceptionInformation[3];

	if (!ShouldRaiseExceptionOnError()) {
		return;
	}

	ExceptionInformation[0] = Error;
	ExceptionInformation[1] = Message ? wcslen(Message) : 0;
	ExceptionInformation[2] = (ULONG_PTR) Message;

	RaiseException(
		EXCEPTION_RO_ORIGINATEERROR,
		0,
		ARRAYSIZE(ExceptionInformation),
		ExceptionInformation);
}

STATIC BOOL LogWinRTError(
	IN	HRESULT	Result,
	IN	PCWSTR	Message OPTIONAL)
{
	NTSTATUS Status;

	if (!FAILED(Result)) {
		return FALSE;
	}

	Status = VxlWriteLogEx(
		KexData->LogHandle,
		KEX_COMPONENT,
		__FILEW__,
		__LINE__,
		L"RoOriginateError",
		LogSeverityDebug,
		L"WINRT: %s: %s", Win32ErrorAsString(Result), Message);

	return NT_SUCCESS(Status);
}

BOOL WINAPI RoOriginateErrorW(
	IN	HRESULT	Result,
	IN	ULONG	Length,
	IN	PCWSTR	Message OPTIONAL)
{
	BOOL Success;

	Success = LogWinRTError(Result, Message);
	RaiseOriginateExceptionIfAppropriate(Result, Message);

	return Success;
}

BOOL WINAPI RoOriginateError(
	IN	HRESULT	Result,
	IN	HSTRING	Message)
{
	return RoOriginateErrorW(
		Result,
		WindowsGetStringLen(Message),
		WindowsGetStringRawBuffer(Message, NULL));
}

KXCOMAPI BOOL WINAPI RoOriginateLanguageException(
	IN	HRESULT		Result,
	IN	HSTRING		Message,
	IN	IUnknown	*LanguageException OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING FaultingModuleName;

	if (SUCCEEDED(Result)) {
		return FALSE;
	}

	RtlInitEmptyUnicodeStringFromTeb(&FaultingModuleName);
	Status = KexLdrGetDllFullNameFromAddress(ReturnAddress(), &FaultingModuleName);

	if (!NT_SUCCESS(Status)) {
		RtlInitConstantUnicodeString(&FaultingModuleName, L"(unknown)");
	}

	KexLogWarningEvent(
		L"Windows Runtime language exception: %s\r\n"
		L"Name of the faulting module: %wZ\r\n"
		L"Result:                      0x%08lx\r\n"
		L"Message:                     %s\r\n"
		L"LanguageException:           0x%p",
		Win32ErrorAsString(Result),
		&FaultingModuleName,
		Result,
		WindowsGetStringRawBuffer(Message, NULL),
		LanguageException);

	return TRUE;
}

STATIC HRESULT IsRestrictedErrorObject(
	IN	IRestrictedErrorInfo	*RestrictedErrorInfo)
{
	HRESULT Result;
	IUnknown *InternalErrorInfo;

	ASSERT (RestrictedErrorInfo != NULL);

	InternalErrorInfo = NULL;

	Result = RestrictedErrorInfo->lpVtbl->QueryInterface(
		RestrictedErrorInfo,
		&IID_IInternalErrorInfo,
		(PPVOID) &InternalErrorInfo);

	if (Result == E_NOINTERFACE) {
		Result = E_INVALIDARG;
	}

	SafeRelease(InternalErrorInfo);
	return Result;
}

HRESULT WINAPI GetRestrictedErrorInfo(
	OUT	IUnknown	**RestrictedErrorInfo)
{
	// TODO: implement this better
	KexLogUnimplementedFunctionEvent();
	return S_FALSE;
}

KXCOMAPI HRESULT WINAPI SetRestrictedErrorInfo(
	IN	IRestrictedErrorInfo	*RestrictedErrorInfo OPTIONAL)
{
	HRESULT Result;
	IErrorInfo *ErrorInfo;

	Result = S_OK;
	ErrorInfo = NULL;

	if (RestrictedErrorInfo) {
		Result = IsRestrictedErrorObject(RestrictedErrorInfo);
		if (FAILED(Result)) {
			goto Exit;
		}

		Result = RestrictedErrorInfo->lpVtbl->QueryInterface(
			RestrictedErrorInfo,
			&IID_IErrorInfo,
			(PPVOID) &ErrorInfo);
	}

	if (SUCCEEDED(Result)) {
		Result = SetErrorInfo(0, ErrorInfo);
	}

Exit:
	SafeRelease(ErrorInfo);
	return Result;
}

KXCOMAPI BOOL WINAPI RoTransformErrorW(
	IN	HRESULT	OldError,
	IN	HRESULT	NewError,
	IN	ULONG	MessageLength,
	IN	PCWSTR	Message OPTIONAL)
{
	BOOL Success;

	if (OldError == NewError) {
		return FALSE;
	}

	if (SUCCEEDED(OldError) && SUCCEEDED(NewError)) {
		return FALSE;
	}

	Success = LogWinRTError(NewError, Message);
	RaiseTransformExceptionIfAppropriate(OldError, NewError, Message);

	return Success;
}

KXCOMAPI BOOL WINAPI RoTransformError(
	IN	HRESULT	OldError,
	IN	HRESULT	NewError,
	IN	HSTRING	Message)
{
	return RoTransformErrorW(
		OldError,
		NewError,
		WindowsGetStringLen(Message),
		WindowsGetStringRawBuffer(Message, NULL));
}

KXCOMAPI VOID NORETURN WINAPI RoFailFastWithErrorContext(
	IN	HRESULT	HResult)
{
	EXCEPTION_RECORD ExceptionRecord;
	CONTEXT Context;

	GetThreadContext(NtCurrentThread(), &Context);

	RtlZeroMemory(&ExceptionRecord, sizeof(ExceptionRecord));
	ExceptionRecord.ExceptionAddress = ReturnAddress();
	ExceptionRecord.ExceptionCode = HResult;
	ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;

	RaiseFailFastException(&ExceptionRecord, &Context, 0);

	NOT_REACHED;
}

KXCOMAPI HRESULT WINAPI RoSetErrorReportingFlags(
	IN	ULONG	Flags)
{
	if (Flags & ~0xF) {
		return E_INVALIDARG;
	}

	KexLogDebugEvent(L"WinRT error reporting flags set: 0x%08lx", Flags);

	WinRTErrorFlags = Flags;
	return S_OK;
}

KXCOMAPI HRESULT WINAPI RoGetErrorReportingFlags(
	OUT	PULONG	Flags)
{
	if (Flags == NULL) {
		return E_POINTER;
	}

	*Flags = WinRTErrorFlags;
	return S_OK;
}