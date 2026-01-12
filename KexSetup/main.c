///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     KexSetup, the VxKex installer (and uninstaller) program.
//
// Author:
//
//     vxiiduu (31-Jan-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               31-Jan-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#define NEED_VERSION_DEFS
#include "kexsetup.h"

BOOLEAN Is64BitOS;
KEXSETUP_OPERATION_MODE OperationMode;
BOOLEAN SilentUnattended;
BOOLEAN PreserveConfig;
WCHAR KexDir[MAX_PATH];
ULONG ExistingVxKexVersion;
ULONG InstallerVxKexVersion;

VOID EntryPoint(
	VOID)
{
	KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	ExistingVxKexVersion = KexSetupGetInstalledVersion();
	InstallerVxKexVersion = KEX_VERSION_DW;
	
	Is64BitOS = IsWow64();
	GetDefaultInstallationLocation(KexDir);
	ProcessCommandLineOptions();

	if (SilentUnattended) {
		KexSetupPerformActions();
	} else {
		DisplayInstallerGUI();
	}

	ExitProcess(STATUS_SUCCESS);
}