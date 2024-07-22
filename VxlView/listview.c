#include "vxlview.h"
#include "resource.h"
#include "backendp.h"

//
// This file contains functions for dealing with the list-view control in
// the main window.
//

//
// If you change any of the column stuff, also make sure to change
// the LOGENTRYCOLUMNS enumeration in VxlView.h!
//
STATIC PWSTR ColumnNames[] = {
	L"Severity",
	L"Date/Time",
	L"Component",
	L"File",
	L"Line",
	L"Function",
	L"Message"
};

STATIC CONST USHORT ColumnDefaultWidths[] = {
	90, 130, 80, 80, 40, 130, 500
};

STATIC CONST USHORT SeverityIcons[] = {
	1027,		// Critical
	98,			// Error
	84,			// Warning
	81,			// Information
	177,		// Detail
	15,			// Debug
};

VOID InitializeListView(
	VOID)
{
	ULONG Index;
	LVCOLUMN ColumnInformation;
	HIMAGELIST ImageList;
	HMODULE ImageRes;
	ULONG SmallIconWidth;
	ULONG SmallIconHeight;

	UNCONST (HWND) ListViewWindow = GetDlgItem(MainWindow, IDC_LISTVIEW);

	// This is responsible for that nice looking blue (or whatever color
	// you have configured) selection highlights.
	SetWindowTheme(ListViewWindow, L"Explorer", NULL);

	ListView_SetExtendedListViewStyle(
		ListViewWindow,
		LVS_EX_FULLROWSELECT |				// select full row instead of only part of the label
		LVS_EX_HEADERDRAGDROP |				// allow user to rearrange the columns to his liking
		LVS_EX_LABELTIP |					// allow user to see hidden text caused by narrow columns
		LVS_EX_DOUBLEBUFFER);				// eliminate flickering and artifacts

	//
	// Insert each column into the list-view.
	//
	ColumnInformation.mask = LVCF_FMT | LVCF_ORDER | LVCF_TEXT | LVCF_WIDTH | LVCF_MINWIDTH;
	for (Index = 0; Index <= ColumnMaxValue - 1; Index++) {
		ColumnInformation.fmt = (Index == ColumnSourceLine) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ColumnInformation.iOrder = Index;
		ColumnInformation.pszText = ColumnNames[Index];
		ColumnInformation.cx = DpiScaleX(ColumnDefaultWidths[Index]);
		ColumnInformation.cxMin = DpiScaleX(ColumnDefaultWidths[Index] / 2);
		ListView_InsertColumn(ListViewWindow, Index, &ColumnInformation);
	}

	//
	// Create and associate the image list containing icons representing
	// the various log-entry severity levels.
	//
	ImageRes = LoadLibraryEx(L"imageres.dll", NULL, LOAD_LIBRARY_AS_DATAFILE);
	SmallIconWidth = GetSystemMetrics(SM_CXSMICON);
	SmallIconHeight = GetSystemMetrics(SM_CYSMICON);

	ImageList = ImageList_Create(
		SmallIconWidth,
		SmallIconHeight,
		ILC_MASK | ILC_COLOR32,
		1, 1);

	for (Index = 0; Index < LogSeverityMaximumValue; Index++) {
		HICON Icon;

		Icon = (HICON) LoadImage(
			ImageRes,
			MAKEINTRESOURCE(SeverityIcons[Index]),
			IMAGE_ICON,
			SmallIconWidth,
			SmallIconHeight,
			0);

		ImageList_AddIcon(ImageList, Icon);
	}

	ListView_SetImageList(ListViewWindow, ImageList, LVSIL_SMALL);
	FreeLibrary(ImageRes);
}

VOID ResizeListView(
	IN	USHORT	MainWindowWidth)
{
	RECT FilterWindowRect;

	//
	// Put the bottom of the list view 10px above the top of the filter window.
	//

	GetWindowRect(FilterWindow, &FilterWindowRect);
	MapWindowRect(HWND_DESKTOP, MainWindow, &FilterWindowRect);
	
	SetWindowPos(
		ListViewWindow,
		NULL,
		0, 0,
		MainWindowWidth, FilterWindowRect.top - DpiScaleY(10),
		SWP_NOZORDER);

	//
	// Make the log text column in the list-view take up all remaining space.
	//
	ListView_SetColumnWidth(ListViewWindow, ColumnText, LVSCW_AUTOSIZE_USEHEADER);
}

VOID ResetListView(
	VOID)
{
	//
	// Make sure that the list view doesn't think there should be any entries
	// in it.
	//
	ListView_SetItemCountEx(ListViewWindow, 0, LVSICF_NOINVALIDATEALL);
}

