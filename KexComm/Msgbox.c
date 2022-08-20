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

LPCWSTR Win32ErrorAsString(
	IN	DWORD	dw)
{
	static WCHAR lpszErrMsg[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dw, 0, lpszErrMsg,
		ARRAYSIZE(lpszErrMsg) - 1, NULL);
	return lpszErrMsg;
}

LPCWSTR NtStatusAsString(
	IN	NTSTATUS st)
{
	return Win32ErrorAsString(RtlNtStatusToDosError(st));
}

LPCWSTR GetLastErrorAsString(
	VOID)
{
	return Win32ErrorAsString(GetLastError());
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

VOID WarningBoxF(
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONWARNING | MB_OK, lpszFmt, ap);
	va_end(ap);
}

VOID InfoBoxF(
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	va_start(ap, lpszFmt);
	MessageBoxV(MB_ICONINFORMATION | MB_OK, lpszFmt, ap);
	va_end(ap);
}