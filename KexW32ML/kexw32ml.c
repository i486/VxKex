///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexw32ml.c
//
// Abstract:
//
//     VxKex Win32-mode General Library
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
#include <KexW32ML.h>

//
// The return value is in milliseconds.
// The return value is positive if FileTime2 is later than FileTime1.
// The return value is negative if FileTime2 is earlier than FileTime1.
//
KW32MLDECLSPEC EXTERN_C LONGLONG KW32MLAPI CompareFileTimes(
	IN	FILETIME	FileTime1,
	IN	FILETIME	FileTime2)
{
	LONGLONG FileTime1AsInt64;
	LONGLONG FileTime2AsInt64;

	FileTime1AsInt64 = *((PLONGLONG) &FileTime1);
	FileTime2AsInt64 = *((PLONGLONG) &FileTime2);

	return (FileTime2AsInt64 - FileTime1AsInt64) / 10000;
}

KW32MLDECLSPEC EXTERN_C PWSTR KW32MLAPI GetCommandLineWithoutImageName(
	VOID)
{
	BOOLEAN DoubleQuoted;
	PWSTR RawCommandLine;

	DoubleQuoted = FALSE;
	RawCommandLine = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;

	while (*RawCommandLine > ' ' || (*RawCommandLine && DoubleQuoted)) {
		if (*RawCommandLine == '"') {
			DoubleQuoted = !DoubleQuoted;
		}

		RawCommandLine++;
	}

	while (*RawCommandLine && *RawCommandLine <= ' ') {
		RawCommandLine++;
	}

	return RawCommandLine;
}

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI PathReplaceIllegalCharacters(
	IN	PWSTR	Path,
	IN	WCHAR	ReplacementCharacter,
	IN	BOOLEAN	AllowPathSeparators)
{
	BOOLEAN AtLeastOneCharacterWasReplaced;

	ASSERT (Path != NULL);

	if (!Path) {
		return FALSE;
	}

	AtLeastOneCharacterWasReplaced = FALSE;

	while (*Path != '\0') {
		switch (*Path) {
		case '<':
		case '>':
		case ':':
		case '"':
		case '|':
		case '?':
		case '*':
			*Path = ReplacementCharacter;
			AtLeastOneCharacterWasReplaced = TRUE;
			break;
		case '/':
		case '\\':
			unless (AllowPathSeparators) {
				*Path = ReplacementCharacter;
				AtLeastOneCharacterWasReplaced = TRUE;
			}
			break;
		}

		Path++;
	}

	return AtLeastOneCharacterWasReplaced;
}

KW32MLDECLSPEC PCWSTR KW32MLAPI Win32ErrorAsString(
	IN	ULONG	Win32ErrorCode)
{
	ULONG NumberOfCharacters;
	STATIC WCHAR StaticBuffer[256];

	NumberOfCharacters = FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		Win32ErrorCode,
		0,
		StaticBuffer,
		ARRAYSIZE(StaticBuffer),
		NULL);

	if (NumberOfCharacters == 0) {
		StaticBuffer[0] = '\0';
	}

	return StaticBuffer;
}

#ifdef KEX_TARGET_TYPE_DLL
ULONG WINAPI DllMain(
	IN	HMODULE		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(DllBase);
	}

	return TRUE;
}
#endif