///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     opnclose.c
//
// Abstract:
//
//     Contains the public routines for opening and closing log files.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              15-Oct-2022  Convert to v2 format.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

VXLSTATUS VXLAPI VxlOpenLogFile(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle,
	IN		PCWSTR			SourceApplication)
{
	return VxlOpenLogFileEx(
		FileName,
		LogHandle,
		SourceApplication,
		VXL_OPEN_WRITE_ONLY | VXL_OPEN_APPEND_IF_EXISTS | VXL_OPEN_CREATE_IF_NOT_EXISTS);
}

VXLSTATUS VXLAPI VxlOpenLogFileReadOnly(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle)
{
	return VxlOpenLogFileEx(
		FileName,
		LogHandle,
		NULL,
		VXL_OPEN_READ_ONLY);
}

//
// Open a log file.
//
VXLSTATUS VXLAPI VxlOpenLogFileEx(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle,
	IN		PCWSTR			SourceApplication OPTIONAL,
	IN		ULONG			Flags)
{
	VXLHANDLE Vxlh;
	VXLSTATUS Status;
	ULONG DesiredAccess;
	ULONG CreationDisposition;
	ULONG FlagsAndAttributes;

	ASSERT (FileName != NULL);
	ASSERT (LogHandle != NULL);

	if (LogHandle) {
		*LogHandle = NULL;
	}

	//
	// Validate parameters and flags
	//
	if (!FileName || !LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

	Status = VxlpValidateFileOpenFlags(Flags);
	if (VXL_FAILED(Status)) {
		return Status;
	}

	if (Flags & VXL_OPEN_WRITE_ONLY) {
		ASSERT (SourceApplication != NULL);
		
		// If opened for Writing, must specify source application
		if (!SourceApplication) {
			return VXL_INVALID_PARAMETER;
		}
	}
	
	//
	// Allocate memory for a VXLCONTEXT structure.
	//
	Vxlh = SafeAlloc(VXLCONTEXT, 1);
	if (!Vxlh) {
		return VXL_OUT_OF_MEMORY;
	}

	//
	// Initialize the VXLCONTEXT structure.
	//

	InitializeCriticalSection(&Vxlh->Lock);
	Vxlh->FileHandle = INVALID_HANDLE_VALUE;
	Vxlh->OpenFlags = Flags;

	//
	// Open a handle to the requested log file.
	//

	Status = VxlpTranslateFileOpenFlags(
		Flags,
		&DesiredAccess,
		&FlagsAndAttributes,
		&CreationDisposition);

	if (VXL_FAILED(Status)) {
		goto Error;
	}

	Vxlh->OpenFlags = Flags;
	Vxlh->FileHandle = CreateFile(
		FileName,
		DesiredAccess,
		FILE_SHARE_READ,
		NULL,
		CreationDisposition,
		FlagsAndAttributes,
		NULL);

	if (Vxlh->FileHandle == INVALID_HANDLE_VALUE) {
		Status = VxlTranslateWin32Error(GetLastError(), VXL_FILE_CANNOT_OPEN);
		goto Error;
	}

	//
	// See if the file is empty (0-bytes). If so, we need to add a new log file
	// header to the file.
	//
	if (VxlpFileIsEmpty(Vxlh->FileHandle)) {
		Status = VxlpInitializeLogFileHeader(&Vxlh->Header, SourceApplication);
		if (VXL_FAILED(Status)) {
			goto Error;
		}

		Status = VxlpWriteLogFileHeader(Vxlh);
		if (VXL_FAILED(Status)) {
			goto Error;
		}
	} else {
		//
		// The file is not empty - Retrieve and scan the file header
		//
		Status = VxlpReadLogFileHeader(Vxlh);
		if (VXL_FAILED(Status)) {
			goto Error;
		}
	}

	if (Flags & VXL_OPEN_READ_ONLY) {
		HANDLE FileMapping;

		//
		// Map the entire file into memory.
		//

		FileMapping = CreateFileMapping(
			Vxlh->FileHandle,
			NULL,
			PAGE_READONLY,
			0, 0,
			NULL);

		if (FileMapping == NULL) {
			Status = VxlTranslateWin32Error(GetLastError(), VXL_FILE_CANNOT_OPEN);
			goto Error;
		}

		Vxlh->MappedFileBase = (PBYTE) MapViewOfFile(
			FileMapping,
			FILE_MAP_READ,
			0, 0, 0);

		if (Vxlh->MappedFileBase == NULL) {
			Status = VxlTranslateWin32Error(GetLastError(), VXL_FILE_CANNOT_OPEN);
			goto Error;
		}

		CloseHandle(FileMapping);

		//
		// Build the index.
		//

		Status = VxlpBuildIndex(Vxlh);
		if (VXL_FAILED(Status)) {
			goto Error;
		}
	}

	*LogHandle = Vxlh;
	return VXL_SUCCESS;

Error:
	if (Vxlh->FileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(Vxlh->FileHandle);
	}

	SafeFree(Vxlh);
	return Status;
}

//
// Close an opened VXLHANDLE. The handle will be set to NULL if the
// function returns successfully.
//
VXLSTATUS VXLAPI VxlCloseLogFile(
	IN OUT	PVXLHANDLE		LogHandle)
{
	VXLHANDLE Vxlh;

	ASSERT (LogHandle != NULL);

	if (!LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

	Vxlh = *LogHandle;

	if (!Vxlh) {
		// nothing to do
		return VXL_SUCCESS;
	}

	if (Vxlh->OpenFlags & VXL_OPEN_READ_ONLY) {
		UnmapViewOfFile(Vxlh->MappedFileBase);
		SafeFree(Vxlh->EntryIndexToFileOffset);
	}

	CloseHandle(Vxlh->FileHandle);
	DeleteCriticalSection(&Vxlh->Lock);
	SafeFree(*LogHandle);

	return VXL_SUCCESS;
}