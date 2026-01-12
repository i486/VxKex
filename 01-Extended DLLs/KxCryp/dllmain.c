#include "buildcfg.h"
#include "kxcrypp.h"

PKEX_PROCESS_DATA KexData;

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