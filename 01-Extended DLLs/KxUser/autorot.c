#include "buildcfg.h"
#include "kxuserp.h"

BOOL WINAPI SetDisplayAutoRotationPreferences(
	IN	ORIENTATION_PREFERENCE	Orientation)
{
	return TRUE;
}

BOOL WINAPI GetDisplayAutoRotationPreferences(
	OUT	PORIENTATION_PREFERENCE	Orientation)
{
	*Orientation = ORIENTATION_PREFERENCE_NONE;
	return TRUE;
}