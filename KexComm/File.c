#include <Windows.h>
#include <Psapi.h>
#include <KexComm.h>
#include <NtDll.h>
#include <BaseDll.h>

BOOLEAN PathReplaceIllegalCharacters(
	IN	PWSTR	Path,
	IN	WCHAR	ReplacementCharacter,
	IN	BOOLEAN	AllowPathSeparators)
{
	BOOLEAN AtLeastOneCharacterWasReplaced;

	if (!Path) {
		return FALSE;
	}

	AtLeastOneCharacterWasReplaced = FALSE;

	while (*Path != '\0') {
		switch (*Path) {
		case '<':
		case '>':
		case ':':
		case '"':
		case '|':
		case '?':
		case '*':
			*Path = ReplacementCharacter;
			AtLeastOneCharacterWasReplaced = TRUE;
			break;
		case '/':
		case '\\':
			unless (AllowPathSeparators) {
				*Path = ReplacementCharacter;
				AtLeastOneCharacterWasReplaced = TRUE;
			}
			break;
		}

		Path++;
	}

	return AtLeastOneCharacterWasReplaced;
}

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

HANDLE CreateTempFile(
	IN	LPCWSTR					lpszPrefix,
	IN	DWORD					dwDesiredAccess,
	IN	DWORD					dwShareMode,
	IN	LPSECURITY_ATTRIBUTES	lpSecurityAttributes)
{
	static WCHAR szTempDir[MAX_PATH];
	static WCHAR szTempPath[MAX_PATH];
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
	lpszBuf = (LPWSTR) AutoAlloc(cb);
	vswprintf_s(lpszBuf, cch, lpszFmt, ap);
	bResult = WriteFile(hFile, lpszBuf, cb - sizeof(WCHAR), &dwDiscard, NULL);
	dwError = GetLastError();
	
	AutoFree(lpszBuf);
	va_end(ap);
	SetLastError(dwError);
	return bResult;
}

LPWSTR ConvertDeviceHarddiskToDosPath(
	IN	LPWSTR lpszPath)
{
	NTSTATUS st;
	WORD wBit = 25;
	DWORD dwLogicalDrives;
	PROCESS_DEVICEMAP_INFORMATION pdi;
	DWORD dwSize = sizeof(pdi);

	st = NtQueryInformationProcess(
		GetCurrentProcess(),
		ProcessDeviceMap,
		&pdi,
		sizeof(pdi),
		NULL);

	if (!NT_SUCCESS(st)) {
		BaseSetLastNTError(st);
		return NULL;
	}

	dwLogicalDrives = pdi.Query.DriveMap;

	do {
		if (dwLogicalDrives & (1 << wBit)) {
			WCHAR szNtDevice[64];
			WCHAR szDosDevice[3] = {'A' + wBit, ':', '\0'};
			SIZE_T cchNtDevice;

			if (QueryDosDevice(szDosDevice, szNtDevice, ARRAYSIZE(szNtDevice))) {
				cchNtDevice = wcslen(szNtDevice);

				if (!wcsnicmp(szNtDevice, lpszPath, cchNtDevice)) {
					LPWSTR lpszDosPath = lpszPath + (cchNtDevice - 2);
					lpszDosPath[0] = szDosDevice[0];
					lpszDosPath[1] = szDosDevice[1];
					return lpszDosPath;
				}
			}
		}

		wBit--;
	} until (wBit == 0);

	return NULL;
}

LPWSTR GetFilePathFromHandle(
	IN	HANDLE	hFile)
{
	HANDLE hMapping = NULL;
	LPVOID lpMapping = NULL;
	STATIC WCHAR szPath[MAX_PATH + 32];

	CHECKED(hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL));
	CHECKED(lpMapping = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 1));
	CHECKED(GetMappedFileName(GetCurrentProcess(), lpMapping, szPath, ARRAYSIZE(szPath)));

	// At this point, szPath contains a path like \Device\HarddiskVolume7\Windows\system32\ntdll.dll
	// It must be converted into a normal path like C:\Windows\system32\ntdll.dll

	return ConvertDeviceHarddiskToDosPath(szPath);

Error:
	if (lpMapping) {
		UnmapViewOfFile(lpMapping);
	}

	if (hMapping) {
		NtClose(hMapping);
	}

	return NULL;
}