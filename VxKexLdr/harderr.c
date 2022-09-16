#include <NtDll.h>
#include "VxKexLdr.h"
#include "resource.h"

#include <WindowsX.h>

STATIC NTSTATUS stHe = 0;
STATIC DWORD dwHe = 0;
STATIC LPWSTR lpszHe1 = NULL;
STATIC LPWSTR lpszHe2 = NULL;

// The possible NTSTATUS codes are:
//   STATUS_IMAGE_MACHINE_TYPE_MISMATCH		- someone tried running an ARM/other architecture binary or dll (%s)
//   STATUS_INVALID_IMAGE_FORMAT			- PE is corrupt (%s)
//   STATUS_DLL_NOT_FOUND					- a DLL is missing (%s)
//   STATUS_ORDINAL_NOT_FOUND				- DLL ordinal function is missing (%d, %s)
//   STATUS_ENTRYPOINT_NOT_FOUND			- DLL function is missing (%s, %s)
STATIC INT_PTR CALLBACK HeDlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	WCHAR szErrStmt[MAX_PATH * 2 + 128];
	STATIC HWND hWndDebugLog = NULL;

	switch (uMsg) {
	case WM_INITDIALOG:
		// play the error sound
		PlaySound((LPCWSTR) SND_ALIAS_SYSTEMHAND, NULL, SND_ALIAS_ID | SND_ASYNC | SND_SYSTEM);

		switch (stHe) {
		case STATUS_DLL_NOT_FOUND:
			SetWindowText(hWnd, L"DLL Not Found");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required %s was not found while attempting to launch %s.",
							lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_ENTRYPOINT_NOT_FOUND:
			SetWindowText(hWnd, L"Missing Function In DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required function %s was not found in %s while attempting to launch %s.",
							lpszHe1, lpszHe2, g_lpszExeBaseName);
			break;
		case STATUS_ORDINAL_NOT_FOUND:
			SetWindowText(hWnd, L"Missing Function In DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required ordinal function #%s was not found in %s while attempting to launch %s.",
							dwHe, lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_INVALID_IMAGE_FORMAT:
			SetWindowText(hWnd, L"Invalid or Corrupt Executable or DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"%s is invalid or corrupt. %s cannot start.",
							lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_IMAGE_MACHINE_TYPE_MISMATCH:
			SetWindowText(hWnd, L"Processor Architecture Mismatch");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"%s is designed for a different processor architecture. %s cannot start.",
							lpszHe1, g_lpszExeBaseName);
			break;
		default:
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"An unrecoverable fatal error has occurred during the initialization of %s.\r\n"
							L"The error code is 0x%08x: %s",
							g_lpszExeBaseName, stHe, NtStatusAsString(stHe));
			break;
		}

		SetDlgItemText(hWnd, IDERRSTATEMENT, szErrStmt);
		SendDlgItemMessage(hWnd, IDERRICON, STM_SETICON, (WPARAM) LoadIcon(NULL, IDI_ERROR), 0);
		hWndDebugLog = GetDlgItem(hWnd, IDDEBUGLOG);

		// give our edit control a monospaced font
		SendMessage(hWndDebugLog, WM_SETFONT, (WPARAM) CreateFont(
					-MulDiv(8, GetDeviceCaps(GetDC(hWndDebugLog), LOGPIXELSY), 72),
					0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					L"Consolas"), MAKELPARAM(TRUE, 0));

		// populate the edit control with the contents of the log
		// the edit control is read-write in case the user wants to censor some private info
		if (g_hLogFile) {
			HANDLE hMapping;
			LPCWSTR lpszDocument;
			CHECKED(hMapping = CreateFileMapping(g_hLogFile, NULL, PAGE_READONLY, 0, 0, NULL));
			CHECKED(lpszDocument = (LPCWSTR) MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
			Edit_SetText(hWndDebugLog, lpszDocument);
			UnmapViewOfFile(lpszDocument);
			CloseHandle(hMapping);
		} else {
Error:
			// whoops...
			Edit_SetText(hWndDebugLog, L"No log file is available.");
		}

		CreateToolTip(hWnd, IDBUGREPORT, L"Opens the GitHub issue reporter in your browser");
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, 0);
		} else if (LOWORD(wParam) == IDCOPYCLIPBOARD) {
			if (OpenClipboard(hWndDebugLog)) {
				CONST INT cchDocument = GetWindowTextLength(hWndDebugLog) + 1;
				HGLOBAL hglbDocument = GlobalAlloc(GMEM_MOVEABLE, cchDocument * sizeof(WCHAR));
				LPWSTR lpszDocument;

				if (hglbDocument == NULL) {
					CloseClipboard();
					break;
				}

				lpszDocument = (LPWSTR) GlobalLock(hglbDocument);
				GetWindowText(hWndDebugLog, lpszDocument, cchDocument);
				GlobalUnlock(hglbDocument);

				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, hglbDocument);
				CloseClipboard();
				GlobalFree(hglbDocument);
			}
		} else if (LOWORD(wParam) == IDBUGREPORT) {
			WCHAR szUrl[512];

			// TODO: we can add text to issue body as well using ?body=whatever
			switch (stHe) {
			case STATUS_DLL_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-dll-not-found--error.md&title=%s+not+found+when+trying+to+run+%s",
					lpszHe1, g_lpszExeBaseName);
				break;
			case STATUS_ORDINAL_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-missing-function--error.md&title=Ordinal+%d+not+found+in+%s+when+trying+to+run+%s",
					dwHe, lpszHe1, g_lpszExeBaseName);
				break;
			case STATUS_ENTRYPOINT_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-missing-function--error.md&title=Function+%s+not+found+in+%s+when+trying+to+run+%s",
					lpszHe1, lpszHe2, g_lpszExeBaseName);
				break;
			default:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?title=Error+when+running+%s",
					g_lpszExeBaseName);
				break;
			}

			ShellExecute(hWnd, L"open", szUrl, NULL, NULL, SW_SHOWNORMAL);
		}

		break;
	}

	return FALSE;
}

VOID VklHardError(
	IN	NTSTATUS	st,
	IN	DWORD		dw,
	IN	LPWSTR		lpsz1,
	IN	LPWSTR		lpsz2)
{
	stHe = st;
	dwHe = dw;
	lpszHe1 = lpsz1;
	lpszHe2 = lpsz2;

	LogF(L"    st=0x%08x,dw=%I32d,lpsz1=%s,lpsz2=%s\r\n", st, dw, lpsz1, lpsz2);
	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, HeDlgProc);
}