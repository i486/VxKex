#include <KexComm.h>
#include <NtDll.h>

//
// This program is a proof of concept of using LdrRegisterDllNotification to track
// DLL loads in-process (i.e. without needing an external debugger process).
// Ldr(Un)RegisterDllNotification is a stable API that has stayed the same from Windows
// XP to Windows 8 (and probably earlier/later as well, although I haven't checked), so
// the same concepts can be applied to other Windows versions as well as 7.
//
// The order and semantics of these DLL load notifications seem to be identical to those
// received in an external debugger process, which makes sense, since this DLL
// notification mechanism is probably used to send signals to debuggers.
//

VOID PrintF(
	IN	LPCWSTR lpszFmt, ...)
{
	va_list ap;
	SIZE_T cch;
	LPWSTR lpszText;
	DWORD dwDiscard;
	va_start(ap, lpszFmt);
	cch = vscwprintf(lpszFmt, ap) + 1;
	lpszText = (LPWSTR) StackAlloc(cch * sizeof(WCHAR));
	vswprintf_s(lpszText, cch, lpszFmt, ap);
	WriteConsole(NtCurrentPeb()->ProcessParameters->StandardOutput, lpszText, (DWORD) cch - 1, &dwDiscard, NULL);
	va_end(ap);
}

VOID NTAPI DllNotificationFunction(
	IN	LDR_DLL_NOTIFICATION_REASON	NotificationReason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context)
{
	PrintF(L"DLL notification received!\r\n");
	PrintF(L"    NotificationReason: %s\r\n", NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED ? L"Loaded" : L"Unloaded");
	PrintF(L"    NotificationData:\r\n");
	PrintF(L"        Flags = %u\r\n", NotificationData->Flags);
	PrintF(L"        FullDllName = %ws\r\n", NotificationData->FullDllName->Buffer);
	PrintF(L"        BaseDllName = %ws\r\n", NotificationData->BaseDllName->Buffer);
	PrintF(L"        DllBase = 0x%p\r\n", NotificationData->DllBase);
	PrintF(L"        SizeOfImage = %I32u\r\n", NotificationData->SizeOfImage);
}

VOID EntryPoint(
	VOID)
{
	PVOID Cookie;
	NTSTATUS st;
	HMODULE hNtdll;
	HMODULE hD3d9;
	HMODULE hDdraw;

	PrintF(L"Registering DLL notification function: NTSTATUS = ");
	st = LdrRegisterDllNotification(0, DllNotificationFunction, NULL, &Cookie);
	PrintF(L"%#010I32x\r\n", st);

	PrintF(L"LoadLibrary ntdll\r\n");
	hNtdll = LoadLibrary(L"ntdll.dll");
	PrintF(L"LoadLibrary d3d9\r\n");
	hD3d9 = LoadLibrary(L"d3d9.dll");
	PrintF(L"LoadLibrary ddraw\r\n");
	hDdraw = LoadLibrary(L"ddraw.dll");

	PrintF(L"FreeLibrary d3d9\r\n");
	FreeLibrary(hD3d9);
	PrintF(L"FreeLibrary ntdll\r\n");
	FreeLibrary(hNtdll);
	PrintF(L"FreeLibrary ddraw\r\n");
	FreeLibrary(hDdraw);

	Sleep(INFINITE);
}