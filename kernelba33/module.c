#include <Windows.h>
#include <Shlwapi.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

// LoadLibrary(Ex)A/W
// GetModuleHandle(Ex)A/W

#pragma warning(disable:4995) // muh deprecated function
STATIC VOID KexNormalizeDllNameW(
	IN	LPCWSTR	lpszLibFileName,
	OUT	LPWSTR	lpszBaseNameWithExt)
{
	INT cchLibFileName = lstrlenW(lpszLibFileName);
	LPCWSTR lpszBaseName;

	if (cchLibFileName == 0) {
		// Don't modify a zero-length name
		*lpszBaseNameWithExt = '\0';
		return;
	}

	if (lpszLibFileName[cchLibFileName - 1] == '.') {
		// The file name string can include a trailing point character '.'
		// to indicate that the module name has no extension.
		lstrcpyW(lpszBaseNameWithExt, lpszLibFileName);
	}
	
	lpszBaseName = PathFindFileNameW(lpszLibFileName);
	lstrcpyW(lpszBaseNameWithExt, lpszBaseName);
	PathAddExtensionW(lpszBaseNameWithExt, L".dll");
}
#pragma warning(default:4995)

// Return TRUE if name was rewritten, FALSE otherwise.
// If the return value is FALSE, the contents of the input string are undefined.
STATIC BOOL KexRewriteDllNameW(
	IN	LPWSTR	lpszBaseNameWithExt)
{
	HKEY hKeyDllRewrite;
	LSTATUS lStatus;
	DWORD dwMaxPath = MAX_PATH;

	lStatus = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite", 0, KEY_QUERY_VALUE, &hKeyDllRewrite);

	if (lStatus != ERROR_SUCCESS) {
		return FALSE;
	}

	lStatus = RegQueryValueExW(hKeyDllRewrite, lpszBaseNameWithExt, 0, NULL, (LPBYTE) lpszBaseNameWithExt, &dwMaxPath);
	RegCloseKey(hKeyDllRewrite);
	return (lStatus == ERROR_SUCCESS);
}

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(LoadLibraryExW) (
	IN	LPCWSTR	lpszLibFileName,
	IN	HANDLE	hFile,
	IN	DWORD	dwFlags)
{
	WCHAR szBaseNameWithExt[MAX_PATH];
	ODS_ENTRY(L"(\"%ws\", %p, %I32u)", lpszLibFileName, hFile, dwFlags);
	KexNormalizeDllNameW(lpszLibFileName, szBaseNameWithExt);

	if (KexRewriteDllNameW(szBaseNameWithExt)) {
		return LoadLibraryExW(szBaseNameWithExt, hFile, dwFlags);
	} else {
		return LoadLibraryExW(lpszLibFileName, hFile, dwFlags);
	}
}

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(LoadLibraryExA) (
	IN	LPCSTR	lpszLibFileName,
	IN	HANDLE	hFile,
	IN	DWORD	dwFlags)
{
	WCHAR szLibFileName[MAX_PATH];
	ODS_ENTRY(L"(\"%hs\", %p, %I32u)", lpszLibFileName, hFile, dwFlags);
	
	if (MultiByteToWideChar(CP_ACP, 0, lpszLibFileName, -1, szLibFileName, ARRAYSIZE(szLibFileName)) == 0) {
		return NULL;
	}

	return P_LoadLibraryExW(szLibFileName, hFile, dwFlags);
}

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(LoadLibraryW) (
	IN	LPCWSTR	lpszLibFileName)
{
	ODS_ENTRY(L"(\"%ws\")", lpszLibFileName);
	return P_LoadLibraryExW(lpszLibFileName, NULL, 0);
}

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(LoadLibraryA) (
	IN	LPCSTR	lpszLibFileName)
{
	ODS_ENTRY(L"(\"%hs\")", lpszLibFileName);
	return P_LoadLibraryExA(lpszLibFileName, NULL, 0);
}

WINBASEAPI BOOL WINAPI PROXY_FUNCTION(GetModuleHandleExW) (
	IN	DWORD	dwFlags,
	IN	LPCWSTR	lpszModuleName OPTIONAL,
	OUT	HMODULE	*lphMod)
{
	WCHAR szBaseNameWithExt[MAX_PATH];
	ODS_ENTRY(L"(%I32u, \"%ws\", %p)", dwFlags, lpszModuleName, lphMod);

	if (lpszModuleName == NULL) {
		return GetModuleHandleExW(dwFlags, lpszModuleName, lphMod);
	}

	KexNormalizeDllNameW(lpszModuleName, szBaseNameWithExt);

	if (KexRewriteDllNameW(szBaseNameWithExt) && !(dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) {
		return GetModuleHandleExW(dwFlags, szBaseNameWithExt, lphMod);
	} else {
		return GetModuleHandleExW(dwFlags, lpszModuleName, lphMod);
	}
}

WINBASEAPI BOOL WINAPI PROXY_FUNCTION(GetModuleHandleExA) (
	IN	DWORD	dwFlags,
	IN	LPCSTR	lpszModuleName OPTIONAL,
	OUT	HMODULE	*lphMod)
{
	WCHAR szLibFileName[MAX_PATH];
	ODS_ENTRY(L"(%I32u, \"%hs\", %p)", dwFlags, lpszModuleName, lphMod);

	if (lpszModuleName == NULL || dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
		return GetModuleHandleExA(dwFlags, lpszModuleName, lphMod);
	}

	if (MultiByteToWideChar(CP_ACP, 0, lpszModuleName, -1, szLibFileName, ARRAYSIZE(szLibFileName)) == 0) {
#ifdef _DEBUG
		__debugbreak();
#else
		*lphMod = NULL;
		return FALSE;
#endif
	}

	return P_GetModuleHandleExW(dwFlags, szLibFileName, lphMod);
}

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(GetModuleHandleW) (
	IN	LPCWSTR	lpszModuleName OPTIONAL)
{
	HMODULE hMod;
	ODS_ENTRY(L"(\"%ws\")", lpszModuleName);
	P_GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpszModuleName, &hMod);
	return hMod;
}

HMODULE WINAPI PROXY_FUNCTION(GetModuleHandleA) (
	IN	LPCSTR	lpszModuleName OPTIONAL)
{
	HMODULE hMod;
	ODS_ENTRY(L"(\"%hs\")", lpszModuleName);
	P_GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpszModuleName, &hMod);
	return hMod;
}