#include <Windows.h>
#include <CommCtrl.h>
#include <KexComm.h>

VOID SetWindowIcon(
	IN	HWND	Window,
	IN	USHORT	IconId)
{
	HMODULE CurrentModule;
	HICON Icon;

	CurrentModule = GetModuleHandle(NULL);

	Icon = (HICON) LoadImage(
		CurrentModule,
		MAKEINTRESOURCE(IconId),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		0);
	SendMessage(Window, WM_SETICON, ICON_SMALL, (LPARAM) Icon);

	Icon = (HICON) LoadImage(
		CurrentModule,
		MAKEINTRESOURCE(IconId),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXICON),
		GetSystemMetrics(SM_CYICON),
		0);
	SendMessage(Window, WM_SETICON, ICON_BIG, (LPARAM) Icon);
}

// Make Window centered on ParentWindow, or centered on the screen if
// ParentWindow is NULL.
// It's meant to put small dialogs in the middle of their parent windows.
VOID CenterWindow(
	IN	HWND	Window,
	IN	HWND	ParentWindow OPTIONAL)
{
	RECT Rect;
	RECT ParentRect;
	RECT TemporaryRect;

	if (!ParentWindow) {
		ParentWindow = GetDesktopWindow();
	}

	GetWindowRect(ParentWindow, &ParentRect);
	GetWindowRect(Window, &Rect);
	CopyRect(&TemporaryRect, &ParentRect);

	OffsetRect(&Rect, -Rect.left, -Rect.top);
	OffsetRect(&TemporaryRect, -TemporaryRect.left, -TemporaryRect.top);
	OffsetRect(&TemporaryRect, -Rect.right, -Rect.bottom);

	SetWindowPos(
		Window,
		HWND_TOP,
		ParentRect.left + (TemporaryRect.right / 2),
		ParentRect.top + (TemporaryRect.bottom / 2),
		0, 0,
		SWP_NOSIZE);
}

ULONG ContextMenu(
	IN	HWND	Window,
	IN	USHORT	MenuId,
	IN	PPOINT	ClickPoint)
{
	BOOLEAN MenuDropRightAligned;
	HMENU Menu;
	ULONG Selection;
	LONG MenuX;
	LONG MenuY;

	if (ClickPoint->x >= 0 && ClickPoint->y >= 0) {
		MenuX = ClickPoint->x;
		MenuY = ClickPoint->y;
	} else { // can happen if user uses menu key, or shift+f10
		RECT WindowRect;

		GetWindowRect(Window, &WindowRect);
		MenuX = WindowRect.left;
		MenuY = WindowRect.top;
	}

	MenuDropRightAligned = GetSystemMetrics(SM_MENUDROPALIGNMENT);
	Menu = LoadMenu(NULL, MAKEINTRESOURCE(MenuId));

	Selection = TrackPopupMenu(
		GetSubMenu(Menu, 0),
		TPM_NONOTIFY | TPM_RETURNCMD | (MenuDropRightAligned ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION
																: TPM_LEFTALIGN | TPM_HORPOSANIMATION),
		MenuX, MenuY,
		0,
		Window,
		NULL);

	DestroyMenu(Menu);
	return Selection;
}

VOID StatusBar_SetTextF(
	IN	HWND	Window,
	IN	INT		Index,
	IN	PCWSTR	Format,
	IN	...)
{
	HRESULT Result;
	SIZE_T NumberOfCharacters;
	PWSTR Text;
	va_list ArgList;

	va_start(ArgList, Format);

	NumberOfCharacters = vscwprintf(Format, ArgList) + 1;
	Text = (PWSTR) StackAlloc(NumberOfCharacters * sizeof(WCHAR));
	Result = StringCchVPrintf(Text, NumberOfCharacters, Format, ArgList);

	if (SUCCEEDED(Result)) {
		StatusBar_SetText(Window, Index, Text);
	}
}

VOID ListView_SetCheckedStateAll(
	IN	HWND	Window,
	IN	BOOLEAN	Checked)
{
	ULONG Index;
	ULONG NumberOfComponents;

	NumberOfComponents = ListView_GetItemCount(Window);

	for (Index = 0; Index < NumberOfComponents; Index++) {
		ListView_SetCheckState(Window, Index, Checked);
	}
}

BOOLEAN SetWindowTextF(
	IN	HWND	Window,
	IN	PCWSTR	Format,
	IN	...)
{
	BOOLEAN Success;
	HRESULT Result;
	SIZE_T NumberOfCharacters;
	PWSTR Text;
	va_list ArgList;

	va_start(ArgList, Format);
	
	NumberOfCharacters = vscwprintf(Format, ArgList) + 1;
	Text = (PWSTR) StackAlloc(NumberOfCharacters * sizeof(WCHAR));
	
	Result = StringCchVPrintf(Text, NumberOfCharacters, Format, ArgList);
	if (FAILED(Result)) {
		return FALSE;
	}

	Success = SetWindowText(Window, Text);

	va_end(ArgList);
	return Success;
}

BOOLEAN SetDlgItemTextF(
	IN	HWND	Window,
	IN	INT		ItemId,
	IN	PCWSTR	Format,
	IN	...)
{
	BOOLEAN Success;
	HRESULT Result;
	SIZE_T NumberOfCharacters;
	PWSTR Text;
	va_list ArgList;

	va_start(ArgList, Format);

	NumberOfCharacters = vscwprintf(Format, ArgList) + 1;
	Text = (PWSTR) StackAlloc(NumberOfCharacters * sizeof(WCHAR));

	Result = StringCchVPrintf(Text, NumberOfCharacters, Format, ArgList);
	if (FAILED(Result)) {
		return FALSE;
	}

	Success = SetDlgItemText(Window, ItemId, Text);

	va_end(ArgList);
	return Success;
}

HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPWSTR	lpszText)
{
	TOOLINFO ToolInfo;
	HWND hWndTool;
	STATIC HWND hWndTip = NULL;

	if (!iToolID || !hDlg || !lpszText) {
		return NULL;
	}

	// Get the window of the tool.
	hWndTool = GetDlgItem(hDlg, iToolID);

	if (!hWndTool) {
		return NULL;
	}

	if (!hWndTip) {
		// Create the tooltip.
		hWndTip = CreateWindowEx(
			0, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			hDlg, NULL, 
			NULL, NULL);

		if (!hWndTip) {
			return NULL;
		}

		SendMessage(hWndTip, TTM_SETMAXTIPWIDTH, 0, (LPARAM) 300);
	}

	// Associate the tooltip with the tool.
	ZeroMemory(&ToolInfo, sizeof(ToolInfo));
	ToolInfo.cbSize = sizeof(ToolInfo);
	ToolInfo.hwnd = hDlg;
	ToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ToolInfo.uId = (UINT_PTR) hWndTool;
	ToolInfo.lpszText = lpszText;
	SendMessage(hWndTip, TTM_ADDTOOL, 0, (LPARAM) &ToolInfo);

	return hWndTip;
}