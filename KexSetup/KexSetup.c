#define CINTERFACE
#define COBJMACROS
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <WinGDI.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <KexComm.h>
#include <NtDll.h>
#include "resource.h"

#define APPNAME L"KexSetup"
#define FRIENDLYAPPNAME L"VxKex Setup"

INT g_iScene = 0;

LPCWSTR g_lpszInstallerVersion = L(KEX_VERSION_STR);
DWORD g_dwInstallerVersion;
LPCWSTR g_lpszInstalledVersion;
DWORD g_dwInstalledVersion;
WCHAR g_szInstalledKexDir[MAX_PATH] = L"";
DWORD g_dwDiskSpaceRequired = 0;

HANDLE g_hWorkThread = NULL;

//
// Worker functions
//

VOID ElevateIfNotElevated(
	IN	LPCWSTR	lpszCmdLine,
	IN	INT		iCmdShow)
{
	// Apparently using the manifest to do this can cause bluescreens on XP if you don't
	// do it properly. Better safe than sorry.
	if (!IsUserAnAdmin()) {
		if (LOBYTE(LOWORD(GetVersion())) >= 6) {
			WCHAR szSelfPath[MAX_PATH];
			GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath));
			ShellExecute(NULL, L"runas", szSelfPath, lpszCmdLine, NULL, iCmdShow);
			ExitProcess(0);
		} else {
			CriticalErrorBoxF(L"You need to be an Administrator to run %s.", APPNAME);
		}
	}
}

DWORD GetInstalledVersion(
	VOID)
{
	DWORD dwResult;

	if (!RegReadDw(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"InstalledVersion", &dwResult)) {
		dwResult = 0;
	}

	return dwResult;
}

LPCWSTR GetInstalledVersionAsString(
	VOID)
{
	static WCHAR szVersion[16]; // "255.255.255.255\0"
	DWORD dwVersion = GetInstalledVersion();
	swprintf_s(szVersion, ARRAYSIZE(szVersion), L"%hhu.%hhu.%hhu.%hhu",
			  HIBYTE(HIWORD(dwVersion)), LOBYTE(HIWORD(dwVersion)),
			  HIBYTE(LOWORD(dwVersion)), LOBYTE(LOWORD(dwVersion)));
	return szVersion;
}

DWORD GetVersionFromString(
	IN	LPCWSTR	lpszVersion)
{
	union {
		DWORD dwVersion;

		struct {
			BYTE bDigit4;
			BYTE bDigit3;
			BYTE bDigit2;
			BYTE bDigit1;
			BYTE extra_padding;	// required because %hhu isnt properly supported in vs2010
								// it is treated as %hu which means 1 extra byte is written
		};
	} u;

	swscanf_s(lpszVersion, L"%hhu.%hhu.%hhu.%hhu", &u.bDigit1, &u.bDigit2, &u.bDigit3, &u.bDigit4);
	return u.dwVersion;
}

INLINE LPCWSTR GetInstallerVersionAsString(
	VOID)
{
	return L(KEX_VERSION_STR);
}

INLINE DWORD GetInstallerVersion(
	VOID)
{
	return GetVersionFromString(GetInstallerVersionAsString());
}

HGDIOBJ SetStaticCtlBk(
	IN	HWND	hWndDlg,
	IN	HWND	hWndCtl,
	IN	HDC		hDC)
{
	static HFONT hFontBold = NULL;

	if (!hFontBold) {
		hFontBold = CreateFont(
			-MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72),
			0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"MS Shell Dlg 2");
	}

	switch (GetWindowLongPtr(hWndCtl, GWLP_ID)) {
	case IDHDRTEXT:
		SelectObject(hDC, hFontBold);
		// fall through
	case IDHDRSUBTEXT:
	case IDS11CHANGELOG:
		// Note: don't use hollow brush, that causes glitches when you change the text
		// in a text control.
		return GetStockObject(WHITE_BRUSH);
	}

	return FALSE;
}

