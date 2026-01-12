#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI GetOsSafeBootMode(
	OUT	PBOOL	IsSafeBootMode)
{
	*IsSafeBootMode = FALSE;
	return TRUE;
}