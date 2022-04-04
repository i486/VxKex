#include <Windows.h>
#include <NtDll.h>

// This DLL gets injected into a shell, such as cmd or powershell. Prior to
// injection, the loader will suspend the shell process. The purpose of this
// DLL is to wait until the loader's child process exits and then resume the
// shell process.
//
// This simulates the desired shell behavior of waiting for a console
// application even if the loader is not a console application and therefore
// does not get this treatment.

BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	PPEB ppeb = NtCurrentPeb();
	HANDLE hChild = ppeb->Ldr->SsHandle;

	NtWaitForSingleObject(hChild, TRUE, NULL);
	NtClose(hChild);
	NtResumeProcess(GetCurrentProcess());
	
	return FALSE;
}