// show the appropriate folder picker interface and return path to the directory
LPCWSTR PickFolder(
	IN	HWND	hWndOwner,
	IN	LPCWSTR	lpszDefaultValue OPTIONAL)
{
	static WCHAR szDirPath[MAX_PATH];
	IFileDialog *pfd = NULL;
	HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, &pfd);

	if (!lpszDefaultValue) {
		lpszDefaultValue = L"";
	}

	if (SUCCEEDED(hr)) {
		// we can use the vista+ folder picker
		IShellItem *psi = NULL;
		LPWSTR lpszShellName;
		DWORD dwFlags;

		IFileDialog_GetOptions(pfd, &dwFlags);
		IFileDialog_SetOptions(pfd, dwFlags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOREADONLYRETURN);
		IFileDialog_Show(pfd, hWndOwner);
		IFileDialog_GetResult(pfd, &psi);
		IFileDialog_Release(pfd);

		if (psi) {
			IShellItem_GetDisplayName(psi, SIGDN_FILESYSPATH, &lpszShellName);
			wcscpy_s(szDirPath, ARRAYSIZE(szDirPath), lpszShellName);
			CoTaskMemFree(lpszShellName);
			IShellItem_Release(psi);
		} else {
			return lpszDefaultValue;
		}
	} else {
		// display old style folder picker
		BROWSEINFO bi;
		LPITEMIDLIST lpIdl;

		bi.hwndOwner		= hWndOwner;
		bi.pidlRoot			= NULL;
		bi.pszDisplayName	= NULL;
		bi.lpszTitle		= L"Select the installation directory.";
		bi.ulFlags			= BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
		bi.lpfn				= NULL;
		bi.lParam			= 0;
		bi.iImage			= 0;

		CoInitialize(NULL);
		lpIdl = SHBrowseForFolder(&bi);

		if (lpIdl) {
			SHGetPathFromIDList(lpIdl, szDirPath);
			CoTaskMemFree(lpIdl);
		}

		CoUninitialize();

		if (!lpIdl) {
			return lpszDefaultValue;
		}
	}

	if (szDirPath[wcslen(szDirPath) - 1] != '\\') {
		wcscat_s(szDirPath, ARRAYSIZE(szDirPath), L"\\");
	}

	wcscat_s(szDirPath, ARRAYSIZE(szDirPath), L"VxKex");
	return szDirPath;
}

