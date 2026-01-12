///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     cmdline.c
//
// Abstract:
//
//     Contains functions related to parsing the command-line arguments of the
//     installer program.
//
// Author:
//
//     vxiiduu (01-Feb-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               01-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "kexsetup.h"

VOID DisplayHelpMessage(
	VOID)
{
	MessageBox(
		NULL,
		L"KexSetup takes the following command-line parameters:\r\n"
		L"\r\n"
		L"/HELP or /? - Display this help message\r\n"
		L"/UNINSTALL - Start the setup application in uninstall mode\r\n"
		L"/PRESERVECONFIG - Keep your settings when uninstalling\r\n"
		L"/SILENTUNATTEND - Enable silent unattended mode\r\n"
		L"\r\n"
		L"Silent unattended mode does not display a GUI and does not prompt for user input. "
		L"In this mode, the setup application will immediately perform the requested action. "
		L"If /UNINSTALL is not specified then the default action is to install the software. "
		L"The following flag may be useful in silent unattend mode:\r\n\r\n"
		L"/KEXDIR:\"<directory path>\" - Specifies the path to which VxKex will be installed. "
		L"You must use quotes around the directory path if it contains space characters.",
		FRIENDLYAPPNAME,
		MB_OK);
}

VOID ProcessCommandLineOptions(
	VOID)
{
	PWSTR CommandLine;
	PWSTR CommandLineBuffer;
	PCWSTR ParentProcessHwnd;
	ULONG CommandLineBufferCch;
	ULONG Index;

	//
	// Make a copy of the process command line. We will modify it in this function,
	// and we don't want to clobber the original since we may need to use it again
	// later on.
	//

	CommandLine = GetCommandLineWithoutImageName();
	CommandLineBufferCch = wcslen(CommandLine) + 1;
	CommandLineBuffer = StackAlloc(WCHAR, CommandLineBufferCch);
	CopyMemory(CommandLineBuffer, CommandLine, CommandLineBufferCch * sizeof(WCHAR));
	CommandLine = CommandLineBuffer;

	if (StringSearchI(CommandLine, L"/HELP") || StringSearch(CommandLine, L"/?")) {
		DisplayHelpMessage();
		ExitProcess(0);
	}

	if (StringSearchI(CommandLine, L"/SILENTUNATTEND")) {
		SilentUnattended = TRUE;
	} else {
		SilentUnattended = FALSE;
	}

	if (StringSearchI(CommandLine, L"/PRESERVECONFIG")) {
		PreserveConfig = TRUE;
	} else {
		PreserveConfig = FALSE;
	}

	ParentProcessHwnd = StringFindI(CommandLine, L"/HWND:");

	if (ParentProcessHwnd) {
		HWND Window;

		if (!SilentUnattended) {
			ErrorBoxF(L"/HWND cannot be used without /SILENTUNATTEND.");
			ExitProcess(STATUS_INVALID_PARAMETER_MIX);
		}

		ParentProcessHwnd += StringLiteralLength(L"/HWND:");
		Window = NULL;
		swscanf_s(ParentProcessHwnd, L"%u", &Window);

		if (IsWindow(Window)) {
			MainWindow = Window;
			KexgApplicationMainWindow = MainWindow;
		} else {
			ErrorBoxF(L"Invalid window handle passed to /HWND.");
			ExitProcess(STATUS_INVALID_PARAMETER);
		}
	}

	if (StringSearchI(CommandLine, L"/UNINSTALL")) {
		if (ExistingVxKexVersion > InstallerVxKexVersion) {
			InfoBoxF(
				L"A version of VxKex is installed on your computer that is "
				L"newer than the version inside this installer. If you would like "
				L"to uninstall, use a newer version of the setup application.");
			ExitProcess(STATUS_VERSION_MISMATCH);
		}

		OperationMode = OperationModeUninstall;
	} else {
		if (PreserveConfig) {
			ErrorBoxF(L"/PRESERVECONFIG cannot be used without /UNINSTALL.");
			ExitProcess(STATUS_INVALID_PARAMETER_MIX);
		}

		if (ExistingVxKexVersion == 0) {
			OperationMode = OperationModeInstall;
		} else if (InstallerVxKexVersion > ExistingVxKexVersion) {
			OperationMode = OperationModeUpgrade;
		} else if (InstallerVxKexVersion == ExistingVxKexVersion) {
			InfoBoxF(L"VxKex is already installed. To uninstall, use the Add/Remove Programs control panel.");
			ExitProcess(STATUS_ALREADY_REGISTERED);
		} else if (ExistingVxKexVersion > InstallerVxKexVersion) {
			InfoBoxF(
				L"A version of VxKex is installed on your computer that is "
				L"newer than the version inside this installer. If you would "
				L"like to downgrade, please uninstall the existing version first.");
			ExitProcess(STATUS_VERSION_MISMATCH);
		} else {
			NOT_REACHED;
		}
	}

	if (OperationMode == OperationModeInstall) {
		PWSTR RequestedInstallationPath;
		WCHAR RequestedInstallationPathExpandedEnvironmentStrings[MAX_PATH];

		RequestedInstallationPath = (PWSTR) StringFindI(CommandLine, L"/KEXDIR:");

		if (RequestedInstallationPath) {
			Index = 0;
			RequestedInstallationPath += StringLiteralLength(L"/KEXDIR:");

			if (RequestedInstallationPath[0] == '"') {
				//
				// We have a quoted path. Scan ahead until the closing quote.
				//

				++RequestedInstallationPath;

				until (RequestedInstallationPath[Index] == '"' ||
					   RequestedInstallationPath[Index] == '\0') {
					++Index;
				}

				if (RequestedInstallationPath[Index] == '"') {
					RequestedInstallationPath[Index] = '\0';
				} else {
					ErrorBoxF(L"Invalid quoted path argument to /KEXDIR.");
					ExitProcess(STATUS_INVALID_PARAMETER);
				}
			} else {
				//
				// Unquoted path. Scan ahead until the next space character,
				// quote character, or the end of the command line arguments.
				//

				until (RequestedInstallationPath[Index] == ' ' ||
					   RequestedInstallationPath[Index] == '"' ||
					   RequestedInstallationPath[Index] == '\0') {
					++Index;
				}

				RequestedInstallationPath[Index] = '\0';
			}

			//
			// Perform some validation and processing on the requested installation location
			// before copying it into the KexDir global variable.
			//

			if (PathIsRelative(RequestedInstallationPath)) {
				ErrorBoxF(L"The argument to /KEXDIR must be an absolute path.");
				ExitProcess(STATUS_INVALID_PARAMETER);
			}

			ExpandEnvironmentStrings(
				RequestedInstallationPath,
				RequestedInstallationPathExpandedEnvironmentStrings,
				ARRAYSIZE(RequestedInstallationPathExpandedEnvironmentStrings));

			RequestedInstallationPath = RequestedInstallationPathExpandedEnvironmentStrings;
			PathCchRemoveBackslash(RequestedInstallationPath, ARRAYSIZE(RequestedInstallationPath));
			PathRemoveBlanks(RequestedInstallationPath);
			PathCchCanonicalize(KexDir, ARRAYSIZE(KexDir), RequestedInstallationPath);
		}
	} else {
		if (StringSearchI(CommandLine, L"/KEXDIR")) {
			if (OperationMode == OperationModeUpgrade) {
				ErrorBoxF(L"/KEXDIR cannot be used because VxKex is already installed.");
				ExitProcess(STATUS_ALREADY_REGISTERED);
			} else if (OperationMode == OperationModeUninstall) {
				ErrorBoxF(L"/KEXDIR cannot be combined with /UNINSTALL.");
				ExitProcess(STATUS_INVALID_PARAMETER_MIX);
			} else {
				NOT_REACHED;
			}
		}
	}
}