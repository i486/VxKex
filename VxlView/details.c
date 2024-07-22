#include "vxlview.h"
#include "resource.h"
#include "backendp.h"

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
		-MulDiv(8, GetDeviceCaps(EditControlDeviceContext, LOGPIXELSY), 72),
		0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		L"Lucida Console");

	ReleaseDC(EditControlWindow, EditControlDeviceContext);

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
	MapWindowRect(HWND_DESKTOP, MainWindow, &FilterWindowRect);

	SetWindowPos(
		DetailsWindow,
		NULL,
		FilterWindowRect.right,
		FilterWindowRect.top,
		MainWindowClientRect.right - FilterWindowRect.right - DpiScaleX(5),
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
		&CacheEntry->LogEntry.Time,
		NULL,
		DateFormat,
		ARRAYSIZE(DateFormat),
		NULL);
	
	GetTimeFormatEx(
		LOCALE_NAME_USER_DEFAULT,
		0,
		&CacheEntry->LogEntry.Time,
		NULL,
		TimeFormat,
		ARRAYSIZE(TimeFormat));

	SetDlgItemTextF(DetailsWindow, IDC_DETAILSSEVERITYTEXT, L"%s (%s.)",
					VxlSeverityToText(CacheEntry->LogEntry.Severity, FALSE),
					VxlSeverityToText(CacheEntry->LogEntry.Severity, TRUE));

	SetDlgItemTextF(DetailsWindow, IDC_DETAILSDATETIMETEXT, L"%s at %s", DateFormat, TimeFormat);
	
	SetDlgItemTextF(DetailsWindow, IDC_DETAILSSOURCETEXT, L"[%04lx:%04lx], %s (%s, line %lu, in function %s)",
					(ULONG) CacheEntry->LogEntry.ClientId.UniqueProcess,
					(ULONG) CacheEntry->LogEntry.ClientId.UniqueThread,
					State->LogHandle->Header->SourceComponents[CacheEntry->LogEntry.SourceComponentIndex],
					State->LogHandle->Header->SourceFiles[CacheEntry->LogEntry.SourceFileIndex],
					CacheEntry->LogEntry.SourceLine,
					State->LogHandle->Header->SourceFunctions[CacheEntry->LogEntry.SourceFunctionIndex]);

	DetailsMessageTextWindow = GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT);
	SetWindowText(DetailsMessageTextWindow, CacheEntry->LogEntry.TextHeader.Buffer);

	if (CacheEntry->LogEntry.Text.Length != 0) {
		// append the rest of the log entry text
		Edit_SetSel(DetailsMessageTextWindow, INT_MAX, INT_MAX);
		Edit_ReplaceSel(DetailsMessageTextWindow, L"\r\n\r\n");
		Edit_SetSel(DetailsMessageTextWindow, INT_MAX, INT_MAX);
		Edit_ReplaceSel(DetailsMessageTextWindow, CacheEntry->LogEntry.Text.Buffer);
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
		HDC DeviceContext;
		HWND Window;

		Window = (HWND) LParam;
		DeviceContext = (HDC) WParam;

		if (Window == GetDlgItem(DetailsWindow, IDC_DETAILSMESSAGETEXT)) {
			SetTextColor(DeviceContext, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor(DeviceContext, GetSysColor(COLOR_WINDOW));
			return (INT_PTR) GetSysColorBrush(COLOR_WINDOW);
		} else {
			return FALSE;
		}
	} else if (Message == WM_SIZE) {
		ULONG Index;
		USHORT NewWidth;
		USHORT NewHeight;

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
			DpiScaleX(80), DpiScaleY(75),
			NewWidth - DpiScaleX(90), NewHeight - DpiScaleY(85),
			0);

		// resize the severity, date/time and source text boxes
		for (Index = IDC_DETAILSSEVERITYTEXT; Index <= IDC_DETAILSSOURCETEXT; Index++) {
			HWND Window;
			RECT ClientRect;

			Window = GetDlgItem(DetailsWindow, Index);
			GetClientRect(Window, &ClientRect);

			ClientRect.right = NewWidth - DpiScaleX(90);

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