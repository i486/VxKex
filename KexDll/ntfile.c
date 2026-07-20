///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntfile.c
//
// Abstract:
//
//     Extended file-related kernel calls.
//
// Author:
//
//     vxiiduu (08-Jul-2026)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              08-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI Ext_NtSetInformationFile(
	IN	HANDLE					FileHandle,
	OUT	PIO_STATUS_BLOCK		IoStatusBlock,
	IN	PVOID					FileInformation,
	IN	ULONG					FileInformationLength,
	IN	FILE_INFORMATION_CLASS	FileInformationClass)
{
	if (FileInformationClass == FileDispositionInformationEx) {
		PFILE_DISPOSITION_INFORMATION_EX DispositionInformation;
		FILE_DISPOSITION_INFORMATION NewDispositionInformation;

		if (FileInformationLength != sizeof(FILE_DISPOSITION_INFORMATION_EX)) {
			return STATUS_INFO_LENGTH_MISMATCH;
		}

		DispositionInformation = (PFILE_DISPOSITION_INFORMATION_EX) FileInformation;

		try {
			if (DispositionInformation->Flags & (~1)) {
				KexLogDebugEvent(
					L"Extended FILE_DISPOSITION_INFORMATION_EX flags ignored\r\n\r\n"
					L"Flags: 0x%08lx",
					DispositionInformation->Flags);
			}

			NewDispositionInformation.DeleteFile = DispositionInformation->Delete;
		} except (EXCEPTION_EXECUTE_HANDLER) {
			KexDebugCheckpoint();
			return GetExceptionCode();
		}

		FileInformation = &NewDispositionInformation;
		FileInformationLength = sizeof(FILE_DISPOSITION_INFORMATION);
		FileInformationClass = FileDispositionInformation;
	} else if (FileInformationClass == FileRenameInformationEx) {
		PFILE_RENAME_INFORMATION NewRenameInformation;

		if (FileInformationLength < sizeof(FILE_RENAME_INFORMATION)) {
			return STATUS_INFO_LENGTH_MISMATCH;
		}

		try {
			NewRenameInformation = (PFILE_RENAME_INFORMATION) StackAlloc(
				BYTE,
				FileInformationLength);

			KexRtlCopyMemory(
				NewRenameInformation,
				FileInformation,
				FileInformationLength);
		} except (EXCEPTION_EXECUTE_HANDLER) {
			KexDebugCheckpoint();
			return GetExceptionCode();
		}

		if (NewRenameInformation->Flags & (~1)) {
			KexLogDebugEvent(
				L"Extended FILE_RENAME_INFORMATION flags ignored\r\n\r\n"
				L"Flags: 0x%08lx",
				NewRenameInformation->Flags);

			NewRenameInformation->Flags &= 1;
		}

		FileInformation = NewRenameInformation;
		FileInformationClass = FileRenameInformation;
	} else if (FileInformationClass > FileRemoteProtocolInformation) {
		KexLogWarningEvent(L"Unsupported info class %d", FileInformationClass);
		KexDebugCheckpoint();
	}

	return NtSetInformationFile(
		FileHandle,
		IoStatusBlock,
		FileInformation,
		FileInformationLength,
		FileInformationClass);
}