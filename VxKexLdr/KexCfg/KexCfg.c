#include <Windows.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <winternl.h>
#include <KexComm.h>

#define APPNAME T("KexCfg")
#define FRIENDLYAPPNAME T("VxKex Configuration Tool")

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

INT APIENTRY tWinMain(
	IN	HINSTANCE	hInstance,
	IN	HINSTANCE	hPrevInstance,
	IN	LPTSTR		lpszCmdLine,
	IN	INT			iCmdShow)
{
	BOOL bRetryAsAdminOnFailure = FALSE;
	TCHAR szExePath[MAX_PATH];
	TCHAR szExeName[MAX_PATH];
	TCHAR szWinVerSpoof[6];
	TCHAR szVxKexLdrPath[MAX_PATH];
	TCHAR szIfeoKey[74 + MAX_PATH] = T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\");
	TCHAR szKexIfeoKey[54 + MAX_PATH] = T("SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\");
	TCHAR szSystem32Path[MAX_PATH];

	DWORD dwEnableVxKex;
	DWORD dwAlwaysShowDebug;
	DWORD dwDisableForChild;
	DWORD dwDisableAppSpecific;
	DWORD dwWaitForChild;

	SetFriendlyAppName(FRIENDLYAPPNAME);

	if (lpszCmdLine == NULL || strlen(lpszCmdLine) == 0) {
		// TODO: display something more useful to the user here e.g. a global configuration panel
		InfoBoxF(T("This application is not intended to be used standalone. To configure VxKex for a program, ")
				 T("open the Properties dialog for that program and select the 'VxKex' tab."));
		ExitProcess(0);
	}

	sscanf_s(lpszCmdLine, T("\"%259[^\"]\" %I32u \"%5[^\"]\" %I32u %I32u %I32u %I32u"),
			 szExePath,				ARRAYSIZE(szExePath),
			 &dwEnableVxKex,
			 szWinVerSpoof,			ARRAYSIZE(szWinVerSpoof),
			 &dwAlwaysShowDebug,
			 &dwDisableForChild,
			 &dwDisableAppSpecific,
			 &dwWaitForChild);

	if (PathIsRelative(szExePath)) {
		CriticalErrorBoxF(T("An absolute path must be passed to ") APPNAME T("."));
	}

	strcpy_s(szExeName, ARRAYSIZE(szExeName), szExePath);
	PathStripPath(szExeName);
	strcat_s(szIfeoKey, ARRAYSIZE(szIfeoKey), szExeName);
	strcat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), szExePath);
	CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath)));
	strcat_s(szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath), T("\\VxKexLdr.exe"));
	CHECKED(GetSystemDirectory(szSystem32Path, ARRAYSIZE(szSystem32Path)));
	bRetryAsAdminOnFailure = TRUE;

	if (dwEnableVxKex) {
		TCHAR szOldDebugger[MAX_PATH];
		
		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, T("Debugger"), szOldDebugger, ARRAYSIZE(szOldDebugger)) &&
			!lstrcmpi(szOldDebugger, szVxKexLdrPath)) {
			// no need to try to write anything to the "Debugger" value
			// do nothing
		} else {
			CHECKED(RegWriteSz(HKEY_LOCAL_MACHINE, szIfeoKey, T("Debugger"), szVxKexLdrPath));
		}
	} else {
		TCHAR szOldDebugger[MAX_PATH];

		// Likewise, only delete the registry key if it exists. This helps avoid spurious
		// UAC prompts.
		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, T("Debugger"), szOldDebugger, ARRAYSIZE(szOldDebugger))) {
			// Don't use CHECKED() here because it will sometimes fail wrongly
			// when the "Debugger" value doesn't exist (e.g. if you disable VxKex twice in a row)
			if (!RegDelValue(HKEY_LOCAL_MACHINE, szIfeoKey, T("Debugger")) && GetLastError() != ERROR_FILE_NOT_FOUND) {
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
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, T("EnableVxKex"), dwEnableVxKex));
	CHECKED(RegWriteSz(HKEY_CURRENT_USER, szKexIfeoKey, T("WinVerSpoof"), szWinVerSpoof));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, T("AlwaysShowDebug"), dwAlwaysShowDebug));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, T("DisableForChild"), dwDisableForChild));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, T("DisableAppSpecific"), dwDisableAppSpecific));
	CHECKED(RegWriteDw(HKEY_CURRENT_USER, szKexIfeoKey, T("WaitForChild"), dwWaitForChild));

	ExitProcess(0);

Error:
	if (bRetryAsAdminOnFailure && GetLastError() == ERROR_ACCESS_DENIED) {
		TCHAR szSelfPath[MAX_PATH];

		// if a CHECKED statement fails in here, we will effectively "fall through" to the
		// other code path which just gives up.
		bRetryAsAdminOnFailure = FALSE;
		
		CHECKED(GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath)));
		CHECKED(ShellExecute(NULL, T("runas"), szSelfPath, lpszCmdLine, NULL, 0) > (HINSTANCE) 32);
		ExitProcess(0);
	} else if (GetLastError() == ERROR_CANCELLED) {
		// don't display an unnecessary warning box if the user simply clicked no on the UAC box
		ExitProcess(0);
	} else {
		CriticalErrorBoxF(T("A critical error was encountered while attempting to operate on %s.\nError code: %#010I32x: %s"),
						  szExePath, GetLastError(), GetLastErrorAsString());
	}
}