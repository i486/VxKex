#include "buildcfg.h"
#include "kxnetp.h"

KXNETAPI ULONG WINAPI WinHttpCreateProxyResolver(
	IN	HINTERNET	SessionHandle,
	OUT	HINTERNET	*Resolver)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	KexDebugCheckpoint();
	*Resolver = (HINTERNET) 0x12345678;
	return ERROR_SUCCESS;
}

KXNETAPI ULONG WINAPI WinHttpGetProxyForUrlEx(
	IN	HINTERNET	Resolver,
	IN	PCWSTR		Url,
	IN	PVOID		AutoProxyOptions,
	IN	ULONG_PTR	Context)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	KexDebugCheckpoint();
	return ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
}

KXNETAPI ULONG WINAPI WinHttpGetProxyResult(
	IN	HINTERNET	Resolver,
	OUT	PVOID		ProxyResult)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	KexDebugCheckpoint();
	return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
}

KXNETAPI VOID WINAPI WinHttpFreeProxyResult(
	IN OUT	PVOID	ProxyResult)
{
	KexLogWarningEvent(L"Unimplemented WinHTTP function called");
	KexDebugCheckpoint();
}