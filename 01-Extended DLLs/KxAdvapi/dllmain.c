#include "buildcfg.h"
#include <KexComm.h>

BOOL WINAPI DllMain(
	IN	HMODULE		DllHandle,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllHandle);
	}

	return TRUE;
}