// If lParam is NULL, this function will add up resource size into g_dwDiskSpaceRequired
// Otherwise lParam will be treated as a LPCWSTR containing the installation directory
// and will install all the resources into that directory.
BOOL WINAPI KexEnumResources(
	IN	HMODULE	hModule OPTIONAL,
	IN	LPCWSTR	lpszType,
	IN	LPWSTR	lpszName,
	IN	LPARAM	lParam)
{
	HRSRC hResource;
	DWORD dwcbData;

	hResource = FindResource(hModule, lpszName, lpszType);
	dwcbData = SizeofResource(hModule, hResource);

	if (!lParam) {
		g_dwDiskSpaceRequired += dwcbData;
	} else {
		LPCWSTR lpszInstallDir = (LPCWSTR) lParam;
		LPVOID lpData = (LPVOID) LoadResource(hModule, hResource);
		WCHAR szFilePath[MAX_PATH];
		HANDLE hFile;
		BOOL bSuccess;
		DWORD dwcbWritten;

		swprintf_s(szFilePath, ARRAYSIZE(szFilePath), L"%s\\%s", lpszInstallDir, lpszName);
		hFile = CreateFile(szFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (!hFile) {
			CriticalErrorBoxF(L"Failed to create or open the file %s: %s\nThe system may be in an inconsistent state. Correct the error before trying again (run the installer again and select Repair).",
								szFilePath, GetLastErrorAsString());
		}

		bSuccess = WriteFile(hFile, lpData, dwcbData, &dwcbWritten, NULL);
		
		if (!bSuccess) {
			// most of the time this is caused by some app that has the DLLs opened.
			CriticalErrorBoxF(L"Failed to write the file %s: %s\nThe system may be in an inconsistent state. Restart the computer before trying again (run the installer again and select Repair).",
								szFilePath, GetLastErrorAsString());
		}

		NtClose(hFile);
	}

	return TRUE;
}

VOID SetScene(
	IN	HWND	hWnd,
	IN	INT		iScene)
{
	g_iScene = iScene;

	// hide everything
	ShowWindow(GetDlgItem(hWnd, IDPROGRESS), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS1GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS1RBUNINST), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS1RBREPAIR), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS1RBUPDATE), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS1CURRENTINFO), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS1INSTALLINFO), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS2GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS2DIRPATH), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS2BROWSE), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS2SPACEREQ), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS2SPACEAVAIL), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS3GUIDETEXT), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS4GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS4KEXCFG), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS4OPENGUIDE), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT2), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS5UNDERSTAND), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS6GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS7GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS8GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS9GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS10GUIDETEXT), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT2), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS11CHANGELOG), FALSE);

	ShowWindow(GetDlgItem(hWnd, IDS12GUIDETEXT), FALSE);
	ShowWindow(GetDlgItem(hWnd, IDS13GUIDETEXT), FALSE);

	SendDlgItemMessage(hWnd, IDPROGRESS, PBM_SETMARQUEE, TRUE, 0);
	EnableWindow(GetDlgItem(hWnd, IDBACK), TRUE);
	EnableWindow(GetDlgItem(hWnd, IDNEXT), TRUE);
	SetDlgItemText(hWnd, IDNEXT, L"&Next >");

	// only show what we want to show

	if (iScene == 1) {
		// operation selection
		WCHAR szCurrentInfo[35 + 15 + MAX_PATH];
		WCHAR szInstallInfo[64];

		SetDlgItemText(hWnd, IDHDRTEXT, L"Select Operation");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"VxKex is already installed. Choose what you want Setup to do:");
		ShowWindow(GetDlgItem(hWnd, IDS1GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBUNINST), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBREPAIR), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBUPDATE), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1CURRENTINFO), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1INSTALLINFO), TRUE);

		swprintf_s(szCurrentInfo, ARRAYSIZE(szCurrentInfo), L"Version %s is currently installed in %s",
				  g_lpszInstalledVersion, g_szInstalledKexDir);
		SetDlgItemText(hWnd, IDS1CURRENTINFO, szCurrentInfo);

		// If installer version is greater than installeD version, update is default action.
		// Otherwise, repair is default action.
		if (g_dwInstallerVersion > g_dwInstalledVersion) {
			EnableWindow(GetDlgItem(hWnd, IDS1RBUPDATE), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDS1RBREPAIR), FALSE);
			CheckDlgButton(hWnd, IDS1RBUPDATE, TRUE);
		} else {
			CheckDlgButton(hWnd, IDS1RBREPAIR, TRUE);
		}

		swprintf_s(szInstallInfo, ARRAYSIZE(szInstallInfo), L"The installer version is %s (Build: %s, %s)",
				   g_lpszInstallerVersion, L(__DATE__), L(__TIME__));
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		SetDlgItemText(hWnd, IDS1INSTALLINFO, szInstallInfo);
	} else if (iScene == 2) {
		// install -> select installation directory
		// not shown during "repair/reinstall" or "update" because we already know kex dir in that case
		WCHAR szInstallDir[MAX_PATH];
		WCHAR szFormattedSize[16];
		WCHAR szLabelStr[21 + ARRAYSIZE(szFormattedSize)] = L"Disk space required: ";
		GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));

		if (!(*szInstallDir)) {
			ExpandEnvironmentStrings(L"%PROGRAMFILES%\\VxKex", szInstallDir, ARRAYSIZE(szInstallDir));
		}

		SetDlgItemText(hWnd, IDHDRTEXT, L"Choose Install Location");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Choose where you want VxKex to install files.");
		ShowWindow(GetDlgItem(hWnd, IDS2GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2DIRPATH), TRUE);
		SetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir);
		ShowWindow(GetDlgItem(hWnd, IDS2BROWSE), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2SPACEREQ), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2SPACEAVAIL), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		SetDlgItemText(hWnd, IDNEXT, L"&Install >");

		EnumResourceNames(NULL, RT_RCDATA, KexEnumResources, 0);
		StrFormatByteSize(g_dwDiskSpaceRequired, szFormattedSize, ARRAYSIZE(szFormattedSize));
		wcscat_s(szLabelStr, ARRAYSIZE(szLabelStr), szFormattedSize);
		SetDlgItemText(hWnd, IDS2SPACEREQ, szLabelStr);
	} else if (iScene == 3) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Installing...");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Installation is now in progress.");
		ShowWindow(GetDlgItem(hWnd, IDS3GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 4) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Installation Complete");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"The components installed are now ready for use.");
		ShowWindow(GetDlgItem(hWnd, IDS4GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS4KEXCFG), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS4OPENGUIDE), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Finish");
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 5) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Uninstall VxKex");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Review the information below, and then click Uninstall");
		ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT2), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS5UNDERSTAND), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Uninstall >");
		Button_SetCheck(GetDlgItem(hWnd, IDS5UNDERSTAND), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 6) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Uninstalling...");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Uninstallation is now in progress.");
		ShowWindow(GetDlgItem(hWnd, IDS6GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 7) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Uninstallation Complete");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"The components have been completely removed from your computer.");
		ShowWindow(GetDlgItem(hWnd, IDS7GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Finish");
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 8) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Repair VxKex");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Review the information below, and then click Repair");
		ShowWindow(GetDlgItem(hWnd, IDS8GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Repair");
	} else if (iScene == 9) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Repairing...");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Repair is now in progress.");
		ShowWindow(GetDlgItem(hWnd, IDS9GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 10) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Repair Complete");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"");
		ShowWindow(GetDlgItem(hWnd, IDS10GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Finish");
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 11) {
		HRSRC hChangeLog = FindResource(NULL, L"ChangeLog.txt", (LPCWSTR) RCTEXT);
		SIZE_T cchChangeLog = SizeofResource(NULL, hChangeLog) / 2;
		LPWSTR lpszChangeLog = (LPWSTR) LoadResource(NULL, hChangeLog);

		//
		// If you get access violations or gibberish displayed here, make sure that
		// ChangeLog.txt ends with two null bytes '\0'. You have probably messed them up
		// with some text editor. Notepad, for example, converts them to spaces when you
		// save the file. Use the Visual Studio text editor which preserves them.
		//
		
		SetDlgItemText(hWnd, IDHDRTEXT, L"Update VxKex");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Review the information below, and then click Update");
		ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT2), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS11CHANGELOG), TRUE);
		SetDlgItemText(hWnd, IDS11CHANGELOG, lpszChangeLog);
		SetDlgItemText(hWnd, IDNEXT, L"&Update");
	} else if (iScene == 12) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Updating...");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"Update is now in progress.");
		ShowWindow(GetDlgItem(hWnd, IDS12GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 13) {
		SetDlgItemText(hWnd, IDHDRTEXT, L"Update Complete");
		SetDlgItemText(hWnd, IDHDRSUBTEXT, L"");
		ShowWindow(GetDlgItem(hWnd, IDS13GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, L"&Finish");
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	}
}

VOID UpdateDiskFreeSpace(
	IN	HWND	hWnd)
{
	WCHAR szInstallDir[MAX_PATH];
	WCHAR szVolumeMnt[MAX_PATH];
	WCHAR szFormattedSize[16];
	WCHAR szLabelStr[22 + ARRAYSIZE(szFormattedSize)] = L"Disk space available: ";
	ULARGE_INTEGER uliFreeSpace;

	GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));
	GetVolumePathName(szInstallDir, szVolumeMnt, ARRAYSIZE(szVolumeMnt));

	if (GetDiskFreeSpaceEx(szVolumeMnt, &uliFreeSpace, NULL, NULL)) {
		if (StrFormatByteSize(uliFreeSpace.QuadPart, szFormattedSize, ARRAYSIZE(szFormattedSize))) {
			wcscat_s(szLabelStr, ARRAYSIZE(szLabelStr), szFormattedSize);
			SetDlgItemText(hWnd, IDS2SPACEAVAIL, szLabelStr);
			return;
		}
	}

	// dont display anything if the directory is invalid
	SetDlgItemText(hWnd, IDS2SPACEAVAIL, L"");
}

