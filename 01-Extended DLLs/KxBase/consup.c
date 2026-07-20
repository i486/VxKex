///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     consup.c
//
// Abstract:
//
//     Console support functions.
//
// Author:
//
//     vxiiduu (05-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu              05-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

STATIC VOID VTClampCursorPosition(
	IN OUT	PCONSOLE_SCREEN_BUFFER_INFO	Sbi)
{
	Sbi->dwCursorPosition.X = max(Sbi->dwCursorPosition.X, 0);
	Sbi->dwCursorPosition.X = min(Sbi->dwCursorPosition.X, Sbi->dwSize.X - 1);
	Sbi->dwCursorPosition.Y = max(Sbi->dwCursorPosition.Y, Sbi->srWindow.Top);
	Sbi->dwCursorPosition.Y = min(Sbi->dwCursorPosition.Y, Sbi->srWindow.Bottom);
}

//
// Get cursor position with VT semantics.
// The top left of the viewport is (1,1) and cursor position returned is relative
// to viewport, not relative to screen buffer. Horizontal viewport size is ignored.
//
BOOLEAN VTGetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	OUT	PCOORD	Position)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Position->X = Sbi.dwCursorPosition.X + 1;
	Position->Y = Sbi.dwCursorPosition.Y - Sbi.srWindow.Top + 1;

	return Success;
}

//
// Set the cursor position with VT semantics.
// This function will clamp the cursor coordinates to the viewport dimensions,
// but only vertically.
//
BOOLEAN VTSetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	AbsolutePosition)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Sbi.dwCursorPosition.X = AbsolutePosition.X - 1;
	Sbi.dwCursorPosition.Y = AbsolutePosition.Y + Sbi.srWindow.Top - 1;
	VTClampCursorPosition(&Sbi);

	return SetConsoleCursorPosition(ConsoleHandle, Sbi.dwCursorPosition);
}

//
// Same as VTSetCursorPosition but leaves vertical position unchanged.
//
BOOLEAN VTSetCursorHorizontalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	HorizontalPosition)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Sbi.dwCursorPosition.X = HorizontalPosition - 1;
	VTClampCursorPosition(&Sbi);

	return SetConsoleCursorPosition(ConsoleHandle, Sbi.dwCursorPosition);
}

//
// Same as VTSetCursorPosition but leaves horizontal position unchanged.
//
BOOLEAN VTSetCursorVerticalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	VerticalPosition)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Sbi.dwCursorPosition.Y = VerticalPosition + Sbi.srWindow.Top - 1;
	VTClampCursorPosition(&Sbi);

	return SetConsoleCursorPosition(ConsoleHandle, Sbi.dwCursorPosition);
}

//
// Move the cursor with VT semantics.
//
BOOLEAN VTMoveCursorRelative(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	RelativeMovement)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Sbi.dwCursorPosition.X += RelativeMovement.X;
	Sbi.dwCursorPosition.Y += RelativeMovement.Y;
	VTClampCursorPosition(&Sbi);

	return SetConsoleCursorPosition(ConsoleHandle, Sbi.dwCursorPosition);
}

//
// Move the cursor vertically and sets horizontal position to 0.
//
BOOLEAN VTMoveCursorVerticalRelativeWithHorizontalReset(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	RelativeMovement)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	Sbi.dwCursorPosition.X = 0;
	Sbi.dwCursorPosition.Y += RelativeMovement;
	VTClampCursorPosition(&Sbi);

	return SetConsoleCursorPosition(ConsoleHandle, Sbi.dwCursorPosition);
}

BOOLEAN GetConsoleTextAttribute(
	IN	HANDLE	ConsoleHandle,
	OUT	PWORD	Attribute)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &ScreenBufferInfo);
	if (!Success) {
		return FALSE;
	}

	*Attribute = ScreenBufferInfo.wAttributes;
	return Success;
}

BOOLEAN InjectStringToConsoleInput(
	IN	HANDLE	ConsoleHandle,
	IN	PCWSTR	String)
{
	BOOLEAN Success;
	PINPUT_RECORD InputRecords;
	ULONG NumberOfInputRecords;
	ULONG Index;
	ULONG InputRecordsWritten;

	NumberOfInputRecords = (ULONG) wcslen(String);
	InputRecords = StackAlloc(INPUT_RECORD, NumberOfInputRecords);

	for (Index = 0; Index < NumberOfInputRecords; ++Index) {
		InputRecords[Index].EventType = KEY_EVENT;
		InputRecords[Index].Event.KeyEvent.bKeyDown				= TRUE;
		InputRecords[Index].Event.KeyEvent.wRepeatCount			= 1;
		InputRecords[Index].Event.KeyEvent.wVirtualKeyCode		= 0;
		InputRecords[Index].Event.KeyEvent.wVirtualScanCode		= 0;
		InputRecords[Index].Event.KeyEvent.uChar.UnicodeChar	= String[Index];
		InputRecords[Index].Event.KeyEvent.dwControlKeyState	= 0;
	}

	Success = WriteConsoleInput(
		ConsoleHandle,
		InputRecords,
		NumberOfInputRecords,
		&InputRecordsWritten);

	ASSERT (Success);
	ASSERT (InputRecordsWritten == NumberOfInputRecords);

	return Success;
}

BOOL WriteConsoleAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWritten OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode)
{
	if (Unicode) {
		return WriteConsoleW(
			ConsoleHandle,
			Buffer,
			CchToWrite,
			CchWritten,
			Reserved);
	} else {
		return WriteConsoleA(
			ConsoleHandle,
			Buffer,
			CchToWrite,
			CchWritten,
			Reserved);
	}
}

BOOLEAN IsConsoleOutputHandle(
	IN	HANDLE	Handle)
{
	CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;

	//
	// GetConsoleScreenBufferInfo only succeeds when called on output handles.
	//

	return IsConsoleHandle(Handle) && GetConsoleScreenBufferInfo(Handle, &ScreenBufferInfo);
}

BOOLEAN IsConsoleInputHandle(
	IN	HANDLE	Handle)
{
	return IsConsoleHandle(Handle) && !IsConsoleOutputHandle(Handle);
}