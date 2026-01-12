#include "buildcfg.h"
#include "kxcomp.h"
#include <KexW32ML.h>

BOOL WINAPI RoOriginateErrorW(
	IN	HRESULT	Result,
	IN	ULONG	Length,
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
		RoOriginateErrorW(Result, 0, L"RestrictedErrorInfo");
	}

	SafeRelease(InternalErrorInfo);
	return Result;
}

HRESULT WINAPI GetRestrictedErrorInfo(
	OUT	IUnknown	**RestrictedErrorInfo)
{
	// TODO: implement this better
	KexLogWarningEvent(L"Unimplemented function GetRestrictedErrorInfo called.");
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
	if (OldError == NewError) {
		return FALSE;
	}

	if (SUCCEEDED(OldError) && SUCCEEDED(NewError)) {
		return FALSE;
	}

	return RoOriginateErrorW(NewError, MessageLength, Message);
}

KXCOMAPI BOOL WINAPI RoTransformError(
	IN	HRESULT	OldError,
	IN	HRESULT	NewError,
	IN	HSTRING	Message)
{
	if (OldError == NewError) {
		return FALSE;
	}

	if (SUCCEEDED(OldError) && SUCCEEDED(NewError)) {
		return FALSE;
	}

	return RoOriginateError(NewError, Message);
}