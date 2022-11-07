#include "vxlview.h"
#include <DbgHelp.h>
#include "resource.h"
#include "backendp.h"

//
// This file contains private functions of the backend.
//

DWORD WINAPI ExportLogThreadProc(
	IN	PVOID	Parameter)
{
	VXLSTATUS Status;
	PCWSTR TextFileName;

	TextFileName = (PCWSTR) Parameter;

	SetWindowText(StatusBarWindow, L"Exporting log entries. Please wait...");
	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_WAIT));
	EnableWindow(MainWindow, FALSE);

	Status = VxlExportLogToText(
		State->LogHandle,
		TextFileName,
		FALSE);

	EnableWindow(MainWindow, TRUE);
	SetClassLongPtr(MainWindow, GCLP_HCURSOR, (LONG_PTR) LoadCursor(NULL, IDC_ARROW));
	SetWindowText(StatusBarWindow, L"Finished.");

	if (VXL_SUCCEEDED(Status)) {
		InfoBoxF(L"Export complete.");
	} else {
		ErrorBoxF(L"Failed to export the log. %s.", VxlErrorLookup(Status));
	}

	return 0;
}

VOID DestroyLogEntryCache(
	IN	ULONG					NumberOfEntries,
	IN	PPLOGENTRYCACHEENTRY	Cache)
{
	ULONG Index;

	if (!Cache) {
		return;
	}

	for (Index = 0; Index < NumberOfEntries; Index++) {
		PLOGENTRYCACHEENTRY CacheEntry;

		CacheEntry = Cache[Index];
		if (!CacheEntry) {
			continue;
		}

		VxlFreeLogEntry(&CacheEntry->LogEntry);
	}
}

VOID PopulateSourceComponents(
	IN	VXLHANDLE	LogHandle)
{
	HWND SourceComponentListViewWindow;
	ULONG Index;

	SourceComponentListViewWindow = GetDlgItem(FilterWindow, IDC_COMPONENTLIST);
	ListView_DeleteAllItems(SourceComponentListViewWindow);

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header.SourceComponents) &&
					LogHandle->Header.SourceComponents[Index][0] != '\0'; ++Index) {
		LVITEM Item;

		Item.mask = LVIF_TEXT;
		Item.iItem = Index;
		Item.iSubItem = 0;
		Item.pszText = LogHandle->Header.SourceComponents[Index];

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

	ZeroMemory(CacheEntry, sizeof(*CacheEntry));
	CacheEntry->LogEntry = LogEntry;
	
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
	PLOGENTRYCACHEENTRY CacheEntry;

	CacheEntry = State->LogEntryCache[EntryIndex];

	if (!CacheEntry) {
		VXLSTATUS Status;
		PVXLLOGENTRY LogEntry;

		Status = VxlReadLogEntry(State->LogHandle, EntryIndex, &LogEntry);
		if (VXL_FAILED(Status)) {
			return NULL;
		}

		CacheEntry = AddLogEntryToCache(EntryIndex, LogEntry);
	}

	return CacheEntry;
}

//
// You can't just pass any random string. You MUST pass the string that is returned
// by VXLL in the VXLLOGENTRY structure. Even if it is char-for-char identical, it
// won't work if it is a different pointer.
//
// This method is way faster than doing anything else.
//
FORCEINLINE USHORT GetSourceComponentIndex(
	IN	PCWSTR	SourceComponentString)
{
	return (USHORT) (((ULONG_PTR) SourceComponentString - (ULONG_PTR) State->LogHandle->Header.SourceComponents[0]) >> 5);
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
	if (State->Filters.SeverityFilters[CacheEntry->LogEntry->Severity] == FALSE) {
		return FALSE;
	}

	// 2. Does this log entry match the source component filter? If not, then we don't
	//    display this log entry.
	if (State->Filters.ComponentFilters[GetSourceComponentIndex(CacheEntry->LogEntry->SourceComponent)] == FALSE) {
		return FALSE;
	}

	// 3. Does this log entry match the text filter? Note: empty filter always matches.
	LogEntryMatchesTextFilter = FALSE;

	if (State->Filters.TextFilter == NULL || State->Filters.TextFilter[0] == '\0') {
		// empty filter
		LogEntryMatchesTextFilter = TRUE;
	} else {
		PCWSTR TextToSearch;

		TextToSearch = CacheEntry->LogEntry->TextHeader;

SearchAgain:
		if (State->Filters.TextFilterWildcardMatch) {
			LogEntryMatchesTextFilter = SymMatchStringW(
				CacheEntry->LogEntry->TextHeader,
				State->Filters.TextFilter,
				State->Filters.TextFilterCaseSensitive);
		} else {
			if (State->Filters.TextFilterCaseSensitive) {
				if (State->Filters.TextFilterExact) {
					if (StringEqual(TextToSearch, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearch(TextToSearch, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			} else {
				if (State->Filters.TextFilterExact) {
					if (StringEqualI(TextToSearch, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (StringSearchI(TextToSearch, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			}
		}

		if (State->Filters.TextFilterWhole && TextToSearch == CacheEntry->LogEntry->TextHeader) {
			TextToSearch = CacheEntry->LogEntry->Text;
			goto SearchAgain;
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

	if (State->NumberOfLogEntries < 200000) {
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
				NumberOfFilteredLogEntries += State->LogHandle->Header.EventSeverityTypeCount[Index];
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