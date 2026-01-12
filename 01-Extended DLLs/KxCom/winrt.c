///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     winrt.c
//
// Abstract:
//
//     Implements the Windows Runtime HSTRING functions.
//
// Author:
//
//     vxiiduu (11-Feb-2024)
//
// Environment:
//
//     Win32 mode
//
// Revision History:
//
//     vxiiduu              11-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxcomp.h"

ULONG WINAPI WindowsGetStringLen(
	IN	HSTRING	String)
{
	if (String) {
		return String->Length;
	}

	return 0;
}

PCWSTR WINAPI WindowsGetStringRawBuffer(
	IN	HSTRING	String,
	OUT	PULONG	Length OPTIONAL)
{
	if (Length) {
		*Length = WindowsGetStringLen(String);
	}

	if (String) {
		return String->StringRef;
	} else {
		return L"";
	}
}

HRESULT WINAPI WindowsCreateString(
	IN	PCNZWCH			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING			*String)
{
	ULONG AllocationCb;
	HSTRING_ALLOCATED *AllocatedString;

	if (!String) {
		return E_INVALIDARG;
	}

	*String = NULL;

	if (!SourceStringCch) {
		// just leave a null pointer in *String
		return S_OK;
	}

	AllocationCb = SourceStringCch * 2;

	// check for overflow
	if (AllocationCb < SourceStringCch) {
		return MEM_E_INVALID_SIZE;
	}

	if (AllocationCb + sizeof(HSTRING_ALLOCATED) < AllocationCb) {
		return MEM_E_INVALID_SIZE;
	}

	AllocationCb += sizeof(HSTRING_ALLOCATED);

	AllocatedString = (HSTRING_ALLOCATED *) SafeAlloc(BYTE, AllocationCb);
	if (!AllocatedString) {
		return E_OUTOFMEMORY;
	}

	// copy and null terminate the string data
	RtlCopyMemory(AllocatedString->Data, SourceString, SourceStringCch * sizeof(WCHAR));
	AllocatedString->Data[SourceStringCch] = '\0';

	AllocatedString->Header.Flags = WRHF_NONE;
	AllocatedString->Header.Length = SourceStringCch;
	AllocatedString->Header.StringRef = AllocatedString->Data;
	AllocatedString->RefCount = 1;
	
	*String = &AllocatedString->Header;

	return S_OK;
}

HRESULT WINAPI WindowsCreateStringReference(
	IN	PCWSTR			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING_HEADER	*StringHeader,
	OUT	HSTRING			*String)
{
	if (!String || !StringHeader) {
		return E_INVALIDARG;
	}

	*String = NULL;

	if (SourceString && SourceString[SourceStringCch] != '\0') {
		return E_STRING_NOT_NULL_TERMINATED;
	}

	if (SourceStringCch && !SourceString) {
		return E_POINTER;
	}

	if (SourceStringCch != 0) {
		StringHeader->Flags = WRHF_STRING_REFERENCE;
		StringHeader->Length = SourceStringCch;
		StringHeader->StringRef = SourceString;
		*String = StringHeader;
	}

	return S_OK;
}

HRESULT WINAPI WindowsDuplicateString(
	IN	HSTRING	OriginalString,
	OUT	HSTRING	*DuplicatedString)
{
	if (!DuplicatedString) {
		return E_INVALIDARG;
	}

	*DuplicatedString = NULL;

	if (!OriginalString) {
		// leave it as NULL
		return S_OK;
	}

	if (OriginalString->Flags & WRHF_STRING_REFERENCE) {
		// allocate a copy
		return WindowsCreateString(
			OriginalString->StringRef,
			OriginalString->Length,
			DuplicatedString);
	} else {
		HSTRING_ALLOCATED *AllocatedString;

		AllocatedString = (HSTRING_ALLOCATED *) OriginalString;

		// just increment the ref count
		InterlockedIncrement(&AllocatedString->RefCount);
		*DuplicatedString = OriginalString;
	}

	return S_OK;
}

HRESULT WINAPI WindowsDeleteString(
	IN	HSTRING	String)
{
	if (String) {
		if (!(String->Flags & WRHF_STRING_REFERENCE)) {
			HSTRING_ALLOCATED *AllocatedString;

			AllocatedString = (HSTRING_ALLOCATED *) String;

			if (InterlockedDecrement(&AllocatedString->RefCount) == 0) {
				SafeFree(AllocatedString);
			}
		}
	}

	return S_OK;
}

BOOL WINAPI WindowsIsStringEmpty(
	IN	HSTRING	String)
{
	if (String != NULL && String->Length == 0) {
		return TRUE;
	}

	return FALSE;
}