BOOL TerminateProcessByImageName(
	LPCWSTR lpszImageName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 psEntry;

	psEntry.dwSize = sizeof(psEntry);

	if (hSnapshot == INVALID_HANDLE_VALUE || !Process32First(hSnapshot, &psEntry)) {
		goto ErrorReturn;
	}

	do {
		if (!lstrcmpi(psEntry.szExeFile, lpszImageName)) {
			HANDLE hProc = OpenProcess(PROCESS_TERMINATE, 0, psEntry.th32ProcessID);

			if (!hProc) {
				NtClose(hProc);
				goto ErrorReturn;
			}

			NtTerminateProcess(hProc, 0);
		}
	} while (Process32Next(hSnapshot, &psEntry));

	NtClose(hSnapshot);
	return TRUE;

ErrorReturn:
	NtClose(hSnapshot);
	return FALSE;
}

// VxKex must still be installed for this function to succeed.
// Namely, the "KexDir" registry entry must still exist.
VOID KexShlExDllInstall(
	IN	BOOL	bInstall)
{
	WCHAR szKexShlEx[MAX_PATH];
	HMODULE hKexShlEx;
	HRESULT (STDAPICALLTYPE *lpfnKexShlExDllInstall)(BOOL, LPCWSTR);

	RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexShlEx, ARRAYSIZE(szKexShlEx));
	wcscat_s(szKexShlEx, ARRAYSIZE(szKexShlEx), L"\\KexShlEx.dll");
	hKexShlEx = LoadLibrary(szKexShlEx);

	if (hKexShlEx) {
		lpfnKexShlExDllInstall = (HRESULT (STDAPICALLTYPE *)(BOOL, LPCWSTR)) GetProcAddress(hKexShlEx, "DllInstall");

		if (lpfnKexShlExDllInstall) {
			lpfnKexShlExDllInstall(bInstall, NULL);
		}

		FreeLibrary(hKexShlEx);
	}
}

