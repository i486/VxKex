#include <KexComm.h>

//
// Convenience Routines for reading and writing the memory of another process.
//

// define this if you need to trace VaRead and VaWrite. Most of the time
// this just produces massive amounts of logs, but sometimes you need it for
// debugging.
//#define ENABLE_VA_LOGGING

VOID LogF(
	IN	LPCWSTR lpszFmt, ...);

extern HANDLE g_hProc;

VOID VaRead(
	IN	ULONG_PTR	ulSrc,
	OUT	LPVOID		lpDst,
	IN	SIZE_T		nSize)
{
	if (ReadProcessMemory(g_hProc, (LPCVOID) ulSrc, lpDst, nSize, NULL) == FALSE) {
		if (GetLastError() != ERROR_PARTIAL_COPY) {
			CriticalErrorBoxF(L"Failed to read process memory at offset %p: %s",
							  ulSrc, GetLastErrorAsString());
		}
	}

#ifdef _DEBUG
#ifdef ENABLE_VA_LOGGING
	switch (nSize) {
	case 1:
		LogF(L"[VA] VaRead ulSrc=%p; *lpDst=0x%02hx ('%hc'); nSize=%Iu\r\n", ulSrc, *(LPBYTE) lpDst, isprint(*(CHAR*) lpDst) ? *(CHAR*) lpDst : ' ', nSize); break;
	case 2:
		LogF(L"[VA] VaRead ulSrc=%p; *lpDst=0x%04hx ('%wc'); nSize=%Iu\r\n", ulSrc, *(LPWORD) lpDst, iswprint(*(WCHAR*) lpDst) ? *(WCHAR*) lpDst : ' ', nSize); break;
	case 4:
		LogF(L"[VA] VaRead ulSrc=%p; *lpDst=0x%08I32x; nSize=%Iu\r\n", ulSrc, *(LPDWORD) lpDst, nSize); break;
	case 8:
		LogF(L"[VA] VaRead ulSrc=%p; *lpDst=0x%016I64x; nSize=%Iu\r\n", ulSrc, *(LPQWORD) lpDst, nSize); break;
	default:
		LogF(L"[VA] VaRead ulSrc=%p; lpDst=%p; nSize=%Iu\r\n", ulSrc, lpDst, nSize); break;
	}
#endif
#endif
}

VOID VaWrite(
	IN	ULONG_PTR	ulDst,
	IN	LPCVOID		lpSrc,
	IN	SIZE_T		nSize)
{
	DWORD dwOldProtect;

#ifdef _DEBUG
#ifdef ENABLE_VA_LOGGING
	switch (nSize) {
	case 1:
		LogF(L"[VA] VaWrite ulDst=%p; *lpSrc=0x%02hx ('%hc'); nSize=%Iu\r\n", ulDst, *(LPBYTE) lpSrc, isprint(*(CHAR*) lpSrc) ? *(CHAR*) lpSrc : ' ', nSize); break;
	case 2:
		LogF(L"[VA] VaWrite ulDst=%p; *lpSrc=0x%04hx ('%wc'); nSize=%Iu\r\n", ulDst, *(LPWORD) lpSrc, iswprint(*(WCHAR*) lpSrc) ? *(WCHAR*) lpSrc : ' ', nSize); break;
	case 4:
		LogF(L"[VA] VaWrite ulDst=%p; *lpSrc=0x%08I32x; nSize=%Iu\r\n", ulDst, *(LPDWORD) lpSrc, nSize); break;
	case 8:
		LogF(L"[VA] VaWrite ulDst=%p; *lpSrc=0x%016I64x; nSize=%Iu\r\n", ulDst, *(LPQWORD) lpSrc, nSize); break;
	default:
		LogF(L"[VA] VaWrite ulDst=%p; lpSrc=%p; nSize=%Iu\r\n", ulDst, lpSrc, nSize); break;
	}
#endif
#endif

	if (VirtualProtectEx(g_hProc, (LPVOID) ulDst, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect) == FALSE) {
		CriticalErrorBoxF(L"Failed to set access protections on process: %s", GetLastErrorAsString());
	}

	if (WriteProcessMemory(g_hProc, (LPVOID) ulDst, lpSrc, nSize, NULL) == FALSE) {
		CriticalErrorBoxF(L"Failed to write process memory at offset %p with %llu bytes of data: %s",
						  ulDst, nSize, GetLastErrorAsString());
	}

	if (VirtualProtectEx(g_hProc, (LPVOID) ulDst, nSize, dwOldProtect, &dwOldProtect) == FALSE) {
		CriticalErrorBoxF(L"Failed to un-set access protections on process: %s", GetLastErrorAsString());
	}
}

