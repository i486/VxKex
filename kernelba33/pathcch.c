#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <ctype.h>

#include "pathcch.h"

#define HRCHECKED(x) hr = (x); if (FAILED(hr)) goto Error

STATIC BOOL StrBeginsWith(
	IN	LPCWSTR	lpszBig,
	IN	LPCWSTR	lpszSmall)
{
	return !wcsncmp(lpszBig, lpszSmall, wcslen(lpszSmall));
}

STATIC BOOL StrBeginsWithI(
	IN	LPCWSTR	lpszBig,
	IN	LPCWSTR	lpszSmall)
{
	return !wcsnicmp(lpszBig, lpszSmall, wcslen(lpszSmall));
}

// check if path begins with \\?\ prefix and has a drive letter and colon after it
STATIC BOOL IsExtendedLengthDosDevicePath(
	IN	LPCWSTR	lpszPath)
{
	if (StrBeginsWith(lpszPath, L"\\\\?\\") && iswalpha(lpszPath[4]) && lpszPath[5] == ':') {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsDosDevicePath(
	IN	LPCWSTR	lpszPath)
{
	if (iswalpha(lpszPath[0]) && lpszPath[1] == ':' && (lpszPath[2] == '\\' || lpszPath[2] == '\0')) {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsVolumeGuidPath(
	IN	LPCWSTR	lpszPath)
{
	if (StrBeginsWithI(lpszPath, L"\\\\?\\Volume{") && wcslen(&lpszPath[10]) >= 38) {
		return TRUE;
	} else {
		return FALSE;
	}
}

STATIC BOOL IsPrefixedPath(
	IN	LPCWSTR	lpszPath)
{
	return StrBeginsWith(lpszPath, L"\\\\?\\");
}

STATIC BOOL IsPrefixedUncPath(
	IN	LPCWSTR	lpszPath)
{
	return StrBeginsWithI(lpszPath, L"\\\\?\\UNC\\");
}

STATIC BOOL IsFullyQualifiedPath(
	IN	LPCWSTR	lpszPath)
{
	return IsDosDevicePath(lpszPath) || StrBeginsWith(lpszPath, L"\\\\");
}

WINPATHCCHAPI HRESULT WINAPI PathAllocCanonicalize(
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut)
{
	LPWSTR lpszPathOut = NULL;
	SIZE_T cchPathIn;
	SIZE_T cchAlloc;
	BOOL bAllowLongPaths = dwFlags & PATHCCH_ALLOW_LONG_PATHS;
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", %I32u, %p)", lpszPathIn, dwFlags, ppszPathOut);

	if (!ppszPathOut) {
		return E_INVALIDARG;
	}

	*ppszPathOut = NULL;
	cchPathIn = wcslen(lpszPathIn);

	if (cchPathIn > PATHCCH_MAX_CCH) {
		return PATHCCH_E_FILENAME_TOO_LONG;
	}

	if (cchPathIn > 0) {
		cchAlloc = cchPathIn + 1;
	} else {
		cchAlloc = 2;
	}

	if (cchAlloc > MAX_PATH && bAllowLongPaths) {
		// add space for \\?\ prefix
		cchAlloc += 6;
	}

	// limit maximum path length
	if (cchAlloc > (bAllowLongPaths ? PATHCCH_MAX_CCH : MAX_PATH)) {
		cchAlloc = (bAllowLongPaths ? PATHCCH_MAX_CCH : MAX_PATH);
	}

	lpszPathOut = (LPWSTR) LocalAlloc(LMEM_ZEROINIT, cchAlloc * sizeof(WCHAR));

	if (!lpszPathOut) {
		return E_OUTOFMEMORY;
	}

	hr = PathCchCanonicalizeEx(lpszPathOut, cchAlloc, lpszPathIn, dwFlags);

	if (FAILED(hr)) {
		LocalFree(lpszPathOut);
		return hr;
	}

	*ppszPathOut = lpszPathOut;
	return S_OK;
}

WINPATHCCHAPI HRESULT WINAPI PathAllocCombine(
	IN	LPCWSTR	lpszPathIn OPTIONAL,
	IN	LPCWSTR	lpszMore OPTIONAL,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut)
{
	SIZE_T cchPathIn = 0;
	SIZE_T cchMore = 0;
	SIZE_T cchPathOut;
	LPWSTR lpszPathOut;
	BOOL bAllowLongPaths = dwFlags & PATHCCH_ALLOW_LONG_PATHS;
	HRESULT hr;

	ODS_ENTRY(L"(L\"%.200ws\", L\"%.200ws\", %I32u, %p)", lpszPathIn, lpszMore, dwFlags, ppszPathOut);

	if (!ppszPathOut) {
		return E_INVALIDARG;
	}

	*ppszPathOut = NULL;

	if (lpszPathIn == NULL && lpszMore == NULL) {
		return E_INVALIDARG;
	}

	if (lpszPathIn) {
		cchPathIn = wcslen(lpszPathIn);

		if (cchPathIn > 0) {
			cchPathIn++; // add space for '\0'
		}

		if (cchPathIn >= PATHCCH_MAX_CCH) {
			return PATHCCH_E_FILENAME_TOO_LONG;
		}
	}

	if (lpszMore) {
		cchMore = wcslen(lpszMore);

		if (cchMore > 0) {
			cchMore++;
		}

		if (cchMore >= PATHCCH_MAX_CCH) {
			return PATHCCH_E_FILENAME_TOO_LONG;
		}
	}

	cchPathOut = cchMore + cchPathIn;

	if (cchPathOut == 0) {
		cchPathOut = 2; // for backslash and null terminator
	} else if (cchPathOut > (bAllowLongPaths ? PATHCCH_MAX_CCH : MAX_PATH)) {
		cchPathOut = bAllowLongPaths ? PATHCCH_MAX_CCH : MAX_PATH;
	}

	if (cchPathOut > MAX_PATH && bAllowLongPaths) {
		cchPathOut += 6; // for \\?\ or \\?\UNC\ 
	}

	lpszPathOut = (LPWSTR) LocalAlloc(LMEM_ZEROINIT, cchPathOut * sizeof(WCHAR));

	if (!lpszPathOut) {
		return E_OUTOFMEMORY;
	}

	hr = PathCchCombineEx(lpszPathOut, cchPathOut, lpszPathIn, lpszMore, dwFlags);

	if (FAILED(hr)) {
		LocalFree(lpszPathOut);
		return hr;
	}

	*ppszPathOut = lpszPathOut;
	return S_OK;
}

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
	HRESULT hr;
	SIZE_T cchPathCopy;
	LPWSTR lpszPathCopy;

	ODS_ENTRY(L"(L\"%.200ws\", %Iu, L\"%.200ws\", %I32u)", lpszPath, cchPath, lpszMore, dwFlags);

	if (!lpszPath) {
		return E_INVALIDARG;
	}

	if (lpszMore && !PathIsUNCEx(lpszMore, NULL) && !StrBeginsWith(lpszMore, L"\\\\?\\")) {
		until (*lpszMore != '\\') {
			lpszMore++;
		}
	}

	cchPathCopy = wcslen(lpszPath) + 1;
	lpszPathCopy = (LPWSTR) AutoAlloc(cchPathCopy * sizeof(WCHAR));

	if (!lpszPathCopy) {
		return E_OUTOFMEMORY;
	}

	RtlCopyMemory(lpszPathCopy, lpszPath, cchPathCopy * sizeof(WCHAR));
	hr = PathCchCombineEx(lpszPath, cchPath, lpszPathCopy, lpszMore, dwFlags);
	AutoFree(lpszPathCopy);
	return hr;
}

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalize(
	OUT		LPWSTR	lpszPathOut,
	IN		SIZE_T	cchPathOut,
	IN		LPCWSTR	lpszPathIn)
{
	return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszPathIn, PATHCCH_NONE);
}

// Test results:
// NULL														-> Access violation
// <empty string>											-> \
// ..C\a\b													-> ..C\a\b
// C														-> C
// C\..														-> \
// C\..\D:\a\b												-> \D:\a\b (Failed)
// C:														-> C:\
// C:D														-> C:D
// C:\														-> C:\
// C:\\														-> C:\\
// C:\\.													-> C:\
// C:\a\\..													-> C:\a\
// C:\a\\.\													-> C:\a\\
// C:\a\b\c\..\d											-> C:\a\b\d
// \\?\														-> \\?\
// \\?\C													-> \\?\C
// \\?\C\..\D												-> \\?\D
// \\?\C\..\D:\a\b											-> \\?\D:\a\b
// \\?\C:													-> C:\
// \\?\C:\Windows\..\Program Files							-> C:\Program Files
// \\?\Volume{00000000-0000-0000-0000-000000000000}			-> \\?\Volume{00000000-0000-0000-0000-000000000000}
// \\?\Volume{00000000-0000-0000-0000-000000000000}\		-> \\?\Volume{00000000-0000-0000-0000-000000000000}\
// \\?\Volume{00000000-0000-0000-0000-000000000000}\..\		-> \\?\Volume{00000000-0000-0000-0000-000000000000}\
// \\?\Volume{Invalid0-0000-0000-0000-000000000000}\..\		-> \\?\
// \\?\Volume{Invalid0-0000-0000-0000-000000000000}\.\		-> \\?\Volume{Invalid0-0000-0000-0000-000000000000}\
// \\a\b\c\d\e\f											-> \\a\b\c\d\e\f
// \\a\..\b\c\d\e\f											-> \\b\c\d\e\f
// \\..\a\..\b\c\d\e\f										-> \\b\c\d\e\f
// \\?\UNC\a\b\c\d											-> \\a\b\c\d
// \\?\UNC\a\b\c\d\											-> \\a\b\c\d\ 
// \\?\UNC\a\b\c\d\.										-> \\a\b\c\d
// \\?\UNC\a\b\c\d\.\										-> \\a\b\c\d\ 
// \\?\UNC\a\..\b\c\d										-> \\b\c\d
WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalizeEx(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags)
{
	LPCWSTR lpszServer;
	LPWSTR lpszOriginalPathOut = lpszPathOut;
	SIZE_T cchOriginalPathOut = cchPathOut;
	SIZE_T cchPathIn;
	BOOL bLongPathAllowed = (dwFlags & (PATHCCH_ALLOW_LONG_PATHS | PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH));
	HRESULT hr;

	ODS_ENTRY(L"(%p, %Iu, L\"%.200ws\", %I32u)", lpszPathOut, cchPathOut, lpszPathIn, dwFlags);

	if (lpszPathOut == lpszPathIn) {
		return E_INVALIDARG;
	}

	HRCHECKED(StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L""));

	if (cchPathOut > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	cchPathIn = wcslen(lpszPathIn) + 1;

	if (cchPathIn > MAX_PATH) {
		if (cchPathIn > PATHCCH_MAX_CCH || !bLongPathAllowed) {
			return PATHCCH_E_FILENAME_TOO_LONG;
		}
	}

	if (!bLongPathAllowed && cchPathOut > MAX_PATH) {
		// limit the number of written characters, if long paths aren't permitted
		cchPathOut = MAX_PATH;
	}

	// Basically there are 6 formatting cases that we handle:
	// 1. Plain DOS-device path: C:\a\b\c (incl. stub such as "C:")
	// 2. Extended DOS-device path: \\?\C:\a\b\c (incl. stub such as "\\?\C:")
	// 3. Volume GUID path: \\?\Volume{guid}\a\b\c
	// 4. Plain UNC path: \\server\share\a\b\c
	// 5. Extended UNC path: \\?\UNC\server\share\a\b\c
	// 6. None of the above
	// In all cases except 5, we will strip away a beginning portion of the path as follows,
	// to produce a "stripped path". (Not the same as what PathCchSkipRoot does):
	// 1. Remove drive letter, colon and first backslash (if present) -> a\b\c
	// 2. Remove \\?\ prefix and treat as (1) -> a\b\c
	// 3. Remove volume GUID prefix and first backslash (if present) -> a\b\c
	// 4. Remove \\ prefix -> server\share\a\b\c
	// 5. Remove \\?\UNC prefix and first backslash (if present) -> server\share\a\b\c

	if (PathIsUNCEx(lpszPathIn, &lpszServer)) {
		// This branch handles both plain and extended UNC paths.
		lpszPathIn = lpszServer;

		if (bLongPathAllowed) {
			HRCHECKED(StringCchCopyEx(lpszPathOut, cchPathOut, L"\\\\?\\UNC\\", &lpszPathOut, &cchPathOut, 0));
		} else {
			HRCHECKED(StringCchCopyEx(lpszPathOut, cchPathOut, L"\\\\", &lpszPathOut, &cchPathOut, 0));
		}
	} else if (IsVolumeGuidPath(lpszPathIn)) {
		// Handles volume GUID paths.
		HRCHECKED(StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, 49, &lpszPathOut, &cchPathOut, 0));
		lpszPathIn += 48;

		if (*lpszPathIn != '\0') {
			lpszPathIn++;
		}
	} else if (IsExtendedLengthDosDevicePath(lpszPathIn)) {
		// Handles extended DOS-device paths.
		lpszPathIn += 4; // skip past \\?\ prefix since we will decide whether to add it separately
		goto HandleDosDevicePath;
	} else if (IsDosDevicePath(lpszPathIn)) {
		// Handles plain DOS-device paths.
		WCHAR wcDriveLetter;

HandleDosDevicePath:
		wcDriveLetter = lpszPathIn[0];
		lpszPathIn += 2;

		if (*lpszPathIn != '\0') {
			lpszPathIn++;
		}

		if (bLongPathAllowed) {
			HRCHECKED(StringCchPrintfEx(lpszPathOut, cchPathOut, &lpszPathOut, &cchPathOut, 0, L"\\\\?\\%wc:\\", wcDriveLetter));
		} else {
			HRCHECKED(StringCchPrintfEx(lpszPathOut, cchPathOut, &lpszPathOut, &cchPathOut, 0, L"%wc:\\", wcDriveLetter));
		}
	}

	// Handle the "stripped path".
	while (*lpszPathIn != '\0') {
		LPCWSTR lpszNextSegment = wcschr(lpszPathIn, '\\');

		if (!lpszNextSegment) {
			// This means we are already at the last path segment. Fast forward until
			// lpszSectionEnd points to the terminating null.
			lpszNextSegment = lpszPathIn;

			until (*lpszNextSegment == '\0') {
				lpszNextSegment++;
			}
		} else {
			lpszNextSegment++;
		}

		// if the current path segment is a . or .. then skip it
		if (lpszPathIn[0] == '.') {
			if (lpszPathIn[1] == '.') {
				if (lpszPathIn[2] == '\\' || lpszPathIn[2] == '\0') {
					lpszPathIn = lpszNextSegment;
					continue;
				}
			} else {
				if (lpszPathIn[1] == '\\' || lpszPathIn[1] == '\0') {
					lpszPathIn = lpszNextSegment;
					continue;
				}
			}
		}

		// if the next path segment is a .. then skip the current one and the next one
		if (lpszNextSegment[0] == '.') {
			if (lpszNextSegment[1] == '.') {
				if (lpszNextSegment[2] == '\\') {
					lpszPathIn = lpszNextSegment + 3;
					continue;
				} else if (lpszNextSegment[2] == '\0') {
					// remove the backslash from the output
					if (lpszPathOut > lpszOriginalPathOut && lpszPathOut[-1] == '\\') {
						lpszPathOut[-1] = '\0';
					}

					break;
				}
			} else if (lpszNextSegment[1] == '\0') {
				// don't append backslash if next segment is \. and the path ends there
				StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, lpszNextSegment - lpszPathIn - 1, &lpszPathOut, &cchPathOut, 0);
				break;
			}
		}

		// otherwise copy the current segment to output
		StringCchCopyNEx(lpszPathOut, cchPathOut, lpszPathIn, lpszNextSegment - lpszPathIn, &lpszPathOut, &cchPathOut, 0);
		lpszPathIn = lpszNextSegment;
	}

	// Remove all trailing periods, unless preceded by a '*' character.
	// In this case, retain 1 trailing period.
	lpszPathOut--;

	if (lpszPathOut > lpszOriginalPathOut) {
		until (*lpszPathOut != '.') {
			lpszPathOut--;
		}

		if (*lpszPathOut == '*') {
			lpszPathOut += 2;
		} else {
			lpszPathOut += 1;
		}

		*lpszPathOut = '\0';
	}

	// If output path is a root (e.g. "C:"), or if the caller specifies it, ensure it has a backslash at the end.
	if ((lpszOriginalPathOut[0] && lpszOriginalPathOut[1] == ':' && lpszOriginalPathOut[2] == '\0')
		|| (dwFlags & PATHCCH_ENSURE_TRAILING_SLASH)) {
		HRCHECKED(PathCchAddBackslash(lpszOriginalPathOut, cchOriginalPathOut));
	} else if (*lpszOriginalPathOut == '\0') {
		// If the output path is empty, replace it with "\"
		HRCHECKED(StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L"\\"));
	}

	return S_OK;

