#include "buildcfg.h"
#include "kxbasep.h"

//
// TODO: Actually redirect wldp.dll to kxbase.dll somehow.
//

KXBASEAPI HRESULT WINAPI WldpQueryWindowsLockdownMode(
	OUT	PWLDP_WINDOWS_LOCKDOWN_MODE	LockdownMode)
{
	ASSERT (LockdownMode != NULL);

	*LockdownMode = WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED;
	return S_OK;
}