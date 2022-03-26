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
#include "resource.h"

#define APPNAME T("KexSetup")
#define FRIENDLYAPPNAME T("VxKex Setup")

INT g_iScene = 0;

LPCTSTR g_lpszInstallerVersion = T(KEX_VERSION_STR);
DWORD g_dwInstallerVersion;
LPCTSTR g_lpszInstalledVersion;
DWORD g_dwInstalledVersion;
TCHAR g_szInstalledKexDir[MAX_PATH] = T("");
DWORD g_dwDiskSpaceRequired = 0;

HANDLE g_hWorkThread = NULL;

//
// Worker functions
//

VOID ElevateIfNotElevated(
	IN	LPCTSTR	lpszCmdLine,
	IN	INT		iCmdShow)
{
	// Apparently using the manifest to do this can cause bluescreens on XP if you don't
	// do it properly. Better safe than sorry.
	if (!IsUserAnAdmin()) {
		if (LOBYTE(LOWORD(GetVersion())) >= 6) {
			TCHAR szSelfPath[MAX_PATH];
			GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath));
			ShellExecute(NULL, T("runas"), szSelfPath, lpszCmdLine, NULL, iCmdShow);
			ExitProcess(0);
		} else {
			CriticalErrorBoxF(T("You need to be an Administrator to run %s."), APPNAME);
		}
	}
}

DWORD GetInstalledVersion(
	VOID)
{
	DWORD dwResult;

	if (!RegReadDw(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("InstalledVersion"), &dwResult)) {
		dwResult = 0;
	}

	return dwResult;
}

LPCTSTR GetInstalledVersionAsString(
	VOID)
{
	static TCHAR szVersion[16]; // "255.255.255.255\0"
	DWORD dwVersion = GetInstalledVersion();
	sprintf_s(szVersion, ARRAYSIZE(szVersion), T("%hhu.%hhu.%hhu.%hhu"),
			  HIBYTE(HIWORD(dwVersion)), LOBYTE(HIWORD(dwVersion)),
			  HIBYTE(LOWORD(dwVersion)), LOBYTE(LOWORD(dwVersion)));
	return szVersion;
}

DWORD GetVersionFromString(
	IN	LPCTSTR	lpszVersion)
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

	sscanf_s(lpszVersion, T("%hhu.%hhu.%hhu.%hhu"), &u.bDigit1, &u.bDigit2, &u.bDigit3, &u.bDigit4);
	return u.dwVersion;
}

