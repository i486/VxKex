#include "buildcfg.h"
#include "kexcfg.h"

STATIC VOID DisplayHelpMessage(
	VOID)
{
	KexCfgMessageBox(
		NULL,
		L"KexCfg takes the following command-line parameters:\r\n"
		L"\r\n"
		L"/HELP or /? - Display this help message\r\n"
		L"/EXE:\"<EXE path>\" - Specifies the full path of the executable to configure\r\n"
		L"/ENABLE:<boolean> - Specifies whether VxKex will be enabled or disabled\r\n"
		L"/DISABLEFORCHILD:<boolean> - Configures whether VxKex will be enabled or disabled for child processes\r\n"
		L"/DISABLEAPPSPECIFIC:<boolean> - Configures whether app-specific hacks will be used\r\n"
		L"/WINVERSPOOF:<decimal or string> - Configures the spoofed Windows version\r\n"
		L"/STRONGSPOOF:<hexadecimal flags> - Configures options for strong version spoofing\r\n"
		L"\r\n"
		L"The <EXE path> argument must be a full absolute path to a file with a .exe extension.\r\n"
		L"Boolean parameters TRUE, YES, 1, FALSE, NO, or 0 are recognized.\r\n"
		L"The /WINVERSPOOF parameter can be a member of the KEX_WIN_VER_SPOOF enumeration or one of the following "
		L"string values: NONE, WIN7SP1, WIN8, WIN81, WIN10, or WIN11.\r\n"
		L"The /STRONGSPOOF parameter takes one or more of the KEX_STRONGSPOOF_* bit flags, defined in KexDll.h.\r\n"
		L"\r\n"
		L"Any values which are not specified on the command line will use default values.",
		FRIENDLYAPPNAME,
		MB_OK);
}

