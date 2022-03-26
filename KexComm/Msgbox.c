#include <Windows.h>
#include <KexComm.h>

#ifdef _DEBUG
TCHAR szFriendlyAppName[64] = T("YOU MUST CALL SetFriendlyAppName()");
#else
TCHAR szFriendlyAppName[64];
#endif

VOID SetFriendlyAppName(
	IN	LPCTSTR	lpszFriendlyName)
{
	strcpy_s(szFriendlyAppName, ARRAYSIZE(szFriendlyAppName), lpszFriendlyName);
}

LPCTSTR GetLastErrorAsString(
	VOID)
{
	static TCHAR lpszErrMsg[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), 0, lpszErrMsg,
		ARRAYSIZE(lpszErrMsg) - 1, NULL);
	return lpszErrMsg;
}

VOID MessageBoxV(
	IN	UINT	uType	OPTIONAL,
	IN	LPCTSTR	lpszFmt,
	IN	va_list	ap)
{
	SIZE_T cch = vscprintf(lpszFmt, ap) + 1;
	LPTSTR lpText = (LPTSTR) malloc(cch * sizeof(TCHAR));
	vsprintf_s(lpText, cch, lpszFmt, ap);
	MessageBox(NULL, lpText, szFriendlyAppName, uType);
	free(lpText);
}

VOID ErrorBoxF(
	IN	LPCTSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONERROR | MB_OK, lpszFmt, ap);
	va_end(ap);
}

NORETURN VOID CriticalErrorBoxF(
	IN	LPCTSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONERROR | MB_OK, lpszFmt, ap);
	va_end(ap);
	ExitProcess(0);
}

VOID InfoBoxF(
	IN	LPCTSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONINFORMATION | MB_OK, lpszFmt, ap);
	va_end(ap);
}