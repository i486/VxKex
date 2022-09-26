#include <Windows.h>
#include <CommCtrl.h>
#include <DbgHelp.h>
#include <intrin.h>
#include <KexComm.h>
#include <VXLL/VXLL.h>
#include "vxlview.h"
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
					LogHandle->Header.SourceComponents[Index][0] != '\0'; Index++) {
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
	PWSTR StringAfterNewline;
	WCHAR DateFormat[32];
	WCHAR TimeFormat[32];

	CacheEntry = (PLOGENTRYCACHEENTRY) DefHeapAlloc(sizeof(*CacheEntry));
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

	//
	// calculate the length of the text that will be displayed directly
	// in the listview.
	// If the log-entry text contains a newline (\r\n) then we will not display
	// the newline or any following text in the list view - this will be considered
	// more detailed/extra information that is only displayed in the details view.
	// If you change how any of this works make sure it doesn't disturb the code in
	// PopulateDetailsWindow (details.c).
	//
	StringAfterNewline = wcsstr(LogEntry->Text, L"\r\n");
	if (StringAfterNewline) {
		CacheEntry->ListViewTextLength = (USHORT) min(StringAfterNewline - LogEntry->Text, USHRT_MAX);
	} else {
		CacheEntry->ListViewTextLength = 0;
	}

	if (CacheEntry->ListViewTextLength != 0) {
		*StringAfterNewline = '\0';
	}

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
USHORT GetSourceComponentIndex(
	IN	PCWSTR	SourceComponentString)
{
	return (USHORT) ((ULONG_PTR) SourceComponentString - (ULONG_PTR) State->LogHandle->Header.SourceComponents[0]) >> 4;
}

#define FastToWUpper(wc) (((wc) >= 'a' && (wc) <= 'z') ? ((wc) - 32) : (wc))

STATIC FORCEINLINE BOOLEAN FastStrStrIW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	WCHAR NeedleFirst;
	
	NeedleFirst = FastToWUpper(*Needle);

	while (TRUE) {
		PCWSTR Needle2 = Needle;

		// find first char of needle in haystack
		while (FastToWUpper(*Haystack) != NeedleFirst) {
			if (!*++Haystack) {
				// at end of haystack - needle not found
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && FastToWUpper(*Haystack) == FastToWUpper(*Needle2)) {
			if (!*++Needle2) {
				// end of needle - this means the needle is entirely in the haystack
				return TRUE;
			}

			if (!*++Haystack) {
				// end of haystack - this means not the entire needle in haystack
				return FALSE;
			}
		}
	}
}

STATIC FORCEINLINE BOOLEAN FastStrStrW(
	IN	PCWSTR	Haystack,
	IN	PCWSTR	Needle)
{
	while (TRUE) {
		PCWSTR Needle2 = Needle;

		// find first char of needle in haystack
		while (*Haystack != *Needle) {
			if (!*++Haystack) {
				// at end of haystack - needle not found
				return FALSE;
			}
		}

		while (*Haystack && *Needle2 && *Haystack == *Needle2) {
			if (!*++Needle2) {
				// end of needle - this means the needle is entirely in the haystack
				return TRUE;
			}

			if (!*++Haystack) {
				// end of haystack - this means not the entire needle in haystack
				return FALSE;
			}
		}
	}
}

// return TRUE if strings are equal
STATIC FORCEINLINE BOOLEAN FastStrEquIW(
	IN	PCWSTR	String1,
	IN	PCWSTR	String2)
{
	while (*String1 && *String2 && FastToWUpper(*String1) == FastToWUpper(*String2)) {
		++String1;
		++String2;
	}

	// compare null terminators, they should both be null if the strings are equal
	return (*String1 == *String2);
}

STATIC FORCEINLINE BOOLEAN FastStrEquW(
	IN	PCWSTR	String1,
	IN	PCWSTR	String2)
{
	while (*String1 && *String2 && *String1 == *String2) {
		++String1;
		++String2;
	}

	return (*String1 == *String2);
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
		if (State->Filters.TextFilterWildcardMatch) {
			LogEntryMatchesTextFilter = SymMatchStringW(
				CacheEntry->LogEntry->Text,
				State->Filters.TextFilter,
				State->Filters.TextFilterCaseSensitive);
		} else {
			if (State->Filters.TextFilterCaseSensitive) {
				if (State->Filters.TextFilterExact) {
					if (FastStrEquW(CacheEntry->LogEntry->Text, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (FastStrStrW(CacheEntry->LogEntry->Text, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
			} else {
				if (State->Filters.TextFilterExact) {
					if (FastStrEquIW(CacheEntry->LogEntry->Text, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				} else {
					if (FastStrStrIW(CacheEntry->LogEntry->Text, State->Filters.TextFilter)) {
						LogEntryMatchesTextFilter = TRUE;
					}
				}
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

	if (State->NumberOfLogEntries < 50000) {
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
	if (State->FilteredLookupCache) {
		DefHeapFree(State->FilteredLookupCache);
		State->FilteredLookupCache = NULL;
	}

	//
	// reallocate and initialize the lookup table
	//
	EstimatedNumberOfFilteredLogEntries = EstimateNumberOfFilteredLogEntries();
	State->EstimatedNumberOfFilteredLogEntries = EstimatedNumberOfFilteredLogEntries;
	State->FilteredLookupCache = (PULONG) DefHeapAlloc(EstimatedNumberOfFilteredLogEntries * sizeof(ULONG));
	__stosd(State->FilteredLookupCache, -1, EstimatedNumberOfFilteredLogEntries);
}