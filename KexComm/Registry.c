#include <Windows.h>
#include <KexComm.h>

BOOL RegReadSz(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey OPTIONAL,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	OUT	LPWSTR	lpszData,
	IN	DWORD	dwcchData)
{
	HKEY hSubKey = hKey;
	LSTATUS lStatus;

	dwcchData *= sizeof(WCHAR);

	if (lpszSubKey) {
		CHECKED((lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey)) == ERROR_SUCCESS);
	}

	CHECKED((lStatus = RegQueryValueEx(hSubKey, lpszValueName, 0, NULL, (LPBYTE) lpszData, &dwcchData)) == ERROR_SUCCESS);

	if (lpszSubKey) {
		RegCloseKey(hSubKey);
	}

	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegWriteSz(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey OPTIONAL,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	IN	LPCWSTR	lpszData OPTIONAL)
{
	HKEY hSubKey = hKey;
	LSTATUS lStatus;

	if (lpszSubKey) {
		CHECKED((lStatus = RegCreateKeyEx(hKey, lpszSubKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hSubKey, NULL)) == ERROR_SUCCESS);
	}

	CHECKED((lStatus = RegSetValueEx(hSubKey, lpszValueName, 0, REG_SZ, (LPBYTE) lpszData, (DWORD) (wcslen(lpszData) + 1) * sizeof(WCHAR))) == ERROR_SUCCESS);

	if (lpszSubKey) {
		RegCloseKey(hSubKey);
	}

	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegReadDw(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey OPTIONAL,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	OUT	LPDWORD	lpdwData)
{
	HKEY hSubKey = hKey;
	LSTATUS lStatus;
	DWORD dwcbData = sizeof(DWORD);

	if (lpszSubKey) {
		CHECKED((lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey)) == ERROR_SUCCESS);
	}

	CHECKED((lStatus = RegQueryValueEx(hSubKey, lpszValueName, 0, NULL, (LPBYTE) lpdwData, &dwcbData)) == ERROR_SUCCESS);

	if (lpszSubKey) {
		RegCloseKey(hSubKey);
	}

	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegWriteDw(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey OPTIONAL,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	IN	DWORD	dwData)
{
	HKEY hSubKey = hKey;
	LSTATUS lStatus;

	if (lpszSubKey) {
		lStatus = RegCreateKeyEx(hKey, lpszSubKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hSubKey, NULL);
	}

	CHECKED((lStatus = RegSetValueEx(hSubKey, lpszValueName, 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD))) == ERROR_SUCCESS);

	if (lpszSubKey) {
		RegCloseKey(hSubKey);
	}

	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegDelValue(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey OPTIONAL,
	IN	LPCWSTR	lpszValueName)
{
	HKEY hSubKey = hKey;
	LSTATUS lStatus;

	if (lpszSubKey) {
		CHECKED((lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey)) == ERROR_SUCCESS);
	}

	CHECKED((lStatus = RegDeleteValue(hSubKey, lpszValueName)) == ERROR_SUCCESS);

	if (lpszSubKey) {
		RegCloseKey(hSubKey);
	}

	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}