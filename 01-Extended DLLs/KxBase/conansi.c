///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     conansi.c
//
// Abstract:
//
//     Contains support code for processing ANSI escape sequences on a
//     Windows console.
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

#define ESC (0x1B)
#define BEL (0x07)
#define VXKEX_CONSOLE_ANSI_OUTPUT	1

#define MAX_CSI_PARAMETER_COUNT		16

//
// Global variables for ANSI escape sequence support.
//

STATIC ULONG g_ConsoleANSISupport = 0; // VXKEX_CONSOLE_*
STATIC COORD g_SavedCursorPosition; // 1-based, viewport-relative.
STATIC WORD g_DefaultTextAttributes;

BOOLEAN BaseIsConsoleAnsiSupportEnabled(
	IN	HANDLE	ConsoleHandle)
{
	return !!g_ConsoleANSISupport;
}

BOOLEAN BaseEnableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle)
{
	BOOLEAN Success;

	ASSERT (IsConsoleHandle(ConsoleHandle));

	Success = VTGetCursorPosition(ConsoleHandle, &g_SavedCursorPosition);
	ASSERT (Success);

	if (!Success) {
		return Success;
	}

	//
	// We need to save the current console attributes instead of using a
	// hardcoded value such as white on black, because apps started from
	// e.g. Powershell will need to take into account the pre-existing white
	// on blue text attributes. Otherwise any attempt to reset text attributes
	// will cause ugliness.
	//
	// Also, the user can change the default console colors, and we want to
	// behave correctly in that situation.
	//

	Success = GetConsoleTextAttribute(ConsoleHandle, &g_DefaultTextAttributes);
	ASSERT (Success);

	// Remove reverse video attribute if it happens to be set.
	// It's very unlikely, but if it were, then SGR 0 would end up enabling
	// reverse video, which is unwanted.
	g_DefaultTextAttributes &= ~COMMON_LVB_REVERSE_VIDEO;

	if (!Success) {
		return Success;
	}

	g_ConsoleANSISupport |= VXKEX_CONSOLE_ANSI_OUTPUT;
	KexLogDebugEvent(L"Enabled escape sequence processing");
	return Success;
}

BOOLEAN BaseDisableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle)
{
	g_ConsoleANSISupport &= ~VXKEX_CONSOLE_ANSI_OUTPUT;
	KexLogDebugEvent(L"Disabled escape sequence processing");
	return TRUE;
}

STATIC ULONG ParseNumber(
	IN	PCWCHAR	Characters,
	IN	ULONG	MaxCch,
	OUT	PULONG	NumberCch OPTIONAL)
{
	ULONG Accumulator;
	ULONG Index;

	Accumulator = 0;
	Index = 0;

	until (!IsDigit(Characters[Index]) || Index >= MaxCch) {
		Accumulator *= 10;
		Accumulator += Characters[Index] - '0';
		++Index;
	}

	if (NumberCch) {
		*NumberCch = Index;
	}

	return Accumulator;
}

STATIC ULONG GetCchToNextEscapeOrEndOfBufferAorW(
	IN	PCVOID	Buffer,
	IN	ULONG	BufferCch,
	IN	BOOLEAN	Unicode)
{
	ULONG Index;

	Index = 0;

	if (Unicode) {
		PCWCHAR BufferW;

		BufferW = (PCWCHAR) Buffer;

		until (BufferCch == 0) {
			if (BufferW[Index] == ESC) {
				return Index;
			}

			--BufferCch;
			++Index;
		}
	} else {
		PCCHAR BufferA;

		BufferA = (PCCHAR) Buffer;

		until (BufferCch == 0) {
			if (BufferA[Index] == ESC) {
				return Index;
			}

			--BufferCch;
			++Index;
		}
	}

	// If we get here it means that there's no escape sequence
	return Index;
}

