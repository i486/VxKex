#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include <KexComm.h>
#include <VXLL/VXLL.h>

// begin configuration
#define PROMPT_FOR_FILE_ON_STARTUP FALSE
// end configuration

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
	PVXLLOGENTRY LogEntry;
	USHORT ListViewTextLength;			// number of chars. until 1st newline in log entry text, or 0 if not applicable
	WCHAR SourceLineAsString[11];
	WCHAR ShortDateTimeAsString[32];
} LOGENTRYCACHEENTRY, *PLOGENTRYCACHEENTRY, **PPLOGENTRYCACHEENTRY, *CONST PCLOGENTRYCACHEENTRY, **CONST PPCLOGENTRYCACHEENTRY;

typedef struct {
	PWSTR TextFilter;					// same as what user typed in search box
	BOOLEAN TextFilterCaseSensitive;
	BOOLEAN TextFilterWildcardMatch;
	BOOLEAN TextFilterInverted;
	BOOLEAN TextFilterExact;
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
VOID LengthenLogEntryText(
	IN OUT	PLOGENTRYCACHEENTRY CacheEntry);
VOID ShortenLogEntryText(
	IN OUT	PLOGENTRYCACHEENTRY CacheEntry);
ULONG GetLogEntryRawIndex(
	IN	ULONG	EntryIndex);
PLOGENTRYCACHEENTRY GetLogEntry(
	IN	ULONG	EntryIndex);
VOID SetBackendFilters(
	IN	PBACKENDFILTERS	Filters);

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