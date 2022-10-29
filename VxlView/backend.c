#include "vxlview.h"
#include "resource.h"
#include "backendp.h"

//
// This file contains functions to interact with VXLL, maintain a cache
// of log entries, and filter/sort the log entries.
//

PBACKENDSTATE State = NULL;

//
// This function must be called before any other function in this file is used.
// Otherwise, the application will crash.
//
VOID InitializeBackend(
	VOID)
{
	STATIC BACKENDSTATE StateInternal;
	ZeroMemory(&StateInternal, sizeof(StateInternal));
	State = &StateInternal;
}

//
// Prepare for application exit. Call before quitting the application.
//
VOID CleanupBackend(
	VOID)
{
	if (State) {
		//
		// The only thing we do is close the log file, because it is necessary for
		// the smooth operation of future applications that handle the log file.
		// All allocated memory will be released by the operating system so there is
		// no reason to waste time trying to clean it.
		//
		VxlCloseLogFile(&State->LogHandle);
	}
}

//
// Return TRUE if a log file is currently opened, or FALSE otherwise.
//
BOOLEAN IsLogFileOpened(
	VOID)
{
	if (State->LogHandle) {
		return TRUE;
	} else {
		return FALSE;
	}
}

//
// Open a log file.
//
BOOLEAN OpenLogFile(
	IN	PCWSTR	LogFileName)
{
	VXLSTATUS Status;
	VXLHANDLE NewLogHandle;
	PPLOGENTRYCACHEENTRY NewLogEntryCache;
	ULONG NewNumberOfLogEntries;
	ULONG SizeOfNewNumberOfLogEntries;

	NewLogHandle = NULL;
	NewLogEntryCache = NULL;

	//
	// Tell VXLL to open the file.
	//
	SetWindowText(StatusBarWindow, L"Opening file, please wait...");
	Status = VxlOpenLogFileReadOnly(LogFileName, &NewLogHandle);
	if (VXL_FAILED(Status)) {
		ErrorBoxF(L"%s failed to open %s. %s.",
				  FRIENDLYAPPNAME, LogFileName, VxlErrorLookup(Status));
		goto OpenFailure;
	}

	//
	// Find how many log entries there are in the file and allocate memory for
	// the log-entry cache.
	//
	SizeOfNewNumberOfLogEntries = sizeof(NewNumberOfLogEntries);
	Status = VxlQueryLogInformation(
		NewLogHandle,
		LogNumberOfEntries,
		&NewNumberOfLogEntries,
		&SizeOfNewNumberOfLogEntries);
	if (VXL_FAILED(Status)) {
		ErrorBoxF(L"%s failed to query information about %s. %s.",
				  FRIENDLYAPPNAME, LogFileName, VxlErrorLookup(Status));
		goto OpenFailure;
	}

	if (NewNumberOfLogEntries == 0) {
		MessageBoxF(0, TD_INFORMATION_ICON, NULL, NULL,
					L"There are no entries in the log file you selected.\r\n"
					L"Please select a log file which is not empty.");
		goto OpenFailure;
	}

	NewLogEntryCache = SafeAlloc(PLOGENTRYCACHEENTRY, NewNumberOfLogEntries);
	if (!NewLogEntryCache) {
		ErrorBoxF(L"%s failed to allocate memory to store the log entry cache. "
				  L"Try closing other applications or browser tabs before trying again.",
				  FRIENDLYAPPNAME);
		goto OpenFailure;
	}

	ZeroMemory(NewLogEntryCache, NewNumberOfLogEntries * sizeof(PLOGENTRYCACHEENTRY));

	//
	// copy values of all New* variables into the global ones
	//

	if (State->LogHandle) {
		VxlCloseLogFile(&State->LogHandle);
	}

	if (State->LogEntryCache) {
		DestroyLogEntryCache(State->NumberOfLogEntries, State->LogEntryCache);
	}

	SafeFree(State->FilteredLookupCache);

	ZeroMemory(State, sizeof(*State));
	State->NumberOfLogEntries = NewNumberOfLogEntries;
	State->LogHandle = NewLogHandle;
	State->LogEntryCache = NewLogEntryCache;

	//
	// perform other misc. actions such as updating the UI text and whatever
	//
	UpdateMainMenu();
	SetWindowText(StatusBarWindow, L"Finished.");
	StatusBar_SetTextF(StatusBarWindow, 1, L"%lu entr%s in file",
					   State->NumberOfLogEntries,
					   State->NumberOfLogEntries == 1 ? L"y" : L"ies");
	SetWindowTextF(MainWindow, L"%s - %s", FRIENDLYAPPNAME, LogFileName);

	PopulateSourceComponents(State->LogHandle);
	ResetFilterControls();

	return TRUE;

OpenFailure:
	if (NewLogHandle) {
		VxlCloseLogFile(&NewLogHandle);
	}

	if (NewLogEntryCache) {
		DestroyLogEntryCache(NewNumberOfLogEntries, NewLogEntryCache);
	}

	SetWindowText(StatusBarWindow, L"Couldn't open the log file.");
	return FALSE;
}