STATIC BOOLEAN ExecuteCSISetGraphicsRendition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	CsiParameters[MAX_CSI_PARAMETER_COUNT],
	IN	ULONG	ParameterCount)
{
	WORD TextAttribute;
	ULONG ParameterIndex;

	GetConsoleTextAttribute(ConsoleHandle, &TextAttribute);

	// set parameter count to 1 if it is 0.
	// if there are no parameters we treat that the same as one parameter with
	// the value 0. Note that the Parameters array is initialized to all zeros
	// if there are no parameters, which is why this works.
	ParameterCount = max(ParameterCount, 1);

	for (ParameterIndex = 0; ParameterIndex < ParameterCount; ++ParameterIndex) {
		SHORT Parameter;

		Parameter = CsiParameters[ParameterIndex];

		if (TextAttribute & COMMON_LVB_REVERSE_VIDEO) {
			// Swap the handling of foreground and background colors
			if (Parameter >= 30 && Parameter <= 39 ||
				Parameter >= 90 && Parameter <= 97) {

				Parameter += 10;
			} else if (Parameter >= 40 && Parameter <= 49 ||
						Parameter >= 100 && Parameter <= 107) {

				Parameter -= 10;
			}
		}

		if (Parameter >= 30 && Parameter <= 39) {
			// setting non-bright foreground color
			TextAttribute &= ~(FOREGROUND_INTENSITY |
								FOREGROUND_RED |
								FOREGROUND_GREEN |
								FOREGROUND_BLUE);
		} else if (Parameter >= 40 && Parameter <= 49) {
			// setting non-bright background color
			TextAttribute &= ~(BACKGROUND_INTENSITY |
								BACKGROUND_RED |
								BACKGROUND_GREEN |
								BACKGROUND_BLUE);
		} else if (Parameter >= 90 && Parameter <= 97) {
			// setting bright foreground color
			TextAttribute |= FOREGROUND_INTENSITY;
			TextAttribute &= ~(FOREGROUND_RED |
								FOREGROUND_GREEN |
								FOREGROUND_BLUE);
			Parameter -= 60;
		} else if (Parameter >= 100 && Parameter <= 107) {
			// setting bright background color
			TextAttribute |= BACKGROUND_INTENSITY;
			TextAttribute &= ~(BACKGROUND_RED |
								BACKGROUND_GREEN |
								BACKGROUND_BLUE);
			Parameter -= 60;
		}

		switch (Parameter) {
		case 0:		// Reset
			TextAttribute = g_DefaultTextAttributes;
			break;
		case 1:		// Bright
			TextAttribute |= FOREGROUND_INTENSITY;
			break;
		case 7:		// Reverse video (negative)
			// This flag has no effect on the console but we just use it to remember
			// the fact that reverse video is enabled
			TextAttribute |= COMMON_LVB_REVERSE_VIDEO;
			// Swap foreground and background colors
			*(PBYTE) &TextAttribute = _rotl8((BYTE) TextAttribute, 4);
			break;
		case 27:	// Turn off reverse video (positive)
			TextAttribute &= ~COMMON_LVB_REVERSE_VIDEO;
			*(PBYTE) &TextAttribute = _rotl8((BYTE) TextAttribute, 4);
			break;
		case 22:	// No bright
			TextAttribute &= ~FOREGROUND_INTENSITY;
			break;
		case 31:	// Red
			TextAttribute |= FOREGROUND_RED;
			break;
		case 32:	// Green
			TextAttribute |= FOREGROUND_GREEN;
			break;
		case 33:	// Yellow
			TextAttribute |= FOREGROUND_RED | FOREGROUND_GREEN;
			break;
		case 34:	// Blue
			TextAttribute |= FOREGROUND_BLUE;
			break;
		case 35:	// Magenta
			TextAttribute |= FOREGROUND_RED | FOREGROUND_BLUE;
			break;
		case 36:	// Cyan
			TextAttribute |= FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		case 37:	// White
			TextAttribute |= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
			break;
		case 39:	// Foreground reset
			TextAttribute |= g_DefaultTextAttributes & (FOREGROUND_INTENSITY |
														FOREGROUND_RED |
														FOREGROUND_GREEN |
														FOREGROUND_BLUE);
			break;
		case 41:	// Background red
			TextAttribute |= BACKGROUND_RED;
			break;
		case 42:	// Background green
			TextAttribute |= BACKGROUND_GREEN;
			break;
		case 43:	// Background yellow
			TextAttribute |= BACKGROUND_RED | BACKGROUND_GREEN;
			break;
		case 44:	// Background blue
			TextAttribute |= BACKGROUND_BLUE;
			break;
		case 45:	// Background magenta
			TextAttribute |= BACKGROUND_RED | BACKGROUND_BLUE;
			break;
		case 46:	// Background cyan
			TextAttribute |= BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;
		case 47:	// Background white
			TextAttribute |= BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;
			break;
		case 49:	// Background reset
			TextAttribute |= g_DefaultTextAttributes & (BACKGROUND_INTENSITY |
														BACKGROUND_RED |
														BACKGROUND_GREEN |
														BACKGROUND_BLUE);
			break;
		case 38:	// Extended foreground color setting
		case 48:	// Extended background color setting
			// We don't support extended fg/bg color setting.
			// These come with additional parameters which will cause
			// random color changes if we were to interpret them as normal
			// SGR parameters, so we'll just ignore the rest of the SGR escape.
			KexLogDebugEvent(
				L"Extended %s color setting ignored",
				Parameter == 38 ? L"foreground" : L"background");
			KexDebugCheckpoint();
			return FALSE;
		}
	}

	return SetConsoleTextAttribute(ConsoleHandle, TextAttribute);
}

