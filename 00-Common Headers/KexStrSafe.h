///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexStrSafe.h
//
// Abstract:
//
//     Safe string handling functions (extended).
//     These functions are much faster than importing from shlwapi.dll
//     or ntdll.dll. The shlwapi functions especially are incredibly slow,
//     about 20-100x slower than these.
//
//     These functions are inlined in the header because their performance
//     is often important (unlike, say, ContextMenu or CriticalErrorBoxF),
//     and their code size is generally small.
//
// Author:
//
//     vxiiduu (01-Oct-2022)
//
// Environment:
//
//     StringAllocPrintf family can be called only when the process heap is
//     available.
//     Other functions can be called anywhere.
//
// Revision History:
//
//     vxiiduu               01-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <SafeAlloc.h>
#include <StrSafe.h>

//
// The StringCchPrintfBufferLength family of functions returns the required
// number of characters, including space for the null terminator, you need
// to allocate in a buffer to hold a formatted string.
//

FORCEINLINE HRESULT StringCchVPrintfBufferLengthA(
	OUT	PSIZE_T	BufferCch,
	IN	PCSTR	Format,
	IN	ARGLIST	ArgList)
{
	INT StringCch;
	
	ASSERT (Format != NULL);
	ASSERT (BufferCch != NULL);

	if (BufferCch) {
		*BufferCch = 0;
	}

	if (!Format || !BufferCch) {
		return STRSAFE_E_INVALID_PARAMETER;
	}

	StringCch = _vscprintf(Format, ArgList);

	if (StringCch == -1) {
		ASSERT (("Invalid parameters passed to _vscprintf", FALSE));
		return STRSAFE_E_INVALID_PARAMETER;
	}

	*BufferCch = StringCch + 1;
	return S_OK;
}

FORCEINLINE HRESULT StringCchVPrintfBufferLengthW(
	OUT	PSIZE_T	BufferCch,
	IN	PCWSTR	Format,
	IN	ARGLIST	ArgList)
{
	INT StringCch;
	
	ASSERT (Format != NULL);
	ASSERT (BufferCch != NULL);

	if (BufferCch) {
		*BufferCch = 0;
	}

	if (!Format || !BufferCch) {
		return STRSAFE_E_INVALID_PARAMETER;
	}

	StringCch = _vscwprintf(Format, ArgList);

	if (StringCch == -1) {
		ASSERT (("Invalid parameters passed to _vscprintf", FALSE));
		return STRSAFE_E_INVALID_PARAMETER;
	}

	*BufferCch = StringCch + 1;
	return S_OK;
}

FORCEINLINE HRESULT StringCchPrintfBufferLengthA(
	OUT	PSIZE_T	BufferCch,
	IN	PCSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	HRESULT Result;

	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLengthA(BufferCch, Format, ArgList);
	va_end(ArgList);

	return Result;
}

FORCEINLINE HRESULT StringCchPrintfBufferLengthW(
	OUT	PSIZE_T	BufferCch,
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	HRESULT Result;

	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLengthW(BufferCch, Format, ArgList);
	va_end(ArgList);

	return Result;
}

//
// The StringAllocPrintf family of functions allocates a buffer on the process
// heap to hold a formatted string.
//
// You need to pass the returned Buffer to SafeFree if the function succeeds.
//