#define ADD_UNINSTALL_VALUE(func, value, data) func(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex", value, data)
#define ADD_DLL_REWRITE_ENTRY(original, rewrite) RegWriteSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite", original, rewrite)

BOOL KexInstall(
	IN	HWND	hWnd OPTIONAL,
	IN	LPWSTR	lpszInstallDir)
{
	PathRemoveBackslash(lpszInstallDir);

	// populate the base entries under HKLM\\SOFTWARE\\VXsoft\\VxKexLdr
	RegWriteSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", lpszInstallDir);
	RegWriteDw(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"InstalledVersion", g_dwInstallerVersion);

	// create uninstall entries so control panel can uninstall
	{
		WCHAR szFormattedDate[9]; // YYYYMMDD\0
		WCHAR szUninstallString[MAX_PATH + 10];
		WCHAR szModifyString[MAX_PATH];
		GetDateFormat(LOCALE_INVARIANT, 0, NULL, L"yyyyMMdd", szFormattedDate, ARRAYSIZE(szFormattedDate));
		swprintf_s(szUninstallString, ARRAYSIZE(szUninstallString), L"%s\\KexSetup.exe /UNINSTALL", lpszInstallDir);
		swprintf_s(szModifyString, ARRAYSIZE(szModifyString), L"%s\\KexSetup.exe", lpszInstallDir);

		ADD_UNINSTALL_VALUE(RegWriteSz, L"DisplayName", L"VxKex API Extensions for Windows 7");
		ADD_UNINSTALL_VALUE(RegWriteSz, L"DisplayVersion", g_lpszInstallerVersion);
		ADD_UNINSTALL_VALUE(RegWriteSz, L"Publisher", L"VXsoft");
		ADD_UNINSTALL_VALUE(RegWriteSz, L"InstallDate", szFormattedDate);
		ADD_UNINSTALL_VALUE(RegWriteSz, L"InstallLocation", lpszInstallDir);
		ADD_UNINSTALL_VALUE(RegWriteSz, L"HelpLink", L"https://github.com/vxiiduu/VxKex");
		ADD_UNINSTALL_VALUE(RegWriteSz, L"UninstallString", szUninstallString);
		ADD_UNINSTALL_VALUE(RegWriteSz, L"ModifyPath", szModifyString);
		ADD_UNINSTALL_VALUE(RegWriteSz, L"Size", L"");
		ADD_UNINSTALL_VALUE(RegWriteDw, L"EstimatedSize", g_dwDiskSpaceRequired / 1024);
		ADD_UNINSTALL_VALUE(RegWriteDw, L"NoRepair", 1);
	}

	#include "dllrewrt.h"

	// create KexDir and subdirectories if it doesn't exist, and copy self-EXE to it
	{
		WCHAR szSelfPath[MAX_PATH];
		WCHAR szDestPath[MAX_PATH];
		WCHAR szKex3264[MAX_PATH];
		CreateDirectory(lpszInstallDir, NULL);
		swprintf_s(szKex3264, ARRAYSIZE(szKex3264), L"%s\\Kex32", lpszInstallDir);
		CreateDirectory(szKex3264, NULL);
#ifdef _WIN64
		swprintf_s(szKex3264, ARRAYSIZE(szKex3264), L"%s\\Kex64", lpszInstallDir);
		CreateDirectory(szKex3264, NULL);
#endif
		GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath));
		swprintf_s(szDestPath, ARRAYSIZE(szDestPath), L"%s\\KEXSETUP.EXE", lpszInstallDir);
		CopyFile(szSelfPath, szDestPath, FALSE);
	}

	// copy VxKex files into KexDir
	if (EnumResourceNames(NULL, RT_RCDATA, KexEnumResources, (LPARAM) lpszInstallDir) == FALSE) {
		CriticalErrorBoxF(L"Unable to enumerate resources: %s\nThe system may be in an inconsistent state. Correct the error before trying again (run the installer again and select Repair).",
						  GetLastErrorAsString());
	}

	// register shell extension
	KexShlExDllInstall(TRUE);
	
	return TRUE;
}

