///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     rtlsec.c
//
// Abstract:
//
//     This file implements an interface to the kernel security driver.
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
//     vxiiduu              05-Sep-2025   Add check for NumberOfBytesToGenerate
//                                        is zero in KexRtlGenerateRandomData.
//     vxiiduu              12-Jun-2026   Add KexRtlDecryptMemory and
//                                        KexRtlEncryptMemory
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS KexRtlInitializeKsec(
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

#define IOCTL_KSEC_CONNECT_LSA					CTL_CODE(FILE_DEVICE_KSEC, 0, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_KSEC_RANDOM_FILL_BUFFER			CTL_CODE(FILE_DEVICE_KSEC, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KSEC_ENCRYPT_MEMORY				CTL_CODE(FILE_DEVICE_KSEC, 3, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_DECRYPT_MEMORY				CTL_CODE(FILE_DEVICE_KSEC, 4, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_ENCRYPT_MEMORY_CROSS_PROC	CTL_CODE(FILE_DEVICE_KSEC, 5, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_DECRYPT_MEMORY_CROSS_PROC	CTL_CODE(FILE_DEVICE_KSEC, 6, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_ENCRYPT_MEMORY_SAME_LOGON	CTL_CODE(FILE_DEVICE_KSEC, 7, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_KSEC_DECRYPT_MEMORY_SAME_LOGON	CTL_CODE(FILE_DEVICE_KSEC, 8, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

KEXAPI NTSTATUS NTAPI KexRtlGenerateRandomData(
	OUT	PVOID	RandomBuffer,
	IN	ULONG	NumberOfBytesToGenerate)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	if (NumberOfBytesToGenerate == 0) {
		// NumberOfBytesToGenerate can be zero sometimes (when called from ProcessPrng).
		// In this case we should just return without doing anything, because apparently
		// ProcessPrng accepts an argument of zero and at least one application
		// (cloudflared) does this.
		return STATUS_SUCCESS;
	}

	ASSUME (RandomBuffer != NULL);
	ASSUME (KexData != NULL);

	if (!KexData->KsecDD) {
		Status = KexRtlInitializeKsec();
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
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

#define RTL_ENCRYPT_OPTION_CROSS_PROCESS	1
#define RTL_ENCRYPT_OPTION_SAME_LOGON		2

// Similar to SystemFunction071 from advapi/cryptbase.
KEXAPI NTSTATUS NTAPI KexRtlDecryptMemory(
	IN OUT	PVOID	Memory,
	IN		ULONG	MemoryCb,
	IN		ULONG	Flags)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	ULONG IoControlCode;

	ASSUME (Memory != NULL);
	ASSUME (MemoryCb != 0);

	if (!KexData->KsecDD) {
		Status = KexRtlInitializeKsec();
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	switch (Flags) {
	case 0:
		IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY;
		break;
	case RTL_ENCRYPT_OPTION_CROSS_PROCESS:
		IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY_CROSS_PROC;
		break;
	case RTL_ENCRYPT_OPTION_SAME_LOGON:
		IoControlCode = IOCTL_KSEC_DECRYPT_MEMORY_SAME_LOGON;
		break;
	default:
		return STATUS_INVALID_PARAMETER;
	}

	Status = NtDeviceIoControlFile(
		KexData->KsecDD,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IoControlCode,
		Memory,
		MemoryCb,
		Memory,
		MemoryCb);

	ASSERT (NT_SUCCESS(Status));
	return Status;
}

KEXAPI NTSTATUS NTAPI KexRtlEncryptMemory(
	IN OUT	PVOID	Memory,
	IN		ULONG	MemoryCb,
	IN		ULONG	Flags)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	ULONG IoControlCode;

	ASSUME (Memory != NULL);
	ASSUME (MemoryCb != 0);

	if (!KexData->KsecDD) {
		Status = KexRtlInitializeKsec();
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	switch (Flags) {
	case 0:
		IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY;
		break;
	case RTL_ENCRYPT_OPTION_CROSS_PROCESS:
		IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY_CROSS_PROC;
		break;
	case RTL_ENCRYPT_OPTION_SAME_LOGON:
		IoControlCode = IOCTL_KSEC_ENCRYPT_MEMORY_SAME_LOGON;
		break;
	default:
		return STATUS_INVALID_PARAMETER;
	}

	Status = NtDeviceIoControlFile(
		KexData->KsecDD,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IoControlCode,
		Memory,
		MemoryCb,
		Memory,
		MemoryCb);

	ASSERT (NT_SUCCESS(Status));
	return Status;
}