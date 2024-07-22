#pragma once

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>

// Use KexCfgMessageBox instead
#undef MessageBox

//
// cmdline.c
//

VOID KexCfgHandleCommandLine(
	IN	PCWSTR	CommandLine);

//
// gui.c
//

VOID KexCfgOpenGUI(
	VOID);

//
// main.c
//

EXTERN BOOLEAN Interactive;
EXTERN ULONG SessionId;

//
// util.c
//

BOOLEAN RunningInInteractiveWindowStation(
	VOID);

INT KexCfgMessageBox(
	IN	HWND	ParentWindow OPTIONAL,
	IN	PWSTR	Message,
	IN	PWSTR	Title,
	IN	ULONG	Flags);

BOOLEAN KexCfgParseBooleanParameter(
	IN	PCWSTR	Parameter);