BOOL KexUninstall(
	IN	HWND	hWnd OPTIONAL,
	IN	BOOL	bPreserveSettings)
{
	WCHAR szKexDir[MAX_PATH];

	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexDir, ARRAYSIZE(szKexDir))) {
		return FALSE;
	}

	// delete user settings if we need to do that
	if (!bPreserveSettings) {
		HKEY hKeyIfeo;
		WCHAR szRegKey[MAX_PATH];
		WCHAR szVxKexLdr[MAX_PATH];
		DWORD dwIdx = 0;

		swprintf_s(szVxKexLdr, ARRAYSIZE(szVxKexLdr), L"%s\\VxKexLdr.exe", szKexDir);
		RegOpenKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options", &hKeyIfeo);
		
		while (RegEnumKey(hKeyIfeo, dwIdx++, szRegKey, ARRAYSIZE(szRegKey)) == ERROR_SUCCESS) {
			WCHAR szRegData[MAX_PATH];
			
			if (RegReadSz(hKeyIfeo, szRegKey, L"Debugger", szRegData, ARRAYSIZE(szRegData))) {
				if (!wcsicmp(szRegData, szVxKexLdr)) {
					RegDelValue(hKeyIfeo, szRegKey, L"Debugger");
				}
			}
		}

		RegCloseKey(hKeyIfeo);
		SHDeleteKey(HKEY_CURRENT_USER, L"SOFTWARE\\VXsoft\\VxKexLdr");
	}

	// unregister property sheet shell extension and extended context menu entry
	KexShlExDllInstall(FALSE);

	// delete KexDir\Kex32 and KexDir\Kex64 recursively as well as KexComponents files
	// DO NOT remove any entries from here even if they "don't exist", because they might
	// be present from previous installs of previous versions.
	{
		WCHAR szDeletionTarget[MAX_PATH + 1];
		SHFILEOPSTRUCTW shop;

		shop.hwnd		= hWnd;
		shop.wFunc		= FO_DELETE;
		shop.pFrom		= szDeletionTarget;
		shop.pTo		= NULL;
		shop.fFlags		= FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\Kex32", szKexDir);
		szDeletionTarget[wcslen(szDeletionTarget) + 1] = L'\0';
		SHFileOperation(&shop);
#ifdef _WIN64
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\Kex64", szKexDir);
		szDeletionTarget[wcslen(szDeletionTarget) + 1] = L'\0';
		SHFileOperation(&shop);
#endif

		// delete PDB files as well
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\VxKexLdr.pdb", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexCfg.pdb", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\CmdSus32.pdb", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\CmdSus64.pdb", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexShlEx.pdb", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexSetup.pdb", szKexDir);
		DeleteFile(szDeletionTarget);

		TerminateProcessByImageName(L"VxKexLdr.exe");
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\VxKexLdr.exe", szKexDir);
		DeleteFile(szDeletionTarget);
		TerminateProcessByImageName(L"KexCfg.exe");
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexCfg.exe", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\CmdSus32.dll", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\CmdSus64.dll", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\CmdSus32.exe", szKexDir);
		DeleteFile(szDeletionTarget);
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexShlEx.dll", szKexDir);
		
		if (!DeleteFile(szDeletionTarget)) {
			// It may be loaded by explorer.exe so we have to kill and restart Explorer.

			if (TerminateProcessByImageName(L"explorer.exe")) {
				// Explorer appears to restart itself when killed in this way. Starting it again
				// will open an unwanted Explorer window.
				Sleep(500);
				DeleteFile(szDeletionTarget); // and hopefully it works now
			}
		}
		
		swprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), L"%s\\KexSetup.exe", szKexDir);

		if (!DeleteFile(szDeletionTarget)) {
			// If we are executing from the KexSetup.exe in KexDir, we can't just delete that.
			// So we just schedule it to be removed when the system reboots.
			MoveFileEx(szDeletionTarget, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		}

		// also delete KexDir if empty, following the same treatment as above
		if (!RemoveDirectory(szKexDir)) {
				MoveFileEx(szKexDir, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
		}
	}

	// Do this as the penultimate step since the absence of these keys will prevent future uninstall attempts.
	SHDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex");
	SHDeleteKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr");

	return TRUE;
}

