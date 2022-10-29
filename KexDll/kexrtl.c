///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexrtl.c
//
// Abstract:
//
//     Various useful run-time routines.
//
// Author:
//
//     vxiiduu (17-Oct-2022)
//
// Revision History:
//
//     vxiiduu              17-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

// Examples:
// C:\Windows\system32\notepad.exe -> notepad.exe
// notepad.exe -> notepad.exe
// dir1\dir2\notepad.exe -> dir1\dir2\notepad.exe
//
// (As you can see, this function only works on FULL paths - otherwise,
// the output path is unchanged.)
KEXAPI NTSTATUS NTAPI KexRtlPathFindFileName(
	IN	PCUNICODE_STRING Path,
	OUT	PUNICODE_STRING FileName)
{
	ULONG LengthWithoutLastElement;

	if (!Path) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!FileName) {
		return STATUS_INVALID_PARAMETER_2;
	}

	//
	// If Path->Buffer contains a path with no backslashes, this function
	// will fail and set LengthWithoutLastElement to zero. This is desired
	// and that's why the return value is not checked.
	//
	RtlGetLengthWithoutLastFullDosOrNtPathElement(0, Path, &LengthWithoutLastElement);

	FileName->Buffer = Path->Buffer + LengthWithoutLastElement;
	FileName->Length = Path->Length - (USHORT) LengthWithoutLastElement;
	FileName->MaximumLength = Path->MaximumLength - (USHORT) LengthWithoutLastElement;

	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI KexRtlGetProcessImageBaseName(
	OUT	PUNICODE_STRING	FileName)
{
	return KexRtlPathFindFileName(&NtCurrentPeb()->ProcessParameters->ImagePathName, FileName);
}

//
// NtQueryKeyValue is too annoying to use in everyday code, RtlQueryRegistryValues
// is unsafe, and RtlpNtQueryKeyValue only supports the default/unnamed key. So
// here is an API that essentially mimics the function of win32 RegGetValue.
//
//   KeyHandle - Handle to an open registry key.
//
//   ValueName - Name of the value to query.
//
//   ValueDataCb - Points to size, in bytes, of the buffer indicated by ValueData.
//                 Upon successful return, contains the size of the data retrieved
//                 from the registry.
//                 If this value is 0 before the function is called, the function
//                 will return with STATUS_INSUFFICIENT_BUFFER, not check the
//                 ValueData parameter, and place the correct buffer size required
//                 to store the requested registry data in *ValueDataCb.
//
//   ValueData - Buffer which holds the returned data. If NULL, the function will
//               fail with STATUS_INVALID_PARAMETER (unless ValueDataCb is zero).
//
//   ValueDataTypeRestrict - Indicates which data types are allowed to be returned.
//                           One or more flags from the REG_RESTRICT_* set can be
//                           passed. If the data type of the value in the registry
//                           does not match these filters, the function will return
//                           STATUS_OBJECT_TYPE_MISMATCH and *ValueDataType will
//                           contain the type of the registry data.
//
//   ValueDataType - If function returns successfully, contains the data type of the
//                   data read from the registry.
//
// If this function returns with a failure code, the buffer pointed to by ValueData
// is unmodified.
//
KEXAPI NTSTATUS NTAPI KexRtlQueryKeyValueData(
	IN		HANDLE				KeyHandle,
	IN		PCUNICODE_STRING	ValueName,
	IN OUT	PULONG				ValueDataCb,
	OUT		PVOID				ValueData OPTIONAL,
	IN		ULONG				ValueDataTypeRestrict,
	OUT		PULONG				ValueDataType OPTIONAL)
{
	NTSTATUS Status;
	PVOID KeyValueBuffer;
	ULONG KeyValueBufferCb;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;

	//
	// Validate parameters.
	//
	if (!KeyHandle || KeyHandle == INVALID_HANDLE_VALUE) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!ValueName) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!ValueDataCb) {
		return STATUS_INVALID_PARAMETER_3;
	}

	if (ValueData != NULL && *ValueDataCb == 0) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	if (!ValueData && *ValueDataCb != 0) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	if (!ValueDataTypeRestrict || (ValueDataTypeRestrict & (~LEGAL_REG_RESTRICT_MASK))) {
		return STATUS_INVALID_PARAMETER_5;
	}

	//
	// First of all, check if the caller just wants to know the length
	// of buffer required.
	//

	if (*ValueDataCb == 0) {
		Status = NtQueryValueKey(
			KeyHandle,
			ValueName,
			KeyValuePartialInformation,
			NULL,
			0,
			ValueDataCb);

		if (Status == STATUS_BUFFER_TOO_SMALL) {
			*ValueDataCb -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);
		}

		return Status;
	}

	//
	// Now we allocate a buffer to store the KEY_VALUE_PARTIAL_INFORMATION
	// structure in addition to any data read from the registry.
	//
	
	KeyValueBufferCb = *ValueDataCb + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
	KeyValueBuffer = SafeAlloc(BYTE, KeyValueBufferCb);

	if (!KeyValueBuffer) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Status = NtQueryValueKey(
		KeyHandle,
		ValueName,
		KeyValuePartialInformation,
		KeyValueBuffer,
		KeyValueBufferCb,
		ValueDataCb);

	if (!NT_SUCCESS(Status)) {
		goto Exit;
	}

	*ValueDataCb -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);
	KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueBuffer;

	//
	// Now, we check the data type of the returned value to make sure it
	// is matched by the ValueDataTypeRestrict filter.
	//

	unless (ValueDataTypeRestrict & (1 << KeyValueInformation->Type)) {
		Status = STATUS_OBJECT_TYPE_MISMATCH;
		goto Exit;
	}

	//
	// Copy the result into the caller's buffer.
	//

	RtlCopyMemory(ValueData, KeyValueInformation->Data, KeyValueInformation->DataLength);

