#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

VOID DllMain_HookCreateProcess(
	VOID);

VOID DllMain_InitWoa(
	VOID);

BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
		DllMain_HookCreateProcess();
		DllMain_InitWoa();
	}

	return TRUE;
}