INLINE LPCTSTR GetInstallerVersionAsString(
	VOID)
{
	return T(KEX_VERSION_STR);
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
			T("MS Shell Dlg 2"));
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
LPCTSTR PickFolder(
	IN	HWND	hWndOwner,
	IN	LPCTSTR	lpszDefaultValue OPTIONAL)
{
	static TCHAR szDirPath[MAX_PATH];
	IFileDialog *pfd = NULL;
	HRESULT hr = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, &pfd);

	if (!lpszDefaultValue) {
		lpszDefaultValue = T("");
	}

	if (SUCCEEDED(hr)) {
		// we can use the vista+ folder picker
		IShellItem *psi = NULL;
		LPTSTR lpszShellName;
		DWORD dwFlags;

		IFileDialog_GetOptions(pfd, &dwFlags);
		IFileDialog_SetOptions(pfd, dwFlags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_NOREADONLYRETURN);
		IFileDialog_Show(pfd, hWndOwner);
		IFileDialog_GetResult(pfd, &psi);
		IFileDialog_Release(pfd);

		if (psi) {
			IShellItem_GetDisplayName(psi, SIGDN_FILESYSPATH, &lpszShellName);
			strcpy_s(szDirPath, ARRAYSIZE(szDirPath), lpszShellName);
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
		bi.lpszTitle		= T("Select the installation directory.");
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

	if (szDirPath[strlen(szDirPath) - 1] != '\\') {
		strcat_s(szDirPath, ARRAYSIZE(szDirPath), T("\\"));
	}

	strcat_s(szDirPath, ARRAYSIZE(szDirPath), T("VxKex"));
	return szDirPath;
}

// If lParam is NULL, this function will add up resource size into g_dwDiskSpaceRequired
// Otherwise lParam will be treated as a LPCTSTR containing the installation directory
// and will install all the resources into that directory.
BOOL WINAPI KexEnumResources(
	IN	HMODULE	hModule OPTIONAL,
	IN	LPCTSTR	lpszType,
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
		LPCTSTR lpszInstallDir = (LPCTSTR) lParam;
		LPVOID lpData = (LPVOID) LoadResource(hModule, hResource);
		TCHAR szFilePath[MAX_PATH];
		HANDLE hFile;
		BOOL bSuccess;
		DWORD dwcbWritten;

		sprintf_s(szFilePath, ARRAYSIZE(szFilePath), T("%s\\%s"), lpszInstallDir, lpszName);
		hFile = CreateFile(szFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (!hFile) {
			CriticalErrorBoxF(T("Failed to create or open the file %s: %s\nThe system may be in an inconsistent state. Correct the error before trying again (run the installer again and select Repair)."),
								szFilePath, GetLastErrorAsString());
		}

		bSuccess = WriteFile(hFile, lpData, dwcbData, &dwcbWritten, NULL);
		
		if (!bSuccess) {
			// most of the time this is caused by some app that has the DLLs opened.
			CriticalErrorBoxF(T("Failed to write the file %s: %s\nThe system may be in an inconsistent state. Restart the computer before trying again (run the installer again and select Repair)."),
								szFilePath, GetLastErrorAsString());
		}

		CloseHandle(hFile);
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
	SetDlgItemText(hWnd, IDNEXT, T("&Next >"));

	// only show what we want to show

	if (iScene == 1) {
		// operation selection
		TCHAR szCurrentInfo[35 + 15 + MAX_PATH];
		TCHAR szInstallInfo[25 + 15 + 1];

		SetDlgItemText(hWnd, IDHDRTEXT, T("Select Operation"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("VxKex is already installed. Choose what you want Setup to do:"));
		ShowWindow(GetDlgItem(hWnd, IDS1GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBUNINST), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBREPAIR), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1RBUPDATE), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1CURRENTINFO), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS1INSTALLINFO), TRUE);

		sprintf_s(szCurrentInfo, ARRAYSIZE(szCurrentInfo), T("Version %s is currently installed in %s"),
				  g_lpszInstalledVersion, g_szInstalledKexDir);
		SetDlgItemText(hWnd, IDS1CURRENTINFO, szCurrentInfo);

		if (g_dwInstallerVersion > g_dwInstalledVersion) {
			EnableWindow(GetDlgItem(hWnd, IDS1RBUPDATE), TRUE);
		}

		sprintf_s(szInstallInfo, ARRAYSIZE(szInstallInfo), T("The installer version is %s"), g_lpszInstallerVersion);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);

		if (!(IsDlgButtonChecked(hWnd, IDS1RBUNINST) || IsDlgButtonChecked(hWnd, IDS1RBREPAIR) || IsDlgButtonChecked(hWnd, IDS1RBUPDATE))) {
			EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
		}
		SetDlgItemText(hWnd, IDS1INSTALLINFO, szInstallInfo);
	} else if (iScene == 2) {
		// install -> select installation directory
		// not shown during "repair/reinstall" or "update" because we already know kex dir in that case
		TCHAR szInstallDir[MAX_PATH];
		TCHAR szFormattedSize[16];
		TCHAR szLabelStr[21 + ARRAYSIZE(szFormattedSize)] = T("Disk space required: ");
		GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));

		if (!(*szInstallDir)) {
			ExpandEnvironmentStrings(T("%PROGRAMFILES%\\VxKex"), szInstallDir, ARRAYSIZE(szInstallDir));
		}

		SetDlgItemText(hWnd, IDHDRTEXT, T("Choose Install Location"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Choose where you want VxKex to install files."));
		ShowWindow(GetDlgItem(hWnd, IDS2GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2DIRPATH), TRUE);
		SetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir);
		ShowWindow(GetDlgItem(hWnd, IDS2BROWSE), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2SPACEREQ), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS2SPACEAVAIL), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		SetDlgItemText(hWnd, IDNEXT, T("&Install >"));

		EnumResourceNames(NULL, RT_RCDATA, KexEnumResources, 0);
		StrFormatByteSize(g_dwDiskSpaceRequired, szFormattedSize, ARRAYSIZE(szFormattedSize));
		strcat_s(szLabelStr, ARRAYSIZE(szLabelStr), szFormattedSize);
		SetDlgItemText(hWnd, IDS2SPACEREQ, szLabelStr);
	} else if (iScene == 3) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Installing..."));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Installation is now in progress."));
		ShowWindow(GetDlgItem(hWnd, IDS3GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 4) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Installation Complete"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("The components installed are now ready for use."));
		ShowWindow(GetDlgItem(hWnd, IDS4GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS4KEXCFG), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS4OPENGUIDE), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Finish"));
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 5) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Uninstall VxKex"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Review the information below, and then click Uninstall"));
		ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS5GUIDETEXT2), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS5UNDERSTAND), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Uninstall >"));
		Button_SetCheck(GetDlgItem(hWnd, IDS5UNDERSTAND), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 6) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Uninstalling..."));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Uninstallation is now in progress."));
		ShowWindow(GetDlgItem(hWnd, IDS6GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 7) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Uninstallation Complete"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("The components have been completely removed from your computer."));
		ShowWindow(GetDlgItem(hWnd, IDS7GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Finish"));
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 8) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Repair VxKex"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Review the information below, and then click Repair"));
		ShowWindow(GetDlgItem(hWnd, IDS8GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Repair"));
	} else if (iScene == 9) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Repairing..."));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Repair is now in progress."));
		ShowWindow(GetDlgItem(hWnd, IDS9GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 10) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Repair Complete"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T(""));
		ShowWindow(GetDlgItem(hWnd, IDS10GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Finish"));
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	} else if (iScene == 11) {
		HRSRC hChangeLog = FindResource(NULL, T("ChangeLog.txt"), (LPCWSTR) RCTEXT);
		LPWSTR lpszChangeLog = (LPWSTR) LoadResource(NULL, hChangeLog);
		lpszChangeLog[SizeofResource(NULL, hChangeLog)] = '\0';
		
		SetDlgItemText(hWnd, IDHDRTEXT, T("Update VxKex"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Review the information below, and then click Update"));
		ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS11GUIDETEXT2), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDS11CHANGELOG), TRUE);
		SetDlgItemText(hWnd, IDS11CHANGELOG, lpszChangeLog);
		SetDlgItemText(hWnd, IDNEXT, T("&Update"));
	} else if (iScene == 12) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Updating..."));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T("Update is now in progress."));
		ShowWindow(GetDlgItem(hWnd, IDS12GUIDETEXT), TRUE);
		ShowWindow(GetDlgItem(hWnd, IDPROGRESS), TRUE);
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
		EnableWindow(GetDlgItem(hWnd, IDNEXT), FALSE);
	} else if (iScene == 13) {
		SetDlgItemText(hWnd, IDHDRTEXT, T("Update Complete"));
		SetDlgItemText(hWnd, IDHDRSUBTEXT, T(""));
		ShowWindow(GetDlgItem(hWnd, IDS13GUIDETEXT), TRUE);
		SetDlgItemText(hWnd, IDNEXT, T("&Finish"));
		EnableWindow(GetDlgItem(hWnd, IDBACK), FALSE);
	}
}