Error:
	StringCchCopy(lpszOriginalPathOut, cchOriginalPathOut, L"");
	return hr;
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
	SIZE_T cchBuf = cchPathOut;
	LPWSTR lpszBuf = NULL;
	HRESULT hr;

	ODS_ENTRY(L"(%p, %Iu, L\"%ws\", L\"%ws\", %I32u)", lpszPathOut, cchPathOut, lpszPathIn, lpszMore, dwFlags);

	if (!lpszPathOut || !cchPathOut || cchPathOut > PATHCCH_MAX_CCH) {
		return E_INVALIDARG;
	}

	if (!lpszPathIn && !lpszMore) {
		HRCHECKED(E_INVALIDARG);
	}

	if (lpszPathIn && wcslen(lpszPathIn) >= PATHCCH_MAX_CCH) {
		HRCHECKED(PATHCCH_E_FILENAME_TOO_LONG);
	}

	if (lpszMore && wcslen(lpszMore) >= PATHCCH_MAX_CCH) {
		HRCHECKED(PATHCCH_E_FILENAME_TOO_LONG);
	}

	// If lpszPathIn is a blank string or NULL, or if lpszMore is fully qualified,
	// it is canonicalized directly to the output buffer without being combined.
	if (!lpszPathIn || *lpszPathIn == '\0' || (lpszMore && IsFullyQualifiedPath(lpszMore))) {
		if (lpszMore) {
			return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszMore, dwFlags);
		}
	}

	// If lpszMore is a blank string or NULL, canonicalize lpszPathIn directly to the
	// output buffer.
	if (!lpszMore || *lpszMore == '\0') {
		if (lpszPathIn) {
			return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszPathIn, dwFlags);
		}
	}

	// If lpszMore begins with a backslash:
	// - copy the root of lpszPathIn to temporary buffer
	// - append lpszMore to temporary buffer
	// - canonicalize temporary buffer to lpszPathOut
	if (*lpszMore == '\\') {
		LPWSTR lpszPathInRootEnd;
		HRCHECKED(PathCchSkipRoot(lpszPathIn, &lpszPathInRootEnd));

		if (lpszPathInRootEnd > lpszPathIn) {
			// lpszPathIn contains a root

			if (lpszPathInRootEnd[-1] == '\\') {
				// ensure no backslash - since we know lpszMore already starts with one
				lpszPathInRootEnd--;
			}

			lpszBuf = (LPWSTR) AutoAlloc(cchBuf * sizeof(WCHAR));

			if (!lpszBuf) {
				HRCHECKED(E_OUTOFMEMORY);
			}

			HRCHECKED(StringCchCopyN(lpszBuf, cchBuf, lpszPathIn, lpszPathInRootEnd - lpszPathIn));
			HRCHECKED(StringCchCat(lpszBuf, cchBuf, lpszMore));
			HRCHECKED(PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszBuf, dwFlags));

			AutoFree(lpszBuf);
			return S_OK;
		} else {
			// lpszPathIn does not contain a root
			return PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszMore, dwFlags);
		}
	}

	// Otherwise:
	// - copy lpszPathIn to temporary buffer
	// - add backslash (if not already present)
	// - append lpszMore
	// - canonicalize temporary buffer to lpszPathOut
	{
		lpszBuf = (LPWSTR) AutoAlloc(cchBuf * sizeof(WCHAR));

		if (!lpszBuf) {
			HRCHECKED(E_OUTOFMEMORY);
		}

		HRCHECKED(StringCchCopy(lpszBuf, cchBuf, lpszPathIn));
		HRCHECKED(PathCchAddBackslash(lpszBuf, cchBuf));
		HRCHECKED(StringCchCat(lpszBuf, cchBuf, lpszMore));
		HRCHECKED(PathCchCanonicalizeEx(lpszPathOut, cchPathOut, lpszBuf, dwFlags));

		AutoFree(lpszBuf);
	}

	return S_OK;

Error:
	AutoFree(lpszBuf);
	StringCchCopy(lpszPathOut, cchPathOut, L"");
	return hr;
}

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