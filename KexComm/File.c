#include <Windows.h>
#include <KexComm.h>

HANDLE CreateTempFile(
	IN	LPCTSTR					lpszPrefix,
	IN	DWORD					dwDesiredAccess,
	IN	DWORD					dwShareMode,
	IN	LPSECURITY_ATTRIBUTES	lpSecurityAttributes)
{
	static TCHAR szTempDir[MAX_PATH];
	static TCHAR szTempPath[MAX_PATH];
	GetTempPath(ARRAYSIZE(szTempDir), szTempDir);
	GetTempFileName(szTempDir, lpszPrefix, 0, szTempPath);
	return CreateFile(szTempPath, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
					  CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, NULL);
}

BOOL WriteFileWF(
	IN	HANDLE	hFile,
	IN	LPCWSTR	lpszFmt, ...)
{
	va_list ap;
	DWORD cch;
	DWORD cb;
	LPWSTR lpszBuf;
	BOOL bResult;
	DWORD dwDiscard;
	DWORD dwError;

	va_start(ap, lpszFmt);
	
	cch = _vscwprintf(lpszFmt, ap) + 1;
	cb = cch * sizeof(WCHAR);
	lpszBuf = (LPTSTR) AutoAlloc(cb);
	vswprintf_s(lpszBuf, cch, lpszFmt, ap);
	bResult = WriteFile(hFile, lpszBuf, cb - sizeof(WCHAR), &dwDiscard, NULL);
	dwError = GetLastError();
	
	AutoFree(lpszBuf);
	va_end(ap);
	SetLastError(dwError);
	return bResult;
}