STATIC BOOLEAN ExecuteCSIErase(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	Parameter,
	IN	BOOLEAN	LineOnly)
{
	BOOLEAN Success;
	CONSOLE_SCREEN_BUFFER_INFO Sbi;
	COORD EraseStart;
	COORD EraseEnd;
	ULONG EraseCount;

	Success = GetConsoleScreenBufferInfo(ConsoleHandle, &Sbi);
	if (!Success) {
		return FALSE;
	}

	switch (Parameter) {
	case 0:
		EraseStart = Sbi.dwCursorPosition;
		EraseEnd.X = Sbi.dwSize.X - 1;

		if (LineOnly) {
			// erase from cursor position to end of current line
			EraseEnd.Y = Sbi.dwCursorPosition.Y;
		} else {
			// erase from cursor position to end of display
			EraseEnd.Y = Sbi.srWindow.Bottom;
		}

		break;
	case 1:
		EraseEnd = Sbi.dwCursorPosition;
		EraseStart.X = 0;

		if (LineOnly) {
			// erase from beginning of line to cursor
			EraseStart.Y = Sbi.dwCursorPosition.Y;
		} else {
			// erase from beginning of display to cursor
			EraseStart.Y = Sbi.srWindow.Top;
		}

		break;
	case 2:
		EraseStart.X = 0;
		EraseEnd.X = Sbi.dwSize.X - 1;

		if (LineOnly) {
			// erase whole line
			EraseStart.Y = Sbi.dwCursorPosition.Y;
			EraseEnd.Y = Sbi.dwCursorPosition.Y;
		} else {
			// erase whole screen
			EraseStart.Y = Sbi.srWindow.Top;
			EraseEnd.Y = Sbi.srWindow.Bottom;
		}

		break;
	default:
		KexLogDebugEvent(L"Invalid parameter %hu to CSI 'J' or 'K' escape", Parameter);
		return FALSE;
	}

	EraseCount = (EraseEnd.Y - EraseStart.Y) * Sbi.dwSize.X;
	EraseCount += EraseEnd.X - EraseStart.X + 1;

	Success = FillConsoleOutputCharacter(
		ConsoleHandle,
		' ',
		EraseCount,
		EraseStart,
		&EraseCount);
	
	if (!Success) {
		return Success;
	}

	return FillConsoleOutputAttribute(
		ConsoleHandle,
		Sbi.wAttributes,
		EraseCount,
		EraseStart,
		&EraseCount);
}

