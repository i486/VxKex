#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI ULONG WINAPI RegisterAppStateChangeNotification(
	IN	PVOID	Routine,
	IN	PVOID	Context OPTIONAL,
	OUT	PPVOID	Registration)
{
	ASSERT (Routine != NULL);
	ASSERT (Registration != NULL);

	// random value which we can use to identify if the app tries to
	// dereference it
	*Registration = (PVOID) 0x81918198;

	return ERROR_SUCCESS;
}

KXBASEAPI VOID WINAPI UnregisterAppStateChangeNotification(
	IN OUT	PVOID	Registration)
{
	ASSERT (Registration == (PVOID) 0x81918198);

	// do nothing
}