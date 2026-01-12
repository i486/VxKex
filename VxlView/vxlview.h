#pragma once
#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

#define FRIENDLYAPPNAME L"Log Viewer"

#define UNCONST(Type) *(Type*)&

typedef enum {
	ColumnSeverity,
	ColumnDateTime,
	ColumnSourceComponent,
	ColumnSourceFile,
	ColumnSourceLine,
	ColumnSourceFunction,
	ColumnText,
	ColumnMaxValue
} LOGENTRYCOLUMNS;

typedef struct {
	VXLLOGENTRY LogEntry;
	WCHAR SourceLineAsString[11];
	WCHAR ShortDateTimeAsString[32];
} LOGENTRYCACHEENTRY, *PLOGENTRYCACHEENTRY, **PPLOGENTRYCACHEENTRY, *CONST PCLOGENTRYCACHEENTRY, **CONST PPCLOGENTRYCACHEENTRY;

typedef struct {
	UNICODE_STRING TextFilter;					// same as what user typed in search box
	BOOLEAN TextFilterCaseSensitive;
	BOOLEAN TextFilterWildcardMatch;
	BOOLEAN TextFilterInverted;
	BOOLEAN TextFilterExact;
	BOOLEAN TextFilterWhole;
	BOOLEAN SeverityFilters[LogSeverityMaximumValue];
	BOOLEAN ComponentFilters[64];
} BACKENDFILTERS, *PBACKENDFILTERS, **PPBACKENDFILTERS, *CONST PCBACKENDFILTERS, **CONST PPCBACKENDFILTERS;

// backend.c

VOID InitializeBackend(
	VOID);
VOID CleanupBackend(
	VOID);
BOOLEAN IsLogFileOpened(
	VOID);
BOOLEAN OpenLogFile(
	IN	PCWSTR	LogFileName);
BOOLEAN OpenLogFileWithPrompt(
	VOID);
VOID ExportLog(
	IN	PCWSTR	TextFileName);
BOOLEAN ExportLogWithPrompt(
	VOID);
ULONG GetLogEntryRawIndex(
	IN	ULONG	EntryIndex);
ULONG GetLogEntryIndexFromRawIndex(
	IN	ULONG	RawIndex);
PLOGENTRYCACHEENTRY GetLogEntry(
	IN	ULONG	EntryIndex);
VOID SetBackendFilters(
	IN	PBACKENDFILTERS	Filters);
NTSTATUS ConvertCacheEntryToText(
	IN	PLOGENTRYCACHEENTRY	CacheEntry,
	OUT	PUNICODE_STRING		ExportedText,
	IN	BOOLEAN				LongForm);

// config.c

VOID SaveWindowPlacement(
	VOID);
VOID RestoreWindowPlacement(
	VOID);
VOID SaveListViewColumns(
	VOID);
VOID RestoreListViewColumns(
	VOID);

// details.c

VOID InitializeDetailsWindow(
	VOID);
VOID ResetDetailsWindow(
	VOID);
VOID ResizeDetailsWindow(
	VOID);
VOID PopulateDetailsWindow(
	IN	ULONG	EntryIndex);
INT_PTR CALLBACK DetailsWndProc(
	IN	HWND	_DetailsWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

// filters.c

VOID InitializeFilterControls(
	VOID);
VOID ResetFilterControls(
	VOID);
VOID ResizeFilterControls(
	VOID);
INT_PTR CALLBACK FilterWndProc(
	IN	HWND	_FilterWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

// globals.c

extern CONST HWND MainWindow;
extern CONST HWND ListViewWindow;
extern CONST HWND StatusBarWindow;
extern CONST HWND DetailsWindow;
extern CONST HWND FilterWindow;

// goto.c

INT_PTR CALLBACK GotoRawDlgProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

// helpabout.c

INT_PTR CALLBACK AboutWndProc(
	IN	HWND	AboutWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

// listview.c

VOID InitializeListView(
	VOID);
VOID ResizeListView(
	IN	USHORT	MainWindowWidth);
VOID ResetListView(
	VOID);
VOID PopulateListViewItem(
	IN	LPLVITEM	Item);
VOID HandleListViewContextMenu(
	IN	PPOINT	ClickPoint);
VOID SelectListViewItemByIndex(
	IN	ULONG	Index);

// statusbar.c

VOID ResizeStatusBar(
	IN	ULONG	MainWindowNewWidth);

// vxlview.c

INT_PTR CALLBACK MainWndProc(
	IN	HWND	_MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);
VOID UpdateMainMenu(
	VOID);