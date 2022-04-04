#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <ctype.h>

#include "pathcch.h"

// check if path begins with \\?\ prefix and has a drive letter and colon after it
STATIC BOOL IsExtendedLengthDosDevicePath(
	IN	LPCWSTR	lpszPath)
{
	if (!wcsncmp(lpszPath, L"\\\\?\\", 4) && iswalpha(lpszPath[4]) && lpszPath[5] == ':') {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsVolumeGuidPath(
	IN	LPCWSTR	lpszPath)
{
	if (!wcsnicmp(lpszPath, L"\\\\?\\Volume{", 11) && wcslen(&lpszPath[10]) >= 38) {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsPrefixedUncPath(
	IN	LPCWSTR	lpszPath)
{
	return !wcsnicmp(lpszPath, L"\\\\?\\UNC\\", 8);
}

#if 0
WINPATHCCHAPI HRESULT WINAPI PathAllocCanonicalize(
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut)
{
	ODS_ENTRY(L"(L\"%.200ws\", %I32u, %p)", lpszPathIn, dwFlags, ppszPathOut);
	return S_OK;
}

WINPATHCCHAPI HRESULT WINAPI PathAllocCombine(
	IN	LPCWSTR	lpszPathIn,
	IN	LPCWSTR	lpszMore,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut)
{
	ODS_ENTRY(L"(L\"%.200ws\", L\"%.200ws\", %I32u, %p)", lpszPathIn, lpszMore, dwFlags, ppszPathOut);
	return S_OK;
}
#endif

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslash(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	return PathCchAddBackslashEx(lpszPath, cchPath, NULL, NULL);
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslashEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		LPWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL)
{
	LPWSTR lpszEnd;
	SIZE_T cchRemaining;
	SIZE_T cchPathLength;
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, %p, %p)", lpszPath, cchPath, ppszEnd, pcchRemaining);

	if (ppszEnd) {
		*ppszEnd = NULL;
	}

	if (pcchRemaining) {
		*pcchRemaining = 0;
	}

	hr = StringCchLength(lpszPath, cchPath, &cchPathLength);

	if (FAILED(hr)) {
		return STRSAFE_E_INSUFFICIENT_BUFFER;
	}

	cchRemaining = cchPath - cchPathLength;
	lpszEnd = &lpszPath[cchPathLength];

	if (cchPathLength != 0 && lpszEnd[-1] != '\\') {
		hr = StringCchCopyEx(lpszEnd, cchRemaining, L"\\", &lpszEnd, &cchRemaining, STRSAFE_NO_TRUNCATION);
	} else {
		hr = S_FALSE;
	}

	if (SUCCEEDED(hr)) {
		if (ppszEnd) {
			*ppszEnd = lpszEnd;
		}

		if (pcchRemaining) {
			*pcchRemaining = cchRemaining;
		}
	}

	return hr;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchAddExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszExt)
{
	HRESULT hr;
	LPCWSTR lpszExistingExt;
	BOOL bExtHasDot = FALSE;
	SIZE_T cchExt;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, L\"%.200ws\")", lpszPath, cchPath, lpszExt);

	if (!lpszPath || !cchPath || !lpszExt || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	// gather information about lpszExt
	for (cchExt = 0; lpszExt[cchExt] != '\0'; cchExt++) {
		switch (lpszExt[cchExt]) {
		case '.':
			if (cchExt == 0) {
				bExtHasDot = TRUE;
				continue;
			} // if the dot isn't at the beginning, it's invalid - fall through
		case ' ':
		case '\\':
			return E_INVALIDARG;
		}
	}

	// If lpszExt is empty, or has only a dot, return S_OK
	if (cchExt == 0 || (cchExt == 1 && bExtHasDot)) {
		return S_OK;
	}

	hr = PathCchFindExtension(lpszPath, cchPath, &lpszExistingExt);

	if (FAILED(hr)) {
		return hr;
	}

	if (*lpszExistingExt != '\0') {
		// there is already an extension
		return S_FALSE;
	}

	if (!bExtHasDot) {
		hr = StringCchCatEx(lpszPath, cchPath, L".", &lpszPath, &cchPath, STRSAFE_NO_TRUNCATION);

		if (FAILED(hr)) {
			return hr;
		}
	}

	return StringCchCatEx(lpszPath, cchPath, lpszExt, NULL, NULL, STRSAFE_NO_TRUNCATION);
}

#if 0
WINPATHCCHAPI HRESULT WINAPI PathCchAppend(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszMore)
{
	return PathCchAppendEx(lpszPath, cchPath, lpszMore, 0);
}

WINPATHCCHAPI HRESULT WINAPI PathCchAppendEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszMore OPTIONAL,
	IN		DWORD	dwFlags)
{
	ODS_ENTRY(L"(L\"%.200ws\", %Iu, L\"%.200ws\", %I32u)", lpszPath, cchPath, lpszMore, dwFlags);
	return S_OK;
}

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalize(
	OUT		LPWSTR	lpszPathOut,
	IN		SIZE_T	cchPathOut,
	IN		LPCWSTR	lpszPathIn)
{
	return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszPathIn, 0);
}

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalizeEx(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags)
{
	ODS_ENTRY(L"(%p, %Iu, L\"%.200ws\", %I32u)", lpszPathOut, cchPathOut, lpszPathIn, dwFlags);
	return S_OK;
}

WINPATHCCHAPI HRESULT WINAPI PathCchCombine(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn OPTIONAL,
	IN	LPCWSTR	lpszMore OPTIONAL)
{
	return PathCchCombineEx(lpszPathOut, cchPathOut, lpszPathIn, lpszMore, 0);
}

WINPATHCCHAPI HRESULT WINAPI PathCchCombineEx(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn OPTIONAL,
	IN	LPCWSTR	lpszMore OPTIONAL,
	IN	DWORD	dwFlags)
{
	ODS_ENTRY(L"(%p, %Iu, L\"%.200ws\", L\"%.200ws\", %I32u)", lpszPathOut, cchPathOut, lpszPathIn, lpszMore, dwFlags);
	return S_OK;
}
#endif

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchFindExtension(
	IN	LPCWSTR	lpszPath,
	IN	SIZE_T	cchPath,
	OUT	LPCWSTR	*ppszExt)
{
	LPCWSTR lpszEnd;
	LPCWSTR lpszTempExt;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, %p)", lpszPath, cchPath, ppszExt);

	*ppszExt = NULL;

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	lpszEnd = &lpszPath[cchPath];
	lpszTempExt = NULL;

	if (lpszPath >= lpszEnd) {
		return E_INVALIDARG;
	}

	do {
		if (*lpszPath == '\0') {
			break;
		}

		if (*lpszPath == '.') {
			lpszTempExt = lpszPath;
		} else if (*lpszPath == '\\' || *lpszPath == ' ') {
			lpszTempExt = NULL;
		}

		lpszPath++;
	} while (lpszPath < lpszEnd);

	if (lpszPath >= lpszEnd) {
		return E_INVALIDARG;
	}

	if (!lpszTempExt) {
		// point the extension to the null-terminator at the end of the path
		lpszTempExt = lpszPath;
	}

	*ppszExt = lpszTempExt;
	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI BOOL WINAPI PathCchIsRoot(
	IN	LPCWSTR	lpszPath)
{
	LPCWSTR lpszServer;

	ODS_ENTRY(L"(L\"%.200ws\")", lpszPath);

	if (!lpszPath || *lpszPath == '\0') {
		return FALSE;
	}

	if (lpszPath[0] == '\\' && lpszPath[1] == '\0') {
		return TRUE;
	}

	if (IsVolumeGuidPath(lpszPath)) {
		if (lpszPath[48] == '\\' && lpszPath[49] == '\0') {
			return TRUE;
		} else {
			return FALSE;
		}
	}

	if (IsExtendedLengthDosDevicePath(lpszPath)) {
		lpszPath = &lpszPath[4];
		goto ProcessDosPath;
	}

	if (PathIsUNCEx(lpszPath, &lpszServer)) {
		// fast forward to either a) end of string, or b) a backslash
		while (*lpszServer && *lpszServer != '\\') lpszServer++;

		if (lpszServer[0] == '\0') {
			return TRUE;
		}

		if (lpszServer[0] == '\\' && lpszServer[1] != '\0') {
			while (*++lpszServer) {
				if (*lpszServer == '\\') {
					return FALSE;
				}
			}

			return TRUE;
		}
	} else {
ProcessDosPath:
		if (iswalpha(lpszPath[0]) && lpszPath[1] == ':' && lpszPath[2] == '\\' && lpszPath[3] == '\0') {
			return TRUE;
		}
	}

	return FALSE;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslash(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	return PathCchRemoveBackslashEx(lpszPath, cchPath, NULL, NULL);
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslashEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		LPWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL)
{
	HRESULT hr;
	SIZE_T cch;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, %p, %p)", lpszPath, cchPath, ppszEnd, pcchRemaining);

	hr = StringCchLength(lpszPath, cchPath, &cch);

	if (FAILED(hr)) {
		goto Error;
	}

	if (*lpszPath == '\0') {
		if (ppszEnd) {
			*ppszEnd = lpszPath;
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath;
		}

		return S_FALSE;
	}

	if (PathCchIsRoot(lpszPath)) {
		if (lpszPath[cch - 1] != '\\') {
			goto NoBackslash;
		}

		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch - 1];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch + 1;
		}

		return S_FALSE;
	}

	if (lpszPath[cch - 1] == '\\') {
		lpszPath[cch - 1] = '\0';
		
		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch - 1];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch + 1;
		}

		return S_OK;
	} else {
NoBackslash:
		if (ppszEnd) {
			*ppszEnd = &lpszPath[cch];
		}

		if (pcchRemaining) {
			*pcchRemaining = cchPath - cch;
		}

		return S_FALSE;
	}

