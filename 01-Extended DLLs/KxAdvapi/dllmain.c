#include "buildcfg.h"
#include "kxadvapip.h"

PKEX_PROCESS_DATA KexData = NULL;

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