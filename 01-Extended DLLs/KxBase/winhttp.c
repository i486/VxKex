#include "buildcfg.h"
#include "kxbasep.h"
#include <winhttp.h>

KXBASEAPI ULONG WINAPI WinHttpCreateProxyResolver(
	IN	HINTERNET	SessionHandle,
	OUT	HINTERNET	*Resolver)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	*Resolver = (HINTERNET) 0x12345678;
	return ERROR_SUCCESS;
}

KXBASEAPI ULONG WINAPI WinHttpGetProxyForUrlEx(
	IN	HINTERNET	Resolver,
	IN	PCWSTR		Url,
	IN	PVOID		AutoProxyOptions,
	IN	ULONG_PTR	Context)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	return ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
}

KXBASEAPI ULONG WINAPI WinHttpGetProxyResult(
	IN	HINTERNET	Resolver,
	OUT	PVOID		ProxyResult)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
}

KXBASEAPI VOID WINAPI WinHttpFreeProxyResult(
	IN OUT	PVOID	ProxyResult)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
}