//
// Since we might be running with elevated privileges, we'll be extra
// careful in the command line parsing code.
//
VOID KexCfgHandleCommandLine(
	IN	PCWSTR	CommandLine)
{
	BOOLEAN Success;
	NTSTATUS Status;
	PCWSTR Parameter;
	WCHAR ExeFullPathTemp[MAX_PATH];
	WCHAR ExeFullPath[MAX_PATH];
	KXCFG_PROGRAM_CONFIGURATION Configuration;
	HANDLE TransactionHandle;

	ExeFullPath[0] = '\0';

	if (StringSearchI(CommandLine, L"/HELP") || StringSearchI(CommandLine, L"/?")) {
		DisplayHelpMessage();
		ExitProcess(STATUS_SUCCESS);
	}

	if (StringSearchI(CommandLine, L"/SCHTASK $(Arg0)")) {
		KexCfgMessageBox(
			NULL,
			L"This scheduled task is not designed to be invoked by the user.",
			FRIENDLYAPPNAME,
			MB_ICONINFORMATION | MB_OK);

		ExitProcess(STATUS_NOT_SUPPORTED);
	}

	//
	// Handle the /EXE parameter.
	//

	Parameter = StringFindI(CommandLine, L"/EXE:");
	if (Parameter) {
		BOOLEAN HasQuotes;

		Parameter += StringLiteralLength(L"/EXE:");

		if (Parameter[0] == '\0') {
			KexCfgMessageBox(
				NULL,
				L"/EXE was specified without an executable name.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		if (Parameter[0] == '"') {
			HasQuotes = TRUE;
			++Parameter;
		}

		StringCchCopy(ExeFullPathTemp, ARRAYSIZE(ExeFullPathTemp), Parameter);

		if (HasQuotes) {
			PWCHAR EndQuote;

			// Find the matching quote and remove it.
			EndQuote = (PWCHAR) StringFind(ExeFullPathTemp, L"\"");

			if (!EndQuote) {
				KexCfgMessageBox(
					NULL,
					L"The argument to /EXE was too long or missing an end quote.",
					FRIENDLYAPPNAME,
					MB_ICONERROR | MB_OK);

				ExitProcess(STATUS_INVALID_PARAMETER);
			}

			*EndQuote = '\0';
		} else {
			PWCHAR EndSpace;

			// A space delimits the end of the argument.
			EndSpace = (PWCHAR) StringFind(ExeFullPathTemp, L" ");

			if (EndSpace) {
				*EndSpace = '\0';
			}
		}

		//
		// Canonicalize the path.
		//

		if (FAILED(PathCchCanonicalize(ExeFullPath, ARRAYSIZE(ExeFullPath), ExeFullPathTemp))) {
			KexCfgMessageBox(
				NULL,
				L"The argument to /EXE could not be canonicalized and is invalid.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		//
		// Validate the path. If it contains any forward slashes, it's invalid.
		// And it has to have a .exe or .msi extension at the end.
		//

		if (!isalpha(ExeFullPath[0]) || ExeFullPath[1] != ':' || ExeFullPath[2] != '\\') {
			KexCfgMessageBox(
				NULL,
				L"The argument to /EXE must be an absolute path with a drive letter. "
				L"If this program is on a network share, you must map the share as a "
				L"network drive (right click -> Map network drive...).",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		{
			WCHAR WinDir[MAX_PATH];
			WCHAR KexDir[MAX_PATH];

			GetWindowsDirectory(WinDir, ARRAYSIZE(WinDir));
			KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));

			if (PathIsPrefix(WinDir, ExeFullPath) || PathIsPrefix(KexDir, ExeFullPath)) {
				KexCfgMessageBox(
					NULL,
					L"The argument to /EXE cannot be in the Windows directory "
					L"or the VxKex installation directory.",
					FRIENDLYAPPNAME,
					MB_ICONERROR | MB_OK);

				ExitProcess(STATUS_INVALID_PARAMETER);
			}
		}

		if (PathFindFileName(ExeFullPath) == PathFindExtension(ExeFullPath) ||
			PathFindFileName(ExeFullPath) == ExeFullPath) {

			KexCfgMessageBox(
				NULL,
				L"The argument to /EXE must have a file name, not just an extension.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		if (!StringEqualI(PathFindExtension(ExeFullPath), L".exe") &&
			!StringEqualI(PathFindExtension(ExeFullPath), L".msi")) {

			KexCfgMessageBox(
				NULL,
				L"The argument to /EXE must have a .exe or .msi file extension.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}
	} else {
		KexCfgMessageBox(
			NULL,
			L"/EXE must be specified.",
			FRIENDLYAPPNAME,
			MB_ICONERROR | MB_OK);

		ExitProcess(STATUS_INVALID_PARAMETER);
	}

	//
	// Now that we have an EXE name, we will fetch the existing configuration for
	// this program (if it exists).
	//

	Success = KxCfgGetConfiguration(
		ExeFullPath,
		&Configuration);

	if (!Success) {
		ZeroMemory(&Configuration, sizeof(Configuration));
	}

	//
	// Handle the /ENABLE, /DISABLEFORCHILD and /DISABLEAPPSPECIFIC
	// boolean parameters.
	//

	Parameter = StringFindI(CommandLine, L"/ENABLE:");
	if (Parameter) {
		Parameter += StringLiteralLength(L"/ENABLE:");
		Configuration.Enabled = KexCfgParseBooleanParameter(Parameter);
	}

	Parameter = StringFindI(CommandLine, L"/DISABLEFORCHILD:");
	if (Parameter) {
		Parameter += StringLiteralLength(L"/DISABLEFORCHILD:");
		Configuration.DisableForChild = KexCfgParseBooleanParameter(Parameter);
	}

	Parameter = StringFindI(CommandLine, L"/DISABLEAPPSPECIFIC:");
	if (Parameter) {
		Parameter += StringLiteralLength(L"/DISABLEAPPSPECIFIC:");
		Configuration.DisableAppSpecificHacks = KexCfgParseBooleanParameter(Parameter);
	}

	//
	// Handle /WINVERSPOOF
	//

	Parameter = StringFindI(CommandLine, L"/WINVERSPOOF:");
	if (Parameter) {
		Parameter += StringLiteralLength(L"/WINVERSPOOF:");

		if (StringBeginsWithI(Parameter, L"NONE")) {
			Configuration.WinVerSpoof = WinVerSpoofNone;
		} else if (StringBeginsWithI(Parameter, L"WIN7SP1")) {
			Configuration.WinVerSpoof = WinVerSpoofWin7;
		} else if (StringBeginsWithI(Parameter, L"WIN81")) {
			Configuration.WinVerSpoof = WinVerSpoofWin8Point1;
		} else if (StringBeginsWithI(Parameter, L"WIN8")) {
			Configuration.WinVerSpoof = WinVerSpoofWin8;
		} else if (StringBeginsWithI(Parameter, L"WIN10")) {
			Configuration.WinVerSpoof = WinVerSpoofWin10;
		} else if (StringBeginsWithI(Parameter, L"WIN11")) {
			Configuration.WinVerSpoof = WinVerSpoofWin11;
		} else {
			ULONG Value;

			if (swscanf_s(Parameter, L"%lu", &Value) != 1) {
				KexCfgMessageBox(
					NULL,
					L"The argument to /WINVERSPOOF could not be parsed.",
					FRIENDLYAPPNAME,
					MB_ICONERROR | MB_OK);

				ExitProcess(STATUS_INVALID_PARAMETER);
			}

			if (Value >= WinVerSpoofMax) {
				KexCfgMessageBox(
					NULL,
					L"The decimal argument to /WINVERSPOOF is out of range.",
					FRIENDLYAPPNAME,
					MB_ICONERROR | MB_OK);

				ExitProcess(STATUS_INVALID_PARAMETER);
			}

			Configuration.WinVerSpoof = (KEX_WIN_VER_SPOOF) Value;
		}
	}

	//
	// Handle /STRONGSPOOF
	//

	Parameter = StringFindI(CommandLine, L"/STRONGSPOOF:");
	if (Parameter) {
		ULONG Value;

		Parameter += StringLiteralLength(L"/STRONGSPOOF:");

		if (StringBeginsWithI(Parameter, L"0x")) {
			Parameter += ARRAYSIZE(L"0x");
		}

		if (swscanf_s(Parameter, L"%lx", &Value) != 1) {
			KexCfgMessageBox(
				NULL,
				L"The argument to /STRONGSPOOF could not be parsed.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		if (Value & (~KEX_STRONGSPOOF_VALID_MASK)) {
			KexCfgMessageBox(
				NULL,
				L"The argument to /STRONGSPOOF contained invalid flags.",
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(STATUS_INVALID_PARAMETER);
		}

		Configuration.StrongSpoofOptions = Value;
	}

	//
	// Apply the new configuration to the program.
	//

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool Transaction");

	Success = KxCfgSetConfiguration(
		ExeFullPath,
		&Configuration,
		TransactionHandle);

	if (Success) {
		Status = NtCommitTransaction(TransactionHandle, TRUE);
		SafeClose(TransactionHandle);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			WCHAR ErrorMessage[256];

			StringCchPrintf(
				ErrorMessage,
				ARRAYSIZE(ErrorMessage),
				L"The transaction for this operation could not be committed. %s",
				NtStatusAsString(Status));

			KexCfgMessageBox(
				NULL,
				ErrorMessage,
				FRIENDLYAPPNAME,
				MB_ICONERROR | MB_OK);

			ExitProcess(Status);
		}
	} else {
		WCHAR ErrorMessage[256];

		Status = NtRollbackTransaction(TransactionHandle, TRUE);
		SafeClose(TransactionHandle);

		ASSERT (NT_SUCCESS(Status));
		
		StringCchPrintf(
			ErrorMessage,
			ARRAYSIZE(ErrorMessage),
			L"The VxKex configuration for \"%s\" could not be applied due to the following error: %s",
			ExeFullPath,
			GetLastErrorAsString());

		KexCfgMessageBox(
			NULL,
			ErrorMessage,
			FRIENDLYAPPNAME,
			MB_ICONERROR | MB_OK);

		ExitProcess(STATUS_UNSUCCESSFUL);
	}
}