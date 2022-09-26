#include <Windows.h>
#include <strsafe.h>
#include "VXLL.h"
#include "VXLP.h"

VXLSTATUS VXLAPI VxlOpenLogFile(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle,
	IN		PCWSTR			SourceApplication,
	IN		PCWSTR			SourceComponent)
{
	return VxlOpenLogFileEx(
		FileName,
		LogHandle,
		SourceApplication,
		SourceComponent,
		VXL_OPEN_WRITE_ONLY | VXL_OPEN_APPEND_IF_EXISTS | VXL_OPEN_CREATE_IF_NOT_EXISTS);
}

VXLSTATUS VXLAPI VxlOpenLogFileReadOnly(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle)
{
	return VxlOpenLogFileEx(
		FileName,
		LogHandle,
		0, 0,
		VXL_OPEN_READ_ONLY);
}

//
// Open a log file.
//
VXLSTATUS VXLAPI VxlOpenLogFileEx(
	IN		PCWSTR			FileName,
	OUT		PVXLHANDLE		LogHandle,
	IN		PCWSTR			SourceApplication,
	IN		PCWSTR			SourceComponent,
	IN		ULONG			Flags)
{
	VXLHANDLE Vxlh;
	VXLSTATUS Status;
	ULONG DesiredAccess;
	ULONG CreationDisposition;
	ULONG FlagsAndAttributes;

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
		// If opened for Writing, must specify source parameters
		if (!SourceApplication || !SourceComponent) {
			return VXL_INVALID_PARAMETER;
		}
	}
	
	//
	// Allocate memory for a VXLCONTEXT structure.
	//
	Vxlh = (VXLHANDLE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VXLCONTEXT));
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
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CreationDisposition,
		FlagsAndAttributes,
		NULL);

	if (Vxlh->FileHandle == INVALID_HANDLE_VALUE) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_CANNOT_OPEN);
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

	VxlpAcquireFileLock(Vxlh);

	if (Flags & VXL_OPEN_WRITE_ONLY) {
		//
		// Figure out which source component (index) we are, and store that in the
		// context structure.
		//
		Status = VxlpFindOrCreateSourceComponentIndex(Vxlh, SourceComponent, NULL);
		if (VXL_FAILED(Status)) {
			goto Error;
		}
	} else if (Flags & VXL_OPEN_READ_ONLY) {
		//
		// Build the index.
		//
		Status = VxlpBuildIndex(Vxlh);
		if (VXL_FAILED(Status)) {
			goto Error;
		}
	}

	*LogHandle = Vxlh;

	if (Flags & VXL_OPEN_WRITE_ONLY) {
		//
		// Reading is an exclusive operation.
		//
		VxlpReleaseFileLock(Vxlh);
	}

	return VXL_SUCCESS;

Error:
	if (Vxlh->FileHandle != INVALID_HANDLE_VALUE) {
		VxlpReleaseFileLock(Vxlh);
		CloseHandle(Vxlh->FileHandle);
	}

	HeapFree(GetProcessHeap(), 0, Vxlh);
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

	if (!LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

	Vxlh = *LogHandle;

	if (!Vxlh) {
		// nothing to do
		return VXL_SUCCESS;
	}

	if (Vxlh->OpenFlags & VXL_OPEN_READ_ONLY) {
		VxlpReleaseFileLock(Vxlh);
	}

	if (!CloseHandle(Vxlh->FileHandle)) {
		return VxlpTranslateWin32Error(GetLastError(), VXL_FAILURE);
	}

	HeapFree(GetProcessHeap(), 0, Vxlh->EntryIndexToFileOffset);
	HeapFree(GetProcessHeap(), 0, Vxlh);

	*LogHandle = NULL;
	return VXL_SUCCESS;
}