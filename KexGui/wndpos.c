///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     wndpos.c
//
// Abstract:
//
//     Functions for saving and restoring window position using the registry.
//     This code used to be in VxlView but moved to KexGui since KexCfg also
//     needed to use it.
//
// Author:
//
//     vxiiduu (19-May-2026)
//
// Environment:
//
//     Win32 graphical.
//
// Revision History:
//
//     vxiiduu               19-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <KexW32ML.h>

KEXGDECLSPEC VOID KEXGAPI SaveWindowPlacement(
	IN	HWND	Window,
	IN	PCWSTR	RegKeyName)
{
	WINDOWPLACEMENT WindowPlacement;

	GetWindowPlacement(Window, &WindowPlacement);

	RegWriteI32(HKEY_CURRENT_USER, RegKeyName, L"WndLeft", WindowPlacement.rcNormalPosition.left);
	RegWriteI32(HKEY_CURRENT_USER, RegKeyName, L"WndTop", WindowPlacement.rcNormalPosition.top);
	RegWriteI32(HKEY_CURRENT_USER, RegKeyName, L"WndRight", WindowPlacement.rcNormalPosition.right);
	RegWriteI32(HKEY_CURRENT_USER, RegKeyName, L"WndBottom", WindowPlacement.rcNormalPosition.bottom);
}

KEXGDECLSPEC VOID KEXGAPI RestoreWindowPlacement(
	IN	HWND	Window,
	IN	PCWSTR	RegKeyName)
{
	ULONG Error;
	WINDOWPLACEMENT WindowPlacement;

	// required for first startup
	SetWindowPos(Window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	GetWindowPlacement(Window, &WindowPlacement);

	Error = 0;
	Error += RegReadI32(HKEY_CURRENT_USER, RegKeyName, L"WndLeft", (PULONG) &WindowPlacement.rcNormalPosition.left);
	Error += RegReadI32(HKEY_CURRENT_USER, RegKeyName, L"WndTop", (PULONG) &WindowPlacement.rcNormalPosition.top);
	Error += RegReadI32(HKEY_CURRENT_USER, RegKeyName, L"WndRight", (PULONG) &WindowPlacement.rcNormalPosition.right);
	Error += RegReadI32(HKEY_CURRENT_USER, RegKeyName, L"WndBottom", (PULONG) &WindowPlacement.rcNormalPosition.bottom);

	SetWindowPlacement(Window, &WindowPlacement);

	if (Error) {
		// typically occurs on first startup
		CenterWindow(Window, HWND_DESKTOP);
	}
}