VOID UpdateDiskFreeSpace(
	IN	HWND	hWnd)
{
	TCHAR szInstallDir[MAX_PATH];
	TCHAR szVolumeMnt[MAX_PATH];
	TCHAR szFormattedSize[16];
	TCHAR szLabelStr[22 + ARRAYSIZE(szFormattedSize)] = T("Disk space available: ");
	ULARGE_INTEGER uliFreeSpace;

	GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));
	GetVolumePathName(szInstallDir, szVolumeMnt, ARRAYSIZE(szVolumeMnt));

	if (GetDiskFreeSpaceEx(szVolumeMnt, &uliFreeSpace, NULL, NULL)) {
		if (StrFormatByteSize(uliFreeSpace.QuadPart, szFormattedSize, ARRAYSIZE(szFormattedSize))) {
			strcat_s(szLabelStr, ARRAYSIZE(szLabelStr), szFormattedSize);
			SetDlgItemText(hWnd, IDS2SPACEAVAIL, szLabelStr);
			return;
		}
	}

	// dont display anything if the directory is invalid
	SetDlgItemText(hWnd, IDS2SPACEAVAIL, T(""));
}

BOOL TerminateProcessByImageName(
	LPCTSTR lpszImageName)
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
				CloseHandle(hProc);
				goto ErrorReturn;
			}

			TerminateProcess(hProc, 0);
		}
	} while (Process32Next(hSnapshot, &psEntry));

	CloseHandle(hSnapshot);
	return TRUE;

