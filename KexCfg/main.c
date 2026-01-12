///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     Main file for KexCfg.
//
// Author:
//
//     vxiiduu (09-Feb-2024)
//
// Environment:
//
//     Win32 mode. Sometimes this program is run as Administrator, sometimes
//     as a standard account, sometimes as the local SYSTEM account.
//
// Revision History:
//
//     vxiiduu              09-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "kexcfg.h"
#include <WtsApi32.h>

BOOLEAN Interactive;
ULONG SessionId;

VOID EntryPoint(
	VOID)
{
	PCWSTR CommandLine;
	
	//
	// When we are running as the local SYSTEM account, we cannot just call
	// MessageBox to display information to the user, because processes running
	// as the SYSTEM account run on a non-interactive session for security
	// reasons (on Windows Vista and later).
	//
	// The WTSSendMessage function allows us to display a message box on the
	// interactive desktop. But in order to do so, we need to have a session ID,
	// which we can determine by enumerating all sessions and finding the first
	// one which is "active" (i.e. logged in with a user).
	//

	Interactive = RunningInInteractiveWindowStation();

	if (Interactive) {
		SessionId = WTS_CURRENT_SESSION;
	} else {
		BOOLEAN Success;
		PWTS_SESSION_INFO SessionInfo;
		ULONG NumberOfSessions;

		Success = WTSEnumerateSessions(
			WTS_CURRENT_SERVER,
			0, 1,
			&SessionInfo,
			&NumberOfSessions);

		try {
			ULONG Index;

			for (Index = 0; Index < NumberOfSessions; ++Index) {
				if (SessionInfo[Index].State == WTSActive) {
					SessionId = SessionInfo[Index].SessionId;
				}
			}
		} finally {
			WTSFreeMemory(SessionInfo);
		}
	}

	//
	// If we have no command line options, then we will open the GUI.
	// Otherwise we will process and act upon our command line arguments.
	//

	CommandLine = GetCommandLineWithoutImageName();;

	if (CommandLine[0] == '\0') {
		KexCfgOpenGUI();
	} else {
		KexCfgHandleCommandLine(CommandLine);
	}
	
	ExitProcess(STATUS_SUCCESS);
}