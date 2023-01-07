///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     logging.c
//
// Abstract:
//
//     Contains functions related to KexSrv's logging.
//
// Author:
//
//     vxiiduu (05-Jan-2023)
//
// Revision History:
//
//     vxiiduu               05-Jan-2023  Initial creation
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsrvp.h"

//
// Open KexSrv's log file. This log file is located in the standard
// directory for VxKex logs, and is simply named KexSrv.vxl
//
NTSTATUS OpenServerLogFile(
	OUT	PVXLHANDLE	LogHandle)
{
	NTSTATUS Status;
	HANDLE DirectoryHandle;
	UNICODE_STRING FileName;
	UNICODE_STRING SourceApplication;
	OBJECT_ATTRIBUTES ObjectAttributes;

	ASSERT (LogHandle != NULL);

	//
	// 1. Open the log directory.
	//

	Status = RtlDosPathNameToNtPathName_U_WithStatus(
		KexData->LogDir.Buffer,
		&FileName,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	InitializeObjectAttributes(
		&ObjectAttributes,
		&FileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = KexRtlCreateDirectoryRecursive(
		&DirectoryHandle,
		FILE_TRAVERSE,
		&ObjectAttributes,
		FILE_SHARE_READ | FILE_SHARE_WRITE);

	RtlFreeUnicodeString(&FileName);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// 2. Open the log file "KexSrv.vxl" under that directory.
	//

	RtlInitConstantUnicodeString(&SourceApplication, L"VxKex");
	RtlInitConstantUnicodeString(&FileName, L"KexSrv.vxl");

	InitializeObjectAttributes(
		&ObjectAttributes,
		&FileName,
		OBJ_CASE_INSENSITIVE,
		DirectoryHandle,
		NULL);

	Status = VxlOpenLog(
		LogHandle,
		&SourceApplication,
		&ObjectAttributes,
		GENERIC_WRITE,
		FILE_OPEN_IF);

	NtClose(DirectoryHandle);
	return Status;
}