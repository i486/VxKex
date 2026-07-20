#include "buildcfg.h"
#include "kxnetp.h"

//
// The WinHTTP proxy functions can easily be fully implemented by applying
// the same kind of asynchronous threadpool-based wrapper around
// WinHttpGetProxyForUrl as was done for GetAddrInfoExW (gaiasync.c).
//
// However as of yet no applications are known to require these.
//

KXNETAPI ULONG WINAPI WinHttpCreateProxyResolver(
	IN	HINTERNET	SessionHandle,
	OUT	HINTERNET	*Resolver)
{
	KexLogUnimplementedFunctionEvent();
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
	KexLogUnimplementedFunctionEvent();
	KexDebugCheckpoint();
	return ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT;
}

KXNETAPI ULONG WINAPI WinHttpGetProxyResult(
	IN	HINTERNET	Resolver,
	OUT	PVOID		ProxyResult)
{
	KexLogUnimplementedFunctionEvent();
	KexDebugCheckpoint();
	return ERROR_WINHTTP_INCORRECT_HANDLE_STATE;
}

KXNETAPI VOID WINAPI WinHttpFreeProxyResult(
	IN OUT	PVOID	ProxyResult)
{
	KexLogUnimplementedFunctionEvent();
	KexDebugCheckpoint();
}