///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ctlsx.c
//
// Abstract:
//
//     Convenience functions for interacting with Windows controls.
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
//     vxiiduu               05-Feb-2024  Add Button_SetShield()
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>

KEXGDECLSPEC EXTERN_C VOID KEXGAPI StatusBar_SetTextF(
	IN	HWND	Window,
	IN	INT		Index,
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
		return;
	}

	Buffer = StackAlloc(WCHAR, BufferCch);
	Result = StringCchVPrintf(Buffer, BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return;
	}

	StatusBar_SetText(Window, Index, Buffer);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI ListView_SetCheckedStateAll(
	IN	HWND	Window,
	IN	BOOLEAN	Checked)
{
	ULONG Index;
	ULONG NumberOfComponents;

	ASSERT (Window != NULL);

	NumberOfComponents = ListView_GetItemCount(Window);

	for (Index = 0; Index < NumberOfComponents; Index++) {
		ListView_SetCheckState(Window, Index, Checked);
	}
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI Button_SetShield(
	IN	HWND	Window,
	IN	BOOLEAN	HasShield)
{
	ASSERT (IsWindow(Window));
	SendMessage(Window, BCM_SETSHIELD, 0, HasShield);
}