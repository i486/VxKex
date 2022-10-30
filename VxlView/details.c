#include "vxlview.h"
#include "resource.h"

VOID InitializeDetailsWindow(
	VOID)
{
	HWND EditControlWindow;
	HDC EditControlDeviceContext;
	HFONT EditControlFont;
	UINT TabStops;

	CreateDialog(NULL, MAKEINTRESOURCE(IDD_DETAILS), MainWindow, DetailsWndProc);
	
	//
	// Make the edit control use a monospace font.
	//
	EditControlWindow = GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT);
	EditControlDeviceContext = GetDC(EditControlWindow);
	EditControlFont = CreateFont(
		-MulDiv(8, GetDeviceCaps(EditControlDeviceContext, LOGPIXELSY), 64),
		0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		L"Lucida Console");

	TabStops = 16; // 4-space tabs
	SendMessage(EditControlWindow, WM_SETFONT, (WPARAM) EditControlFont, MAKELPARAM(TRUE, 0));
	Edit_SetTabStops(EditControlWindow, 1, &TabStops);
	Edit_SetReadOnly(EditControlWindow, TRUE);
	
	ResetDetailsWindow();
}

VOID ResetDetailsWindow(
	VOID)
{
	SetDlgItemText(DetailsWindow, IDC_DETAILSSEVERITYTEXT, L"");
	SetDlgItemText(DetailsWindow, IDC_DETAILSDATETIMETEXT, L"");
	SetDlgItemText(DetailsWindow, IDC_DETAILSSOURCETEXT, L"");
	SetDlgItemText(DetailsWindow, IDC_DETAILSMESSAGETEXT, L"(No log entry selected.)");
}

VOID ResizeDetailsWindow(
	VOID)
{
	RECT MainWindowClientRect;
	RECT FilterWindowRect;
	RECT DetailsWindowClientRect;

	GetClientRect(DetailsWindow, &DetailsWindowClientRect);
	GetClientRect(MainWindow, &MainWindowClientRect);
	GetWindowRect(FilterWindow, &FilterWindowRect);
	MapWindowPoints(HWND_DESKTOP, MainWindow, (PPOINT) &FilterWindowRect, 2);

	SetWindowPos(
		DetailsWindow,
		NULL,
		FilterWindowRect.right,
		FilterWindowRect.top,
		MainWindowClientRect.right - (FilterWindowRect.right + 10) - 5,
		FilterWindowRect.bottom - FilterWindowRect.top,
		SWP_NOZORDER);
}

VOID PopulateDetailsWindow(
	IN	ULONG	EntryIndex)
{
	WCHAR DateFormat[64];
	WCHAR TimeFormat[32];
	PLOGENTRYCACHEENTRY CacheEntry;
	HWND DetailsMessageTextWindow;

	CacheEntry = GetLogEntry(EntryIndex);
	if (!CacheEntry) {
		return;
	}

	GetDateFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		DATE_AUTOLAYOUT | DATE_LONGDATE,
		&CacheEntry->LogEntry->Time,
		NULL,
		DateFormat,
		ARRAYSIZE(DateFormat),
		NULL);
	
	GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		0,
		&CacheEntry->LogEntry->Time,
		NULL,
		TimeFormat,
		ARRAYSIZE(TimeFormat));

	SetDlgItemTextF(DetailsWindow, IDC_DETAILSSEVERITYTEXT, L"%s (%s.)",
					VxlSeverityLookup(CacheEntry->LogEntry->Severity, FALSE),
					VxlSeverityLookup(CacheEntry->LogEntry->Severity, TRUE));

	SetDlgItemTextF(DetailsWindow, IDC_DETAILSDATETIMETEXT, L"%s at %s", DateFormat, TimeFormat);
	
	SetDlgItemTextF(DetailsWindow, IDC_DETAILSSOURCETEXT, L"%s (%s, line %lu, in function %s)",
					CacheEntry->LogEntry->SourceComponent,
					CacheEntry->LogEntry->SourceFile,
					CacheEntry->LogEntry->SourceLine,
					CacheEntry->LogEntry->SourceFunction);

	DetailsMessageTextWindow = GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT);
	SetWindowText(DetailsMessageTextWindow, CacheEntry->LogEntry->TextHeader);

	if (CacheEntry->LogEntry->Text[0] != '\0') {
		// append the rest of the log entry text
		Edit_SetSel(DetailsMessageTextWindow, INT_MAX, INT_MAX);
		Edit_ReplaceSel(DetailsMessageTextWindow, L"\r\n");
		Edit_SetSel(DetailsMessageTextWindow, INT_MAX, INT_MAX);
		Edit_ReplaceSel(DetailsMessageTextWindow, CacheEntry->LogEntry->Text);
	}
}

INT_PTR CALLBACK DetailsWndProc(
	IN	HWND	_DetailsWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		UNCONST (HWND) DetailsWindow = _DetailsWindow;
	} else if (Message == WM_CTLCOLORSTATIC) {
		HWND Window;

		Window = (HWND) LParam;

		if (Window == GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT)) {
			return (INT_PTR) GetStockObject(WHITE_BRUSH);
		} else {
			return FALSE;
		}
	} else if (Message == WM_SIZE) {
		ULONG Index;
		LONG DialogUnitHorizontal;
		USHORT NewWidth;
		USHORT NewHeight;

		DialogUnitHorizontal = LOWORD(GetDialogBaseUnits());
		NewWidth = LOWORD(LParam);
		NewHeight = HIWORD(LParam);

		// resize the border to match the window size
		SetWindowPos(
			GetDlgItem(DetailsWindow, IDC_GBDETAILSBORDER),
			HWND_TOP,
			0, 0,
			NewWidth, NewHeight,
			0);

		// resize the edit control
		SetWindowPos(
			GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT),
			HWND_TOP,
			10 * DialogUnitHorizontal, 75,
			NewWidth - (10 * DialogUnitHorizontal) - 10, 210,
			0);

		// resize the severity, date/time and source text boxes
		for (Index = IDC_DETAILSSEVERITYTEXT; Index <= IDC_DETAILSSOURCETEXT; Index++) {
			HWND Window;
			RECT ClientRect;

			Window = GetDlgItem(DetailsWindow, Index);
			GetClientRect(Window, &ClientRect);

			ClientRect.right = NewWidth - 90;

			SetWindowPos(
				Window,
				HWND_TOP,
				0, 0,
				ClientRect.right, ClientRect.bottom,
				SWP_NOMOVE);
		}
	} else {
		return FALSE;
	}

	return TRUE;
}