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
//     vxiiduu              30-May-2026  Add "console enhancements" toggle.
//     vxiiduu              24-Jun-2026  Don't overwrite settings which aren't
//                                       exposed through the GUI.
//     vxiiduu              24-Jun-2026  Add "Open in Registry Editor" button.
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
		// MLS: translate the static dialog text into the user's language
		//

		MlsgTranslateWindow(Window);

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

		ComboBox_AddString(WinVerComboBox, _(L"Windows 7 Service Pack 1"));
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

		EnableWindow(GetDlgItem(Window, IDOPENREGEDIT), Success);

		if (Success) {
			CheckDlgButton(Window, IDUSEVXKEX,				!!ProgramConfiguration.Enabled);

			if (ProgramConfiguration.IfeoParameters.WinVerSpoof != WinVerSpoofNone) {
				EnableWindow(WinVerComboBox, TRUE);
				EnableWindow(GetDlgItem(Window, IDSTRONGSPOOF), TRUE);
				CheckDlgButton(Window, IDSPOOFVERSIONCHECK, BST_CHECKED);

				ComboBox_SetCurSel(
					WinVerComboBox,
					ProgramConfiguration.IfeoParameters.WinVerSpoof - 1);
			}

			CheckDlgButton(
				Window,
				IDSTRONGSPOOF,
				!!ProgramConfiguration.IfeoParameters.StrongVersionSpoof);

			CheckDlgButton(
				Window,
				IDDISABLEFORCHILD,
				!!ProgramConfiguration.IfeoParameters.DisableForChild);

			CheckDlgButton(
				Window,
				IDDISABLEAPPSPECIFIC,
				!!ProgramConfiguration.IfeoParameters.DisableAppSpecific);

			CheckDlgButton(
				Window,
				IDDISABLECONENHANCE,
				!!ProgramConfiguration.IfeoParameters.DisableConsoleEnhancements);
		}

		//
		// Add tooltips to all the settings.
		//

		ToolTip(Window, IDUSEVXKEX,
			_(L"Enable or disable the main VxKex compatibility layer."));
		ToolTip(Window, IDSPOOFVERSIONCHECK, _(
			L"Some applications check the Windows version and refuse to run if it is too low. "
			L"This option can help these applications to run correctly.\r\n\r\n"
			L"Generally, you should not use a higher Windows version than required to run the "
			L"application, because this can degrade application compatibility."));
		ToolTip(Window, IDSTRONGSPOOF, _(
			L"Some applications check the Windows version using uncommon methods. This option "
			L"can help trick them into working. Do not enable this setting unless you are having "
			L"a problem with version detection."));
		ToolTip(Window, IDDISABLEFORCHILD, _(
			L"By default, all other programs that are started by this program run with VxKex "
			L"enabled. This option disables that behavior."));
		ToolTip(Window, IDDISABLEAPPSPECIFIC, _(
			L"For some applications, VxKex may use application-specific workarounds or patches. "
			L"This option disables that behavior. Using this option may degrade application "
			L"compatibility."));
		ToolTip(Window, IDDISABLECONENHANCE, _(
			L"By default, VxKex enables ANSI escape sequence support for console applications. "
			L"If you would like to use third-party console windows instead, you may disable this "
			L"support."));

		SetFocus(GetDlgItem(Window, IDUSEVXKEX));
		return FALSE;
	} else if (Message == WM_COMMAND && LOWORD(WParam) == IDOPENREGEDIT) {
		if (PropSheetData) {
			BOOLEAN Success;

			// If we're using the KexCfg scheduled task to set configuration (i.e. if
			// user account control is enabled), then the detection code that checks
			// whether the IFEO key still exists after an OK/Apply might not work due
			// to the asynchronous nature of the operation.
			// So if OpenIfeoRegKey fails we will just disable the button.

			Success = OpenIfeoRegKey(Window, PropSheetData->ExeFullPath);
			EnableWindow(GetDlgItem(Window, IDOPENREGEDIT), Success);
		}
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

		BOOLEAN Success;
		KXCFG_PROGRAM_CONFIGURATION ProgramConfiguration;

		// Get existing configuration. The purpose of this, rather than just
		// overwriting with all new values, is so that we don't clobber configuration
		// that isn't exposed through the GUI (e.g. TLS force enabled/disabled
		// protocols).
		Success = KxCfgGetConfiguration(PropSheetData->ExeFullPath, &ProgramConfiguration);
		
		if (!Success) {
			// If failed to get configuration then just start from scratch.
			KexRtlZeroMemory(&ProgramConfiguration, sizeof(ProgramConfiguration));
		}

		ProgramConfiguration.Enabled = IsDlgButtonChecked(Window, IDUSEVXKEX);

		if (IsDlgButtonChecked(Window, IDSPOOFVERSIONCHECK)) {
			ProgramConfiguration.IfeoParameters.WinVerSpoof = (KEX_WIN_VER_SPOOF)
				(ComboBox_GetCurSel(GetDlgItem(Window, IDWINVERCOMBOBOX)) + 1);
		} else {
			ProgramConfiguration.IfeoParameters.WinVerSpoof = WinVerSpoofNone;
		}

		ProgramConfiguration.IfeoParameters.StrongVersionSpoof =
			IsDlgButtonChecked(Window, IDSTRONGSPOOF) ? KEX_STRONGSPOOF_VALID_MASK : 0;

		ProgramConfiguration.IfeoParameters.DisableForChild =
			IsDlgButtonChecked(Window, IDDISABLEFORCHILD);

		ProgramConfiguration.IfeoParameters.DisableAppSpecific =
			IsDlgButtonChecked(Window, IDDISABLEAPPSPECIFIC);

		ProgramConfiguration.IfeoParameters.DisableConsoleEnhancements =
			IsDlgButtonChecked(Window, IDDISABLECONENHANCE);

		//
		// All the configuration is inside the ProgramConfiguration structure.
		// Call a helper function to carry out the steps necessary to write it
		// to the registry.
		//

		if (PropSheetData) {
			BOOLEAN RegKeyExists;

			KxCfgSetConfiguration(PropSheetData->ExeFullPath, &ProgramConfiguration, NULL);
			PropSheetData->SettingsChanged = FALSE;

			if (KexRtlIsZeroMemory(&ProgramConfiguration, sizeof(ProgramConfiguration))) {
				// If we have an all-zero configuration structure, then there won't be
				// any VxKex configuration for this program.
				RegKeyExists = FALSE;
			} else {
				// Check if the IFEO reg key exists after updating the configuration and update
				// the Open in Registry Editor button state as appropriate.
				RegKeyExists = KxCfgGetConfiguration(PropSheetData->ExeFullPath, NULL);
			}

			EnableWindow(GetDlgItem(Window, IDOPENREGEDIT), RegKeyExists);
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