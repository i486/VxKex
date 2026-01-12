///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     gui.c
//
// Abstract:
//
//     Implements the user interface logic of the shell extension.
//
// Author:
//
//     vxiiduu (08-Feb-2024)
//
// Environment:
//
//     Inside explorer.exe
//
// Revision History:
//
//     vxiiduu              08-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "KexShlEx.h"
#include "resource.h"

INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	PKEXSHLEX_PROPSHEET_DATA PropSheetData;

	PropSheetData = (PKEXSHLEX_PROPSHEET_DATA) GetWindowLongPtr(Window, GWLP_USERDATA);

	if (Message == WM_INITDIALOG) {
		BOOLEAN Success;
		IKexShlEx *CKexShlEx;
		LPPROPSHEETPAGE PropSheetPage;
		PCWSTR ExeFullPath;
		HWND WinVerComboBox;
		KXCFG_PROGRAM_CONFIGURATION ProgramConfiguration;
		ULONG Index;

		ASSERT (PropSheetData == NULL);
		PropSheetPage = (LPPROPSHEETPAGE) LParam;
		ASSERT (PropSheetPage != NULL);
		CKexShlEx = (IKexShlEx *) PropSheetPage->lParam;
		ASSERT (CKexShlEx != NULL);
		ExeFullPath = CKexShlEx->ExeFullPath;

		//
		// Allocate a structure for us to store the path to the executable
		// as well as record whether any of the settings have changed.
		//

		PropSheetData = SafeAlloc(KEXSHLEX_PROPSHEET_DATA, 1);
		if (!PropSheetData) {
			for (Index = 110; Index < 130; ++Index) {
				EnableWindow(GetDlgItem(Window, Index), FALSE);
			}
		}

		if (PropSheetData) {
			PropSheetData->ExeFullPath		= ExeFullPath;
			PropSheetData->SettingsChanged	= FALSE;
		}

		//
		// Associate the data structure with the property sheet.
		//

		SetWindowLongPtr(Window, GWLP_USERDATA, (LONG_PTR) PropSheetData);

		//
		// Populate the Windows version combo box.
		//

		WinVerComboBox = GetDlgItem(Window, IDWINVERCOMBOBOX);

		ComboBox_AddString(WinVerComboBox, L"Windows 7 Service Pack 1");
		ComboBox_AddString(WinVerComboBox, L"Windows 8");
		ComboBox_AddString(WinVerComboBox, L"Windows 8.1");
		ComboBox_AddString(WinVerComboBox, L"Windows 10");
		ComboBox_AddString(WinVerComboBox, L"Windows 11");

		// Set default selection to Windows 10.
		ComboBox_SetCurSel(WinVerComboBox, 3);

		//
		// Query the VxKex configuration for the current program.
		//

		Success = KxCfgGetConfiguration(
			ExeFullPath,
			&ProgramConfiguration);

		if (Success) {
			CheckDlgButton(Window, IDUSEVXKEX,				!!ProgramConfiguration.Enabled);

			if (ProgramConfiguration.WinVerSpoof != WinVerSpoofNone) {
				EnableWindow(WinVerComboBox, TRUE);
				EnableWindow(GetDlgItem(Window, IDSTRONGSPOOF), TRUE);
				CheckDlgButton(Window, IDSPOOFVERSIONCHECK, BST_CHECKED);
				ComboBox_SetCurSel(WinVerComboBox, ProgramConfiguration.WinVerSpoof - 1);
			}

			CheckDlgButton(Window, IDSTRONGSPOOF,			!!ProgramConfiguration.StrongSpoofOptions);
			CheckDlgButton(Window, IDDISABLEFORCHILD,		!!ProgramConfiguration.DisableForChild);
			CheckDlgButton(Window, IDDISABLEAPPSPECIFIC,	!!ProgramConfiguration.DisableAppSpecificHacks);
		}

		//
		// Add tooltips to all the settings.
		//

		ToolTip(Window, IDUSEVXKEX,
			L"Enable or disable the main VxKex compatibility layer.");
		ToolTip(Window, IDSPOOFVERSIONCHECK,
			L"Some applications check the Windows version and refuse to run if it is too low. "
			L"This option can help these applications to run correctly.\r\n\r\n"
			L"Generally, you should not use a higher Windows version than required to run the "
			L"application, because this can degrade application compatibility.");
		ToolTip(Window, IDSTRONGSPOOF,
			L"Some applications check the Windows version using uncommon methods. This option "
			L"can help trick them into working. Do not enable this setting unless you are having "
			L"a problem with version detection.");
		ToolTip(Window, IDDISABLEFORCHILD,
			L"By default, all other programs that are started by this program run with VxKex "
			L"enabled. This option disables that behavior.");
		ToolTip(Window, IDDISABLEAPPSPECIFIC,
			L"For some applications, VxKex may use application-specific workarounds or patches. "
			L"This option disables that behavior. Using this option may degrade application "
			L"compatibility.");
		ToolTip(Window, IDREPORTBUG, _L(KEX_BUGREPORT_STR));

		SetFocus(GetDlgItem(Window, IDUSEVXKEX));
		return FALSE;
	} else if (Message == WM_COMMAND) {
		if (LOWORD(WParam) == IDSPOOFVERSIONCHECK) {
			BOOLEAN VersionSpoofEnabled;

			VersionSpoofEnabled = !!IsDlgButtonChecked(Window, IDSPOOFVERSIONCHECK);

			// enable the win ver combo box when the user enables version spoof
			EnableWindow(GetDlgItem(Window, IDWINVERCOMBOBOX), VersionSpoofEnabled);

			// enable the strong spoof checkbox when user enables version spoof
			EnableWindow(GetDlgItem(Window, IDSTRONGSPOOF), VersionSpoofEnabled);
		}

		// this causes the "Apply" button to be enabled
		PropSheet_Changed(GetParent(Window), Window);

		// Record the change to the settings.
		if (PropSheetData) {
			PropSheetData->SettingsChanged = TRUE;
		}
	} else if (Message == WM_NOTIFY && ((LPNMHDR) LParam)->code == PSN_APPLY &&
			   PropSheetData && PropSheetData->SettingsChanged) {

		//
		// The OK or Apply button was clicked and we need to apply new settings.
		//

		KXCFG_PROGRAM_CONFIGURATION ProgramConfiguration;

		ZeroMemory(&ProgramConfiguration, sizeof(ProgramConfiguration));

		ProgramConfiguration.Enabled					= IsDlgButtonChecked(Window, IDUSEVXKEX);

		if (IsDlgButtonChecked(Window, IDSPOOFVERSIONCHECK)) {
			ProgramConfiguration.WinVerSpoof = (KEX_WIN_VER_SPOOF) (ComboBox_GetCurSel(GetDlgItem(Window, IDWINVERCOMBOBOX)) + 1);
		} else {
			ProgramConfiguration.WinVerSpoof			= WinVerSpoofNone;
		}

		ProgramConfiguration.StrongSpoofOptions			= IsDlgButtonChecked(Window, IDSTRONGSPOOF) ? KEX_STRONGSPOOF_VALID_MASK : 0;
		ProgramConfiguration.DisableForChild			= IsDlgButtonChecked(Window, IDDISABLEFORCHILD);
		ProgramConfiguration.DisableAppSpecificHacks	= IsDlgButtonChecked(Window, IDDISABLEAPPSPECIFIC);

		//
		// All the configuration is inside the ProgramConfiguration structure.
		// Call a helper function to carry out the steps necessary to write it
		// to the registry.
		//

		if (PropSheetData) {
			KxCfgSetConfiguration(PropSheetData->ExeFullPath, &ProgramConfiguration, NULL);
			PropSheetData->SettingsChanged = FALSE;
		}
	} else if (Message == WM_NOTIFY && WParam == IDREPORTBUG) {
		ULONG NotificationCode;

		NotificationCode = ((LPNMHDR) LParam)->code;

		if (NotificationCode == NM_CLICK || NotificationCode == NM_RETURN) {
			// user clicked the report bug button - open in his default browser
			ShellExecute(Window, L"open", _L(KEX_BUGREPORT_STR), NULL, NULL, SW_NORMAL);
		}
	} else {
		return FALSE;
	}

	return TRUE;
}

UINT WINAPI PropSheetCallbackProc(
	IN		HWND				Window,
	IN		UINT				Message,
	IN OUT	LPPROPSHEETPAGE		PropSheetPage)
{
	IKexShlEx *CKexShlEx;

	ASSERT (PropSheetPage != NULL);

	CKexShlEx = (IKexShlEx *) PropSheetPage->lParam;
	ASSERT (CKexShlEx != NULL);

	if (Message == PSPCB_ADDREF) {
		CKexShlEx_AddRef(CKexShlEx);
	} else if (Message == PSPCB_RELEASE) {
		CKexShlEx_Release(CKexShlEx);
	}

	return TRUE;
}