#include "buildcfg.h"
#include "kxcomp.h"

//
// Note: The authentic Windows 8+ API uses a structure pointer
// as the cookie rather than a HANDLE.
//

STATIC DWORD WINAPI KxCompMTAUsageIncrementerThread(
	IN	PVOID	Parameter)
{
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	SleepEx(INFINITE, TRUE);
	CoUninitialize();
	return 0;
}

KXCOMAPI HRESULT WINAPI CoIncrementMTAUsage(
	OUT	PCO_MTA_USAGE_COOKIE	Cookie)
{
	if (!Cookie) {
		return E_INVALIDARG;
	}

	*Cookie = CreateThread(
		NULL,
		0,
		KxCompMTAUsageIncrementerThread,
		NULL,
		0,
		NULL);

	if (!*Cookie) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXCOMAPI HRESULT WINAPI CoDecrementMTAUsage(
	IN	CO_MTA_USAGE_COOKIE		Cookie)
{
	NTSTATUS Status;

	if (!Cookie) {
		return E_INVALIDARG;
	}

	Status = NtAlertThread(Cookie);
	NtClose(Cookie);

	if (NT_SUCCESS(Status)) {
		return S_OK;
	} else {
		return HRESULT_FROM_WIN32(RtlNtStatusToDosError(Status));
	}
}