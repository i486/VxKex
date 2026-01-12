///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     For the rewrite we're bringing back this classic VxKex component that
//     used to be the core of VxKex. But this time, it's serving a purpose which
//     is very different, yet very similar.
//
//     This loader application is invoked mainly by Windows Explorer when the
//     user clicks on VxKex options in either the normal context menu or the
//     extended context menu. But there is a GUI as well, for if users disable
//     VxKex integration into the context menus.
//
// Author:
//
//     vxiiduu (29-Feb-2024)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu              29-Feb-2024  Initial creation.
//     vxiiduu              19-Mar-2024  Patch CPIW version check as well.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "vxkexldr.h"

VOID EntryPoint(
	VOID)
{
	NTSTATUS Status;
	PWSTR CommandLine;

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;

	//
	// Initialize the propagation system. This can be done independently from the
	// rest of KexDll initialization. The propagation system is what lets us run
	// specific programs under VxKex even if they weren't configured in IFEO.
	//

	Status = KexInitializePropagation();
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		CriticalErrorBoxF(
			L"Propagation could not be initialized.\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));

		NOT_REACHED;
	}

	//
	// Patch the CreateProcessInternalW subsystem version check.
	// Failure here is non-critical.
	//

	Status = KexPatchCpiwSubsystemVersionCheck();
	ASSERT (NT_SUCCESS(Status));

	//
	// Parse command line.
	//

	CommandLine = GetCommandLineWithoutImageName();

	if (CommandLine[0] != '\0') {
		BOOLEAN IsQuotedPath;
		PCWSTR FilePath;
		PCWSTR Arguments;

		//
		// Extract the file path.
		//

		FilePath = NULL;
		Arguments = NULL;

		if (*CommandLine == '"') {
			IsQuotedPath = TRUE;

			// Quoted path. Next closing quote or end of string marks
			// end of file name or path.
			++CommandLine;
			FilePath = CommandLine;

			until (*CommandLine == '"' || *CommandLine == '\0') {
				++CommandLine;
			}
		} else {
			IsQuotedPath = FALSE;
			FilePath = CommandLine;

			// Non quoted path. First space or end of string marks end of
			// file name or path.
			until (*CommandLine == ' ' || *CommandLine == '\0') {
				++CommandLine;
			}
		}

		if (IsQuotedPath) {
			if (*CommandLine != '"') {
				// Expected matching quote, but reached end of string.
				// Malformed command line without an end quote.
				CriticalErrorBoxF(L"Malformed command line. A closing quote must be supplied.");
				NOT_REACHED;
			}
		}

		if (*CommandLine != '\0') {
			// Quoted path, or non quoted path that may have arguments.
			// Separate the path from the arguments by adding a terminator.
			*CommandLine = '\0';
			
			// Go past whitespace.
			do {
				++CommandLine;
			} until (*CommandLine != ' ');
		}

		if (*CommandLine != '\0') {
			// There are arguments.
			Arguments = CommandLine;
		}
		
		VklCreateProcess(FilePath, Arguments);
	} else {
		//
		// No command line parameters. Launch the GUI.
		//

		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		DialogBox(NULL, MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, VklDialogProc);
	}

	ExitProcess(0);
}