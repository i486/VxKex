#include "VxKexLdr.h"
#include "resource.h"

#include <CommCtrl.h>

HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPWSTR	lpszText)
{
	TOOLINFO ToolInfo;
	HWND hWndTool;
	HWND hWndTip;

	if (!iToolID || !hDlg || !lpszText) {
		return NULL;
	}

	// Get the window of the tool.
	hWndTool = GetDlgItem(hDlg, iToolID);

	// Create the tooltip.
	hWndTip = CreateWindowEx(
		0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL, 
		NULL, NULL);

	if (!hWndTool || !hWndTip) {
		return NULL;
	}

	// Associate the tooltip with the tool.
	ZeroMemory(&ToolInfo, sizeof(ToolInfo));
	ToolInfo.cbSize = sizeof(ToolInfo);
	ToolInfo.hwnd = hDlg;
	ToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ToolInfo.uId = (UINT_PTR) hWndTool;
	ToolInfo.lpszText = lpszText;
	SendMessage(hWndTip, TTM_ADDTOOL, 0, (LPARAM) &ToolInfo);

	return hWndTip;
}

INT_PTR CALLBACK DlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	LSTATUS lStatus;
	HKEY hKeyUserSpecificConfig;
	CONST WCHAR szUserSpecificConfigKey[] = L"SOFTWARE\\VXsoft\\" APPNAME;
	WCHAR szFilename[MAX_PATH + 2] = L"";
	DWORD dwcbFilename = sizeof(szFilename);
	BOOL bShouldShowDebugInfo;
	DWORD dwcbShouldShowDebugInfo = sizeof(bShouldShowDebugInfo);

	switch (uMsg) {
	case WM_INITDIALOG:
		// read last successful command line from registry
		lStatus = RegOpenKeyEx(
			HKEY_CURRENT_USER,
			szUserSpecificConfigKey,
			0, KEY_QUERY_VALUE | KEY_WOW64_64KEY,
			&hKeyUserSpecificConfig);

		if (lStatus == ERROR_SUCCESS) {
			RegQueryValueEx(
				hKeyUserSpecificConfig, L"LastCmdLine",
				0, NULL, (LPBYTE) szFilename, &dwcbFilename);
			lStatus = RegQueryValueEx(
				hKeyUserSpecificConfig, L"ShowDebugInfo",
				0, NULL, (LPBYTE) &bShouldShowDebugInfo, &dwcbShouldShowDebugInfo);

			if (lStatus != ERROR_SUCCESS) {
				bShouldShowDebugInfo = 0;
			}

			CheckDlgButton(hWnd, IDCHKDEBUG, bShouldShowDebugInfo ? BST_CHECKED : BST_UNCHECKED);
			RegCloseKey(hKeyUserSpecificConfig);
			SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		}

		CreateToolTip(hWnd, IDCHKDEBUG, L"Creates a console window to display additional debugging information");
		DragAcceptFiles(hWnd, TRUE);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDBROWSE) {
			OPENFILENAME ofn;

			// If user was typing in the edit box and pressed enter, just run the program
			// instead of displaying the file selector (which is confusing and unintended)
			if (GetFocus() == GetDlgItem(hWnd, IDFILEPATH)) {
				PostMessage(hWnd, WM_COMMAND, IDOK, 0);
				break;
			}

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize			= sizeof(ofn);
			ofn.hwndOwner			= hWnd;
			ofn.lpstrFilter			= L"Applications (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile			= szFilename + 1;
			ofn.nMaxFile			= MAX_PATH;
			ofn.lpstrInitialDir		= L"C:\\Program Files\\";
			ofn.Flags				= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt			= L"exe";
			
			if (GetOpenFileName(&ofn)) {				
				// Give the filename some quotes.
				szFilename[0] = '\"';
				wcscat_s(szFilename, ARRAYSIZE(szFilename), L"\"");
				SetDlgItemText(hWnd, IDFILEPATH, szFilename);
			}

			break;
		} else if (LOWORD(wParam) == IDOK) {
			GetDlgItemText(hWnd, IDFILEPATH, szFilename, dwcbFilename);
			bShouldShowDebugInfo = IsDlgButtonChecked(hWnd, IDCHKDEBUG);
			EndDialog(hWnd, 0);
			SpawnProgramUnderLoader(szFilename, TRUE, bShouldShowDebugInfo, TRUE);

			// save dialog information in the registry
			lStatus = RegCreateKeyEx(
				HKEY_CURRENT_USER,
				szUserSpecificConfigKey,
				0, NULL, 0, KEY_WRITE, NULL,
				&hKeyUserSpecificConfig, NULL);

			if (lStatus == ERROR_SUCCESS) {
				RegSetValueEx(
					hKeyUserSpecificConfig,
					L"LastCmdLine", 0, REG_SZ,
					(LPBYTE) szFilename,
					(DWORD) wcslen(szFilename) * sizeof(WCHAR));
				RegSetValueEx(
					hKeyUserSpecificConfig,
					L"ShowDebugInfo", 0, REG_DWORD,
					(LPBYTE) &bShouldShowDebugInfo,
					sizeof(bShouldShowDebugInfo));
				RegCloseKey(hKeyUserSpecificConfig);
			}

			Exit(0);
		} else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, 0);
		} else if (LOWORD(wParam) == IDFILEPATH) {
			if (HIWORD(wParam) == EN_CHANGE) {
				INT iTextLength = GetWindowTextLength(GetDlgItem(hWnd, IDFILEPATH));
				EnableWindow(GetDlgItem(hWnd, IDOK), !!iTextLength);
			}
		}

		break;
	case WM_DROPFILES:
		szFilename[0] = '"';
		DragQueryFile((HDROP) wParam, 0, szFilename + 1, MAX_PATH);
		StringCchCat(szFilename, ARRAYSIZE(szFilename), L"\"");
		SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		DragFinish((HDROP) wParam);
		break;
	}
	
	return FALSE;
}