STATIC BOOLEAN ExecuteCSI(
	IN	HANDLE	ConsoleHandle,
	IN	WCHAR	Command,
	IN	SHORT	CsiParameters[MAX_CSI_PARAMETER_COUNT],
	IN	ULONG	ParameterCount)
{
	COORD Coord;

	switch (Command) {
	case 'A': // Cursor Up
		Coord.X = 0;
		Coord.Y = -max(1, CsiParameters[0]);
		return VTMoveCursorRelative(ConsoleHandle, Coord);
	case 'B': // Cursor Down
		Coord.X = 0;
		Coord.Y = max(1, CsiParameters[0]);
		return VTMoveCursorRelative(ConsoleHandle, Coord);
	case 'C': // Cursor Forward (Right)
		Coord.X = max(1, CsiParameters[0]);
		Coord.Y = 0;
		return VTMoveCursorRelative(ConsoleHandle, Coord);
	case 'D': // Cursor Backward (Left)
		Coord.X = -max(1, CsiParameters[0]);
		Coord.Y = 0;
		return VTMoveCursorRelative(ConsoleHandle, Coord);
	case 'E': // Cursor Next Line
		return VTMoveCursorVerticalRelativeWithHorizontalReset(
			ConsoleHandle,
			max(1, CsiParameters[0]));
	case 'F': // Cursor Previous Line
		return VTMoveCursorVerticalRelativeWithHorizontalReset(
			ConsoleHandle,
			-max(1, CsiParameters[0]));
	case 'G': // Cursor Horizontal Absolute
		return VTSetCursorHorizontalPosition(ConsoleHandle, CsiParameters[0]);
	case 'd': // Vertical Line Position Absolute
		return VTSetCursorVerticalPosition(ConsoleHandle, CsiParameters[0]);
	case 'H': // Cursor Position
	case 'f': // Horizontal Vertical Position
		Coord.Y = CsiParameters[0];
		Coord.X = CsiParameters[1];
		return VTSetCursorPosition(ConsoleHandle, Coord);
	case 'J': // Erase in Display
		return ExecuteCSIErase(ConsoleHandle, CsiParameters[0], FALSE);
	case 'K': // Erase in Line
		return ExecuteCSIErase(ConsoleHandle, CsiParameters[0], TRUE);
	case 'c': // Device Attributes
		if (CsiParameters[0] == 0) {
			BOOLEAN Success;

			Success = InjectStringToConsoleInput(
				GetStdHandle(STD_INPUT_HANDLE),
				L"\x1b[?1;0c");

			ASSERT (Success);
			return Success;
		} else {
			KexLogDebugEvent(L"Invalid parameter %hu to CSI 'c' escape", CsiParameters[0]);
			KexDebugCheckpoint();
			return FALSE;
		}

		break;
	case 'm': // Set Graphics Rendition (SGR)
		return ExecuteCSISetGraphicsRendition(ConsoleHandle, CsiParameters, ParameterCount);
	case 's': // Save Cursor (ANSI.SYS)
		return VTGetCursorPosition(ConsoleHandle, &g_SavedCursorPosition);
	case 'u': // Restore Cursor (ANSI.SYS)
		return VTSetCursorPosition(ConsoleHandle, g_SavedCursorPosition);
	default:
		KexLogDebugEvent(L"Unrecognized CSI ANSI escape '%c'", Command);
		KexDebugCheckpoint();
		return FALSE;
	}
}

