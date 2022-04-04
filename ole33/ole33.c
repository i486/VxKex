#include <Windows.h>
#include <KexDll.h>
DECLARE_HANDLE(CO_MTA_USAGE_COOKIE);

DLLAPI HRESULT WINAPI CoIncrementMTAUsage(
	OUT	CO_MTA_USAGE_COOKIE *pCookie)
{
	*pCookie = NULL;
	return E_NOTIMPL;
}