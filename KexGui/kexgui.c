#include "buildcfg.h"
#include <KexComm.h>

#ifdef _DEBUG
KEXGDECLSPEC PCWSTR KexgApplicationFriendlyName = L"YOU MUST SET AN APPLICATION FRIENDLY NAME";
#else
KEXGDECLSPEC PCWSTR KexgApplicationFriendlyName = L"";
#endif

KEXGDECLSPEC HWND KexgApplicationMainWindow = NULL;

#ifdef KEX_TARGET_TYPE_DLL
BOOL WINAPI DllMain(
	IN	HMODULE		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(DllBase);
	}

	return TRUE;
}
#endif