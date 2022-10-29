///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     opnclose.c
//
// Abstract:
//
//     Contains the public routines for reading from an open log file.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              07-Oct-2022  Convert to memory mapped file.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

//
// Free a log entry returned by VxlReadLogEntry.
//
VXLSTATUS VXLAPI VxlFreeLogEntry(
	IN OUT	PPVXLLOGENTRY	Entry)
{
	ASSERT (Entry != NULL);

	if (!Entry) {
		return VXL_INVALID_PARAMETER;
	}

	if (!*Entry) {
		return VXL_SUCCESS;
	}

	SafeFree(*Entry);
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
	ULONG FileOffset;
	VXLSTATUS Status;

	ASSERT (LogHandle != NULL);
	ASSERT (Entry != NULL);

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
	FileOffset = LogHandle->EntryIndexToFileOffset[EntryIndex];

	//
	// read log entry
	//

	Status = VxlpLogFileEntryToLogEntry(
		LogHandle,
		(PVXLLOGFILEENTRY) (LogHandle->MappedFileBase + FileOffset),
		Entry);

	return Status;
}