///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     reggval.c
//
// Abstract:
//
//     Contains an extended RegGetValue function.
//
// Author:
//
//     vxiiduu (22-Oct-2024)
//
// Revision History:
//
//     vxiiduu              22-Oct-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxadvapip.h"

//
// Helper function declarations.
//

STATIC LSTATUS RegGetValueInternalAorW(
	IN		HKEY	KeyHandle,
	IN		PCVOID	SubKeyName OPTIONAL,
	IN		PCVOID	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	Data OPTIONAL,
	IN OUT	PULONG	DataCb OPTIONAL,
	IN		BOOLEAN	Ansi);

//
// These two exported functions dispatch to the shared
// RegGetValueInternalAorW logic.
//

KXADVAPI LSTATUS WINAPI Ext_RegGetValueA(
	IN		HKEY	KeyHandle,
	IN		PCSTR	SubKeyName OPTIONAL,
	IN		PCSTR	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	Data OPTIONAL,
	IN OUT	PULONG	DataCb OPTIONAL)
{
	return RegGetValueInternalAorW(
		KeyHandle,
		SubKeyName,
		ValueName,
		Flags,
		DataType,
		Data,
		DataCb,
		TRUE);
}

KXADVAPI LSTATUS WINAPI Ext_RegGetValueW(
	IN		HKEY	KeyHandle,
	IN		PCWSTR	SubKeyName OPTIONAL,
	IN		PCWSTR	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	Data OPTIONAL,
	IN OUT	PULONG	DataCb OPTIONAL)
{
	return RegGetValueInternalAorW(
		KeyHandle,
		SubKeyName,
		ValueName,
		Flags,
		DataType,
		Data,
		DataCb,
		FALSE);
}

//
// RegGetValue behaves differently on Windows 7 and on Windows 8.1 and later
// versions. On Windows 7, when Flags:
//   - does not contain RRF_RT_ANY (0xFFFF), and
//   - contains RRF_RT_REG_EXPAND_SZ, and
//   - does not contain RRF_NOEXPAND
// the function will fail with ERROR_INVALID_PARAMETER.
//
// In other words, RegGetValue on Wind ows 7 will refuse to automatically
// expand REG_EXPAND_SZ values. RegGetValue on Windows 8.1 and above will
// automatically expand these values.
//
// In order to work around this problem, if Flags contains RRF_RT_REG_EXPAND_SZ
// but not RRF_RT_ANY and not RRF_NOEXPAND, then we will add RRF_NOEXPAND to
// the flags and set a boolean indicating that we have done so.
//
// We will then call the original RegGetValue. If it turns out that the data
// type is REG_EXPAND_SZ, then we will manually expand it by calling
// ExpandEnvironmentStrings.
//

STATIC LSTATUS RegGetValueInternalAorW(
	IN		HKEY	KeyHandle,
	IN		PCVOID	SubKeyName OPTIONAL,
	IN		PCVOID	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	DataBuffer OPTIONAL,
	IN OUT	PULONG	DataBufferCb OPTIONAL,
	IN		BOOLEAN	Ansi)
{
	LSTATUS LStatus;
	BOOLEAN ModifiedFlags;
	ULONG DataTypeBuffer;
	ULONG DataBufferCbOriginal;
	ULONG DataBufferCch;
	ULONG DataCchExpanded;
	PVOID TemporaryDataBuffer;

	ModifiedFlags = FALSE;
	DataBufferCbOriginal = 0;

	if ((Flags & RRF_RT_REG_EXPAND_SZ) &&
		(Flags & RRF_NOEXPAND) == 0 &&
		(Flags & RRF_RT_ANY) != RRF_RT_ANY) {

		Flags |= RRF_NOEXPAND;
		ModifiedFlags = TRUE;

		if (DataBuffer != NULL) {
			if (DataType == NULL) {
				// We need to get the data type of the returned data, if any,
				// so we can know whether to call ExpandEnvironmentStrings.
				DataType = &DataTypeBuffer;
			}

			if (DataBufferCb != NULL) {
				// Save the original size of the caller-supplied buffer.
				DataBufferCbOriginal = *DataBufferCb;
			}
		}
	}

	if (Ansi) {
		LStatus = RegGetValueA(
			KeyHandle,
			(PCSTR) SubKeyName,
			(PCSTR) ValueName,
			Flags,
			DataType,
			DataBuffer,
			DataBufferCb);
	} else {
		LStatus = RegGetValueW(
			KeyHandle,
			(PCWSTR) SubKeyName,
			(PCWSTR) ValueName,
			Flags,
			DataType,
			DataBuffer,
			DataBufferCb);
	}

	if (!ModifiedFlags || LStatus != ERROR_SUCCESS ||
		DataBuffer == NULL || *DataType != REG_EXPAND_SZ) {

		// We don't have anything further to do here.
		return LStatus;
	}

	if (DataBufferCb == NULL || *DataBufferCb == 0) {
		// Can't expand an empty string.
		return LStatus;
	}

	//
	// If we've reached this point:
	//   - RegGetValue has succeeded, and written data into the buffer
	//   - we modified flags for compatibility reasons, and the caller has
	//     requested us to expand environment strings in the returned data
	//   - the type of returned data is REG_EXPAND_SZ
	//
	// We need to expand the environment strings.
	//

	ASSERT (ModifiedFlags == TRUE);
	ASSERT (LStatus == ERROR_SUCCESS);
	
	ASSERT (DataBuffer != NULL);
	
	ASSERT (DataType != NULL);
	ASSERT (*DataType == REG_EXPAND_SZ);

	ASSERT (DataBufferCb != NULL);
	ASSERT (*DataBufferCb != 0);
	ASSERT (DataBufferCbOriginal != 0);

	KexLogDebugEvent(L"RegGetValue compatibility workaround used");
	
	//
	// Copy the returned data to a temporary buffer and then use either
	// ExpandEnvironmentStringsA or ExpandEnvironmentStringsW to put it back
	// into the caller-supplied buffer.
	//

	// DataCb is the actual size of the returned data, not the buffer size
	TemporaryDataBuffer = (PVOID) SafeAlloc(BYTE, *DataBufferCb);

	if (TemporaryDataBuffer == NULL) {
		if (Flags & RRF_ZEROONFAILURE) {
			RtlZeroMemory(DataBuffer, DataBufferCbOriginal);
		}

		return ERROR_OUTOFMEMORY;
	}

	RtlCopyMemory(TemporaryDataBuffer, DataBuffer, *DataBufferCb);

	if (Ansi) {
		DataBufferCch = DataBufferCbOriginal;

		DataCchExpanded = ExpandEnvironmentStringsA(
			(PCSTR) TemporaryDataBuffer,
			(PSTR) DataBuffer,
			DataBufferCch);
	} else {
		DataBufferCch = DataBufferCbOriginal / sizeof(WCHAR);

		DataCchExpanded = ExpandEnvironmentStringsW(
			(PCWSTR) TemporaryDataBuffer,
			(PWSTR) DataBuffer,
			DataBufferCch);
	}

	SafeFree(TemporaryDataBuffer);

	if (DataCchExpanded > DataBufferCch) {
		// The expanded string is longer than what the caller buffer could
		// hold.

		if (Flags & RRF_ZEROONFAILURE) {
			RtlZeroMemory(DataBuffer, DataBufferCbOriginal);
		}

		return ERROR_MORE_DATA;
	}

	return ERROR_SUCCESS;
}