ErrorReturn:
	CloseHandle(hSnapshot);
	return FALSE;
}

// VxKex must still be installed for this function to succeed.
// Namely, the "KexDir" registry entry must still exist.
VOID KexShlExDllInstall(
	IN	BOOL	bInstall)
{
	TCHAR szKexShlEx[MAX_PATH];
	HMODULE hKexShlEx;
	HRESULT (STDAPICALLTYPE *lpfnKexShlExDllInstall)(BOOL, LPCWSTR);

	RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexShlEx, ARRAYSIZE(szKexShlEx));
	strcat_s(szKexShlEx, ARRAYSIZE(szKexShlEx), T("\\KexShlEx.dll"));
	hKexShlEx = LoadLibrary(szKexShlEx);

	if (hKexShlEx) {
		lpfnKexShlExDllInstall = (HRESULT (STDAPICALLTYPE *)(BOOL, LPCWSTR)) GetProcAddress(hKexShlEx, "DllInstall");

		if (lpfnKexShlExDllInstall) {
			lpfnKexShlExDllInstall(bInstall, NULL);
		}

		FreeLibrary(hKexShlEx);
	}
}

#define ADD_UNINSTALL_VALUE(func, value, data) func(HKEY_LOCAL_MACHINE, T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex"), value, data)
#define ADD_DLL_REWRITE_ENTRY(original, rewrite) RegWriteSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite"), original, rewrite)

