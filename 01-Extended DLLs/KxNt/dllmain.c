#include "buildcfg.h"
#include <KexComm.h>

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllBase);
	}

	return TRUE;
}