//
// Called by MainWndProc when the list-view control wants data.
//
VOID PopulateListViewItem(
	IN OUT	LPLVITEM	Item)
{
	PLOGENTRYCACHEENTRY CacheEntry;

	CacheEntry = GetLogEntry(Item->iItem);
	if (!CacheEntry) {
		return;
	}

	switch (Item->iSubItem) {
	case ColumnSeverity:
		Item->iImage = CacheEntry->LogEntry.Severity;
		Item->pszText = (PWSTR) VxlSeverityToText(CacheEntry->LogEntry.Severity, FALSE);
		break;
	case ColumnDateTime:
		Item->pszText = CacheEntry->ShortDateTimeAsString;
		break;
	case ColumnSourceComponent:
		Item->pszText = State->LogHandle->Header->SourceComponents[CacheEntry->LogEntry.SourceComponentIndex];
		break;
	case ColumnSourceFile:
		Item->pszText = State->LogHandle->Header->SourceFiles[CacheEntry->LogEntry.SourceFileIndex];
		break;
	case ColumnSourceLine:
		Item->pszText = CacheEntry->SourceLineAsString;
		break;
	case ColumnSourceFunction:
		Item->pszText = State->LogHandle->Header->SourceFunctions[CacheEntry->LogEntry.SourceFunctionIndex];
		break;
	case ColumnText:
		Item->pszText = CacheEntry->LogEntry.TextHeader.Buffer;
		break;
	default:
		ASSUME (FALSE);
	}
}

VOID HandleListViewContextMenu(
	IN	PPOINT	ClickPoint)
{
	HWND ListViewHeaderWindow;
	RECT ListViewHeaderWindowRect;
	LVHITTESTINFO HitTestInfo;
	ULONG EntryIndex;

	NTSTATUS Status;
	HGLOBAL CopiedText;
	UNICODE_STRING LogEntryText;
	PWCHAR TextBuffer;
	PLOGENTRYCACHEENTRY CacheEntry;
	ULONG MenuSelection;

	ListViewHeaderWindow = ListView_GetHeader(ListViewWindow);
	GetWindowRect(ListViewHeaderWindow, &ListViewHeaderWindowRect);

	if (PtInRect(&ListViewHeaderWindowRect, *ClickPoint)) {
		// user right clicked on the column headers
		return;
	}

	HitTestInfo.pt = *ClickPoint;
	ScreenToClient(ListViewWindow, &HitTestInfo.pt);
	EntryIndex = ListView_HitTest(ListViewWindow, &HitTestInfo);

	if (EntryIndex == -1) {
		// user right clicked on a blank space in the list view
		return;
	}

	MenuSelection = ContextMenu(ListViewWindow, IDM_LISTVIEWMENU, ClickPoint);
	if (!MenuSelection) {
		// context menu was cancelled without selecting a menu item
		return;
	}

	CacheEntry = GetLogEntry(EntryIndex);
	if (!CacheEntry) {
		return;
	}

	Status = ConvertCacheEntryToText(
		CacheEntry,
		&LogEntryText,
		MenuSelection == M_COPYLONG ? TRUE : FALSE);

	if (!NT_SUCCESS(Status)) {
		return;
	}

	CopiedText = GlobalAlloc(GMEM_MOVEABLE, LogEntryText.Length + sizeof(WCHAR));
	if (!CopiedText) {
		return;
	}

	TextBuffer = (PWCHAR) GlobalLock(CopiedText);
	RtlCopyMemory(TextBuffer, LogEntryText.Buffer, LogEntryText.Length + sizeof(WCHAR));
	ASSERT (wcslen(TextBuffer) == LogEntryText.Length / sizeof(WCHAR));
	RtlFreeUnicodeString(&LogEntryText);
	GlobalUnlock(CopiedText);
		
	if (!NT_SUCCESS(Status)) {
		GlobalFree(CopiedText);
		return;
	}

	if (OpenClipboard(MainWindow)) {
		EmptyClipboard();
		SetClipboardData(CF_UNICODETEXT, CopiedText);
		CloseClipboard();
		SetWindowText(StatusBarWindow, L"Text copied to clipboard.");
	}
}

VOID SelectListViewItemByIndex(
	IN	ULONG	Index)
{
	ListView_SetItemState(
		ListViewWindow,
		Index,
		LVNI_SELECTED | LVNI_FOCUSED,
		LVNI_SELECTED | LVNI_FOCUSED);

	ListView_EnsureVisible(
		ListViewWindow,
		Index,
		FALSE);
}