VOID VaWriteByte(
	IN	ULONG_PTR	ulDst,
	IN	BYTE		btData)
{
	VaWrite(ulDst, &btData, sizeof(BYTE));
}

VOID VaWriteWord(
	IN	ULONG_PTR	ulDst,
	IN	WORD		wData)
{
	VaWrite(ulDst, &wData, sizeof(WORD));
}

VOID VaWriteDword(
	IN	ULONG_PTR	ulDst,
	IN	DWORD		dwData)
{
	VaWrite(ulDst, &dwData, sizeof(DWORD));
}

VOID VaWriteQword(
	IN	ULONG_PTR	ulDst,
	IN	QWORD		qwData)
{
	VaWrite(ulDst, &qwData, sizeof(QWORD));
}

VOID VaWritePtr(
	IN	ULONG_PTR	ulDst,
	IN	ULONG_PTR	ulData)
{
	VaWrite(ulDst, &ulData, sizeof(ULONG_PTR));
}

// write the ANSI string including the ending 0 byte
VOID VaWriteSzA(
	IN	ULONG_PTR	ulDst,
	IN	LPCWSTR		lpszSrc)
{
	SIZE_T cch = wcslen(lpszSrc) + 1;
#ifdef UNICODE
	CHAR szAnsiString[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, lpszSrc, -1, szAnsiString, sizeof(szAnsiString), NULL, NULL);
	VaWrite(ulDst, szAnsiString, cch);
#else
	VaWrite(ulDst, lpszSrc, cch);
#endif
}

BYTE VaReadByte(
	IN	ULONG_PTR	ulSrc)
{
	BYTE btRet;
	VaRead(ulSrc, &btRet, sizeof(BYTE));
	return btRet;
}

WORD VaReadWord(
	IN	ULONG_PTR	ulSrc)
{
	WORD wRet;
	VaRead(ulSrc, &wRet, sizeof(WORD));
	return wRet;
}

DWORD VaReadDword(
	IN	ULONG_PTR	ulSrc)
{
	DWORD dwRet;
	VaRead(ulSrc, &dwRet, sizeof(DWORD));
	return dwRet;
}

QWORD VaReadQword(
	IN	ULONG_PTR	ulSrc)
{
	QWORD qwRet;
	VaRead(ulSrc, &qwRet, sizeof(QWORD));
	return qwRet;
}

ULONG_PTR VaReadPtr(
	IN	ULONG_PTR	ulSrc)
{
	ULONG_PTR ulRet;
	VaRead(ulSrc, &ulRet, sizeof(ULONG_PTR));
	return ulRet;
}

VOID VaReadSzA(
	IN	ULONG_PTR	ulSrc,
	IN	LPSTR		lpszDest,
	IN	DWORD		dwcchDest)
{
	DWORD i = 0;
	while (i < (dwcchDest - 1) && (lpszDest[i++] = VaReadByte(ulSrc++)));
}

VOID VaReadSzW(
	IN	ULONG_PTR	ulSrc,
	IN	LPWSTR		lpszDest,
	IN	DWORD		dwcchDest)
{
	DWORD i = 0;
	while (i < (dwcchDest - 1) && (lpszDest[i++] = VaReadWord(ulSrc))) ulSrc += 2;
}