///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     support.c
//
// Abstract:
//
//     Contains miscellaneous support routines used by KXBASE.
//
// Author:
//
//     vxiiduu (11-Feb-2022)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              11-Feb-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

//
// This function translates a Win32-style timeout (milliseconds) into a
// NT-style timeout (100-nanosecond intervals).
//
// The return value of this function can be directly passed to most
// NT APIs that accept a timeout parameter.
//
PLARGE_INTEGER BaseFormatTimeOut(
	OUT	PLARGE_INTEGER	TimeOut,
	IN	ULONG			Milliseconds)
{
	if (Milliseconds == INFINITE) {
		return NULL;
	}

	TimeOut->QuadPart = UInt32x32To64(Milliseconds, 10000);
	TimeOut->QuadPart *= -1;
	return TimeOut;
}

HANDLE WINAPI BaseGetNamedObjectDirectory(
	VOID)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE OriginalTokenHandle;
	HANDLE DirectoryHandle;
	UNICODE_STRING DirectoryName;
	PPEB Peb;

	if (KexData->BaseNamedObjects) {
		ASSERT (VALID_HANDLE(KexData->BaseNamedObjects));
		return KexData->BaseNamedObjects;
	}

	//
	// Get a pointer to BASE_STATIC_SERVER_DATA.
	// We want the NamedObjectDirectory member, which is a UNICODE_STRING which might
	// look like "\Sessions\1\BaseNamedObjects".
	//
	// Note: Peb->ReadOnlyStaticServerData is NULL prior to the initialization of the
	// BASE DLLs.
	//

	Peb = NtCurrentPeb();
	ASSERT (Peb->ReadOnlyStaticServerData != NULL);

	if (Peb->ReadOnlyStaticServerData == NULL) {
		return NULL;
	}

	if (KexRtlOperatingSystemBitness() != KexRtlCurrentProcessBitness()) {
		PCBASE_STATIC_SERVER_DATA_WOW64 BaseStaticServerData;

		ASSERT (KexRtlCurrentProcessBitness() == 32);
		ASSERT (KexRtlOperatingSystemBitness() == 64);

		//
		// On WOW64, BASE_STATIC_SERVER_DATA is still a 64-bit structure.
		// That's why we need to use the special WOW64 version of the structure.
		// Note that all the pointers are still guaranteed to be within the
		// 32-bit range.
		//

		BaseStaticServerData = (PBASE_STATIC_SERVER_DATA_WOW64) Peb->ReadOnlyStaticServerData[2];
		ASSERT (BaseStaticServerData != NULL);

		DirectoryName.Length		= BaseStaticServerData->NamedObjectDirectory.Length;
		DirectoryName.MaximumLength	= BaseStaticServerData->NamedObjectDirectory.MaximumLength;
		DirectoryName.Buffer		= BaseStaticServerData->NamedObjectDirectory.Buffer;
	} else {
		PCBASE_STATIC_SERVER_DATA BaseStaticServerData;

		BaseStaticServerData = (PBASE_STATIC_SERVER_DATA) Peb->ReadOnlyStaticServerData[1];
		ASSERT (BaseStaticServerData != NULL);

		DirectoryName				= BaseStaticServerData->NamedObjectDirectory;
	}

	ASSERT (NtCurrentPeb()->IsProtectedProcess == 0);
	ASSERT (VALID_UNICODE_STRING(&DirectoryName));

	//
	// If we're impersonating, save the impersonation token, and revert
	// to self for the duration of directory creation.
	//

	if (NtCurrentTeb()->IsImpersonating) {
		HANDLE NewToken;

		Status = NtOpenThreadToken(
			NtCurrentThread(),
			TOKEN_IMPERSONATE,
			TRUE,
			&OriginalTokenHandle);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return NULL;
		}
		
		NewToken = NULL;

		Status = NtSetInformationThread(
			NtCurrentThread(),
			ThreadImpersonationToken,
			&NewToken,
			sizeof(NewToken));

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			SafeClose(OriginalTokenHandle);
			return NULL;
		}
	} else {
		OriginalTokenHandle = NULL;
	}

	InitializeObjectAttributes(
		&ObjectAttributes,
		&DirectoryName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = NtOpenDirectoryObject(
		&DirectoryHandle,
		DIRECTORY_ALL_ACCESS & ~(STANDARD_RIGHTS_REQUIRED),
		&ObjectAttributes);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (VALID_HANDLE(DirectoryHandle));

	if (!NT_SUCCESS(Status)) {
		DirectoryHandle = NULL;
	}

	if (OriginalTokenHandle) {
		Status = NtSetInformationThread(
			NtCurrentThread(),
			ThreadImpersonationToken,
			&OriginalTokenHandle,
			sizeof(OriginalTokenHandle));

		ASSERT (NT_SUCCESS(Status));
		SafeClose(OriginalTokenHandle);
	}

	return DirectoryHandle;
}

HANDLE WINAPI BaseGetUntrustedNamedObjectDirectory(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING DirectoryName;
	OBJECT_ATTRIBUTES ObjectAttributes;

	if (KexData->UntrustedNamedObjects) {
		return KexData->UntrustedNamedObjects;
	}

	RtlInitConstantUnicodeString(&DirectoryName, L"Untrusted");
	InitializeObjectAttributes(
		&ObjectAttributes,
		&DirectoryName,
		OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
		KexData->BaseNamedObjects,
		NULL);

	Status = KexRtlCreateUntrustedDirectoryObject(
		&KexData->UntrustedNamedObjects,
		DIRECTORY_ALL_ACCESS & ~STANDARD_RIGHTS_REQUIRED,
		&ObjectAttributes);

	ASSERT (NT_SUCCESS(Status));

	return KexData->UntrustedNamedObjects;
}

PVOID BaseGetBaseDllHandle(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING Kernel32;

	if (KexData->BaseDllBase) {
		return KexData->BaseDllBase;
	}

	RtlInitConstantUnicodeString(&Kernel32, L"kernel32.dll");

	Status = LdrGetDllHandleByName(
		&Kernel32,
		NULL,
		&KexData->BaseDllBase);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (KexData->BaseDllBase != NULL);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return NULL;
	}

	return KexData->BaseDllBase;
}