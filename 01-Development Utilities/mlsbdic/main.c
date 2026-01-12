///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     This file contains the entry point for the multi-language support (MLS)
//     binary dictionary (BDI) compiler.
//
//     This application uses a textual dictionary (.dic) as input and turns it
//     into a binary dictionary (.bdi) suitable for use with the MLS library.
//
// Author:
//
//     vxiiduu (21-May-2025)
//
// Environment:
//
//     Win32 GUI
//
// Revision History:
//
//     vxiiduu              21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "mlsbdic.h"

VOID EntryPoint(
	VOID)
{
	PWSTR CommandLine;

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	CommandLine = GetCommandLineWithoutImageName();

	if (CommandLine[0] != '\0') {
		HandleCommandLine(CommandLine);
	} else {
		ExitProcess(DialogBox(NULL, MAKEINTRESOURCE(IDD_MAINWINDOW), NULL, BdicDialogProc));
	}

	ExitProcess(STATUS_SUCCESS);
}