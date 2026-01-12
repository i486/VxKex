#pragma once

#include <KexComm.h>

#ifndef WINPATHCCHAPI
#  define WINPATHCCHAPI
#  pragma comment(lib, "KexPathCch.lib")
#endif

// Values for various "dwFlags" parameters
#define PATHCCH_MAX_CCH										((SIZE_T) 32768)
#define PATHCCH_NONE										0x00000000
#define PATHCCH_ALLOW_LONG_PATHS							0x00000001
#define PATHCCH_FORCE_ENABLE_LONG_NAME_PROCESS				0x00000002
#define PATHCCH_FORCE_DISABLE_LONG_NAME_PROCESS				0x00000004
#define PATHCCH_DO_NOT_NORMALIZE_SEGMENTS					0x00000008
#define PATHCCH_ENSURE_IS_EXTENDED_LENGTH_PATH				0x00000010
#define PATHCCH_ENSURE_TRAILING_SLASH						0x00000020

#define PATHCCH_E_FILENAME_TOO_LONG							((HRESULT) 0x800700CE)

WINPATHCCHAPI HRESULT WINAPI PathAllocCanonicalize(
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut);

WINPATHCCHAPI HRESULT WINAPI PathAllocCombine(
	IN	LPCWSTR	lpszPathIn,
	IN	LPCWSTR	lpszMore,
	IN	DWORD	dwFlags,
	OUT	LPWSTR	*ppszPathOut);

WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslash(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI HRESULT WINAPI PathCchAddBackslashEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		LPWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL);

WINPATHCCHAPI HRESULT WINAPI PathCchAddExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszExt);

WINPATHCCHAPI HRESULT WINAPI PathCchAppend(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszMore);

WINPATHCCHAPI HRESULT WINAPI PathCchAppendEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszMore OPTIONAL,
	IN		DWORD	dwFlags);

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalize(
	OUT		LPWSTR	lpszPathOut,
	IN		SIZE_T	cchPathOut,
	IN		LPCWSTR	lpszPathIn);

WINPATHCCHAPI HRESULT WINAPI PathCchCanonicalizeEx(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn,
	IN	DWORD	dwFlags);

WINPATHCCHAPI HRESULT WINAPI PathCchCombine(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn OPTIONAL,
	IN	LPCWSTR	lpszMore OPTIONAL);

WINPATHCCHAPI HRESULT WINAPI PathCchCombineEx(
	OUT	LPWSTR	lpszPathOut,
	IN	SIZE_T	cchPathOut,
	IN	LPCWSTR	lpszPathIn OPTIONAL,
	IN	LPCWSTR	lpszMore OPTIONAL,
	IN	DWORD	dwFlags);

WINPATHCCHAPI HRESULT WINAPI PathCchFindExtension(
	IN	LPCWSTR	lpszPath,
	IN	SIZE_T	cchPath,
	OUT	LPCWSTR	*ppszExt);

WINPATHCCHAPI BOOL WINAPI PathCchIsRoot(
	IN	LPCWSTR	lpszPath);

WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslash(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI HRESULT WINAPI PathCchRemoveBackslashEx(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	OUT		LPWSTR	*ppszEnd OPTIONAL,
	OUT		PSIZE_T	pcchRemaining OPTIONAL);

WINPATHCCHAPI HRESULT WINAPI PathCchRemoveExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI HRESULT WINAPI PathCchRemoveFileSpec(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI HRESULT WINAPI PathCchRenameExtension(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath,
	IN		LPCWSTR	lpszExt);

WINPATHCCHAPI HRESULT WINAPI PathCchSkipRoot(
	IN	LPCWSTR	lpszPath,
	OUT	LPCWSTR	*ppszRootEnd);

WINPATHCCHAPI HRESULT WINAPI PathCchStripPrefix(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI HRESULT WINAPI PathCchStripToRoot(
	IN OUT	LPWSTR	lpszPath,
	IN		SIZE_T	cchPath);

WINPATHCCHAPI BOOL WINAPI PathIsUNCEx(
	IN	LPCWSTR	lpszPath,
	OUT	LPCWSTR	*ppszServer OPTIONAL);

#pragma deprecated(PathAddBackslash)
#pragma deprecated(PathAddExtension)
#pragma deprecated(PathAppend)
#pragma deprecated(PathCanonicalize)
#pragma deprecated(PathCombine)
#pragma deprecated(PathIsRoot)
#pragma deprecated(PathRemoveBackslash)
#pragma deprecated(PathRemoveFileSpec)
#pragma deprecated(PathRenameExtension)
#pragma deprecated(PathSkipRoot)
#pragma deprecated(PathStripPrefix)
#pragma deprecated(PathStripToRoot)