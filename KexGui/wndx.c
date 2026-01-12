///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     wndx.c
//
// Abstract:
//
//     Various miscellaneous convenience functions involving the creation
//     of tooltips, menus, etc.
//
// Author:
//
//     vxiiduu (01-Oct-2022)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               01-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>

//
// Create a context menu at the specified screen coordinates.
//
// Returns the menu item identifier (resource ID) of the menu item which the
// user clicked. If the user got rid of the menu without clicking any items,
// the return value is 0.
//
// The DefaultMenuItem specifies a menu item ID which is bolded. Specify -1 if
// you do not want a bolded menu item.
//
KEXGDECLSPEC EXTERN_C ULONG KEXGAPI ContextMenuEx(
	IN	HWND	Window,
	IN	USHORT	MenuId,
	IN	PPOINT	ClickPoint,
	IN	INT		DefaultMenuItem)
{
	BOOLEAN MenuDropRightAligned;
	HMENU Menu;
	HMENU SubMenu;
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
	SubMenu = GetSubMenu(Menu, 0);
	SetMenuDefaultItem(SubMenu, DefaultMenuItem, FALSE);

	Selection = TrackPopupMenu(
		SubMenu,
		TPM_NONOTIFY | TPM_RETURNCMD | (MenuDropRightAligned ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION
																: TPM_LEFTALIGN | TPM_HORPOSANIMATION),
		MenuX, MenuY,
		0,
		Window,
		NULL);

	DestroyMenu(Menu);
	return Selection;
}

KEXGDECLSPEC EXTERN_C ULONG KEXGAPI ContextMenu(
	IN	HWND	Window,
	IN	USHORT	MenuId,
	IN	PPOINT	ClickPoint)
{
	return ContextMenuEx(Window, MenuId, ClickPoint, -1);
}

KEXGDECLSPEC EXTERN_C HWND KEXGAPI ToolTip(
	IN	HWND	DialogWindow,
	IN	INT		ToolId,
	IN	PWSTR	Format,
	IN	...)
{
	TOOLINFO ToolInfo;
	HWND ToolWindow;
	STATIC HWND ToolTipWindow = NULL;

	HRESULT Result;
	SIZE_T BufferCch;
	PWSTR Buffer;
	ARGLIST ArgList;

	ASSERT (DialogWindow != NULL);
	ASSERT (Format != NULL);

	ToolWindow = GetDlgItem(DialogWindow, ToolId);
	ASSERT (ToolWindow != NULL);

	if (!ToolWindow) {
		return NULL;
	}

	// Create the tooltip if:
	// 1. we haven't been called before
	// 2. our old tooltip window was destroyed because
	//    the parent dialog window was closed
	if (!ToolTipWindow || !IsWindow(ToolTipWindow)) {
		ToolTipWindow = CreateWindowEx(
			0, TOOLTIPS_CLASS, NULL,
			WS_POPUP | TTS_ALWAYSTIP,
			CW_USEDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, CW_USEDEFAULT,
			DialogWindow, NULL, 
			NULL, NULL);

		if (!ToolTipWindow) {
			return NULL;
		}

		SendMessage(ToolTipWindow, TTM_SETMAXTIPWIDTH, 0, (LPARAM) 300);
		SendMessage(ToolTipWindow, TTM_SETDELAYTIME, TTDT_AUTOPOP, 10000);
	}

	//
	// Format the message we are going to use for this tool-tip.
	//
	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLength(&BufferCch, Format, ArgList);
	
	ASSERT (SUCCEEDED(Result));
	if (SUCCEEDED(Result)) {
		Buffer = StackAlloc(WCHAR, BufferCch);
		Result = StringCchVPrintf(Buffer, BufferCch, Format, ArgList);

		if (FAILED(Result)) {
			// hopefully this is ok, better than no tooltip.
			Buffer = Format;
		}
	} else {
		Buffer = Format;
	}

	//
	// Associate the tooltip with the tool.
	//
	ZeroMemory(&ToolInfo, sizeof(ToolInfo));
	ToolInfo.cbSize = TTTOOLINFOW_V1_SIZE; // Win2k compat
	ToolInfo.hwnd = DialogWindow;
	ToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ToolInfo.uId = (UINT_PTR) ToolWindow;
	ToolInfo.lpszText = Buffer;
	SendMessage(ToolTipWindow, TTM_ADDTOOL, 0, (LPARAM) &ToolInfo);

	return ToolTipWindow;
}