FORCEINLINE HRESULT StringAllocPrintfA(
	OUT	PPSTR	Buffer,
	OUT	PSIZE_T	BufferCch OPTIONAL,
	IN	PCSTR	Format,
	IN	...)
{
	HRESULT Result;
	SIZE_T BufferCchInternal;
	PSTR BufferInternal = NULL;
	ARGLIST ArgList;

	ASSERT (Buffer != NULL);

	if (!Buffer) {
		return STRSAFE_E_INVALID_PARAMETER;
	}

	*Buffer = NULL;

	if (BufferCch) {
		*BufferCch = 0;
	}

	va_start(ArgList, Format);

	//
	// Find length of the buffer we need to allocate.
	//

	Result = StringCchVPrintfBufferLengthA(&BufferCchInternal, Format, ArgList);

	if (FAILED(Result)) {
		goto Exit;
	}

	//
	// Allocate the buffer.
	//

	BufferInternal = SafeAlloc(CHAR, BufferCchInternal);
	if (!BufferInternal) {
		Result = E_OUTOFMEMORY;
		goto Exit;
	}

	//
	// Format the caller-supplied string into the buffer.
	//

	Result = StringCchVPrintfA(BufferInternal, BufferCchInternal, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		goto Exit;
	}

	//
	// Return results to caller.
	//

	if (BufferCch) {
		*BufferCch = BufferCchInternal;
	}

	*Buffer = BufferInternal;
	Result = S_OK;

Exit:
	if (FAILED(Result)) {
		SafeFree(BufferInternal);
	}

	va_end(ArgList);
	return Result;
}

FORCEINLINE HRESULT StringAllocPrintfW(
	OUT	PPWSTR	Buffer,
	OUT	PSIZE_T	BufferCch OPTIONAL,
	IN	PCWSTR	Format,
	IN	...)
{
	HRESULT Result;
	SIZE_T BufferCchInternal;
	PWSTR BufferInternal = NULL;
	ARGLIST ArgList;

	ASSERT (Buffer != NULL);

	if (!Buffer) {
		return STRSAFE_E_INVALID_PARAMETER;
	}

	*Buffer = NULL;

	if (BufferCch) {
		*BufferCch = 0;
	}

	va_start(ArgList, Format);

	//
	// Find length of the buffer we need to allocate.
	//

	Result = StringCchVPrintfBufferLengthW(&BufferCchInternal, Format, ArgList);

	if (FAILED(Result)) {
		goto Exit;
	}

	//
	// Allocate the buffer.
	//

	BufferInternal = SafeAlloc(WCHAR, BufferCchInternal);
	if (!BufferInternal) {
		Result = E_OUTOFMEMORY;
		goto Exit;
	}

	//
	// Format the caller-supplied string into the buffer.
	//

	Result = StringCchVPrintfW(BufferInternal, BufferCchInternal, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		goto Exit;
	}

	//
	// Return results to caller.
	//

	Result = S_OK;

Exit:
	if (FAILED(Result)) {
		SafeFree(BufferInternal);
	} else {
		if (BufferCch) {
			*BufferCch = BufferCchInternal;
		}

		*Buffer = BufferInternal;
	}

	va_end(ArgList);
	return Result;
}

// These macros are generic and can operate on both ANSI and Unicode.
#define ToUpper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - 32) : (c))
#define ToLower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + 32) : (c))

// The StringEqual family of functions returns 0 if the strings are
// different and 1 if the strings are equal.
// The StringEqualI family does the same things except it is case
// insensitive.

FORCEINLINE BOOLEAN StringEqualA(
	IN	PCSTR	String1,
	IN	PCSTR	String2)
{
	ASSERT (String1 != NULL);
	ASSERT (String2 != NULL);

	while (*String1 && *String2 && *String1 == *String2) {
		++String1;
		++String2;
	}

	return (*String1 == *String2);
}

FORCEINLINE BOOLEAN StringEqualIA(
	IN	PCSTR	String1,
	IN	PCSTR	String2)
{
	ASSERT (String1 != NULL);
	ASSERT (String2 != NULL);

	while (*String1 && *String2 && ToUpper(*String1) == ToUpper(*String2)) {
		++String1;
		++String2;
	}

	return (*String1 == *String2);
}

FORCEINLINE BOOLEAN StringEqualW(
	IN	PCWSTR	String1,
	IN	PCWSTR	String2)
{
	ASSERT (String1 != NULL);
	ASSERT (String2 != NULL);

	while (*String1 && *String2 && *String1 == *String2) {
		++String1;
		++String2;
	}

	return (*String1 == *String2);
}

