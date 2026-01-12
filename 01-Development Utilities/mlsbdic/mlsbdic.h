///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     mlsbdic.h
//
// Abstract:
//
//     Internal header file for the BDI compiler
//
// Author:
//
//     vxiiduu (21-May-2025)
//
// Environment:
//
//     N/A
//
// Revision History:
//
//     vxiiduu              21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexGui.h>
#include <KexMls.h>
#include "resource.h"

//
// cmdline.c
//

VOID HandleCommandLine(
	IN	PWSTR	CommandLine);

//
// compile.c
//

BOOLEAN BdicCompileBdiDic(
	IN	PCWSTR	InputFilePath,
	IN	PCWSTR	OutputFilePath);

//
// dlgproc.c
//

INT_PTR CALLBACK BdicDialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

//
// help.c
//

VOID DisplayHelpMessage(
	VOID);