HRESULT WINAPI WindowsStringHasEmbeddedNull(
	IN	HSTRING	String,
	OUT	PBOOL	HasEmbeddedNull)
{
	PCWCHAR Pointer;
	PCWCHAR EndOfString;

	if (HasEmbeddedNull == NULL) {
		return E_INVALIDARG;
	}

	*HasEmbeddedNull = FALSE;

	if (!String || String->Length == 0) {
		return S_OK;
	}

	if (String->Flags & WRHF_EMBEDDED_NULLS_COMPUTED) {
		// We know whether or not this string has embedded nulls, because
		// this string has already been through this function at least once.
		*HasEmbeddedNull = (String->Flags & WRHF_HAS_EMBEDDED_NULLS);
		return S_OK;
	}
		
	//
	// We don't know yet whether this string has embedded nulls.
	//

	Pointer = String->StringRef;
	EndOfString = &String->StringRef[String->Length];

	while (Pointer < EndOfString) {
		if (*Pointer == '\0') {
			String->Flags |= WRHF_HAS_EMBEDDED_NULLS;
			*HasEmbeddedNull = TRUE;
			break;
		}

		++Pointer;
	}

	//
	// Note down that we have already calculated whether this string has
	// embedded null characters in it, so next time this function is called,
	// we don't have to do much work.
	//

	String->Flags |= WRHF_EMBEDDED_NULLS_COMPUTED;
	return S_OK;
}

//
// *ComparisonResult will be:
//  -1 if String2 is "greater than" String1
//   0 if the strings are equal
//   1 if String1 is "greater than" String2
//
HRESULT WINAPI WindowsCompareStringOrdinal(
	IN	HSTRING	String1,
	IN	HSTRING	String2,
	OUT	PINT	ComparisonResult)
{
	INT TemporaryComparisonResult;
	BOOLEAN String2WasNull;

	if (ComparisonResult == NULL) {
		return E_INVALIDARG;
	}

	if (String1 == String2) {
		// the string pointers are equal so the strings must be equal
		*ComparisonResult = 0;
		return S_OK;
	}

	if (String2 == NULL) {
		// I have no idea what this is intended to accomplish, but it's in the original
		// Windows function...
		String2 = String1;
		String1 = NULL;
		String2WasNull = TRUE;
	}

	if (String1 == NULL) {
		if (String2->Length == 0) {
			*ComparisonResult = 0;
		} else {
			if (String2WasNull) {
				*ComparisonResult = 1;
			} else {
				*ComparisonResult = -1;
			}
		}

		return S_OK;
	}

	TemporaryComparisonResult = CompareStringOrdinal(
		String1->StringRef,
		String1->Length,
		String2->StringRef,
		String2->Length,
		FALSE);

	*ComparisonResult = 0;

	if (TemporaryComparisonResult == CSTR_LESS_THAN) {
		*ComparisonResult = -1;
	} else if (TemporaryComparisonResult == CSTR_GREATER_THAN) {
		*ComparisonResult = 1;
	}

	return S_OK;
}

HRESULT WINAPI WindowsSubstring(
	IN	HSTRING	String,
	IN	ULONG	StartIndex,
	OUT	HSTRING	*NewString)
{
	if (!NewString) {
		return E_INVALIDARG;
	}

	*NewString = NULL;

	if (String != NULL && StartIndex > String->Length ||
		String == NULL && StartIndex != 0) {

		// start index is beyond the end of the original string
		return E_BOUNDS;
	}

	if (String == NULL || String->Length == 0 || StartIndex == String->Length) {
		// leave an empty (NULL) string as the output
		return S_OK;
	}

	return WindowsCreateString(
		&String->StringRef[StartIndex],
		String->Length - StartIndex,
		NewString);
}

HRESULT WINAPI WindowsSubstringWithSpecifiedLength(
	IN	HSTRING	OriginalString,
	IN	ULONG	StartIndex,
	IN	ULONG	SubstringLength,
	OUT	HSTRING	*NewString)
{
	if (!NewString) {
		return E_INVALIDARG;
	}

	*NewString = NULL;

	if (OriginalString != NULL && StartIndex > OriginalString->Length ||
		OriginalString == NULL && (StartIndex != 0 || SubstringLength != 0)) {

		return E_BOUNDS;
	}

	if (StartIndex + SubstringLength < StartIndex) {
		return HRESULT_ARITHMETIC_OVERFLOW;
	}

	if (OriginalString != NULL && StartIndex + SubstringLength > OriginalString->Length) {
		return E_BOUNDS;
	}

	if (!OriginalString || OriginalString->Length == 0 || SubstringLength == 0) {
		// leave empty NULL string as *NewString
		return S_OK;
	}

	return WindowsCreateString(
		&OriginalString->StringRef[StartIndex],
		SubstringLength,
		NewString);
}