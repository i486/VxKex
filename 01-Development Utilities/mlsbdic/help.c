///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     help.c
//
// Abstract:
//
//     This file contains the function which shows a message box detailing
//     what command-line arguments are supported.
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

VOID DisplayHelpMessage(
	VOID)
{
	MessageBox(
		KexgApplicationMainWindow,
		L"MLSBDIC.EXE takes the following command-line parameters:\r\n"
		L"\r\n"
		L"/HELP or /? - Display this help message\r\n"
		L"/IN:\"<Input file path>\" - Specifies the path to the input file (.bdi or .dic)\r\n"
		L"/OUT:\"<Output file path>\" - Specifies the path to the output file (.bdi or .dic)\r\n"
		L"\r\n"
		L"The /IN parameter is mandatory for command-line usage. The /OUT parameter is optional.\r\n"
		L"This application can convert dictionaries from .dic to .bdi format, and from .bdi to .dic format.",
		FRIENDLYAPPNAME,
		MB_OK);
}