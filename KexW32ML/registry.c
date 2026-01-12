///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     registry.c
//
// Abstract:
//
//     Registry convenience functions
//
// Author:
//
//     vxiiduu (02-Oct-2022)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegReadI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PULONG	Data)
{
	ULONG ErrorCode;
	DWORD Discard;

	ASSERT (Key != NULL);
	ASSERT (Key != INVALID_HANDLE_VALUE);
	ASSERT (Data != NULL);
	
	Discard = sizeof(*Data);
	
	ErrorCode = RegGetValue(
		Key,
		SubKey,
		ValueName,
		RRF_RT_DWORD,
		NULL,
		Data,
		&Discard);

	if (ErrorCode != ERROR_SUCCESS) {
		*Data = 0;
	}

	return ErrorCode;
}

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegWriteI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	ULONG	Data)
{
	ASSERT (Key != NULL);
	ASSERT (Key != INVALID_HANDLE_VALUE);

	return RegSetKeyValue(
		Key,
		SubKey,
		ValueName,
		REG_DWORD,
		&Data,
		sizeof(Data));
}

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegReadString(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PWSTR	Buffer,
	IN	ULONG	BufferCch)
{
	ULONG BufferCb;

	BufferCb = BufferCch * sizeof(WCHAR);

	if (Buffer && BufferCch) {
		*Buffer = '\0';
	}

	return RegGetValue(
		Key,
		SubKey,
		ValueName,
		RRF_RT_REG_SZ,
		NULL,
		Buffer,
		&BufferCb);
}

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegWriteString(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	PCWSTR	Data)
{
	ULONG DataCb;

	ASSERT (Key != NULL);
	ASSERT (Key != INVALID_HANDLE_VALUE);
	ASSERT (Data != NULL);

	DataCb = (ULONG) (wcslen(Data) + 1) * sizeof(WCHAR);

	return RegSetKeyValue(
		Key,
		SubKey,
		ValueName,
		REG_SZ,
		Data,
		DataCb);
}

// Used for reopening a specific key with different permissions.
// This function closes the existing key.
// Upon failure, this function returns FALSE and does not modify
// the existing key. Call GetLastError for more information.
KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI RegReOpenKey(
	IN OUT	PHKEY		KeyHandle,
	IN		ACCESS_MASK	NewAccessMask,
	IN		HANDLE		TransactionHandle OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING ObjectName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HKEY NewKeyHandle;

	ASSERT (KeyHandle != NULL);
	ASSERT (*KeyHandle != NULL);
	ASSERT (*KeyHandle != INVALID_HANDLE_VALUE);

	RtlInitConstantUnicodeString(&ObjectName, L"");
	InitializeObjectAttributes(&ObjectAttributes, &ObjectName, 0, (HANDLE) *KeyHandle, NULL);

	if (TransactionHandle) {
		Status = NtOpenKeyTransacted(
			(PHANDLE) &NewKeyHandle,
			NewAccessMask,
			&ObjectAttributes,
			TransactionHandle);
	} else {
		Status = NtOpenKey(
			(PHANDLE) &NewKeyHandle,
			NewAccessMask,
			&ObjectAttributes);
	}

	SetLastError(RtlNtStatusToDosError(Status));

	if (NT_SUCCESS(Status)) {
		NtClose(*KeyHandle);
		*KeyHandle = NewKeyHandle;
		return TRUE;
	} else {
		return FALSE;
	}
}