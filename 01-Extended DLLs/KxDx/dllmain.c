#include "buildcfg.h"
#include "kxdxp.h"

PKEX_PROCESS_DATA KexData = NULL;

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllBase);

		KexDataInitialize(&KexData);
		KexLogDebugEvent(L"DllMain called with DLL_PROCESS_ATTACH");
	}

	return TRUE;
}