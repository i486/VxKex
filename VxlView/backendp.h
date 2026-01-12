#pragma once

//
// Private header file for the backend.
//

typedef struct {
	VXLHANDLE LogHandle;
	PPLOGENTRYCACHEENTRY LogEntryCache;	// Array of pointers to LOGENTRYCACHEENTRY structures. Some of the
										// pointers may be NULL. The following condition holds:
										//     if (LogEntryCache) {
										//         DefHeapSize(LogEntryCache) / sizeof(PLOGENTRYCACHEENTRY) == NumberOfLogEntries;
										//     }
	BACKENDFILTERS Filters;
	ULONG EstimatedNumberOfFilteredLogEntries;
	PULONG FilteredLookupCache;			// display entry -> cache entry lookup table
	
	ULONG NumberOfLogEntries;			// number of log entries in the file
	ULONG FilteredNumberOfLogEntries;	// number of log entries that are displayed by the user's filter selection
} BACKENDSTATE, *PBACKENDSTATE, **PPBACKENDSTATE, *CONST PCBACKENDSTATE, **CONST PPCBACKENDSTATE;

//
// Global variables, defined in backend.c
//
extern PBACKENDSTATE State;

//
// Private functions, defined in backendp.c
//
NTSTATUS NTAPI ExportLogThreadProc(
	IN	PVOID	Parameter);
VOID PopulateSourceComponents(
	IN	VXLHANDLE	LogHandle);
PLOGENTRYCACHEENTRY GetLogEntryRaw(
	IN	ULONG	EntryIndex);
PLOGENTRYCACHEENTRY AddLogEntryToCache(
	IN	ULONG			EntryIndex,
	IN	PVXLLOGENTRY	LogEntry);
VOID RebuildFilterCache(
	VOID);
BOOLEAN LogEntryMatchesFilters(
	IN	PLOGENTRYCACHEENTRY	CacheEntry);