#include "buildcfg.h"
#include "kxmip.h"

KXMIAPI HRESULT WINAPI WldpQueryWindowsLockdownMode(
	OUT	PWLDP_WINDOWS_LOCKDOWN_MODE	LockdownMode)
{
	ASSERT (LockdownMode != NULL);

	if (!LockdownMode) {
		return E_POINTER;
	}

	*LockdownMode = WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED;
	return S_OK;
}

KXMIAPI HRESULT WINAPI WldpGetLockdownPolicy(
	IN	PVOID		HostInformation,
	OUT	PULONG		LockdownState,
	IN	ULONG		LockdownFlags)
{
	ASSERT (LockdownState != NULL);

	if (!LockdownState) {
		return E_POINTER;
	}

	*LockdownState = 0x80000000;
	return S_OK;
}