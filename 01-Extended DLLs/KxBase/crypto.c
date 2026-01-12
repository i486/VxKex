#include "buildcfg.h"
#include "kxbasep.h"
#include <bcrypt.h>

//
// This function creates random data and places it into the specified
// buffer.
//
KXBASEAPI BOOL WINAPI ProcessPrng(
	OUT	PBYTE	Buffer,
	IN	SIZE_T	BufferCb)
{
	NTSTATUS Status;
	HANDLE KsecDD;
	UNICODE_STRING DeviceName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	ASSERT (Buffer != NULL);
	ASSERT (BufferCb <= ULONG_MAX);

	RtlInitConstantUnicodeString(&DeviceName, L"\\Device\\KsecDD");
	InitializeObjectAttributes(&ObjectAttributes, &DeviceName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenFile(
		&KsecDD,
		FILE_READ_DATA | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	Status = NtDeviceIoControlFile(
		KsecDD,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_KSEC_RANDOM_FILL_BUFFER,
		NULL,
		0,
		Buffer,
		(ULONG) BufferCb);

	ASSERT (NT_SUCCESS(Status));
	SafeClose(KsecDD);

	return NT_SUCCESS(Status);
}

//
// This is just a convenience function which wraps other functions in bcrypt.dll.
//

KXBASEAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PBYTE				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PBYTE				Input,
	IN	ULONG				InputCb,
	OUT	PBYTE				Output,
	IN	ULONG				OutputCb)
{
	NTSTATUS Status;
	BCRYPT_HASH_HANDLE HashHandle;

	HashHandle = NULL;

	Status = BCryptCreateHash(Algorithm, &HashHandle, 0, 0, Secret, SecretCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptHashData(HashHandle, Input, InputCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptFinishHash(HashHandle, Output, OutputCb, 0);

CleanupExit:
	if (HashHandle) {
		BCryptDestroyHash(HashHandle);
	}

	return Status;
}