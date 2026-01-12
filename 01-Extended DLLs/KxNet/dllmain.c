#include "buildcfg.h"
#include <KexComm.h>

PKEX_PROCESS_DATA KexData = NULL;

BOOL WINAPI DllMain(
	IN	HMODULE		DllHandle,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllHandle);
		KexDataInitialize(&KexData);
	}

	return TRUE;
}