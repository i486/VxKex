///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     rtlrng.c
//
// Abstract:
//
//     This file implements an interface to the kernel security driver for
//     the purpose of generating random bytes. Various VxKex components require
//     the ability to generate random identifiers for things.
//
// Author:
//
//     vxiiduu (21-Mar-2024)
//
// Environment:
//
//     Native mode. Access to the KsecDD device object is required.
//
// Revision History:
//
//     vxiiduu              21-Mar-2024   Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS KexRtlInitializeRandomNumberGenerator(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING DeviceName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE DeviceHandle;
	HANDLE OldValue;

	ASSUME (KexData != NULL);

	RtlInitConstantUnicodeString(&DeviceName, L"\\Device\\KsecDD");

	InitializeObjectAttributes(
		&ObjectAttributes,
		&DeviceName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	if (KexData->KsecDD != NULL) {
		return STATUS_ALREADY_INITIALIZED;
	}

	Status = NtOpenFile(
		&DeviceHandle,
		FILE_READ_DATA | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (VALID_HANDLE(DeviceHandle));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	OldValue = InterlockedCompareExchangePointer(
		&KexData->KsecDD,
		DeviceHandle,
		NULL);

	ASSERT (VALID_HANDLE(KexData->KsecDD));

	if (OldValue != NULL) {
		// Another thread has initialized the handle (race condition).
		// Clean up the handle we opened and notify the caller.
		KexDebugCheckpoint();
		SafeClose(DeviceHandle);
		return STATUS_ALREADY_INITIALIZED;
	}

	return STATUS_SUCCESS;
}

#define IOCTL_KSEC_RANDOM_FILL_BUFFER CTL_CODE(FILE_DEVICE_KSEC, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

KEXAPI NTSTATUS NTAPI KexRtlGenerateRandomData(
	OUT	PVOID	RandomBuffer,
	IN	ULONG	NumberOfBytesToGenerate)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	ASSUME (RandomBuffer != NULL);
	ASSUME (KexData != NULL);

	if (!KexData->KsecDD) {
		Status = KexRtlInitializeRandomNumberGenerator();
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	ASSERT (VALID_HANDLE(KexData->KsecDD));

	if (!VALID_HANDLE(KexData->KsecDD)) {
		return STATUS_UNSUCCESSFUL;
	}

	Status = NtDeviceIoControlFile(
		KexData->KsecDD,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_KSEC_RANDOM_FILL_BUFFER,
		NULL,
		0,
		RandomBuffer,
		NumberOfBytesToGenerate);

	ASSERT (NT_SUCCESS(Status));
	return Status;
}