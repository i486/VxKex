#include <KexComm.h>
#include <NtDll.h>
#include "VxKexLdr.h"

#include <Shlwapi.h>

//
// Utility functions
//

HANDLE KexOpenLogFile(
	IN	LPCWSTR	lpszExeBaseName)
{
	HANDLE hLogFile;
	WCHAR szLogFileName[MAX_PATH] = L"";

	CHECKED(GetTempPath(ARRAYSIZE(szLogFileName), szLogFileName));
	CHECKED(SUCCEEDED(StringCchCat(szLogFileName, ARRAYSIZE(szLogFileName), L"VxKexLog\\")));

	CreateDirectory(szLogFileName, NULL);

	CHECKED(SUCCEEDED(StringCchCat(szLogFileName, ARRAYSIZE(szLogFileName), lpszExeBaseName)));
	CHECKED(PathRenameExtension(szLogFileName, L".log"));

	hLogFile = CreateFile(
		szLogFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL);

	CHECKED(hLogFile != INVALID_HANDLE_VALUE);
	return hLogFile;

Error:
#ifdef _DEBUG
	WarningBoxF(L"Unable to open the log file %s: %s", szLogFileName, GetLastErrorAsString());
#endif // ifdef _DEBUG

	return NULL;
}

VOID LogF(
	IN	LPCWSTR lpszFmt, ...)
{
	SIZE_T cch;
	LPWSTR lpszText;
	DWORD dwDiscard;

	if (g_hLogFile || g_KexData.IfeoParameters.dwAlwaysShowDebug) {
		va_list ap;
		va_start(ap, lpszFmt);
		cch = vscwprintf(lpszFmt, ap) + 1;
		lpszText = (LPWSTR) StackAlloc(cch * sizeof(WCHAR));
		vswprintf_s(lpszText, cch, lpszFmt, ap);
		va_end(ap);
	}

	WriteConsole(NtCurrentPeb()->ProcessParameters->StandardOutput, lpszText, (DWORD) cch - 1, &dwDiscard, NULL);

	if (g_hLogFile) {
		WriteFile(g_hLogFile, lpszText, (DWORD) (cch - 1) * sizeof(WCHAR), &dwDiscard, NULL);
	}
}

VOID Pause(
	VOID)
{
	HWND hWndConsole = GetConsoleWindow();
	if (hWndConsole && IsWindowVisible(hWndConsole)) {
		PrintF(L"\r\nYou may now close the console window.");
		Sleep(INFINITE);
	}
}

NORETURN VOID Exit(
	IN	DWORD	dwExitCode)
{
	if (g_hLogFile) {
		NtClose(g_hLogFile);
	}

	Pause();
	ExitProcess(dwExitCode);
}

BOOL ShouldAllocConsole(
	VOID)
{
	DWORD dwShouldAllocConsole = FALSE;
	RegReadDw(HKEY_CURRENT_USER, L"SOFTWARE\\VXsoft\\VxKexLdr", L"ShowDebugInfoByDefault", &dwShouldAllocConsole);
	return dwShouldAllocConsole || g_KexData.IfeoParameters.dwAlwaysShowDebug;
}