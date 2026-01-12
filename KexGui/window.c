///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     window.c
//
// Abstract:
//
//     Convenience functions for dealing with windows.
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

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI SetWindowTextF(
	IN	HWND	Window,
	IN	PCWSTR	Format,
	IN	...)
{
	HRESULT Result;
	SIZE_T BufferCch;
	PWSTR Buffer;
	ARGLIST ArgList;

	ASSERT (Window != NULL);
	ASSERT (Format != NULL);

	va_start(ArgList, Format);

	Result = StringCchVPrintfBufferLength(&BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return FALSE;
	}
	
	Buffer = StackAlloc(WCHAR, BufferCch);	
	Result = StringCchVPrintf(Buffer, BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return FALSE;
	}

	return SetWindowText(Window, Buffer);
}

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI SetDlgItemTextF(
	IN	HWND	Window,
	IN	USHORT	ItemId,
	IN	PCWSTR	Format,
	IN	...)
{
	HRESULT Result;
	SIZE_T BufferCch;
	PWSTR Buffer;
	ARGLIST ArgList;

	ASSERT (Window != NULL);
	ASSERT (Format != NULL);

	va_start(ArgList, Format);

	Result = StringCchVPrintfBufferLength(&BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return FALSE;
	}
	
	Buffer = StackAlloc(WCHAR, BufferCch);	
	Result = StringCchVPrintf(Buffer, BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return FALSE;
	}

	return SetDlgItemText(Window, ItemId, Buffer);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI SetWindowIcon(
	IN	HWND	Window,
	IN	USHORT	IconId)
{
	HMODULE CurrentModule;
	HICON Icon;

	ASSERT (Window != NULL);

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

//
// Make Window centered on ParentWindow, or centered on the screen if
// ParentWindow is NULL.
//
KEXGDECLSPEC EXTERN_C VOID KEXGAPI CenterWindow(
	IN	HWND	Window,
	IN	HWND	ParentWindow OPTIONAL)
{
	RECT Rect;
	RECT ParentRect;
	RECT TemporaryRect;

	ASSERT (Window != NULL);

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

KEXGDECLSPEC EXTERN_C VOID KEXGAPI BanishWindow(
	IN	HWND	Window)
{
	ASSERT (Window != NULL);

	EnableWindow(Window, FALSE);
	ShowWindow(Window, SW_HIDE);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI SummonWindow(
	IN	HWND	Window)
{
	ASSERT (Window != NULL);

	ShowWindow(Window, SW_NORMAL);
	EnableWindow(Window, TRUE);
}