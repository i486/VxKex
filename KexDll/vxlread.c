///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     vxlread.c
//
// Abstract:
//
//     Contains the public routines for reading entries from a log file.
//
// Author:
//
//     vxiiduu (19-Nov-2022)
//
// Revision History:
//
//     vxiiduu	            19-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC FORCEINLINE NTSTATUS VxlpReadLogInternal(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndex,
	OUT		PVXLLOGENTRY	Entry)
{
	PVXLLOGFILEENTRY FileEntry;
	TIME_FIELDS TimeFields;
	LONGLONG LocalTime;

	ASSERT (Entry != NULL);
	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->MappedFile != NULL);
	ASSERT (LogHandle->EntryIndexToFileOffset != NULL);
	ASSERT (LogHandle->EntryIndexToFileOffset[LogEntryIndex] != 0);

	//
	// Use the index to look up the file offset for this log entry, and then
	// convert it into a pointer to the log entry.
	//

	FileEntry = (PVXLLOGFILEENTRY) RVA_TO_VA(
		LogHandle->MappedFile,
		LogHandle->EntryIndexToFileOffset[LogEntryIndex]);

	//
	// Fill out the caller's provided VXLLOGENTRY structure.
	//

	RtlZeroMemory(Entry, sizeof(*Entry));

	if (FileEntry->TextHeaderCch != 0) {
		Entry->TextHeader.Length		= (FileEntry->TextHeaderCch - 1) * sizeof(WCHAR);
		Entry->TextHeader.MaximumLength	= Entry->TextHeader.Length + sizeof(WCHAR);
		Entry->TextHeader.Buffer		= FileEntry->Text;
	}

	if (FileEntry->TextCch != 0) {
		Entry->Text.Length				= (FileEntry->TextCch - 1) * sizeof(WCHAR);
		Entry->Text.MaximumLength		= Entry->Text.Length + sizeof(WCHAR);
		Entry->Text.Buffer				= FileEntry->Text + FileEntry->TextHeaderCch;
	}

	Entry->SourceComponentIndex		= FileEntry->SourceComponentIndex;
	Entry->SourceFileIndex			= FileEntry->SourceFileIndex;
	Entry->SourceFunctionIndex		= FileEntry->SourceFunctionIndex;
	Entry->SourceLine				= FileEntry->SourceLine;

	Entry->ClientId.UniqueProcess	= (HANDLE) FileEntry->ProcessId;
	Entry->ClientId.UniqueThread	= (HANDLE) FileEntry->ThreadId;

	Entry->Severity					= FileEntry->Severity;

	//
	// Fill out the SYSTEMTIME structure in the VXLLOGENTRY structure.
	// First, convert the 64-bit timestamp in the VXLLOGFILEENTRY from UTC
	// to local time.
	//

	do {
		LocalTime = FileEntry->Time64 - *(PLONGLONG) &SharedUserData->TimeZoneBias;
	} until (SharedUserData->TimeZoneBias.High1Time == SharedUserData->TimeZoneBias.High2Time);

	//
	// Now convert the local time into a SYSTEMTIME.
	//

	RtlTimeToTimeFields(&LocalTime, &TimeFields);
	Entry->Time.wYear			= TimeFields.Year;
	Entry->Time.wMonth			= TimeFields.Month;
	Entry->Time.wDay			= TimeFields.Day;
	Entry->Time.wDayOfWeek		= TimeFields.Weekday;
	Entry->Time.wHour			= TimeFields.Hour;
	Entry->Time.wMinute			= TimeFields.Minute;
	Entry->Time.wSecond			= TimeFields.Second;
	Entry->Time.wMilliseconds	= TimeFields.Milliseconds;

	return STATUS_SUCCESS;
}

NTSTATUS NTAPI VxlReadLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndex,
	OUT		PVXLLOGENTRY	Entry)
{
	ULONG MaximumIndex;

	//
	// Parameter validation
	//

	if (!LogHandle || !Entry) {
		return STATUS_INVALID_PARAMETER;
	}

	if (LogHandle->OpenMode != GENERIC_READ) {
		return STATUS_INVALID_OPEN_MODE;
	}

	MaximumIndex = VxlpGetTotalLogEntryCount(LogHandle);

	if (MaximumIndex == -1) {
		return STATUS_NO_MORE_ENTRIES;
	}

	if (LogEntryIndex > MaximumIndex) {
		return STATUS_NO_MORE_ENTRIES;
	}

	return VxlpReadLogInternal(LogHandle, LogEntryIndex, Entry);
}

NTSTATUS NTAPI VxlReadMultipleEntriesLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndexStart,
	IN		ULONG			LogEntryIndexEnd,
	OUT		PVXLLOGENTRY	Entry[])
{
	NTSTATUS Status;
	ULONG Index;
	ULONG MaximumIndex;

	//
	// Parameter validation
	//

	if (!LogHandle || !Entry) {
		return STATUS_INVALID_PARAMETER;
	}

	if (LogEntryIndexEnd < LogEntryIndexStart) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	MaximumIndex = VxlpGetTotalLogEntryCount(LogHandle) - 1;

	if (MaximumIndex == -1) {
		return STATUS_NO_MORE_ENTRIES;
	}

	if (LogEntryIndexStart > MaximumIndex) {
		return STATUS_NO_MORE_ENTRIES;
	}

	if (LogEntryIndexEnd > MaximumIndex) {
		LogEntryIndexEnd = MaximumIndex;
	}

	//
	// Fetch the requested log entries.
	//

	for (Index = LogEntryIndexStart; Index < LogEntryIndexEnd; ++Index) {
		Status = VxlpReadLogInternal(LogHandle, Index, Entry[Index - LogEntryIndexStart]);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		++Index;
	}

	return Status;
}