DWORD WINAPI InstallThreadProc(
	IN	LPVOID	lpParam OPTIONAL)
{
	WCHAR szInstallDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));
	KexInstall(hWnd, szInstallDir);
	SetScene(hWnd, 4);
	NtClose(g_hWorkThread);
	g_hWorkThread = NULL;

	if (lpParam) {
		ExitThread(0);
	} else {
		ExitProcess(0);
	}
}

DWORD WINAPI UninstallThreadProc(
	IN	LPVOID	lpParam OPTIONAL)
{
	HWND hWnd = (HWND) lpParam;
	KexUninstall(hWnd, FALSE);
	SetScene(hWnd, 7);
	NtClose(g_hWorkThread);
	g_hWorkThread = NULL;

	if (lpParam) {
		ExitThread(0);
	} else {
		ExitProcess(0);
	}
}

DWORD WINAPI RepairThreadProc(
	IN	LPVOID	lpParam OPTIONAL)
{
	WCHAR szKexDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexDir, ARRAYSIZE(szKexDir));
	KexUninstall(hWnd, TRUE);
	KexInstall(hWnd, szKexDir);
	SetScene(hWnd, 10);
	NtClose(g_hWorkThread);
	g_hWorkThread = NULL;

	if (lpParam) {
		ExitThread(0);
	} else {
		ExitProcess(0);
	}
}

DWORD WINAPI UpdateThreadProc(
	IN	LPVOID	lpParam OPTIONAL)
{
	WCHAR szKexDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexDir, ARRAYSIZE(szKexDir));
	KexUninstall(hWnd, TRUE);
	KexInstall(hWnd, szKexDir);
	SetScene(hWnd, 13);
	NtClose(g_hWorkThread);
	g_hWorkThread = NULL;

	if (lpParam) {
		ExitThread(0);
	} else {
		ExitProcess(0);
	}
}

//
// Dialog functions
//

