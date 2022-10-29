///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexGui.h
//
// Abstract:
//
//     Convenience functions and macros for GUI applications.
//
// Author:
//
//     vxiiduu (01-Oct-2022)
//
// Environment:
//
//     This header is automatically included in all EXE targets.
//     You may include the header in DLL projects manually if required; however,
//     do not use it in any DLLs to be loaded in a kex process.
//
// Revision History:
//
//     vxiiduu               01-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef KEX_ENV_NATIVE
#  error This header file cannot be used in a native mode project.
#endif

//
// Enable visual styles.
//
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' \
						version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' \
						language='*'\"")

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

#include <KexTypes.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <Uxtheme.h>

#ifndef KEXGDECLSPEC
#  define KEXGDECLSPEC DECLSPEC_IMPORT
#  pragma comment(lib, "KexGui.lib")
#endif

#define KEXGAPI CDECL

#define StatusBar_SetParts(StatusBarWindow, NumberOfParts, SizeArray) (SendMessage((StatusBarWindow), SB_SETPARTS, (NumberOfParts), (LPARAM) (SizeArray)))
#define StatusBar_SetText(StatusBarWindow, Index, Text) (SendMessage((StatusBarWindow), SB_SETTEXT, (Index), (LPARAM) (Text)))

//
// ctlsx.c
//

KEXGDECLSPEC EXTERN_C VOID KEXGAPI StatusBar_SetTextF(
	IN	HWND	Window,
	IN	INT		Index,
	IN	PCWSTR	Format,
	IN	...);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI ListView_SetCheckedStateAll(
	IN	HWND	Window,
	IN	BOOLEAN	Checked);

//
// kexgui.c
//

// This name is displayed in the captions of message boxes.
KEXGDECLSPEC extern PCWSTR KexgApplicationFriendlyName;

// This can be NULL if the application has no main window.
KEXGDECLSPEC extern HWND KexgApplicationMainWindow;

//
// msgbox.c
//

KEXGDECLSPEC EXTERN_C INT KEXGAPI MessageBoxV(
	IN	ULONG	Buttons OPTIONAL,
	IN	PCWSTR	Icon OPTIONAL,
	IN	PCWSTR	WindowTitle OPTIONAL,
	IN	PCWSTR	MessageTitle OPTIONAL,
	IN	PCWSTR	Format,
	IN	va_list	ArgList);

KEXGDECLSPEC EXTERN_C INT KEXGAPI MessageBoxF(
	IN	ULONG	Buttons OPTIONAL,
	IN	PCWSTR	Icon OPTIONAL,
	IN	PCWSTR	WindowTitle OPTIONAL,
	IN	PCWSTR	MessageTitle OPTIONAL,
	IN	PCWSTR	Format,
	IN	...);

KEXGDECLSPEC EXTERN_C NORETURN VOID KEXGAPI CriticalErrorBoxF(
	IN	PCWSTR	Format,
	IN	...);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI ErrorBoxF(
	IN	PCWSTR Format,
	IN	...);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI WarningBoxF(
	IN	PCWSTR Format,
	IN	...);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI InfoBoxF(
	IN	PCWSTR Format,
	IN	...);

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI ReportAssertionFailure(
	IN	PCWSTR	SourceFile,
	IN	ULONG	SourceLine,
	IN	PCWSTR	SourceFunction,
	IN	PCWSTR	AssertCondition);

//
// window.c
//

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI SetWindowTextF(
	IN	HWND	Window,
	IN	PCWSTR	Format,
	IN	...);

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI SetDlgItemTextF(
	IN	HWND	Window,
	IN	USHORT	ItemId,
	IN	PCWSTR	Format,
	IN	...);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI SetWindowIcon(
	IN	HWND	Window,
	IN	USHORT	IconId);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI CenterWindow(
	IN	HWND	Window,
	IN	HWND	ParentWindow OPTIONAL);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI BanishWindow(
	IN	HWND	Window);

KEXGDECLSPEC EXTERN_C VOID KEXGAPI SummonWindow(
	IN	HWND	Window);

//
// wndx.c
//
KEXGDECLSPEC EXTERN_C ULONG KEXGAPI ContextMenu(
	IN	HWND	Window,
	IN	USHORT	MenuId,
	IN	PPOINT	ClickPoint);

KEXGDECLSPEC EXTERN_C HWND KEXGAPI ToolTip(
	IN	HWND	DialogWindow,
	IN	INT		ToolId,
	IN	PWSTR	Format,
	IN	...);