Exit:
	if (NT_SUCCESS(Status) || Status == STATUS_OBJECT_TYPE_MISMATCH) {
		if (ValueDataType) {
			*ValueDataType = KeyValueInformation->Type;
		}
	}

	SafeFree(KeyValueBuffer);
	return Status;
}

//
// Query multiple values of a key.
//
// KeyHandle
//   Handle to an open registry key under which to query values.
//
// QueryTable
//   Pointer to an array of KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY
//   structures which provide space to store input and output parameters to
//   the KexRtlQueryKeyValueData routine.
//
// NumberOfQueryTableElements
//   Pointer to number of elements in the array pointed to by QueryTable.
//   Upon return, the number pointed to by this parameter contains the number
//   of values successfully queried.
//
// Flags
//   Valid "Flags" parameters start with QUERY_KEY_MULTIPLE_VALUE_:
//
//   QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST (1)
//     Fail and return a failure code if one of the values in the query
//     table cannot be queried. By default, on failure to query a value
//     this function will simply record failure status inside the query
//     table entry, continue to the next entry and return success once
//     all values have been queried.
//
KEXAPI NTSTATUS NTAPI KexRtlQueryKeyMultipleValueData(
	IN		HANDLE												KeyHandle,
	IN		PKEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY	QueryTable,
	IN OUT	PULONG												NumberOfQueryTableElements,
	IN		ULONG												Flags)
{
	ULONG Counter;

	if (!QueryTable) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (!NumberOfQueryTableElements || *NumberOfQueryTableElements == 0) {
		return STATUS_INVALID_PARAMETER_3;
	}

	Counter = *NumberOfQueryTableElements;
	*NumberOfQueryTableElements = 0;

	if (Flags & ~(QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST)) {
		return STATUS_INVALID_PARAMETER_4;
	}

	do {
		QueryTable->Status = KexRtlQueryKeyValueData(
			KeyHandle,
			&QueryTable->ValueName,
			&QueryTable->ValueDataCb,
			QueryTable->ValueData,
			QueryTable->ValueDataTypeRestrict,
			&QueryTable->ValueDataType);

		if (Flags & QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST) {
			if (!NT_SUCCESS(QueryTable->Status)) {
				return STATUS_UNSUCCESSFUL;
			}
		}

		++QueryTable;
		++*NumberOfQueryTableElements;
	} while (--Counter);

	return STATUS_SUCCESS;
}

//
// Check whether a string ends with another string.
// For example, you can use this to see if a filename has a particular
// extension.
//
KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringEndsWith(
	IN	PCUNICODE_STRING	String,
	IN	PCUNICODE_STRING	EndsWith,
	IN	BOOLEAN				CaseInsensitive)
{
	UNICODE_STRING EndOfString;

	//
	// Create a subset of the String that just contains the
	// end of it (with the number of characters that EndsWith
	// contains).
	//

	EndOfString.Buffer = String->Buffer + KexRtlUnicodeStringCch(String) - KexRtlUnicodeStringCch(EndsWith);
	EndOfString.Length = EndsWith->Length;
	EndOfString.MaximumLength = EndsWith->Length;

	if (EndOfString.Buffer < String->Buffer) {
		// EndsWith length greater than String length
		return FALSE;
	}

	//
	// Now perform the actual check.
	//

	return RtlEqualUnicodeString(&EndOfString, EndsWith, CaseInsensitive);
}