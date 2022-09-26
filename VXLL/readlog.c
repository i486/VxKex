#include <Windows.h>
#include "VXLL.h"
#include "VXLP.h"

//
// Free a log entry returned by VxlReadLogEntry.
//
VXLSTATUS VXLAPI VxlFreeLogEntry(
	IN OUT	PPVXLLOGENTRY	Entry)
{
	if (!Entry) {
		return VXL_INVALID_PARAMETER;
	}

	if (!*Entry) {
		return VXL_SUCCESS;
	}

	if ((*Entry)->SourceFile) {
		HeapFree(GetProcessHeap(), 0, (PVOID) (*Entry)->SourceFile);
	}

	if ((*Entry)->SourceFunction) {
		HeapFree(GetProcessHeap(), 0, (PVOID) (*Entry)->SourceFunction);
	}

	if ((*Entry)->Text) {
		HeapFree(GetProcessHeap(), 0, (PVOID) (*Entry)->Text);
	}

	HeapFree(GetProcessHeap(), 0, *Entry);
	*Entry = NULL;
	return VXL_SUCCESS;
}

//
// Read a single log entry from the log file.
// Any severities listed in the SeverityFilters flag (VXL_FILTER_SEVERITY_xxx)
// are NOT counted for the purposes of index calculation. In other words, the
// SeverityFilters mask excludes certain severity types from appearing in the
// result of VxlReadLogEntry.
//
VXLSTATUS VXLAPI VxlReadLogEntry(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			EntryIndex,
	OUT		PPVXLLOGENTRY	Entry)
{
	ULONG MaximumEntryIndex;
	ULONG BytesRead;
	LARGE_INTEGER FilePointer;
	VXLLOGFILEENTRY FileEntry;
	BOOLEAN Success;
	VXLSTATUS Status;

	// param validation
	if (!LogHandle || !Entry) {
		return VXL_INVALID_PARAMETER;
	}

	*Entry = NULL;

	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	//
	// figure out how many log entries are in the file, minus any entries that
	// are filtered out
	//
	MaximumEntryIndex =
		LogHandle->Header.EventSeverityTypeCount[LogSeverityCritical] +
		LogHandle->Header.EventSeverityTypeCount[LogSeverityError] +
		LogHandle->Header.EventSeverityTypeCount[LogSeverityWarning] +
		LogHandle->Header.EventSeverityTypeCount[LogSeverityInformation] +
		LogHandle->Header.EventSeverityTypeCount[LogSeverityDetail] +
		LogHandle->Header.EventSeverityTypeCount[LogSeverityDebug] - 1;

	if (EntryIndex > MaximumEntryIndex) {
		return VXL_ENTRY_OUT_OF_RANGE;
	}

	//
	// get file offset of the correct entry
	//
	FilePointer = LogHandle->EntryIndexToFileOffset[EntryIndex];

	//
	// read log entry
	//

	Success = SetFilePointerEx(
		LogHandle->FileHandle,
		FilePointer,
		NULL,
		FILE_BEGIN);

	if (!Success) {
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	Success = ReadFile(
		LogHandle->FileHandle,
		&FileEntry,
		sizeof(FileEntry),
		&BytesRead,
		NULL);

	if (!Success || BytesRead != sizeof(FileEntry)) {
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	Status = VxlpLogFileEntryToLogEntry(
		LogHandle,
		&FileEntry,
		Entry);

	return Status;
}

//
// Return the last critical log entry posted by the current application.
// This is intended for final-error processing, i.e. displaying the critical
// error message to the user. Does NOT work for getting the last critical log
// entry in a file opened for read-only access - use VxlQueryLogInformation
// followed by VxlReadLogEntry for doing that.
//
// Do not free the log entry returned by this function. It is managed by the
// library.
//
VXLSTATUS VXLAPI VxlReadLastCriticalEntry(
	IN		VXLHANDLE		LogHandle,
	OUT		PPCVXLLOGENTRY	Entry)
{
	VXLSTATUS Status;

	// param validation
	if (!LogHandle || !Entry) {
		return VXL_INVALID_PARAMETER;
	}

	if (LogHandle->OpenFlags & VXL_OPEN_READ_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	Status = VxlpAcquireFileLock(LogHandle);
	if (VXL_FAILED(Status)) {
		goto Error;
	}

	if (LogHandle->LastCriticalEntry.Text == NULL) {
		Status = VXL_ENTRY_NOT_FOUND;
		goto Error;
	}

	*Entry = &LogHandle->LastCriticalEntry;
	Status = VXL_SUCCESS;

Error:
	VxlpReleaseFileLock(LogHandle);
	return Status;
}