BOOL KexInstall(
	IN	HWND	hWnd OPTIONAL,
	IN	LPTSTR	lpszInstallDir)
{
	PathRemoveBackslash(lpszInstallDir);

	// populate the base entries under HKLM\\SOFTWARE\\VXsoft\\VxKexLdr
	RegWriteSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), lpszInstallDir);
	RegWriteDw(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("InstalledVersion"), g_dwInstallerVersion);

	// create uninstall entries so control panel can uninstall
	{
		TCHAR szFormattedDate[9]; // YYYYMMDD\0
		TCHAR szUninstallString[MAX_PATH + 10];
		TCHAR szModifyString[MAX_PATH];
		GetDateFormat(LOCALE_INVARIANT, 0, NULL, T("yyyyMMdd"), szFormattedDate, ARRAYSIZE(szFormattedDate));
		sprintf_s(szUninstallString, ARRAYSIZE(szUninstallString), T("%s\\KexSetup.exe /UNINSTALL"), lpszInstallDir);
		sprintf_s(szModifyString, ARRAYSIZE(szModifyString), T("%s\\KexSetup.exe"), lpszInstallDir);

		ADD_UNINSTALL_VALUE(RegWriteSz, T("DisplayName"), T("VX Kernel Extension for Windows 7"));
		ADD_UNINSTALL_VALUE(RegWriteSz, T("DisplayVersion"), g_lpszInstallerVersion);
		ADD_UNINSTALL_VALUE(RegWriteSz, T("Publisher"), T("VXsoft"));
		ADD_UNINSTALL_VALUE(RegWriteSz, T("InstallDate"), szFormattedDate);
		ADD_UNINSTALL_VALUE(RegWriteSz, T("InstallLocation"), lpszInstallDir);
		ADD_UNINSTALL_VALUE(RegWriteSz, T("HelpLink"), T("https://github.com/vxiiduu/VxKex"));
		ADD_UNINSTALL_VALUE(RegWriteSz, T("UninstallString"), szUninstallString);
		ADD_UNINSTALL_VALUE(RegWriteSz, T("ModifyPath"), szModifyString);
		ADD_UNINSTALL_VALUE(RegWriteSz, T("Size"), T(""));
		ADD_UNINSTALL_VALUE(RegWriteDw, T("EstimatedSize"), g_dwDiskSpaceRequired / 1024);
		ADD_UNINSTALL_VALUE(RegWriteDw, T("NoRepair"), 1);
	}

	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-com-l1-1-0.dll"),						T("ole33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-com-l1-1-1.dll"),						T("ole33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-rtlsupport-l1-2-0.dll"),				T("ntdll.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-psapi-l1-1-0.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-sidebyside-l1-1-0.dll"),				T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-console-l2-1-0.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-datetime-l1-1-1.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-errorhandling-l1-1-1.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-fibers-l1-1-1.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-file-l1-2-2.dll"),						T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-file-l2-1-1.dll"),						T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-heap-obsolete-l1-1-0.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-heap-l1-2-0.dll"),						T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-heap-l2-1-0.dll"),						T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-io-l1-1-1.dll"),						T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-kernel32-legacy-l1-1-0.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-kernel32-legacy-l1-1-1.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-largeinteger-l1-1-0.dll"),				T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-libraryloader-l1-2-0.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-libraryloader-l1-2-1.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-localization-l1-2-1.dll"),				T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-localization-obsolete-l1-2-0.dll"),	T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-processthreads-l1-1-1.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-processthreads-l1-1-2.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-processthreads-l1-1-3.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-processtopology-obsolete-l1-1-0.dll"),	T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-downlevel-kernel32-l2-1-0.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-synch-l1-2-0.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-synch-l1-2-1.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-sysinfo-l1-2-0.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-sysinfo-l1-2-1.dll"),					T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-systemtopology-l1-1-0.dll"),			T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-threadpool-legacy-l1-1-0.dll"),		T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-threadpool-l1-2-0.dll"),				T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-versionansi-l1-1-0.dll"),				T("version.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-version-l1-1-0.dll"),					T("version.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-winrt-error-l1-1-0.dll"),				T("combase.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-winrt-error-l1-1-1.dll"),				T("combase.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-winrt-l1-1-0.dll"),					T("combase.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-winrt-string-l1-1-0.dll"),				T("combase.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-registry-l1-1-0.dll"),					T("advapi32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-core-registry-l2-1-0.dll"),					T("advapi32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-eventing-classicprovider-l1-1-0.dll"),		T("advapi32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-eventing-provider-l1-1-0.dll"),				T("advapi32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-security-systemfunctions-l1-1-0.dll"),		T("advapi32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-mm-time-l1-1-0.dll"),						T("winmm.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-ntuser-sysparams-l1-1-0.dll"),				T("user33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-shell-namespace-l1-1-0.dll"),				T("shell32.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-shcore-obsolete-l1-1-0.dll"),				T("shcore.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-shcore-scaling-l1-1-0.dll"),				T("shcore.dll"));
	ADD_DLL_REWRITE_ENTRY(T("api-ms-win-shcore-scaling-l1-1-1.dll"),				T("shcore.dll"));
	ADD_DLL_REWRITE_ENTRY(T("ext-ms-win-uiacore-l1-1-0.dll"),						T("uiautomationcore.dll"));
	ADD_DLL_REWRITE_ENTRY(T("ext-ms-win-uiacore-l1-1-1.dll"),						T("uiautomationcore.dll"));
	ADD_DLL_REWRITE_ENTRY(T("dxgi.dll"),											T("dxg1.dll"));
	ADD_DLL_REWRITE_ENTRY(T("kernel32.dll"),										T("kernel33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("ole32.dll"),											T("ole33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("user32.dll"),											T("user33.dll"));
	ADD_DLL_REWRITE_ENTRY(T("xinput1_4.dll"),										T("xinput1_3.dll"));

	// create KexDir and subdirectories if it doesn't exist, and copy self-EXE to it
	{
		TCHAR szSelfPath[MAX_PATH];
		TCHAR szDestPath[MAX_PATH];
		TCHAR szKex3264[MAX_PATH];
		CreateDirectory(lpszInstallDir, NULL);
		sprintf_s(szKex3264, ARRAYSIZE(szKex3264), T("%s\\Kex32"), lpszInstallDir);
		CreateDirectory(szKex3264, NULL);
#ifdef _WIN64
		sprintf_s(szKex3264, ARRAYSIZE(szKex3264), T("%s\\Kex64"), lpszInstallDir);
		CreateDirectory(szKex3264, NULL);
#endif
		GetModuleFileName(NULL, szSelfPath, ARRAYSIZE(szSelfPath));
		sprintf_s(szDestPath, ARRAYSIZE(szDestPath), T("%s\\KEXSETUP.EXE"), lpszInstallDir);
		CopyFile(szSelfPath, szDestPath, FALSE);
	}

	// copy VxKex files into KexDir
	if (EnumResourceNames(NULL, RT_RCDATA, KexEnumResources, (LPARAM) lpszInstallDir) == FALSE) {
		CriticalErrorBoxF(T("Unable to enumerate resources: %s\nThe system may be in an inconsistent state. Correct the error before trying again (run the installer again and select Repair)."),
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
	TCHAR szKexDir[MAX_PATH];

	if (!RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexDir, ARRAYSIZE(szKexDir))) {
		return FALSE;
	}

	// delete user settings if we need to do that
	if (!bPreserveSettings) {
		HKEY hKeyIfeo;
		TCHAR szRegKey[MAX_PATH];
		TCHAR szVxKexLdr[MAX_PATH];
		DWORD dwIdx = 0;

		sprintf_s(szVxKexLdr, ARRAYSIZE(szVxKexLdr), T("%s\\VxKexLdr.exe"), szKexDir);
		RegOpenKey(HKEY_LOCAL_MACHINE, T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options"), &hKeyIfeo);
		
		while (RegEnumKey(hKeyIfeo, dwIdx++, szRegKey, ARRAYSIZE(szRegKey)) == ERROR_SUCCESS) {
			TCHAR szRegData[MAX_PATH];
			
			if (RegReadSz(hKeyIfeo, szRegKey, T("Debugger"), szRegData, ARRAYSIZE(szRegData))) {
				if (!stricmp(szRegData, szVxKexLdr)) {
					RegDelValue(hKeyIfeo, szRegKey, T("Debugger"));
				}
			}
		}

		RegCloseKey(hKeyIfeo);
		SHDeleteKey(HKEY_CURRENT_USER, T("SOFTWARE\\VXsoft\\VxKexLdr"));
	}

	// unregister property sheet shell extension and extended context menu entry
	KexShlExDllInstall(FALSE);

	// delete KexDir\Kex32 and KexDir\Kex64 recursively as well as:
	// KexDir\VxKexLdr.exe
	// KexDir\KexCfg.exe
	// KexDir\KexShlEx.dll
	{
		TCHAR szDeletionTarget[MAX_PATH + 1];
		SHFILEOPSTRUCTW shop;

		shop.hwnd		= hWnd;
		shop.wFunc		= FO_DELETE;
		shop.pFrom		= szDeletionTarget;
		shop.pTo		= NULL;
		shop.fFlags		= FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\Kex32"), szKexDir);
		szDeletionTarget[strlen(szDeletionTarget) + 1] = T('\0');
		SHFileOperation(&shop);
#ifdef _WIN64
		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\Kex64"), szKexDir);
		szDeletionTarget[strlen(szDeletionTarget) + 1] = T('\0');
		SHFileOperation(&shop);
#endif

		TerminateProcessByImageName(T("VxKexLdr.exe"));
		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\VxKexLdr.exe"), szKexDir);
		DeleteFile(szDeletionTarget);
		TerminateProcessByImageName(T("KexCfg.exe"));
		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\KexCfg.exe"), szKexDir);
		DeleteFile(szDeletionTarget);
		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\KexShlEx.dll"), szKexDir);
		
		if (!DeleteFile(szDeletionTarget)) {
			// It may be loaded by explorer.exe so we have to kill and restart Explorer.
			STARTUPINFO startupInfo;
			PROCESS_INFORMATION procInfo;
			TCHAR szExplorer[MAX_PATH];

			if (TerminateProcessByImageName(T("explorer.exe"))) {
				Sleep(500);
				DeleteFile(szDeletionTarget); // and hopefully it works now
				GetStartupInfo(&startupInfo);
				ExpandEnvironmentStrings(T("%WINDIR\\explorer.exe"), szExplorer, ARRAYSIZE(szExplorer));
				CreateProcess(szExplorer, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo);
			}
		}
		
		sprintf_s(szDeletionTarget, ARRAYSIZE(szDeletionTarget), T("%s\\KexSetup.exe"), szKexDir);

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

	// Do this as the penultimate step since the absence of this key will prevent future uninstall attempts.
	SHDeleteKey(HKEY_LOCAL_MACHINE, T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex"));
	SHDeleteKey(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"));

	// TODO: Remove uninstallation entry.

	return TRUE;
}

DWORD WINAPI InstallThreadProc(
	IN	LPVOID	lpParam)
{
	TCHAR szInstallDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	GetDlgItemText(hWnd, IDS2DIRPATH, szInstallDir, ARRAYSIZE(szInstallDir));
	KexInstall(hWnd, szInstallDir);
	SetScene(hWnd, 4);
	CloseHandle(g_hWorkThread);
	g_hWorkThread = NULL;
	ExitThread(0);
}

DWORD WINAPI UninstallThreadProc(
	IN	LPVOID	lpParam)
{
	HWND hWnd = (HWND) lpParam;
	KexUninstall(hWnd, FALSE);
	SetScene(hWnd, 7);
	CloseHandle(g_hWorkThread);
	g_hWorkThread = NULL;
	ExitThread(0);
}

DWORD WINAPI RepairThreadProc(
	IN	LPVOID	lpParam)
{
	TCHAR szKexDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexDir, ARRAYSIZE(szKexDir));
	KexUninstall(hWnd, TRUE);
	KexInstall(hWnd, szKexDir);
	SetScene(hWnd, 10);
	CloseHandle(g_hWorkThread);
	g_hWorkThread = NULL;
	ExitThread(0);
}

DWORD WINAPI UpdateThreadProc(
	IN	LPVOID	lpParam)
{
	TCHAR szKexDir[MAX_PATH];
	HWND hWnd = (HWND) lpParam;
	RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexDir, ARRAYSIZE(szKexDir));
	KexUninstall(hWnd, TRUE);
	KexInstall(hWnd, szKexDir);
	SetScene(hWnd, 13);
	CloseHandle(g_hWorkThread);
	g_hWorkThread = NULL;
	ExitThread(0);
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
				
				if (MessageBox(hWnd, T("Installation or removal is in progress. If you cancel now, the software may be left in an inconsistent state. Cancel anyway?"),
							   T("Confirm Cancellation"), MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2 | MB_APPLMODAL) == IDYES) {
					EndDialog(hWnd, 0);
				} else {
					ResumeThread(g_hWorkThread);
				}
			} else if (g_iScene == 1 || g_iScene == 4 || g_iScene == 5 || g_iScene == 7) {
				EndDialog(hWnd, 0);
			} else {
				if (MessageBox(hWnd, T("Are you sure you want to cancel Setup?"), T("Confirm Cancellation"),
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
			TCHAR szOldValue[MAX_PATH];
			GetDlgItemText(hWnd, IDS2DIRPATH, szOldValue, ARRAYSIZE(szOldValue));
			SetDlgItemText(hWnd, IDS2DIRPATH, PickFolder(hWnd, szOldValue));
		} else if (LOWORD(wParam) == IDS2DIRPATH) {
			UpdateDiskFreeSpace(hWnd);
		} else if (LOWORD(wParam) == IDS1RBUNINST ||
				   LOWORD(wParam) == IDS1RBREPAIR ||
				   LOWORD(wParam) == IDS1RBUPDATE) {
			EnableWindow(GetDlgItem(hWnd, IDNEXT), IsDlgButtonChecked(hWnd, LOWORD(wParam)));
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
		(BOOL (WINAPI *) (HANDLE, LPBOOL)) GetProcAddress(GetModuleHandle(T("kernel32.dll")), "IsWow64Process");

	if (lpfnIsWow64Process) {
		if (lpfnIsWow64Process(GetCurrentProcess(), &bWow64) && bWow64) {
			return TRUE;
		}
	}
	
	return FALSE;
}

INT APIENTRY tWinMain(
	IN	HINSTANCE	hInstance,
	IN	HINSTANCE	hPrevInstance,
	IN	LPTSTR		lpszCmdLine,
	IN	INT			iCmdShow)
{
	SetFriendlyAppName(FRIENDLYAPPNAME);

#if !defined(_WIN64) && !defined(_DEBUG)
	if (IsWow64()) {
		CriticalErrorBoxF(T("Installing the 32-bit version of the software on a 64-bit system is not recommended. Please use the 64-bit installer instead."));
	}
#endif

	ElevateIfNotElevated(lpszCmdLine, iCmdShow);

	g_dwInstalledVersion = GetInstalledVersion();
	g_dwInstallerVersion = GetInstallerVersion();
	g_lpszInstalledVersion = GetInstalledVersionAsString();
	g_lpszInstallerVersion = GetInstallerVersionAsString();
	RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), g_szInstalledKexDir, ARRAYSIZE(g_szInstalledKexDir));

	if (StrStrI(lpszCmdLine, T("/UNINSTALL"))) {
		g_iScene = 5;
	}

	InitCommonControls();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	ExitProcess(0);
}