INT_PTR CALLBACK DlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	if (uMsg == WM_INITDIALOG) {
		if (!g_iScene) {
			if (*g_szInstalledKexDir) {
				SetScene(hWnd, 1);
			} else {
				SetScene(hWnd, 2);
			}
		} else {
			SetScene(hWnd, g_iScene);
		}

		return TRUE;
	} else if (uMsg == WM_COMMAND) {
		if (LOWORD(wParam) == IDCANCEL) {
			if (g_hWorkThread) {
				SuspendThread(g_hWorkThread);
				
				if (MessageBox(hWnd, L"Installation or removal is in progress. If you cancel now, the software may be left in an inconsistent state. Cancel anyway?",
							   L"Confirm Cancellation", MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL) == IDYES) {
					EndDialog(hWnd, 0);
				} else {
					ResumeThread(g_hWorkThread);
				}
			} else if (g_iScene == 1 || g_iScene == 4 || g_iScene == 5 || g_iScene == 7) {
				EndDialog(hWnd, 0);
			} else {
				if (MessageBox(hWnd, L"Are you sure you want to cancel Setup?", L"Confirm Cancellation",
							   MB_ICONINFORMATION | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL) == IDYES) {
					EndDialog(hWnd, 0);
				}
			}
		} else if (LOWORD(wParam) == IDBACK) {
			switch (g_iScene) {
			case 5:
			case 8:
			case 11:
				SetScene(hWnd, 1);
			}
		} else if (LOWORD(wParam) == IDNEXT) {
			switch (g_iScene) {
			case 1:
				// set scene based on radio button selection
				if (IsDlgButtonChecked(hWnd, IDS1RBUNINST)) {
					SetScene(hWnd, 5);
				} else if (IsDlgButtonChecked(hWnd, IDS1RBREPAIR)) {
					SetScene(hWnd, 8);
				} else if (IsDlgButtonChecked(hWnd, IDS1RBUPDATE)) {
					SetScene(hWnd, 11);
				}

				break;
			case 2:
				SetScene(hWnd, 3);
				g_hWorkThread = CreateThread(NULL, 0, InstallThreadProc, hWnd, 0, NULL);
				break;
			case 5:
				SetScene(hWnd, 6);
				g_hWorkThread = CreateThread(NULL, 0, UninstallThreadProc, hWnd, 0, NULL);
				break;
			case 8:
				SetScene(hWnd, 9);
				g_hWorkThread = CreateThread(NULL, 0, RepairThreadProc, hWnd, 0, NULL);
				break;
			case 11:
				SetScene(hWnd, 12);
				g_hWorkThread = CreateThread(NULL, 0, UpdateThreadProc, hWnd, 0, NULL);
				break;

			case 4: // fallthrough
			case 7:
			case 10:
			case 13:
				EndDialog(hWnd, 0);
				break;
			}
		} else if (LOWORD(wParam) == IDS2BROWSE) {
			WCHAR szOldValue[MAX_PATH];
			GetDlgItemText(hWnd, IDS2DIRPATH, szOldValue, ARRAYSIZE(szOldValue));
			SetDlgItemText(hWnd, IDS2DIRPATH, PickFolder(hWnd, szOldValue));
		} else if (LOWORD(wParam) == IDS2DIRPATH) {
			UpdateDiskFreeSpace(hWnd);
		} else if (LOWORD(wParam) == IDS5UNDERSTAND) {
			EnableWindow(GetDlgItem(hWnd, IDNEXT), IsDlgButtonChecked(hWnd, LOWORD(wParam)));
		}
		
		return TRUE;
	} else if (uMsg == WM_CTLCOLORSTATIC) {
		return (INT_PTR) SetStaticCtlBk(hWnd, (HWND) lParam, (HDC) wParam);
	}

	return FALSE;
}

BOOL IsWow64(
	VOID)
{
	BOOL bWow64;
	BOOL (WINAPI *lpfnIsWow64Process) (HANDLE, LPBOOL) =
		(BOOL (WINAPI *) (HANDLE, LPBOOL)) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "IsWow64Process");

	if (lpfnIsWow64Process) {
		if (lpfnIsWow64Process(GetCurrentProcess(), &bWow64) && bWow64) {
			return TRUE;
		}
	}
	
	return FALSE;
}

INT APIENTRY wWinMain(
	IN	HINSTANCE	hInstance,
	IN	HINSTANCE	hPrevInstance,
	IN	LPWSTR		lpszCmdLine,
	IN	INT			iCmdShow)
{
	BOOL bSilentUnattend = FALSE;
	SetFriendlyAppName(FRIENDLYAPPNAME);

#if !defined(_WIN64) && !defined(_DEBUG)
	if (IsWow64()) {
		CriticalErrorBoxF(L"Installing the 32-bit version of the software on a 64-bit system is not recommended. Please use the 64-bit installer instead.");
	}
#endif

	ElevateIfNotElevated(lpszCmdLine, iCmdShow);

	g_dwInstalledVersion = GetInstalledVersion();
	g_dwInstallerVersion = GetInstallerVersion();
	g_lpszInstalledVersion = GetInstalledVersionAsString();
	g_lpszInstallerVersion = GetInstallerVersionAsString();
	RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", g_szInstalledKexDir, ARRAYSIZE(g_szInstalledKexDir));

	// SILENTUNATTEND means don't display all the fancy GUI boxes and shit.
	// Error boxes may still be displayed.
	bSilentUnattend = !!StrStrI(lpszCmdLine, L"/SILENTUNATTEND");

	if (StrStrI(lpszCmdLine, L"/UNINSTALL")) {
		if (bSilentUnattend) {
			UninstallThreadProc(NULL);
		} else {
			g_iScene = 5;
		}
	} else if (StrStrI(lpszCmdLine, L"/REPAIR")) {
		if (bSilentUnattend) {
			RepairThreadProc(NULL);
		} else {
			g_iScene = 8;
		}
	}

	InitCommonControls();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	ExitProcess(0);
}