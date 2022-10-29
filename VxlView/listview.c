#include "vxlview.h"
#include "resource.h"

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
		ColumnInformation.cx = ColumnDefaultWidths[Index];
		ColumnInformation.cxMin = ColumnDefaultWidths[Index] / 2;
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
	MapWindowPoints(HWND_DESKTOP, MainWindow, (PPOINT) &FilterWindowRect, 2);
	
	SetWindowPos(
		ListViewWindow,
		NULL,
		0, 0,
		MainWindowWidth, FilterWindowRect.top - 10,
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
		Item->iImage = CacheEntry->LogEntry->Severity;
		Item->pszText = (PWSTR) VxlSeverityLookup(CacheEntry->LogEntry->Severity, FALSE);
		break;
	case ColumnDateTime:
		Item->pszText = CacheEntry->ShortDateTimeAsString;
		break;
	case ColumnSourceComponent:
		Item->pszText = (PWSTR) CacheEntry->LogEntry->SourceComponent;
		break;
	case ColumnSourceFile:
		Item->pszText = (PWSTR) CacheEntry->LogEntry->SourceFile;
		break;
	case ColumnSourceLine:
		Item->pszText = CacheEntry->SourceLineAsString;
		break;
	case ColumnSourceFunction:
		Item->pszText = (PWSTR) CacheEntry->LogEntry->SourceFunction;
		break;
	case ColumnText:
		Item->pszText = (PWSTR) CacheEntry->LogEntry->TextHeader;
		break;
	default:
		ASSUME (FALSE);
	}
}

VOID HandleListViewContextMenu(
	IN	PPOINT	ClickPoint)
{
	LVHITTESTINFO HitTestInfo;

	HitTestInfo.pt = *ClickPoint;
	MapWindowPoints(HWND_DESKTOP, ListViewWindow, &HitTestInfo.pt, 1);
	ListView_HitTest(ListViewWindow, &HitTestInfo);

	if (HitTestInfo.flags & LVHT_ONITEM) {
		VXLSTATUS Status;
		HGLOBAL CopiedText;
		ULONG TextLength;
		PLOGENTRYCACHEENTRY CacheEntry;
		ULONG EntryIndex = HitTestInfo.iItem;
		ULONG MenuSelection;

		MenuSelection = ContextMenu(ListViewWindow, IDM_LISTVIEWMENU, ClickPoint);
		if (!MenuSelection) {
			return;
		}

		CacheEntry = GetLogEntry(EntryIndex);
		if (!CacheEntry) {
			return;
		}

		Status = VxlConvertLogEntryToText(
			CacheEntry->LogEntry,
			NULL,
			&TextLength,
			(MenuSelection == M_COPYLONG));
		if (Status != VXL_INSUFFICIENT_BUFFER) {
			return;
		}

		CopiedText = GlobalAlloc(GMEM_MOVEABLE, TextLength * sizeof(WCHAR));
		if (!CopiedText) {
			return;
		}

		Status = VxlConvertLogEntryToText(
			CacheEntry->LogEntry,
			(PWSTR) GlobalLock(CopiedText),
			&TextLength,
			(MenuSelection == M_COPYLONG));

		GlobalUnlock(CopiedText);
		
		if (VXL_FAILED(Status)) {
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
}