///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     vxlpriv.c
//
// Abstract:
//
//     Contains miscellaneous private routines.
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
#include "kexdllp.h"

#pragma warning(disable:4244)

NTSTATUS VxlpFlushLogFileHeader(
	IN	VXLHANDLE			LogHandle)
{
	PVOID Base;
	SIZE_T Size;
	IO_STATUS_BLOCK IoStatusBlock;

	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->Header != NULL);
	ASSERT (LogHandle->OpenMode == GENERIC_WRITE);

	Base = LogHandle->Header;
	Size = sizeof(*LogHandle->Header);

	return NtFlushVirtualMemory(
		NtCurrentProcess(),
		&Base,
		&Size,
		&IoStatusBlock);
}

ULONG VxlpGetTotalLogEntryCount(
	IN	VXLHANDLE			LogHandle)
{
	ULONG Index;
	ULONG Total;

	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->Header != NULL);

	Total = 0;

	ForEachArrayItem (LogHandle->Header->EventSeverityTypeCount, Index) {
		Total += LogHandle->Header->EventSeverityTypeCount[Index];
	}

	return Total;
}

ULONG VxlpSizeOfLogFileEntry(
	IN	PVXLLOGFILEENTRY	Entry)
{
	ULONG Size;

	ASSERT (Entry != NULL);

	Size = sizeof(VXLLOGFILEENTRY);
	Size += Entry->TextHeaderCch * sizeof(WCHAR);
	Size += Entry->TextCch * sizeof(WCHAR);

	return Size;
}

NTSTATUS VxlpBuildIndex(
	IN	VXLHANDLE			LogHandle)
{
	PVXLLOGFILEENTRY Entry;
	ULONG TotalLogEntryCount;
	ULONG Index;
	ULONG SeverityIndex[LogSeverityMaximumValue];

	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->OpenMode == GENERIC_READ);
	ASSERT (LogHandle->EntryIndexToFileOffset == NULL);

	TotalLogEntryCount = VxlpGetTotalLogEntryCount(LogHandle);

	if (!TotalLogEntryCount) {
		return STATUS_NO_MORE_ENTRIES;
	}

	//
	// Allocate memory for the index.
	//

	LogHandle->EntryIndexToFileOffset = SafeAllocSeh(ULONG, TotalLogEntryCount);

	Entry = (PVXLLOGFILEENTRY) (LogHandle->MappedFile + sizeof(VXLLOGFILEHEADER));
	RtlZeroMemory(SeverityIndex, sizeof(SeverityIndex));

	for (Index = 0; TotalLogEntryCount--; ++Index) {
		//
		// record file offset of the Index'th entry into the index,
		// for fast seeking to any particular log entry
		//

		LogHandle->EntryIndexToFileOffset[Index] = (ULONG) VA_TO_RVA(LogHandle->MappedFile, Entry);

		//
		// skip ahead to next entry
		//

		Entry = (PVXLLOGFILEENTRY) RVA_TO_VA(Entry, VxlpSizeOfLogFileEntry(Entry));
	}

	return STATUS_SUCCESS;
}

NTSTATUS VxlpFindOrCreateSourceComponentIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceComponent,
	OUT	PUCHAR				SourceComponentIndex)
{
	ULONG Index;

	ASSERT (LogHandle != NULL);
	ASSERT (SourceComponent != NULL);
	ASSERT (SourceComponentIndex != NULL);
	ASSERT (wcslen(SourceComponent) < ARRAYSIZE(LogHandle->Header->SourceComponents[0]));

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header->SourceComponents); ++Index) {
		if (StringEqual(LogHandle->Header->SourceComponents[Index], SourceComponent)) {
			*SourceComponentIndex = Index;
			return STATUS_SUCCESS;
		} else if (LogHandle->Header->SourceComponents[Index][0] == '\0') {
			HRESULT Result;

			Result = StringCchCopy(
				LogHandle->Header->SourceComponents[Index],
				ARRAYSIZE(LogHandle->Header->SourceComponents[Index]),
				SourceComponent);

			if (FAILED(Result)) {
				return STATUS_BUFFER_TOO_SMALL;
			}

			*SourceComponentIndex = Index;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_TOO_MANY_INDICES;
}

NTSTATUS VxlpFindOrCreateSourceFileIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceFile,
	OUT	PUCHAR				SourceFileIndex)
{
	ULONG Index;

	ASSERT (LogHandle != NULL);
	ASSERT (SourceFile != NULL);
	ASSERT (SourceFileIndex != NULL);
	ASSERT (wcslen(SourceFile) < ARRAYSIZE(LogHandle->Header->SourceFiles[0]));

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header->SourceFiles); ++Index) {
		if (StringEqual(LogHandle->Header->SourceFiles[Index], SourceFile)) {
			*SourceFileIndex = Index;
			return STATUS_SUCCESS;
		} else if (LogHandle->Header->SourceFiles[Index][0] == '\0') {
			HRESULT Result;

			Result = StringCchCopy(
				LogHandle->Header->SourceFiles[Index],
				ARRAYSIZE(LogHandle->Header->SourceFiles[Index]),
				SourceFile);

			if (FAILED(Result)) {
				return STATUS_BUFFER_TOO_SMALL;
			}

			*SourceFileIndex = Index;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_TOO_MANY_INDICES;
}

NTSTATUS VxlpFindOrCreateSourceFunctionIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceFunction,
	OUT	PUCHAR				SourceFunctionIndex)
{
	ULONG Index;

	ASSERT (LogHandle != NULL);
	ASSERT (SourceFunction != NULL);
	ASSERT (SourceFunctionIndex != NULL);
	ASSERT (wcslen(SourceFunction) < ARRAYSIZE(LogHandle->Header->SourceFunctions[0]));

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header->SourceFunctions); ++Index) {
		if (StringEqual(LogHandle->Header->SourceFunctions[Index], SourceFunction)) {
			*SourceFunctionIndex = Index;
			return STATUS_SUCCESS;
		} else if (LogHandle->Header->SourceFunctions[Index][0] == '\0') {
			HRESULT Result;

			Result = StringCchCopy(
				LogHandle->Header->SourceFunctions[Index],
				ARRAYSIZE(LogHandle->Header->SourceFunctions[Index]),
				SourceFunction);

			if (FAILED(Result)) {
				return STATUS_BUFFER_TOO_SMALL;
			}

			*SourceFunctionIndex = Index;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_TOO_MANY_INDICES;
}