//
// Open a dialog to ask the user for a log file, and then open it.
//
BOOLEAN OpenLogFileWithPrompt(
	VOID)
{
	BOOLEAN Success;
	OPENFILENAME OpenDialogInfo;
	WCHAR OpenFileName[MAX_PATH];

	ZeroMemory(&OpenDialogInfo, sizeof(OpenDialogInfo));
	OpenFileName[0] = '\0';
	OpenDialogInfo.lStructSize				= sizeof(OpenDialogInfo);
	OpenDialogInfo.hwndOwner				= MainWindow;
	OpenDialogInfo.lpstrFilter				= L"VXLog Files (*.vxl)\0*.vxl\0All Files (*.*)\0*.*\0";
	OpenDialogInfo.nMaxFile					= ARRAYSIZE(OpenFileName);
	OpenDialogInfo.lpstrFile				= OpenFileName;
	OpenDialogInfo.lpstrTitle				= L"Select a log file...";
	OpenDialogInfo.Flags					= OFN_PATHMUSTEXIST;
	OpenDialogInfo.lpstrDefExt				= L"vxl";

	Success = GetOpenFileName(&OpenDialogInfo);

	if (Success) {
		Success = OpenLogFile(OpenFileName);
	}

	return Success;
}

//
// Export the currently opened log to a text file.
//
VOID ExportLog(
	IN	PCWSTR	TextFileName)
{
	HANDLE ThreadHandle;

	ThreadHandle = CreateThread(
		NULL,
		0,
		ExportLogThreadProc,
		(PVOID) TextFileName,
		0,
		NULL);

	CloseHandle(ThreadHandle);
}

//
// Open a dialog to ask the user for a save file location, and then
// export the currently opened log.
//
BOOLEAN ExportLogWithPrompt(
	VOID)
{
	BOOLEAN Success;
	HRESULT Result;
	OPENFILENAME SaveDialogInfo;
	STATIC WCHAR SaveFileName[MAX_PATH];
	PCWSTR FileNameFormat = L"Exported log from %s.txt";

	ZeroMemory(&SaveDialogInfo, sizeof(SaveDialogInfo));

	//
	// Set a default file name for the exported .txt file.
	//

	Result = StringCchPrintf(
		SaveFileName,
		ARRAYSIZE(SaveFileName),
		FileNameFormat,
		State->LogHandle->Header.SourceApplication);
	if (FAILED(Result)) {
		StringCchCopy(SaveFileName, ARRAYSIZE(SaveFileName), L"Exported Log.txt");
	}

	PathReplaceIllegalCharacters(SaveFileName, '_', FALSE);

	SaveDialogInfo.lStructSize				= sizeof(SaveDialogInfo);
	SaveDialogInfo.hwndOwner				= MainWindow;
	SaveDialogInfo.lpstrFilter				= L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
	SaveDialogInfo.nMaxFile					= ARRAYSIZE(SaveFileName);
	SaveDialogInfo.lpstrFile				= SaveFileName;
	SaveDialogInfo.Flags					= OFN_OVERWRITEPROMPT;
	SaveDialogInfo.lpstrDefExt				= L"txt";

	Success = GetSaveFileName(&SaveDialogInfo);

	if (Success) {
		ExportLog(SaveFileName);
	}

	return Success;
}

VOID SetBackendFilters(
	IN	PBACKENDFILTERS	Filters)
{
	CopyMemory(&State->Filters, Filters, sizeof(*Filters));
	RebuildFilterCache();
	ListView_SetItemCount(ListViewWindow, State->EstimatedNumberOfFilteredLogEntries);
}

ULONG GetLogEntryRawIndex(
	IN	ULONG	EntryIndex)
{
	ULONG DisplayIndex;
	ULONG RawIndex;

	//
	// if we already cached this, we don't need to evaluate filters
	//
	if (State->FilteredLookupCache[EntryIndex] != ((ULONG) -1)) {
		return State->FilteredLookupCache[EntryIndex];
	}

	for (DisplayIndex = 0;
		 DisplayIndex < State->EstimatedNumberOfFilteredLogEntries && DisplayIndex <= EntryIndex;
		 DisplayIndex++) {

		//
		// Run every entry against the filters up to the requested display index,
		// if it hasn't been filtered already.
		//
		if (State->FilteredLookupCache[DisplayIndex] != -1) {
			continue;
		}

		if (DisplayIndex != 0) {
			RawIndex = State->FilteredLookupCache[DisplayIndex - 1] + 1;
		} else {
			RawIndex = 0;
		}

		for (; RawIndex < State->NumberOfLogEntries; RawIndex++) {
			PLOGENTRYCACHEENTRY CacheEntry;

			CacheEntry = GetLogEntryRaw(RawIndex);

			if (!CacheEntry) {
				continue;
			}

			if (LogEntryMatchesFilters(CacheEntry)) {
				State->FilteredLookupCache[DisplayIndex] = RawIndex;
				break;
			}

			if (RawIndex == (State->NumberOfLogEntries - 1)) {
				// we have reached the end, there are no more filtered entries
				ListView_SetItemCountEx(ListViewWindow, DisplayIndex, LVSICF_NOINVALIDATEALL);
			}
		}
	}

	return State->FilteredLookupCache[EntryIndex];
}

//
// Get a log entry, respecting the current filters.
//
PLOGENTRYCACHEENTRY GetLogEntry(
	IN	ULONG	EntryIndex)
{
	ULONG RawIndex;

	RawIndex = GetLogEntryRawIndex(EntryIndex);

	if (RawIndex == -1) {
		return NULL;
	}

	return GetLogEntryRaw(RawIndex);
}