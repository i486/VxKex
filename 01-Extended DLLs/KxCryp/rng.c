#include "buildcfg.h"
#include "kxcrypp.h"

STATIC HANDLE KsecDD = NULL;

STATIC NTSTATUS InitializeRng(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING DeviceName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	RtlInitConstantUnicodeString(&DeviceName, L"\\Device\\KsecDD");

	InitializeObjectAttributes(
		&ObjectAttributes,
		&DeviceName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = NtOpenFile(
		&KsecDD,
		FILE_READ_DATA | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (VALID_HANDLE(KsecDD));

	return Status;
}

//
// This function creates random data and places it into the specified
// buffer.
//
KXCRYPAPI BOOL WINAPI ProcessPrng(
	OUT	PBYTE	Buffer,
	IN	SIZE_T	BufferCb)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	ASSERT (Buffer != NULL);
	ASSERT (BufferCb <= ULONG_MAX);

	if (!KsecDD) {
		Status = InitializeRng();
		if (!NT_SUCCESS(Status)) {
			return FALSE;
		}
	}

	ASSERT (VALID_HANDLE(KsecDD));

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
	return NT_SUCCESS(Status);
}