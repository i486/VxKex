#include "buildcfg.h"
#include "kxcomp.h"
#include <KexW32ML.h>

BOOL WINAPI RoOriginateErrorW(
	IN	HRESULT	Result,
	IN	ULONG	Length,
	IN	PCWSTR	Message)
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

HRESULT WINAPI GetRestrictedErrorInfo(
	OUT	IUnknown	**RestrictedErrorInfo)
{
	return S_FALSE;
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
		RtlInitConstantUnicodeString(&FaultingModuleName, L"");
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