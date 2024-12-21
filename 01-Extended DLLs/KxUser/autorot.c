#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI SetDisplayAutoRotationPreferences(
	IN	ORIENTATION_PREFERENCE	Orientation)
{
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetDisplayAutoRotationPreferences(
	OUT	PORIENTATION_PREFERENCE	Orientation)
{
	*Orientation = ORIENTATION_PREFERENCE_NONE;
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetAutoRotationState(
	OUT	PAR_STATE	AutoRotationState)
{
	if (!AutoRotationState) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	*AutoRotationState = AR_NOSENSOR;
	return TRUE;
}