FORCEINLINE BOOLEAN StringEqualIW(
	IN	PCWSTR	String1,
	IN	PCWSTR	String2)
{
	ASSERT (String1 != NULL);
	ASSERT (String2 != NULL);

	while (*String1 && *String2 && ToUpper(*String1) == ToUpper(*String2)) {
		++String1;
		++String2;
	}

	return (*String1 == *String2);
}

//
// The StringSearch family of functions returns TRUE if Needle was found as
// a substring of Haystack.
// The StringSearchI family of functions is case insensitive.
//
// If you need the starting address of the first match of the substring,
// then these functions are not suitable. Use StrStr(I)(A/W) from shlwapi.dll
// instead.
//

FORCEINLINE BOOLEAN StringSearchA(
	IN	PCSTR	Haystack,
	IN	PCSTR	Needle)
{
	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	while (TRUE) {
		PCSTR Needle2 = Needle;

		// find first char of needle in haystack
		while (*Haystack != *Needle) {
			if (!*++Haystack) {
				// at end of haystack - needle not found
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && *Haystack == *Needle2) {
			if (!*++Needle2) {
				// end of needle - this means the needle is entirely in the haystack
				return TRUE;
			}

			if (!*++Haystack) {
				// end of haystack - this means not the entire needle in haystack
				return FALSE;
			}
		}
	}
}

FORCEINLINE BOOLEAN StringSearchIA(
	IN	PCSTR	Haystack,
	IN	PCSTR	Needle)
{
	CHAR NeedleFirst;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);
	
	// cache the first char of the needle for faster search
	NeedleFirst = ToUpper(*Needle);

	while (TRUE) {
		PCSTR Needle2 = Needle;

		while (ToUpper(*Haystack) != NeedleFirst) {
			if (!*++Haystack) {
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && ToUpper(*Haystack) == ToUpper(*Needle2)) {
			if (!*++Needle2) {
				return TRUE;
			}

			if (!*++Haystack) {
				return FALSE;
			}
		}
	}
}

FORCEINLINE BOOLEAN StringSearchW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	while (TRUE) {
		PCWSTR Needle2 = Needle;

		// find first char of needle in haystack
		while (*Haystack != *Needle) {
			if (!*++Haystack) {
				// at end of haystack - needle not found
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && *Haystack == *Needle2) {
			if (!*++Needle2) {
				// end of needle - this means the needle is entirely in the haystack
				return TRUE;
			}

			if (!*++Haystack) {
				// end of haystack - this means not the entire needle in haystack
				return FALSE;
			}
		}
	}
}

FORCEINLINE BOOLEAN StringSearchIW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	WCHAR NeedleFirst;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);
	
	// cache the first char of the needle for faster search
	NeedleFirst = ToUpper(*Needle);

	while (TRUE) {
		PCWSTR Needle2 = Needle;

		while (ToUpper(*Haystack) != NeedleFirst) {
			if (!*++Haystack) {
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && ToUpper(*Haystack) == ToUpper(*Needle2)) {
			if (!*++Needle2) {
				return TRUE;
			}

			if (!*++Haystack) {
				return FALSE;
			}
		}
	}
}

#ifdef _UNICODE
#  define StringCchVPrintfBufferLength StringCchVPrintfBufferLengthW
#  define StringCchPrintfBufferLength StringCchPrintfBufferLengthW
#  define StringAllocPrintf StringAllocPrintfW
#  define StringEqual StringEqualW
#  define StringEqualI StringEqualIW
#  define StringSearch StringSearchW
#  define StringSearchI StringSearchIW
#else
#  define StringCchVPrintfBufferLength StringCchVPrintfBufferLengthA
#  define StringCchPrintfBufferLength StringCchPrintfBufferLengthA
#  define StringAllocPrintf StringAllocPrintfA
#  define StringEqual StringEqualA
#  define StringEqualI StringEqualIA
#  define StringSearch StringSearchA
#  define StringSearchI StringSearchIA
#endif

#pragma deprecated(wcscmp)
#pragma deprecated(_wcsicmp)
#pragma deprecated(_vscprintf)
#pragma deprecated(_vscwprintf)