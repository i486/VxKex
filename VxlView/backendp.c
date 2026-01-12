#include "vxlview.h"
#include <DbgHelp.h>
#include "resource.h"
#include "backendp.h"

//
// This file contains private functions of the backend.
//

NTSTATUS NTAPI ExportLogThreadProc(
	IN	PVOID	Parameter)
{
	NTSTATUS Status;
	HANDLE FileHandle;
	UNICODE_STRING TextFileNameNt;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PCWSTR TextFileNameWin32;
	ULONG EntryIndex;
	ULONG MaxEntryIndex;
	ULONG SizeofMaxEntryIndex;
	ULONG CompletedPercentage;
	ULONG PreviousCompletedPercentage;

	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_WAIT));
	EnableWindow(MainWindow, FALSE);

	TextFileNameWin32 = (PCWSTR) Parameter;
	EntryIndex = 0;
	SizeofMaxEntryIndex = sizeof(MaxEntryIndex);
	CompletedPercentage = 0;
	PreviousCompletedPercentage = 0;

	//
	// find out total number of log entries, for calculating percentage
	//

	Status = VxlQueryInformationLog(
		State->LogHandle,
		LogTotalNumberOfEvents,
		&MaxEntryIndex,
		&SizeofMaxEntryIndex);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	// safe to do this because we refuse to open log files with no entries
	--MaxEntryIndex;

	//
	// Convert win32 name to NT name and open the destination file
	//

	Status = RtlDosPathNameToNtPathName_U_WithStatus(
		TextFileNameWin32,
		&TextFileNameNt,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	InitializeObjectAttributes(&ObjectAttributes, &TextFileNameNt, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtCreateFile(
		&FileHandle,
		GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OVERWRITE_IF,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0);

	RtlFreeUnicodeString(&TextFileNameNt);

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	//
	// loop over every log entry, convert to text and write out to the file
	//

	while (EntryIndex <= MaxEntryIndex) {
		PLOGENTRYCACHEENTRY CacheEntry;
		UNICODE_STRING ExportedText;
		LONGLONG ByteOffset;

		CacheEntry = GetLogEntryRaw(EntryIndex++);
		ConvertCacheEntryToText(CacheEntry, &ExportedText, FALSE);

		//
		// Write out the text to the file.
		//

		ByteOffset = -1; // write at end of file
		
		// wait for previous write to complete before starting a new one
		Status = NtWaitForSingleObject(FileHandle, FALSE, NULL);
		ASSERT (Status == STATUS_SUCCESS);

		Status = NtWriteFile(
			FileHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			ExportedText.Buffer,
			ExportedText.Length,
			&ByteOffset,
			NULL);

		RtlFreeUnicodeString(&ExportedText);
		ASSERT (NT_SUCCESS(Status));

		//
		// Calculate percentage completed, for UI update
		//

		CompletedPercentage = (EntryIndex + 1) * 100 / (MaxEntryIndex + 1);
		ASSERT (CompletedPercentage <= 100);
		ASSERT (PreviousCompletedPercentage <= 100);

		//
		// Avoid expensive UI update if the percentage hasn't changed
		//

		if (CompletedPercentage != PreviousCompletedPercentage) {
			SetWindowTextF(StatusBarWindow, L"Exporting log entries. Please wait... (%ld%%)", CompletedPercentage);
			PreviousCompletedPercentage = CompletedPercentage;
		}
	}

	NtClose(FileHandle);

Finished:
	EnableWindow(MainWindow, TRUE);
	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_ARROW));
	SetWindowText(StatusBarWindow, L"Finished.");

	if (NT_SUCCESS(Status)) {
		InfoBoxF(L"Export complete.");
	} else {
		ErrorBoxF(L"Failed to export the log (%s)", KexRtlNtStatusToString(Status));
	}

	return Status;
}

VOID PopulateSourceComponents(
	IN	VXLHANDLE	LogHandle)
{
	HWND SourceComponentListViewWindow;
	ULONG Index;

	SourceComponentListViewWindow = GetDlgItem(FilterWindow, IDC_COMPONENTLIST);
	ListView_DeleteAllItems(SourceComponentListViewWindow);

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header->SourceComponents) &&
					LogHandle->Header->SourceComponents[Index][0] != '\0'; ++Index) {
		LVITEM Item;

		Item.mask = LVIF_TEXT;
		Item.iItem = Index;
		Item.iSubItem = 0;
		Item.pszText = LogHandle->Header->SourceComponents[Index];

		ListView_InsertItem(SourceComponentListViewWindow, &Item);
		ListView_SetCheckState(SourceComponentListViewWindow, Index, TRUE);
	}
}

PLOGENTRYCACHEENTRY AddLogEntryToCache(
	IN	ULONG			EntryIndex,
	IN	PVXLLOGENTRY	LogEntry)
{
	PLOGENTRYCACHEENTRY CacheEntry;
	WCHAR DateFormat[32];
	WCHAR TimeFormat[32];

	CacheEntry = SafeAlloc(LOGENTRYCACHEENTRY, 1);
	if (!CacheEntry) {
		return NULL;
	}

	RtlZeroMemory(CacheEntry, sizeof(*CacheEntry));
	CacheEntry->LogEntry = *LogEntry;
	
	//
	// pre-calc some strings
	//

	GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_AUTOLAYOUT | DATE_SHORTDATE,
		&LogEntry->Time,
		NULL,
		DateFormat,
		ARRAYSIZE(DateFormat),
		NULL);

	GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT,
		&LogEntry->Time,
		NULL,
		TimeFormat,
		ARRAYSIZE(CacheEntry->ShortDateTimeAsString));

	StringCchPrintf(
		CacheEntry->ShortDateTimeAsString,
		ARRAYSIZE(CacheEntry->ShortDateTimeAsString),
		L"%s %s", DateFormat, TimeFormat);

	StringCchPrintf(
		CacheEntry->SourceLineAsString,
		ARRAYSIZE(CacheEntry->SourceLineAsString),
		L"%lu", LogEntry->SourceLine);

	State->LogEntryCache[EntryIndex] = CacheEntry;
	return CacheEntry;
}

