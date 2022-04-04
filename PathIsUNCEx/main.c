#include <Windows.h>
#include <StdArg.h>
#include <KexComm.h>
#include <NtDll.h>

#pragma warning(disable:4995) // shut the fuck up dude
#pragma warning(disable:4996)

#undef cwprintf

VOID cwprintf(
	IN	LPCWSTR	lpszFmt, ...)
{
	INT cch;
	LPWSTR lpszBuf;
	DWORD dwDiscard;
	va_list ap;
	va_start(ap, lpszFmt);

	cch = vscprintf(lpszFmt, ap) + 1;
	lpszBuf = (LPWSTR) StackAlloc(cch * sizeof(WCHAR));
	_vswprintf(lpszBuf, lpszFmt, ap);
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), lpszBuf, cch - 1, &dwDiscard, NULL);
	va_end(ap);
}

VOID EntryPoint(
	VOID)
{
	LPWSTR lpszCommandLine = GetCommandLine();
	LPWSTR lpszServer;

	HMODULE hMod = GetModuleHandle(L"kernelbase");
	BOOL (WINAPI *PathIsUNCEx)(PCWSTR, PCWSTR *) = (BOOL (WINAPI *)(PCWSTR, PCWSTR *)) GetProcAddress(hMod, "PathIsUNCEx");

	until (*lpszCommandLine == ' ' or *lpszCommandLine == '\0') {
		lpszCommandLine++;
	}

	lpszCommandLine += 2;
	cwprintf(L"lpszCommandLine = %s\n", lpszCommandLine);
	cwprintf(L"PathIsUNCEx(lpszCommandLine, &lpszServer) = %s\n", PathIsUNCEx(lpszCommandLine, &lpszServer) ? L"TRUE" : L"FALSE");
	cwprintf(L"lpszServer = %s\n", lpszServer);
	
	ExitProcess(0);
}