STATIC BOOLEAN ProcessEscapeSequenceAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	BufferCch,
	OUT	PULONG	CchProcessedOut OPTIONAL,
	IN	BOOLEAN	Unicode)
{
	WCHAR EscapeSequence[260];
	ULONG EscapeSequenceCch;
	ULONG CchProcessed;
	ULONG Index;

	ASSERT (BufferCch != 0);

	CchProcessed = 0;
	Index = 0;

	//
	// Fetch the escape sequence into the buffer, and convert to Unicode if it
	// is in ANSI format.
	// The buffer size is the maximum reasonable length of an escape sequence.
	//

	EscapeSequenceCch = min(BufferCch, ARRAYSIZE(EscapeSequence));

	if (EscapeSequenceCch < 2) {
		// Minimum length of an escape sequence is 2 characters.
		// If it's 1 character then we will just ignore it.
		// This means that there was just a stray ESC character at the end of
		// the buffer passed to WriteConsole.
		CchProcessed = EscapeSequenceCch;
		goto Exit;
	}
	
	if (Unicode) {
		RtlCopyMemory(EscapeSequence, Buffer, EscapeSequenceCch * sizeof(WCHAR));
	} else {
		EscapeSequenceCch = MultiByteToWideChar(
			CP_ACP,
			0,
			(PCSTR) Buffer,
			min(BufferCch, ARRAYSIZE(EscapeSequence) - 1),
			EscapeSequence,
			ARRAYSIZE(EscapeSequence));

		ASSERT (EscapeSequenceCch != 0);

		if (EscapeSequenceCch == 0) {
			// We're screwed
			return FALSE;
		}
	}

	//
	// Parse escape sequence and act on it if necessary.
	// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
	//

	ASSERT (EscapeSequence[0] == ESC);
	ASSERT (EscapeSequenceCch >= 2);

	CchProcessed = 2;

	switch (EscapeSequence[1]) {
	case 'M': {
		COORD RelativeMovement;
		// Reverse Index.
		// Move the cursor up by one line and maintain horizontal position.
		RelativeMovement.X = 0;
		RelativeMovement.Y = -1;
		VTMoveCursorRelative(ConsoleHandle, RelativeMovement);
		break;
			  }
	case '7':
		// Save Cursor Position in Memory
		VTGetCursorPosition(ConsoleHandle, &g_SavedCursorPosition);
		break;
	case '8':
		// Restore Cursor Position from Memory
		VTSetCursorPosition(ConsoleHandle, g_SavedCursorPosition);
		break;
	case '[': {
		PWCHAR Csi;
		ULONG CsiLength;
		SHORT CsiParameters[MAX_CSI_PARAMETER_COUNT] = {0};
		WCHAR Command;
		ULONG ParameterCount;

		// CSI

		if (EscapeSequenceCch < 3) {
			KexLogDebugEvent(L"Unterminated CSI ANSI escape");
			goto Exit;
		}

		//
		// Determine length of CSI sequence
		//

		Csi = &EscapeSequence[2];
		CsiLength = 0;

		until (Csi[CsiLength] >= 0x40 && Csi[CsiLength] <= 0x7E) {
			++CsiLength;

			if (CchProcessed + CsiLength >= EscapeSequenceCch) {
				KexLogDebugEvent(L"Unterminated CSI ANSI escape");
				goto Exit;
			}
		}

		// add 1 for the terminating character, which is the command
		++CsiLength;
		CchProcessed += CsiLength;

		// final character determines command
		Command = Csi[CsiLength - 1];

		//
		// Parse CSI sequence parameters
		//

		Index = 0;
		ParameterCount = 0;

		while (CsiLength != 0) {
			ULONG Parameter;
			ULONG NumberCch;

			Parameter = ParseNumber(Csi, CsiLength, &NumberCch);
			Csi += NumberCch;
			CsiLength -= NumberCch;

			if (NumberCch == 0) {
				if (CsiLength > 0 && *Csi == ';') {
					// treat as default 0
					ASSUME (Parameter == 0);
				} else {
					// no more parameters
					break;
				}
			}

			if (CsiLength > 0 && *Csi == ';') {
				// skip past semicolon separator
				++Csi;
				--CsiLength;
			}

			if (ParameterCount >= ARRAYSIZE(CsiParameters)) {
				KexLogDebugEvent(L"Too many numeric parameters in CSI ANSI escape");
				goto Exit;
			}

			if (Parameter > SHRT_MAX) {
				KexLogDebugEvent(L"Malformed number in CSI ANSI escape");
				goto Exit;
			}

			CsiParameters[ParameterCount] = (SHORT) Parameter;
			++ParameterCount;
		}

		ExecuteCSI(ConsoleHandle, Command, CsiParameters, ParameterCount);
		break;
			  }
	case ']': {
		PWCHAR Osc;
		ULONG OscLength;
		ULONG NumberOfTerminatingCharacters;

		// OSC

		//
		// Determine length of OSC sequence.
		// OSC sequences are terminated by BEL or by ST (ESC \)
		//

		Osc = &EscapeSequence[2];
		OscLength = 0;

		until (Osc[OscLength] == BEL || Osc[OscLength] == ESC) {
			++OscLength;

			if (CchProcessed + OscLength >= EscapeSequenceCch) {
				KexLogDebugEvent(L"Unterminated OSC ANSI escape sequence");
				goto Exit;
			}
		}

		if (Osc[OscLength] == BEL) {
			// add the BEL character
			NumberOfTerminatingCharacters = 1;
		} else {
			ASSUME (Osc[OscLength] == ESC);
			NumberOfTerminatingCharacters = 2;
		}

		OscLength += NumberOfTerminatingCharacters;
		CchProcessed += OscLength;

		if ((Osc[0] == '0' || Osc[0] == '2') && Osc[1] == ';') {
			BOOL Success;
			WCHAR ConsoleTitle[256];
			ULONG ConsoleTitleCch;

			// Set Window Title

			ConsoleTitleCch = OscLength - 2 - NumberOfTerminatingCharacters;

			//
			// Copy and null-terminate the title string.
			// MS docs say that 255 is the maximum length.
			//

			RtlCopyMemory(
				ConsoleTitle,
				&Osc[2],
				ConsoleTitleCch * sizeof(WCHAR));

			ConsoleTitle[ConsoleTitleCch] = '\0';

			//
			// Set the console title.
			//

			Success = SetConsoleTitle(ConsoleTitle);
			ASSERT (Success);
		}

		break;
			  }
	default:
		// Unknown two character escape
		KexLogDebugEvent(L"Unrecognized two-character ANSI escape '%c'", EscapeSequence[1]);
		break;
	}

Exit:
	if (CchProcessedOut) {
		*CchProcessedOut = CchProcessed;
	}

	return TRUE;
}

BOOL WriteConsoleWithEscapeSequencesAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWrittenOut OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode)
{
	BOOL Success;
	ULONG CchWritten;
	ULONG CbOfChar;

	Success = TRUE;
	CchWritten = 0;
	CbOfChar = Unicode ? sizeof(WCHAR) : sizeof(CHAR);

	until (CchToWrite == 0) {
		ULONG CchToNextEscape;

		//
		// Write to the console until we hit an escape sequence or hit the
		// end of the input buffer.
		//

		CchToNextEscape = GetCchToNextEscapeOrEndOfBufferAorW(
			Buffer,
			CchToWrite,
			Unicode);

		if (CchToNextEscape > 0) {
			ULONG CchWrittenThisCall;

			Success = WriteConsoleAorW(
				ConsoleHandle,
				Buffer,
				CchToNextEscape,
				&CchWrittenThisCall,
				NULL,
				Unicode);

			if (!Success) {
				goto Exit;
			}

			CchWritten += CchWrittenThisCall;
			CchToWrite -= CchWrittenThisCall;
			Buffer = RVA_TO_VA(Buffer, CchWrittenThisCall * CbOfChar);
		}

		//
		// If there's still more characters, that means we've hit an escape
		// sequence.
		//

		if (CchToWrite > 0) {
			ULONG CchProcessedThisCall;

			Success = ProcessEscapeSequenceAorW(
				ConsoleHandle,
				Buffer,
				CchToWrite,
				&CchProcessedThisCall,
				Unicode);

			if (!Success) {
				goto Exit;
			}

			CchWritten += CchProcessedThisCall;
			CchToWrite -= CchProcessedThisCall;
			Buffer = RVA_TO_VA(Buffer, CchProcessedThisCall * CbOfChar);
		}
	}

Exit:
	if (CchWrittenOut) {
		*CchWrittenOut = CchWritten;
	}

	return Success;
}