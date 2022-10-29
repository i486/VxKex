///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     index.c
//
// Abstract:
//
//     Contains the private index creation routine.
//     The index is extremely important for reading from the log file. Without
//     it, reading entries is extremely slow and does not scale.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

VXLSTATUS VxlpBuildIndex(
	IN	VXLHANDLE	LogHandle)
{
	ULONG CurrentEntryIndex;
	ULONG NumberOfEntries;
	ULONG NumberOfEntriesLength;
	ULONG FileOffset;
	PVXLLOGFILEENTRY FileEntry;
	VXLSTATUS Status;

	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->OpenFlags & VXL_OPEN_READ_ONLY);

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

	LogHandle->EntryIndexToFileOffset = SafeAlloc(ULONG, NumberOfEntries);
	if (!LogHandle->EntryIndexToFileOffset) {
		return VXL_OUT_OF_MEMORY;
	}

	//
	// Traverse the log file.
	//

	CurrentEntryIndex = 0;
	FileOffset = sizeof(VXLLOGFILEHEADER);

	do {
		//
		// Read VXLLOGFILEENTRY structure.
		//
		FileEntry = (PVXLLOGFILEENTRY) (LogHandle->MappedFileBase + FileOffset);

		//
		// The library can use EntryIndexToFileOffset to find the file offset of any
		// given log entry (by index).
		//
		LogHandle->EntryIndexToFileOffset[CurrentEntryIndex] = FileOffset;

		//
		// Advance the file pointer to the next entry in the file.
		//
		FileOffset += sizeof(VXLLOGFILEENTRY);
		FileOffset += FileEntry->SourceFileLength * sizeof(WCHAR);
		FileOffset += FileEntry->SourceFunctionLength * sizeof(WCHAR);
		FileOffset += FileEntry->TextHeaderLength * sizeof(WCHAR);
		FileOffset += FileEntry->TextLength * sizeof(WCHAR);

		if (FileEntry->TextLength > 0) {
			FileOffset += sizeof(WCHAR); // extra null terminator
		}

	} while (CurrentEntryIndex++ < NumberOfEntries - 1);

	return VXL_SUCCESS;
}