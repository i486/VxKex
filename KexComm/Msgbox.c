#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>

#ifdef _DEBUG
WCHAR szFriendlyAppName[64] = L"YOU MUST CALL SetFriendlyAppName()";
#else
WCHAR szFriendlyAppName[64];
#endif

VOID SetFriendlyAppName(
	IN	LPCWSTR	lpszFriendlyName)
{
	wcscpy_s(szFriendlyAppName, ARRAYSIZE(szFriendlyAppName), lpszFriendlyName);
}

LPCWSTR GetLastErrorAsString(
	VOID)
{
	static WCHAR lpszErrMsg[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), 0, lpszErrMsg,
		ARRAYSIZE(lpszErrMsg) - 1, NULL);
	return lpszErrMsg;
}

VOID MessageBoxV(
	IN	UINT	uType	OPTIONAL,
	IN	LPCWSTR	lpszFmt,
	IN	va_list	ap)
{
	SIZE_T cch = vscwprintf(lpszFmt, ap) + 1;
	LPWSTR lpText = (LPWSTR) StackAlloc(cch * sizeof(WCHAR));
	vswprintf_s(lpText, cch, lpszFmt, ap);
	MessageBox(NULL, lpText, szFriendlyAppName, uType);
}

VOID ErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONERROR | MB_OK, lpszFmt, ap);
	va_end(ap);
}

NORETURN VOID CriticalErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONERROR | MB_OK, lpszFmt, ap);
	va_end(ap);
	ExitProcess(0);
}

VOID InfoBoxF(
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONINFORMATION | MB_OK, lpszFmt, ap);
	va_end(ap);
}