//
// Retrieve a log entry from the cache or from the log file.
// This function does not apply any filters.
//
PLOGENTRYCACHEENTRY GetLogEntryRaw(
	IN	ULONG	EntryIndex)
{
	NTSTATUS Status;
	PLOGENTRYCACHEENTRY CacheEntry;

	CacheEntry = State->LogEntryCache[EntryIndex];

	if (!CacheEntry) {
		VXLLOGENTRY LogEntry;

		Status = VxlReadLog(State->LogHandle, EntryIndex, &LogEntry);
		if (!NT_SUCCESS(Status)) {
			return NULL;
		}

		CacheEntry = AddLogEntryToCache(EntryIndex, &LogEntry);
	}

	return CacheEntry;
}

BOOLEAN LogEntryMatchesFilters(
	IN	PLOGENTRYCACHEENTRY	CacheEntry)
{
	BOOLEAN LogEntryMatchesTextFilter;

	//
	// The idea with this function is to apply the easiest/cheapest (in terms of
	// computation power) filters first. So that means, anything which requires
	// string comparisons and regex matching comes last.
	//
	// This function is one of the most critical when it comes to user experience,
	// since it is called potentially hundreds of thousands of times when the user
	// wants to search for text.
	//

	// 1. Does the log entry match the severity filter? If not, then we don't display
	//    this log entry.
	if (State->Filters.SeverityFilters[CacheEntry->LogEntry.Severity] == FALSE) {
		return FALSE;
	}

	// 2. Does this log entry match the source component filter? If not, then we don't
	//    display this log entry.
	if (State->Filters.ComponentFilters[CacheEntry->LogEntry.SourceComponentIndex] == FALSE) {
		return FALSE;
	}

	// 3. Does this log entry match the text filter? Note: empty filter always matches.
	LogEntryMatchesTextFilter = FALSE;

	if (State->Filters.TextFilter.Length == 0) {
		// empty filter
		LogEntryMatchesTextFilter = TRUE;
	} else {
		PUNICODE_STRING TextToSearch;

		TextToSearch = &CacheEntry->LogEntry.TextHeader;

SearchAgain:
		if (State->Filters.TextFilterWildcardMatch) {
			LogEntryMatchesTextFilter = RtlIsNameInExpression(
				&State->Filters.TextFilter,
				TextToSearch,
				!State->Filters.TextFilterCaseSensitive,
				NULL);
		} else {
			if (State->Filters.TextFilterCaseSensitive) {
				if (State->Filters.TextFilterExact) {
					if (StringEqual(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearch(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			} else {
				if (State->Filters.TextFilterExact) {
					if (StringEqualI(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearchI(TextToSearch->Buffer, State->Filters.TextFilter.Buffer)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			}
		}

		if (State->Filters.TextFilterWhole && TextToSearch == &CacheEntry->LogEntry.TextHeader) {
			if (CacheEntry->LogEntry.Text.Buffer != NULL) {
				TextToSearch = &CacheEntry->LogEntry.Text;
				goto SearchAgain;
			}
		}
	}

	if (State->Filters.TextFilterInverted) {
		LogEntryMatchesTextFilter = !LogEntryMatchesTextFilter;
	}

	return LogEntryMatchesTextFilter;
}

//
// Returns an ESTIMATE of the number of log entries that will be displayed
// by those filters. This estimate is not accurate for log files with many
// entries, because creating an accurate value for the number of log entries
// for any given filter requires evaluating the filter on all of the log
// entries.
// The estimate will always be equal to or greater than the actual number of
// log entries that match the filters.
//
ULONG EstimateNumberOfFilteredLogEntries(
	VOID)
{
	ULONG NumberOfFilteredLogEntries;
	ULONG Index;

	NumberOfFilteredLogEntries = 0;

	if (State->NumberOfLogEntries < 2000000) {
		//
		// do an accurate estimate - evaluate all the entries
		//
		for (Index = 0; Index < State->NumberOfLogEntries; Index++) {
			PLOGENTRYCACHEENTRY CacheEntry;

			CacheEntry = GetLogEntryRaw(Index);

			if (LogEntryMatchesFilters(CacheEntry)) {
				NumberOfFilteredLogEntries++;
			}
		}
	} else {
		//
		// inaccurate estimate - just wing it
		//
		for (Index = 0; Index < LogSeverityMaximumValue; Index++) {
			if (State->Filters.SeverityFilters[Index]) {
				NumberOfFilteredLogEntries += State->LogHandle->Header->EventSeverityTypeCount[Index];
			}
		}
	}

	return NumberOfFilteredLogEntries;
}

VOID RebuildFilterCache(
	VOID)
{
	ULONG EstimatedNumberOfFilteredLogEntries;

	//
	// clear the filtered entries lookup table
	//
	SafeFree(State->FilteredLookupCache);

	//
	// reallocate and initialize the lookup table
	//
	EstimatedNumberOfFilteredLogEntries = EstimateNumberOfFilteredLogEntries();
	State->EstimatedNumberOfFilteredLogEntries = EstimatedNumberOfFilteredLogEntries;
	State->FilteredLookupCache = SafeAlloc(ULONG, EstimatedNumberOfFilteredLogEntries);
	FillMemory(State->FilteredLookupCache, EstimatedNumberOfFilteredLogEntries * sizeof(ULONG), 0xFF);
}