Error:
	if (ppszEnd) {
		*ppszEnd = NULL;
	}

	if (pcchRemaining) {
		*pcchRemaining = 0;
	}

	return E_INVALIDARG;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	HRESULT hr;
	LPWSTR lpszExt;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu)", lpszPath, cchPath);

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	// Limit non-\\?\ paths to a maximum length of MAX_PATH
	if (wcsncmp(lpszPath, L"\\\\?\\", 4) && cchPath > MAX_PATH) {
		cchPath = MAX_PATH;
	}

	hr = PathCchFindExtension(lpszPath, cchPath, &lpszExt);

	if (FAILED(hr)) {
		return hr;
	}

	if (*lpszExt != '\0') {
		// extension was removed
		*lpszExt = '\0';
		return S_OK;
	} else {
		// extension was not found and not removed
		return S_FALSE;
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRemoveFileSpec(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	LPCWSTR lpszPathBufEnd;
	LPWSTR lpszPtr;
	LPWSTR lpszBackslash;
	LPWSTR lpszRootEnd;
	LPWSTR lpszPathOriginal = lpszPath;
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu)", lpszPath, cchPath);

	if (!lpszPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	hr = PathCchSkipRoot(lpszPath, &lpszRootEnd);

	// if path has a root, skip past it
	if (SUCCEEDED(hr)) {
		lpszPath = lpszRootEnd;
	}

	lpszPathBufEnd = &lpszPath[cchPath];

	if (lpszPath >= lpszPathBufEnd) {
		return E_INVALIDARG;
	}

	for (lpszPtr = lpszPath;; lpszPtr = lpszBackslash + 1) {
		lpszBackslash = wcschr(lpszPtr, '\\');

		if (!lpszBackslash) {
			break;
		}

		lpszPath = lpszBackslash;

		if (lpszBackslash >= lpszPathBufEnd) {
			return E_INVALIDARG;
		}
	}

	if (*lpszPath != '\0') {
		*lpszPath = '\0';
		hr = PathCchRemoveBackslash(lpszPathOriginal, cchPath);

		if (FAILED(hr)) {
			return hr;
		}

		return S_OK;
	} else {
		return PathCchRemoveBackslash(lpszPathOriginal, cchPath);
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchRenameExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszExt)
{
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, L\"%.200ws\")", lpszPath, cchPath, lpszExt);

	hr = PathCchRemoveExtension(lpszPath, cchPath);
	if (FAILED(hr)) return hr;

	hr = PathCchAddExtension(lpszPath, cchPath, lpszExt);
	if (FAILED(hr)) return hr;

	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
// Examples:
// C:\Windows				-> S_OK, Windows
// C:\						-> S_OK, <empty string>
// C:						-> S_OK, <empty string>
// C						-> E_INVALIDARG, NULL
// <empty string>			-> E_INVALIDARG, NULL
// \						-> S_OK, <empty string>
// \whatever				-> S_OK, whatever
// \\whatever				-> S_OK, <empty string>
// \\\whatever				-> S_OK, <empty string>
// \\\s1\s2\s3				-> S_OK, s2\s3
// \\server\share			-> S_OK, <empty string>
// \\server\share\1\2\3		-> S_OK, 1\2\3
// \\?\C:\Windows			-> S_OK, Windows
// \\?\C					-> E_INVALIDARG, NULL
// \\?\UNC					-> E_INVALIDARG, NULL
// \\?\UNC\					-> S_OK, <empty string>
// \\?\UNC\server\share		-> S_OK, <empty string>
// \\?\UNC\server\share\1\2	-> S_OK, 1\2
// \\garbage\C:\Windows		-> S_OK, Windows
// \\s1\s2\C:\Windows		-> S_OK, C:\Windows
// \\s1\s2\s3\C:\Windows	-> S_OK, s3\C:\Windows
// 
WINPATHCCHAPI HRESULT WINAPI PathCchSkipRoot(
	IN	LPCWSTR	lpszPath,
	OUT	LPCWSTR	*ppszRootEnd)
{
	ODS_ENTRY(L"(L\"%.200ws\", %p)", lpszPath, ppszRootEnd);

	if (!lpszPath || !ppszRootEnd) {
		return E_INVALIDARG;
	}

	if (lpszPath[0] == '\\') {
		if (lpszPath[1] == '\\') {
			if (lpszPath[2] == '?') {
				if (lpszPath[3] == '\\') {
					// there are three possibilities here
					// either the following characters are "UNC\" (in which case we will process
					// the remaining string as a \\server\share\... path)
					// or the following characters are "Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
					// or we will simply process the string after \\?\ as a normal DOS-style path
					if (!wcsnicmp(&lpszPath[4], L"UNC\\", 4)) {
						lpszPath = &lpszPath[8];
						goto ProcessServerSharePath;
					} else if (!wcsnicmp(&lpszPath[4], L"Volume{", 7) && wcslen(&lpszPath[10]) >= 38) {
						if (lpszPath[48] == '\\') {
							*ppszRootEnd = &lpszPath[49];
						} else {
							*ppszRootEnd = &lpszPath[48];
						}

						return S_OK;
					} else {
						lpszPath = &lpszPath[4];
						goto ProcessDosPath;
					}
				}
			} else {
				// consume up to two more backslashes and then return the string after that
				SIZE_T i;
				INT nBackslashes;
				lpszPath += 2;

ProcessServerSharePath:
				i = 0;
				nBackslashes = 0;

				while (lpszPath[i] != '\0') {
					if (lpszPath[i++] == '\\') {
						if (++nBackslashes == 2) {
							break;
						}

						if (nBackslashes == 1 && lpszPath[i] == '\\') {
							// Apparently we're not supposed to skip over the "share" part of the
							// UNC path if it's empty. Dunno why but Windows 10 does it.
							break;
						}
					}
				}

				*ppszRootEnd = &lpszPath[i];
				return S_OK;
			}
		} else {
			// simply return the string after the first backslash
			*ppszRootEnd = &lpszPath[1];
			return S_OK;
		}
	} else {
ProcessDosPath:
		// the path must begin with a single [a-zA-Z] char, immediately followed by a colon ':'
		if (iswalpha(lpszPath[0]) && lpszPath[1] == ':') {
			if (lpszPath[2] == '\\') {
				// if there is a backslash immediately after the colon,
				// return the string directly after it.
				// Note: this rule does not apply to forward slashes, since
				// it isn't like that in Win10. Maybe it's a bug.
				*ppszRootEnd = &lpszPath[3];
			} else {
				// otherwise, return the string directly after the colon
				*ppszRootEnd = &lpszPath[2];
			}

			return S_OK;
		}
	}

	*ppszRootEnd = NULL;
	return E_INVALIDARG;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchStripPrefix(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu)", lpszPath, cchPath);

	if (!lpszPath || !cchPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	if (IsExtendedLengthDosDevicePath(lpszPath)) {
		hr = StringCchCopyW(lpszPath, cchPath, &lpszPath[4]);
	} else if (IsPrefixedUncPath(lpszPath)) {
		hr = StringCchCopyW(&lpszPath[2], cchPath - 2, &lpszPath[8]);
	} else {
		return S_FALSE;
	}

	if (SUCCEEDED(hr)) {
		return S_OK;
	} else {
		return E_INVALIDARG;
	}
}

// ALL TESTS PASSED, DO NOT MODIFY
//
WINPATHCCHAPI HRESULT WINAPI PathCchStripToRoot(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath)
{
	LPWSTR lpszAfterRoot;
	LPWSTR lpszServerPart;
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu)", lpszPath, cchPath);

	if (!lpszPath || !cchPath || cchPath > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	hr = PathCchSkipRoot(lpszPath, &lpszAfterRoot);

	if (FAILED(hr)) {
		return hr;
	}

	if (lpszAfterRoot >= &lpszPath[cchPath]) {
		return E_INVALIDARG;
	}

	if (PathIsUNCEx(lpszPath, &lpszServerPart) && *lpszServerPart && lpszAfterRoot[-1] == '\\') {
		lpszAfterRoot[-1] = '\0';
	} else if (lpszAfterRoot[0] != '\0') {
		lpszAfterRoot[0] = '\0';
	} else {
		return S_FALSE;
	}

	return S_OK;
}

// ALL TESTS PASSED, DO NOT MODIFY
//
// Examples taken from testing on a Windows 10 system:
// C:\Windows				-> FALSE, NULL
// \\server\share\whatever	-> TRUE, server\share\whatever
// \\?\C:\Users				-> FALSE, NULL
// \\?\UNC					-> FALSE, NULL
// \\?\UNC\					-> FALSE, <empty string>
// \\?\UNC\Whatever			-> TRUE, Whatever
// \\.\UNC\Whatever			-> TRUE, .\UNC\Whatever
WINPATHCCHAPI BOOL WINAPI PathIsUNCEx(
	IN	LPCWSTR	lpszPath,
	OUT	LPCWSTR	*ppszServer OPTIONAL)
{
	ODS_ENTRY(L"(L\"%.200ws\", %p)", lpszPath, ppszServer);

	// Windows 10 does not check whether lpszPath is NULL.
	// If it is, we will simply AV. "Not my problem"

	if (lpszPath[0] == '\\' && lpszPath[1] == '\\') {
		if (lpszPath[2] == '?') {
			if (!wcsnicmp(&lpszPath[3], L"\\UNC\\", 5)) {
				if (ppszServer) {
					*ppszServer = &lpszPath[8];
				}

				return TRUE;
			}
		} else {
			if (ppszServer) {
				*ppszServer = &lpszPath[2];
			}

			return TRUE;
		}
	}

	if (ppszServer) {
		*ppszServer = NULL;
	}

	return FALSE;
}