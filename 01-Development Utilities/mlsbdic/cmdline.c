///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     cmdline.c
//
// Abstract:
//
//     Code to handle and act on command-line arguments.
//
// Author:
//
//     vxiiduu (21-May-2025)
//
// Environment:
//
//     Console (GUI for errors)
//
// Revision History:
//
//     vxiiduu              21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "mlsbdic.h"

//
// Note: This code is mostly stolen out of KexCfg\cmdline.c.
// TODO: Deduplicate this code by moving it into KexW32ML or something.
//
NTSTATUS NTAPI CmdlineParseStringParameter(
	IN	PCWSTR	CommandLine,
	IN	PCWSTR	FlagString,
	IN	ULONG	FlagStringCch,
	OUT	PWSTR	ParameterBuffer,
	IN	ULONG	ParameterBufferCch)
{
	PCWSTR Parameter;

	if (!CommandLine || !FlagString || !ParameterBuffer ||
		FlagStringCch == 0 || ParameterBufferCch == 0) {

		return STATUS_INVALID_PARAMETER;
	}

	ParameterBuffer[0] = '\0';

	Parameter = StringFindI(CommandLine, FlagString);
	if (!Parameter) {
		return STATUS_NOT_FOUND;
	}

	if (Parameter) {
		BOOLEAN HasQuotes;

		Parameter += FlagStringCch;

		if (Parameter[0] == '\0') {
			return STATUS_NO_DATA_DETECTED;
		}

		if (Parameter[0] == '"') {
			HasQuotes = TRUE;
			++Parameter;
		}

		// A failure in this function is ignored. We will determine the validity
		// of the arguments independently.
		// StringCchCopy is guaranteed to always null terminate ParameterBuffer
		// even if there was an error copying.
		StringCchCopy(ParameterBuffer, ParameterBufferCch, Parameter);

		if (HasQuotes) {
			PWCHAR EndQuote;

			// Find the matching quote and remove it.
			EndQuote = (PWCHAR) StringFind(ParameterBuffer, L"\"");

			if (!EndQuote) {
				// The ending quote was not found.
				// Either the buffer is too small or there is no end quote.
				return STATUS_COULD_NOT_INTERPRET;
			}

			*EndQuote = '\0';
		} else {
			PWCHAR EndSpace;

			// A space, or the end of the command line, delimits the end of the argument
			// in the case that the parameter is not quoted.
			EndSpace = (PWCHAR) StringFind(ParameterBuffer, L" ");

			if (EndSpace) {
				*EndSpace = '\0';
			}
		}
	}

	return STATUS_SUCCESS;
}

STATIC VOID BdicParseInOutParameter(
	IN	PCWSTR	CommandLine,
	IN	PCWSTR	FlagString,
	IN	ULONG	FlagStringCch,
	OUT	PWSTR	ParameterBuffer,
	IN	ULONG	ParameterBufferCch)
{
	NTSTATUS Status;
	HRESULT Result;

	Status = CmdlineParseStringParameter(
		CommandLine,
		FlagString,
		FlagStringCch,
		ParameterBuffer,
		ParameterBufferCch);

	if (NT_SUCCESS(Status)) {
		PCWSTR Extension;

		// Validate the path (ensure .dic or .bdi extension).
		Result = PathCchFindExtension(ParameterBuffer, ParameterBufferCch, &Extension);
		if (FAILED(Result) || (!StringEqualI(Extension, L".dic") && !StringEqualI(Extension, L".bdi"))) {

			ErrorBoxF(L"The argument to %s must be a file with a .dic or .bdi extension.", FlagString);
			ExitProcess(STATUS_INVALID_PARAMETER);
		}
	} else unless (Status == STATUS_NOT_FOUND) {
		switch (Status) {
		case STATUS_NO_DATA_DETECTED:
			ErrorBoxF(L"%s was specified without a file name.", FlagString);
			break;
		case STATUS_COULD_NOT_INTERPRET:
			ErrorBoxF(L"The argument to %*s was too long or missing an end quote.", FlagString);
			break;
		default:
			ASSUME (("Other status values indicate a coding error", FALSE));
		}

		ExitProcess(Status);
	}
}

VOID HandleCommandLine(
	IN	PWSTR	CommandLine)
{
	WCHAR InputFilePath[MAX_PATH];
	WCHAR OutputFilePath[MAX_PATH];

	if (StringSearchI(CommandLine, L"/HELP") || StringSearch(CommandLine, L"/?")) {
		DisplayHelpMessage();
		ExitProcess(STATUS_SUCCESS);
	}

	BdicParseInOutParameter(
		CommandLine,
		L"/IN:",
		StringLiteralLength(L"/IN:"),
		InputFilePath,
		ARRAYSIZE(InputFilePath));

	BdicParseInOutParameter(
		CommandLine,
		L"/OUT:",
		StringLiteralLength(L"/OUT:"),
		OutputFilePath,
		ARRAYSIZE(OutputFilePath));

	if (InputFilePath[0] == '\0') {
		ErrorBoxF(L"You must specify an input file. Use /? for more information.");
		ExitProcess(STATUS_NOT_FOUND);
	}

	//
	// If there's no output file specified, just use the input file but replace the
	// extension (.dic -> .bdi and vice versa).
	//

	if (OutputFilePath[0] == '\0') {
		HRESULT Result;
		PCWSTR Extension;

		// This call shouldn't fail because the destination buffer is the same size
		// as the source buffer.
		Result = StringCchCopy(OutputFilePath, ARRAYSIZE(OutputFilePath), InputFilePath);
		ASSERT (SUCCEEDED(Result));

		// This call shouldn't fail because BdicParseInOutParameter already made sure
		// we have a valid extension.
		Result = PathCchFindExtension(OutputFilePath, ARRAYSIZE(OutputFilePath), &Extension);
		ASSERT (SUCCEEDED(Result));

		if (StringEqualI(Extension, L".dic")) {
			Result = PathCchRenameExtension(OutputFilePath, ARRAYSIZE(OutputFilePath), L".bdi");
		} else {
			ASSUME (StringEqualI(Extension, L".bdi"));
			Result = PathCchRenameExtension(OutputFilePath, ARRAYSIZE(OutputFilePath), L".dic");
		}

		// Shouldn't be any error since both extensions are the same length.
		ASSERT (SUCCEEDED(Result));
	}

	BdicCompileBdiDic(InputFilePath, OutputFilePath);
}