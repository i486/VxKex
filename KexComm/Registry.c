#include <Windows.h>
#include <KexComm.h>

BOOL RegReadSz(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	OUT	LPTSTR	lpszData,
	IN	DWORD	dwcchData)
{
	HKEY hSubKey;
	LSTATUS lStatus;

	dwcchData *= sizeof(TCHAR);
	lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);
	if (lStatus != ERROR_SUCCESS) goto Error;
	lStatus = RegQueryValueEx(hSubKey, lpszValueName, 0, NULL, (LPBYTE) lpszData, &dwcchData);
	if (lStatus != ERROR_SUCCESS) goto Error;

	RegCloseKey(hSubKey);
	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegWriteSz(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	IN	LPCTSTR	lpszData OPTIONAL)
{
	HKEY hSubKey;
	LSTATUS lStatus;

	lStatus = RegCreateKeyEx(hKey, lpszSubKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hSubKey, NULL);
	if (lStatus != ERROR_SUCCESS) goto Error;
	lStatus = RegSetValueEx(hSubKey, lpszValueName, 0, REG_SZ, (LPBYTE) lpszData, (DWORD) (strlen(lpszData) + 1) * sizeof(TCHAR));
	if (lStatus != ERROR_SUCCESS) goto Error;

	RegCloseKey(hSubKey);
	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegReadDw(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	OUT	LPDWORD	lpdwData)
{
	HKEY hSubKey;
	LSTATUS lStatus;
	DWORD dwcbData = sizeof(DWORD);

	lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_QUERY_VALUE, &hSubKey);
	if (lStatus != ERROR_SUCCESS) goto Error;
	lStatus = RegQueryValueEx(hSubKey, lpszValueName, 0, NULL, (LPBYTE) lpdwData, &dwcbData);
	if (lStatus != ERROR_SUCCESS) goto Error;

	RegCloseKey(hSubKey);
	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegWriteDw(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	IN	DWORD	dwData)
{
	HKEY hSubKey;
	LSTATUS lStatus;

	lStatus = RegCreateKeyEx(hKey, lpszSubKey, 0, NULL, 0, KEY_SET_VALUE, NULL, &hSubKey, NULL);
	if (lStatus != ERROR_SUCCESS) goto Error;
	lStatus = RegSetValueEx(hSubKey, lpszValueName, 0, REG_DWORD, (LPBYTE) &dwData, sizeof(DWORD));
	if (lStatus != ERROR_SUCCESS) goto Error;

	RegCloseKey(hSubKey);
	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}

BOOL RegDelValue(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName)
{
	HKEY hSubKey;
	LSTATUS lStatus;

	lStatus = RegOpenKeyEx(hKey, lpszSubKey, 0, KEY_SET_VALUE, &hSubKey);
	if (lStatus != ERROR_SUCCESS) goto Error;
	lStatus = RegDeleteValue(hSubKey, lpszValueName);
	if (lStatus != ERROR_SUCCESS) goto Error;

	RegCloseKey(hSubKey);
	return TRUE;

Error:
	SetLastError(lStatus);
	return FALSE;
}