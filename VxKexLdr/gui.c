#include "buildcfg.h"
#include "vxkexldr.h"

STATIC PKEX_PROCESS_DATA KexData = NULL;

STATIC INT_PTR CALLBACK MoreOptionsDlgProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	ASSERT (KexData != NULL);

	if (Message == WM_INITDIALOG) {
		HWND ComboBoxWindow;

		ComboBoxWindow = GetDlgItem(Window, IDC_WINVER);
		ComboBox_AddString(ComboBoxWindow, L"Windows 7 Service Pack 1");
		ComboBox_AddString(ComboBoxWindow, L"Windows 8");
		ComboBox_AddString(ComboBoxWindow, L"Windows 8.1");
		ComboBox_AddString(ComboBoxWindow, L"Windows 10");
		ComboBox_AddString(ComboBoxWindow, L"Windows 11");
		ComboBox_SetCurSel(ComboBoxWindow, 3);
	} else if (Message == WM_COMMAND) {
		HWND ControlWindow;
		ULONG ControlId;
		ULONG NotificationCode;

		ControlWindow = (HWND) LParam;
		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDC_ENABLESPOOF) {
			if (Button_GetCheck(ControlWindow) == BST_CHECKED) {
				EnableWindow(GetDlgItem(Window, IDC_WINVER), TRUE);
				EnableWindow(GetDlgItem(Window, IDC_STRONGSPOOF), TRUE);

				KexData->IfeoParameters.WinVerSpoof =
					(KEX_WIN_VER_SPOOF) (ComboBox_GetCurSel(GetDlgItem(Window, IDC_WINVER)) + 1);
			} else {
				EnableWindow(GetDlgItem(Window, IDC_WINVER), FALSE);
				EnableWindow(GetDlgItem(Window, IDC_STRONGSPOOF), FALSE);

				KexData->IfeoParameters.WinVerSpoof = WinVerSpoofNone;
			}
		} else if (ControlId == IDC_WINVER && NotificationCode == CBN_SELCHANGE) {
			KexData->IfeoParameters.WinVerSpoof =
				(KEX_WIN_VER_SPOOF) (ComboBox_GetCurSel(GetDlgItem(Window, IDC_WINVER)) + 1);
		} else if (ControlId == IDC_STRONGSPOOF) {
			KexData->IfeoParameters.StrongVersionSpoof =
				!!Button_GetCheck(ControlWindow) ? KEX_STRONGSPOOF_VALID_MASK : 0;
		} else if (ControlId == IDC_DISABLEFORCHILD) {
			KexData->IfeoParameters.DisableAppSpecific = !!Button_GetCheck(ControlWindow);
		} else if (ControlId == IDC_DISABLEASH) {
			KexData->IfeoParameters.DisableAppSpecific = !!Button_GetCheck(ControlWindow);
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

INT_PTR CALLBACK VklDialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		KexgApplicationMainWindow = Window;
		SetWindowIcon(Window, IDI_APPICON);

		KexDataInitialize(&KexData);

		//
		// Allow files to be dragged and dropped onto the main window when
		// running as admin.
		//

		ChangeWindowMessageFilterEx(Window, WM_DROPFILES, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(Window, WM_COPYDATA, MSGFLT_ALLOW, NULL);
		ChangeWindowMessageFilterEx(Window, WM_COPYGLOBALDATA, MSGFLT_ALLOW, NULL);

		//
		// Enable shell autocompletion on edit controls.
		// For the arguments edit control we are only enabling autocompletion to take
		// advantage of the Ctrl+Backspace feature, so autosuggestions are disabled.
		//

		SHAutoComplete(GetDlgItem(Window, IDC_FILEPATH), SHACF_FILESYS_ONLY | SHACF_USETAB);
		SHAutoComplete(GetDlgItem(Window, IDC_ARGUMENTS), SHACF_AUTOSUGGEST_FORCE_OFF | SHACF_AUTOAPPEND_FORCE_OFF);

		//
		// Limit the maximum length of text that can be typed into edit controls.
		//

		Edit_LimitText(GetDlgItem(Window, IDC_FILEPATH), MAX_PATH - 1);
		Edit_LimitText(GetDlgItem(Window, IDC_ARGUMENTS), 32767 - MAX_PATH);

		//
		// Create the extra options child dialog. This is beyond the window boundaries
		// and is invisible until the user clicks the More options button.
		//

		CreateDialog(NULL, MAKEINTRESOURCE(IDD_MOREOPTIONS), Window, MoreOptionsDlgProc);
	} else if (Message == WM_COMMAND) {
		HWND ControlWindow;
		ULONG ControlId;
		ULONG NotificationCode;

		ControlWindow = (HWND) LParam;
		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDCANCEL) {
			EndDialog(Window, 0);
		} else if (ControlId == IDOK) {
			WCHAR FilePath[MAX_PATH];
			WCHAR Arguments[MAX_PATH];
			BOOLEAN Success;

			GetDlgItemText(Window, IDC_FILEPATH, FilePath, ARRAYSIZE(FilePath));
			GetDlgItemText(Window, IDC_ARGUMENTS, Arguments, ARRAYSIZE(Arguments));

			Success = VklCreateProcess(FilePath, Arguments);

			if (Success) {
				EndDialog(Window, 0);
			}
		} else if (ControlId == IDC_BROWSE) {
			OPENFILENAME OpenFileInfo;
			WCHAR FilePath[MAX_PATH];

			GetDlgItemText(Window, IDC_FILEPATH, FilePath, ARRAYSIZE(FilePath));

			RtlZeroMemory(&OpenFileInfo, sizeof(OpenFileInfo));
			OpenFileInfo.lStructSize	= sizeof(OpenFileInfo);
			OpenFileInfo.hwndOwner		= Window;
			OpenFileInfo.lpstrFilter	= L"Programs (*.exe, *.msi)\0*.exe;*.msi\0All Files (*.*)\0*.*\0";
			OpenFileInfo.lpstrFile		= FilePath;
			OpenFileInfo.nMaxFile		= ARRAYSIZE(FilePath);
			OpenFileInfo.lpstrTitle		= L"Select Program";
			OpenFileInfo.Flags			= OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			OpenFileInfo.lpstrDefExt	= L"exe";

			if (GetOpenFileName(&OpenFileInfo) == FALSE) {
				ASSERT (CommDlgExtendedError() == 0);
			} else {
				ASSERT (FilePath[0] != '\0');
				SetDlgItemText(Window, IDC_FILEPATH, FilePath);
			}
		} else if (ControlId == IDC_FILEPATH && NotificationCode == EN_CHANGE) {
			// enable/disable Run button based on whether there is text in the
			// edit control
			EnableWindow(
				GetDlgItem(Window, IDOK),
				(GetWindowTextLength(ControlWindow) != 0));
		} else if (ControlId == IDC_MOREOPTIONS) {
			STATIC BOOL MoreOptionsDisplayed = FALSE;
			STATIC ULONG CollapsedWindowHeight = 0;
			STATIC ULONG ExpandedWindowHeight;
			STATIC ULONG WindowWidth;

			if (CollapsedWindowHeight == 0) {
				RECT WindowRect;

				GetWindowRect(Window, &WindowRect);
				WindowWidth = WindowRect.right - WindowRect.left;
				CollapsedWindowHeight = WindowRect.bottom - WindowRect.top;
				ExpandedWindowHeight = CollapsedWindowHeight + DpiScaleY(95);
			}

			//
			// More Options button was clicked.
			//

			if (MoreOptionsDisplayed) {
				//
				// The extra options are currently displayed and we need to
				// hide them.
				//

				SetWindowPos(
					Window,
					NULL,
					0, 0,
					WindowWidth, CollapsedWindowHeight,
					SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW |
					SWP_NOSENDCHANGING | SWP_NOZORDER);

				SetDlgItemText(Window, IDC_MOREOPTIONS, L"▼ More &options");
			} else {
				//
				// The extra options are currently not displayed and we need
				// to display them.
				//

				SetWindowPos(
					Window,
					NULL,
					0, 0,
					WindowWidth, ExpandedWindowHeight,
					SWP_NOACTIVATE | SWP_NOMOVE |
					SWP_NOSENDCHANGING | SWP_NOZORDER);

				SetDlgItemText(Window, IDC_MOREOPTIONS, L"▲ Hide &options");
			}

			MoreOptionsDisplayed = !MoreOptionsDisplayed;
		} else {
			return FALSE;
		}
	} else if (Message == WM_DROPFILES) {
		HDROP DroppedItem;
		WCHAR DroppedFilePath[MAX_PATH];

		DroppedItem = (HDROP) WParam;

		DragQueryFile(DroppedItem, 0, DroppedFilePath, ARRAYSIZE(DroppedFilePath));
		SetDlgItemText(Window, IDC_FILEPATH, DroppedFilePath);
		DragFinish(DroppedItem);
	} else {
		return FALSE;
	}

	return TRUE;
}