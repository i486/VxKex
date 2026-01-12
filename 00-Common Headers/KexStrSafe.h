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

#ifndef until
#  define until(x) while(!(x))
#endif

//
// The StringCchPrintfBufferLength family of functions returns the required
// number of characters, including space for the null terminator, you need
// to allocate in a buffer to hold a formatted string.
//

INLINE HRESULT StringCchVPrintfBufferLengthA(
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

INLINE HRESULT StringCchVPrintfBufferLengthW(
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

INLINE HRESULT StringCchPrintfBufferLengthA(
	OUT	PSIZE_T	BufferCch,
	IN	PCSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	HRESULT Result;

	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLengthA(BufferCch, Format, ArgList);
	return Result;
}

INLINE HRESULT StringCchPrintfBufferLengthW(
	OUT	PSIZE_T	BufferCch,
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	HRESULT Result;

	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLengthW(BufferCch, Format, ArgList);
	return Result;
}

//
// The StringAllocPrintf family of functions allocates a buffer on the process
// heap to hold a formatted string.
//
// You need to pass the returned Buffer to SafeFree if the function succeeds.
//

INLINE HRESULT StringAllocPrintfA(
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
	
	return Result;
}

INLINE HRESULT StringAllocPrintfW(
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

	return Result;
}

// These macros are generic and can operate on both ANSI and Unicode.
#define ToUpper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - 32) : (c))
#define ToLower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) + 32) : (c))

// The StringEqual family of functions returns 0 if the strings are
// different and 1 if the strings are equal.
// The StringEqualI family does the same things except it is case
// insensitive.

INLINE BOOLEAN StringEqualA(
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

INLINE BOOLEAN StringEqualIA(
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

INLINE BOOLEAN StringEqualW(
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

INLINE BOOLEAN StringEqualIW(
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

INLINE BOOLEAN StringSearchA(
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

INLINE BOOLEAN StringSearchIA(
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

INLINE BOOLEAN StringSearchW(
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

INLINE BOOLEAN StringSearchIW(
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

INLINE BOOLEAN StringBeginsWithW(
	IN	PCWSTR	String,
	IN	PCWSTR	Prefix)
{
	ULONG Index;

	ASSERT (String != NULL);
	ASSERT (Prefix != NULL);

	if (String[0] == '\0') {
		return FALSE;
	}

	if (Prefix[0] == '\0') {
		return TRUE;
	}

	Index = 0;

	while (String[Index] == Prefix[Index] && Prefix[Index] != '\0') {
		++Index;
	}

	if (Prefix[Index] == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

INLINE BOOLEAN StringBeginsWithA(
	IN	PCSTR	String,
	IN	PCSTR	Prefix)
{
	ULONG Index;

	ASSERT (String != NULL);
	ASSERT (Prefix != NULL);

	if (String[0] == '\0') {
		return FALSE;
	}

	if (Prefix[0] == '\0') {
		return TRUE;
	}

	Index = 0;

	while (String[Index] == Prefix[Index] && Prefix[Index] != '\0') {
		++Index;
	}

	if (Prefix[Index] == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

INLINE BOOLEAN StringBeginsWithIW(
	IN	PCWSTR	String,
	IN	PCWSTR	Prefix)
{
	ULONG Index;

	ASSERT (String != NULL);
	ASSERT (Prefix != NULL);

	if (String[0] == '\0') {
		return FALSE;
	}

	if (Prefix[0] == '\0') {
		return TRUE;
	}

	Index = 0;

	while (towupper(String[Index]) == towupper(Prefix[Index]) && Prefix[Index] != '\0') {
		++Index;
	}

	if (Prefix[Index] == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

INLINE BOOLEAN StringBeginsWithIA(
	IN	PCSTR	String,
	IN	PCSTR	Prefix)
{
	ULONG Index;

	ASSERT (String != NULL);
	ASSERT (Prefix != NULL);

	if (String[0] == '\0') {
		return FALSE;
	}

	if (Prefix[0] == '\0') {
		return TRUE;
	}

	Index = 0;

	while (toupper(String[Index]) == toupper(Prefix[Index]) && Prefix[Index] != '\0') {
		++Index;
	}

	if (Prefix[Index] == '\0') {
		return TRUE;
	} else {
		return FALSE;
	}
}

INLINE PCSTR StringFindA(
	IN	PCSTR	Haystack,
	IN	PCSTR	Needle)
{
	CHAR NeedleFirst;
	ULONG Index;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	if (Haystack[0] == '\0' || Needle[0] == '\0') {
		return NULL;
	}

	NeedleFirst = Needle[0];
	Index = 0;
	
	while (TRUE) {
		until (Haystack[Index] == NeedleFirst || Haystack[Index] == '\0') {
			++Index;
		}

		if (Haystack[Index] == '\0') {
			return NULL;
		}

		if (StringBeginsWithA(&Haystack[Index], Needle)) {
			return &Haystack[Index];
		}

		++Index;
	}
}

INLINE PCSTR StringFindIA(
	IN	PCSTR	Haystack,
	IN	PCSTR	Needle)
{
	CHAR NeedleFirst;
	ULONG Index;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	if (Haystack[0] == '\0' || Needle[0] == '\0') {
		return NULL;
	}

	NeedleFirst = ToUpper(Needle[0]);
	Index = 0;
	
	while (TRUE) {
		until (ToUpper(Haystack[Index]) == NeedleFirst || Haystack[Index] == '\0') {
			++Index;
		}

		if (Haystack[Index] == '\0') {
			return NULL;
		}

		if (StringBeginsWithIA(&Haystack[Index], Needle)) {
			return &Haystack[Index];
		}

		++Index;
	}
}

INLINE PCWSTR StringFindW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	WCHAR NeedleFirst;
	ULONG Index;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	if (Haystack[0] == '\0' || Needle[0] == '\0') {
		return NULL;
	}

	NeedleFirst = Needle[0];
	Index = 0;
	
	while (TRUE) {
		until (Haystack[Index] == NeedleFirst || Haystack[Index] == '\0') {
			++Index;
		}

		if (Haystack[Index] == '\0') {
			return NULL;
		}

		if (StringBeginsWithW(&Haystack[Index], Needle)) {
			return &Haystack[Index];
		}

		++Index;
	}
}

INLINE PCWSTR StringFindIW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	WCHAR NeedleFirst;
	ULONG Index;

	ASSERT (Haystack != NULL);
	ASSERT (Needle != NULL);

	if (Haystack[0] == '\0' || Needle[0] == '\0') {
		return NULL;
	}

	NeedleFirst = ToUpper(Needle[0]);
	Index = 0;
	
	while (TRUE) {
		until (ToUpper(Haystack[Index]) == NeedleFirst || Haystack[Index] == '\0') {
			++Index;
		}

		if (Haystack[Index] == '\0') {
			return NULL;
		}

		if (StringBeginsWithIW(&Haystack[Index], Needle)) {
			return &Haystack[Index];
		}

		++Index;
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
#  define StringBeginsWith StringBeginsWithW
#  define StringBeginsWithI StringBeginsWithIW
#  define StringFind StringFindW
#  define StringFindI StringFindIW
#else
#  define StringCchVPrintfBufferLength StringCchVPrintfBufferLengthA
#  define StringCchPrintfBufferLength StringCchPrintfBufferLengthA
#  define StringAllocPrintf StringAllocPrintfA
#  define StringEqual StringEqualA
#  define StringEqualI StringEqualIA
#  define StringSearch StringSearchA
#  define StringSearchI StringSearchIA
#  define StringBeginsWith StringBeginsWithA
#  define StringBeginsWithI StringBeginsWithIA
#  define StringFind StringFindA
#  define StringFindI StringFindIA
#endif

#define StringLiteralLength(StringLiteral) (ARRAYSIZE(StringLiteral) - 1)

#pragma deprecated(wcscmp)
#pragma deprecated(_wcsicmp)
#pragma deprecated(_vscprintf)
#pragma deprecated(_vscwprintf)
#pragma deprecated(wcsstr)
#pragma deprecated(strstr)