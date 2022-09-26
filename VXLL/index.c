#include <Windows.h>
#include "VXLL.h"
#include "VXLP.h"

VXLSTATUS VxlpBuildIndex(
	IN	VXLHANDLE	LogHandle)
{
	ULONG CurrentEntryIndex;
	ULONG NumberOfEntries;
	ULONG NumberOfEntriesLength;
	LARGE_INTEGER FilePointer;
	VXLLOGFILEENTRY FileEntry;
	ULONG BytesWritten;
	BOOLEAN Success;
	VXLSTATUS Status;

	// param validation
	if (!LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	//
	// Find out how many entries there are in the file.
	//
	NumberOfEntriesLength = sizeof(NumberOfEntries);
	Status = VxlQueryLogInformation(
		LogHandle,
		LogNumberOfEntries,
		&NumberOfEntries,
		&NumberOfEntriesLength);

	if (VXL_FAILED(Status)) {
		return Status;
	}

	if (NumberOfEntries == 0) {
		// Nothing to do.
		return VXL_SUCCESS;
	}

	//
	// Allocate correct sized memory structures.
	//

	LogHandle->EntryIndexToFileOffset = (PLARGE_INTEGER) HeapAlloc(GetProcessHeap(), 0, NumberOfEntries * sizeof(LARGE_INTEGER));
	if (LogHandle->EntryIndexToFileOffset == NULL) {
		Status = VXL_OUT_OF_MEMORY;
		goto Error;
	}

	//
	// Traverse the log file.
	//

	CurrentEntryIndex = 0;
	FilePointer.QuadPart = sizeof(VXLLOGFILEHEADER);

	do {
		Success = SetFilePointerEx(
			LogHandle->FileHandle,
			FilePointer,
			NULL,
			FILE_BEGIN);

		if (!Success) {
			Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
			goto Error;
		}

		//
		// Read VXLLOGFILEENTRY structure.
		//
		Success = ReadFile(
			LogHandle->FileHandle,
			&FileEntry,
			sizeof(FileEntry),
			&BytesWritten,
			NULL);

		if (!Success || BytesWritten != sizeof(FileEntry)) {
			Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
			goto Error;
		}

		//
		// The library can use EntryIndexToFileOffset to find the file offset of any
		// given log entry (by index).
		//
		LogHandle->EntryIndexToFileOffset[CurrentEntryIndex] = FilePointer;

		//
		// Advance the file pointer to the next entry in the file.
		//
		FilePointer.QuadPart += sizeof(VXLLOGFILEENTRY);
		FilePointer.QuadPart += FileEntry.SourceFileLength * sizeof(WCHAR);
		FilePointer.QuadPart += FileEntry.SourceFunctionLength * sizeof(WCHAR);
		FilePointer.QuadPart += FileEntry.TextLength * sizeof(WCHAR);

	} while (CurrentEntryIndex++ < NumberOfEntries - 1);

	Status = VXL_SUCCESS;

Error:
	if (VXL_FAILED(Status)) {
		//
		// free all memory allocated in this function
		//
		HeapFree(GetProcessHeap(), 0, LogHandle->EntryIndexToFileOffset);
		LogHandle->EntryIndexToFileOffset = NULL;
	}

	return Status;
}