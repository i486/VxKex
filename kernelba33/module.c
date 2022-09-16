#include <Windows.h>
#include <Shlwapi.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

#include "module.h"

#pragma warning(disable:4995) // muh deprecated function
WINBASEAPI VOID WINAPI KexNormalizeDllNameW(
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
WINBASEAPI BOOL WINAPI KexRewriteDllNameW(
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
	ODS_ENTRY(L"(\"%ws\", %p, 0x%I32x)", lpszLibFileName, hFile, dwFlags);
	KexNormalizeDllNameW(lpszLibFileName, szBaseNameWithExt);

	// Get rid of LOAD_LIBRARY_REQUIRE_SIGNED_TARGET and LOAD_LIBRARY_SAFE_CURRENT_DIRS
	// flags, which require windows 8.1+ and windows 10
	dwFlags &= ~(LOAD_LIBRARY_REQUIRE_SIGNED_TARGET | LOAD_LIBRARY_SAFE_CURRENT_DIRS);

	if (KexRewriteDllNameW(szBaseNameWithExt)) {
		// If this is a vxkex DLL, we don't want to respect the original dwFlags parameter.
		// The LOAD_LIBRARY_SEARCH_*** flags will not load DLLs out of the PATH environment
		// variable.
		dwFlags &= ~(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
					 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
					 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
					 LOAD_LIBRARY_SEARCH_SYSTEM32 |
					 LOAD_LIBRARY_SEARCH_USER_DIRS);

		ODS(L"%s -> %s (0x%I32d)", lpszLibFileName, szBaseNameWithExt, dwFlags);
		return LoadLibraryExW(szBaseNameWithExt, hFile, dwFlags);
	} else {
		LPVOID (WINAPI *AddDllDirectory)(LPCWSTR) = (LPVOID (WINAPI *)(LPCWSTR)) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "AddDllDirectory");
		BOOL (WINAPI *RemoveDllDirectory)(LPVOID) = (BOOL (WINAPI *)(LPVOID)) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "RemoveDllDirectory");
		LPVOID lpCookie = NULL;
		HMODULE result;

		// If any of the pesky LOAD_LIBRARY_SEARCH_*** flags are set, we need to ensure that
		// LOAD_LIBRARY_SEARCH_USER_DIRS is set, and also temporarily add the KexDLL directory
		// to the search path.

		if (dwFlags & (LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
					   LOAD_LIBRARY_SEARCH_DEFAULT_DIRS |
					   LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR |
					   LOAD_LIBRARY_SEARCH_SYSTEM32 |
					   LOAD_LIBRARY_SEARCH_USER_DIRS)) {
			WCHAR szKexDllDir[MAX_PATH];
#ifdef _WIN64
			StringCchPrintf(szKexDllDir, ARRAYSIZE(szKexDllDir), L"%s\\Kex64", KexData->KexDir);
#else
			StringCchPrintf(szKexDllDir, ARRAYSIZE(szKexDllDir), L"%s\\Kex32", KexData->KexDir);
#endif
			dwFlags |= LOAD_LIBRARY_SEARCH_USER_DIRS;

			if (AddDllDirectory != NULL) {
				ODS(L"Found LOAD_LIBRARY_SEARCH_*** in dwFlags - adding temporary KexDLL directory to search path");
				lpCookie = AddDllDirectory(szKexDllDir);

				if (lpCookie == NULL) {
					ODS(L"Returned cookie from AddDllDirectory is NULL");
				}
			} else {
				ODS(L"Unable to call AddDllDirectory()");
			}
		}

		result = LoadLibraryExW(lpszLibFileName, hFile, dwFlags);

		if (lpCookie != NULL) {
			if (RemoveDllDirectory != NULL) {
				ODS(L"Removing temporary KexDLL directory");
				RemoveDllDirectory(lpCookie);
			} else {
				ODS(L"Unable to call RemoveDllDirectory()");
			}
		}

		return result;
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

WINBASEAPI HMODULE WINAPI PROXY_FUNCTION(GetModuleHandleA) (
	IN	LPCSTR	lpszModuleName OPTIONAL)
{
	HMODULE hMod;
	ODS_ENTRY(L"(\"%hs\")", lpszModuleName);
	P_GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpszModuleName, &hMod);
	return hMod;
}

WINBASEAPI FARPROC WINAPI PROXY_FUNCTION(GetProcAddress) (
	IN	HMODULE	hMod,
	IN	LPCSTR	lpszProcName)
{
	if (IsBadReadPtr(lpszProcName, 1)) {
		ODS_ENTRY(L"(%p, %p)", hMod, lpszProcName);
	} else {
		ODS_ENTRY(L"(%p, \"%hs\")", hMod, lpszProcName);
	}

	return GetProcAddress(hMod, lpszProcName);
}

// This function is particularly problematic because it stops our KexDLLs from getting
// loaded, if it is called.
// Microsoft documentation states:
//
//   It is not possible to revert to the standard DLL search path
//   or remove any directory specified with SetDefaultDllDirectories
//   from the search path.
//
// So our strategy will be: if this function is called, add LOAD_LIBRARY_SEARCH_USER_DIRS
// to the dwFlags, use AddDllDirectory to add Kex32/Kex64 directories to the search path,
// and that way the pesky application cannot avoid loading our DLLs anymore. Wa la.
WINBASEAPI BOOL WINAPI PROXY_FUNCTION(SetDefaultDllDirectories) (
	IN	DWORD	dwFlags)
{
	LPVOID (*AddDllDirectory)(LPCWSTR) = (LPVOID (*)(LPCWSTR)) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "AddDllDirectory");

	ODS_ENTRY(L"(0x%I32x)", dwFlags);
	dwFlags |= LOAD_LIBRARY_SEARCH_USER_DIRS;

	if (AddDllDirectory != NULL) {
		WCHAR szKexDllDir[MAX_PATH];
#ifdef _WIN64
		StringCchPrintf(szKexDllDir, ARRAYSIZE(szKexDllDir), L"%s\\Kex64", KexData->KexDir);
#else
		StringCchPrintf(szKexDllDir, ARRAYSIZE(szKexDllDir), L"%s\\Kex32", KexData->KexDir);
#endif
		if (AddDllDirectory(szKexDllDir) != NULL) {		
			ODS(L"Added KexDLL directory \"%s\" to DLL search path", szKexDllDir);
		} else {
			ODS(L"AddDllDirectory() returned NULL");
		}
	} else {
		ODS(L"Unable to call AddDllDirectory()");
	}

	return TRUE;
}