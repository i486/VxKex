#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS KexOpenVxlLogForCurrentApplication(
	OUT	PVXLHANDLE	LogHandle) PROTECTED_FUNCTION
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
	ASSERT (KexData->LogHandle == NULL);

	Status = RtlDosPathNameToNtPathName_U_WithStatus(
		KexData->LogDir.Buffer,
		&LogDir,
		NULL,
		NULL);

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
		RtlInt64ToUnicodeString(*(PULONGLONG) &SharedUserData->SystemTime, 0, &LogFileName);
		KexRtlShiftUnicodeString(&LogFileName, 20 - (LogFileName.Length / sizeof(WCHAR)), '0');
		KexRtlRetreatUnicodeString(&LogFileName, TemporaryLength);

		RtlAppendUnicodeToString(&LogFileName, L".vxl");
		KexRtlPathReplaceIllegalCharacters(&LogFileName, &LogFileName, 0, FALSE);
		
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
	} finally {
		RtlFreeUnicodeString(&LogDir);
		SafeClose(LogDirHandle);
	}

	return Status;
} PROTECTED_FUNCTION_END