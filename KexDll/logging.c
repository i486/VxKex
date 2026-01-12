///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     logging.c
//
// Abstract:
//
//     Logging-related utility functions.
//
// Author:
//
//     vxiiduu (23-Feb-2024)
//
// Environment:
//
//     During initialization.
//
// Revision History:
//
//     vxiiduu              23-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS KexOpenVxlLogForCurrentApplication(
	OUT	PVXLHANDLE	LogHandle)
{
	NTSTATUS Status;
	UNICODE_STRING LogDir;
	WCHAR LogFileBuffer[MAX_PATH];
	UNICODE_STRING LogFileName;
	UNICODE_STRING SourceApplication;
	HANDLE LogDirHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	USHORT TemporaryLength;

	ASSERT (LogHandle != NULL);
	ASSERT (KexData != NULL);
	ASSERT (KexData->LogHandle == NULL);

	if (KexData->Flags & KEXDATA_FLAG_DISABLE_LOGGING) {
		// Don't open a log file.
		*LogHandle = NULL;
		return STATUS_USER_DISABLED;
	}

	Status = RtlDosPathNameToNtPathName_U_WithStatus(
		KexData->LogDir.Buffer,
		&LogDir,
		NULL,
		NULL);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	LogDirHandle = NULL;

	try {
		//
		// Open the root log directory.
		//

		InitializeObjectAttributes(
			&ObjectAttributes,
			&LogDir,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

		Status = KexRtlCreateDirectoryRecursive(
			&LogDirHandle,
			FILE_TRAVERSE,
			&ObjectAttributes,
			FILE_SHARE_READ | FILE_SHARE_WRITE);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		//
		// Assemble the log file name.
		//

		RtlInitEmptyUnicodeString(&LogFileName, LogFileBuffer, ARRAYSIZE(LogFileBuffer));
		RtlAppendUnicodeStringToString(&LogFileName, &KexData->ImageBaseName);
		KexRtlPathRemoveExtension(&LogFileName, &LogFileName);
		RtlAppendUnicodeToString(&LogFileName, L"-");

		// append system time, zero-padded to 20 characters for easy sorting
		TemporaryLength = LogFileName.Length;
		KexRtlAdvanceUnicodeString(&LogFileName, TemporaryLength);
		RtlInt64ToUnicodeString(*(PULONGLONG) &SharedUserData->SystemTime, 10, &LogFileName);
		KexRtlShiftUnicodeString(&LogFileName, 20 - (LogFileName.Length / sizeof(WCHAR)), '0');
		KexRtlRetreatUnicodeString(&LogFileName, TemporaryLength);

		RtlAppendUnicodeToString(&LogFileName, L".vxl");

		//
		// Open the log file.
		//

		InitializeObjectAttributes(
			&ObjectAttributes,
			&LogFileName,
			OBJ_CASE_INSENSITIVE,
			LogDirHandle,
			NULL);

		RtlInitConstantUnicodeString(&SourceApplication, L"VxKex");
		Status = VxlOpenLog(
			LogHandle,
			&SourceApplication,
			&ObjectAttributes,
			GENERIC_WRITE,
			FILE_OVERWRITE_IF);

		ASSERT (NT_SUCCESS(Status));
	} finally {
		RtlFreeUnicodeString(&LogDir);
		SafeClose(LogDirHandle);
	}

	return Status;
}