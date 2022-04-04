#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <winternl.h>
#include <KexComm.h>

#define APPNAME L"KexCfg"
#define FRIENDLYAPPNAME L"VxKex Configuration Tool"

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// KexCfg is a helper program which is used to enable or disable VxKex for
// specific .EXE files. It is only meant to be invoked by the configuration
// shell extension or other programs which are associated with VxKex. The
// reason why we need a helper program is because this program might need to
// be restarted with administrator privileges in order to change the IFEO
// registry options.
//
// The command line must be in the following format:
// A STR parameter is a quoted string. The string must be double quoted.
// There is no escape character and therefore STR parameters cannot contain
// double quotes.
// A BOOL parameter is an unquoted number either 1 or 0.
//
//   <<STR1>> <BOOL1> <STR2> <BOOL2> <BOOL3> <BOOL4> <BOOL5>
//
// STR1 parameter indicates the full path of the .exe file of the program
// which configuration is being set for.
// BOOL1 parameter is whether or not to enable VxKex for this program.
// STR2 parameter may consist of the following values and determines whether
// the Windows version reported to the application will be spoofed:
//   NONE - no spoofing
//   WIN8 - Windows 8
//   WIN81 - Windows 8.1
//   WIN10 - Windows 10
//   WIN11 - Windows 11
// Any unrecognized value is equivalent to NONE.
// BOOL2 parameter is whether debugging information will always be shown.
// BOOL3 parameter is whether VxKex will be disabled for child processes.
// BOOL4 parameter is whether all app-specific hacks will be disabled.
// BOOL5 parameter is whether the VxKexLdr will wait for its child to exit
// before itself exiting.
//
// The behavior of the program when arguments are provided but do not match
// the template is undefined.

INT APIENTRY wWinMain(
	IN	HINSTANCE	hInstance,
	IN	HINSTANCE	hPrevInstance,
	IN	LPWSTR		lpszCmdLine,
	IN	INT			iCmdShow)
{
	BOOL bRetryAsAdminOnFailure = FALSE;
	WCHAR szExePath[MAX_PATH];
	WCHAR szExeName[MAX_PATH];
	WCHAR szWinVerSpoof[6];
	WCHAR szVxKexLdrPath[MAX_PATH];
	WCHAR szIfeoKey[74 + MAX_PATH] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
	WCHAR szKexIfeoKey[54 + MAX_PATH] = L"SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\";
	WCHAR szSystem32Path[MAX_PATH];

	DWORD dwEnableVxKex;
	DWORD dwAlwaysShowDebug;
	DWORD dwDisableForChild;
	DWORD dwDisableAppSpecific;
	DWORD dwWaitForChild;

	SetFriendlyAppName(FRIENDLYAPPNAME);

	if (lpszCmdLine == NULL || wcslen(lpszCmdLine) == 0) {
		// TODO: display something more useful to the user here e.g. a global configuration panel
		InfoBoxF(L"This application is not intended to be used standalone. To configure VxKex for a program, "
				 L"open the Properties dialog for that program and select the 'VxKex' tab.");
		ExitProcess(0);
	}

	swscanf_s(lpszCmdLine, L"\"%259[^\"]\" %I32u \"%5[^\"]\" %I32u %I32u %I32u %I32u",
			  szExePath,				ARRAYSIZE(szExePath),
			  &dwEnableVxKex,
			  szWinVerSpoof,			ARRAYSIZE(szWinVerSpoof),
			  &dwAlwaysShowDebug,
			  &dwDisableForChild,
			  &dwDisableAppSpecific,
			  &dwWaitForChild);

	if (PathIsRelative(szExePath)) {
		CriticalErrorBoxF(L"An absolute path must be passed to " APPNAME L".");
	}

	wcscpy_s(szExeName, ARRAYSIZE(szExeName), szExePath);
	PathStripPath(szExeName);
	wcscat_s(szIfeoKey, ARRAYSIZE(szIfeoKey), szExeName);
	wcscat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), szExePath);
	CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath)));
	wcscat_s(szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath), L"\\VxKexLdr.exe");
	CHECKED(GetSystemDirectory(szSystem32Path, ARRAYSIZE(szSystem32Path)));
	bRetryAsAdminOnFailure = TRUE;

	if (dwEnableVxKex) {
		WCHAR szOldDebugger[MAX_PATH];
		
		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, L"Debugger", szOldDebugger, ARRAYSIZE(szOldDebugger)) &&
			!lstrcmpi(szOldDebugger, szVxKexLdrPath)) {
			// no need to try to write anything to the "Debugger" value
			// do nothing
		} else {
			CHECKED(RegWriteSz(HKEY_LOCAL_MACHINE, szIfeoKey, L"Debugger", szVxKexLdrPath));
		}
	} else {
		WCHAR szOldDebugger[MAX_PATH];

		// Likewise, only delete the registry key if it exists. This helps avoid spurious
		// UAC prompts.
		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, L"Debugger", szOldDebugger, ARRAYSIZE(szOldDebugger))) {
			// Don't use CHECKED() here because it will sometimes fail wrongly
			// when the "Debugger" value doesn't exist (e.g. if you disable VxKex twice in a row)
			if (!RegDelValue(HKEY_LOCAL_MACHINE, szIfeoKey, L"Debugger") && GetLastError() != ERROR_FILE_NOT_FOUND) {
				goto Error;
			}
		}
	}

	// This creates a tree structure under the kex IFEO key.
	// VxKexLdr
	//   Image File Execution Options
	//     C:
	//       Users
	//         Bob
	//           Programs
	//             program.exe
	//               EnableVxKex(REG_DWORD) = 1
	//               WinVerSpoof(REG_SZ) = "WIN10"
	// etc., since backslashes aren't permitted in registry key names.
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, L"EnableVxKex", dwEnableVxKex));
	CHECKED(RegWriteSz(HKEY_CURRENT_USER, szKexIfeoKey, L"WinVerSpoof", szWinVerSpoof));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, L"AlwaysShowDebug", dwAlwaysShowDebug));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, L"DisableForChild", dwDisableForChild));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, L"DisableAppSpecific", dwDisableAppSpecific));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, L"WaitForChild", dwWaitForChild));

	ExitProcess(0);

Error:
	if (bRetryAsAdminOnFailure && GetLastError() == ERROR_ACCESS_DENIED) {
		WCHAR szSelfPath[MAX_PATH];

		// if a CHECKED statement fails in here, we will effectively "fall through" to the
		// other code path which just gives up.
		bRetryAsAdminOnFailure = FALSE;
		
		CHECKED(GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath)));
		CHECKED(ShellExecute(NULL, L"runas", szSelfPath, lpszCmdLine, NULL, 0) > (HINSTANCE) 32);
		ExitProcess(0);
	} else if (GetLastError() == ERROR_CANCELLED) {
		// don't display an unnecessary warning box if the user simply clicked no on the UAC box
		ExitProcess(0);
	} else {
		CriticalErrorBoxF(L"A critical error was encountered while attempting to operate on %s.\nError code: %#010I32x: %s",
						  szExePath, GetLastError(), GetLastErrorAsString());
	}
}