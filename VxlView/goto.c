#include "buildcfg.h"
#include "vxlview.h"

INT_PTR CALLBACK GotoRawDlgProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		CenterWindow(Window, MainWindow);
		SendMessage(GetDlgItem(Window, scr1), UDM_SETRANGE32, 1, INT_MAX);
		Edit_LimitText(GetDlgItem(Window, edt1), 10);
		SetDlgItemInt(Window, edt1, 1, FALSE);
	} else if (Message == WM_COMMAND) {
		ULONG ControlId;

		ControlId = GET_WM_COMMAND_ID(WParam, LParam);

		if (ControlId == IDOK) {
			BOOL SuccessfulConversion;
			ULONG RawItemIndex;
			ULONG ItemIndex;

			ItemIndex = (ULONG) -1;
			RawItemIndex = GetDlgItemInt(Window, edt1, &SuccessfulConversion, FALSE);

			if (RawItemIndex > 0) {
				--RawItemIndex;
			}

			if (SuccessfulConversion) {
				ItemIndex = GetLogEntryIndexFromRawIndex(RawItemIndex);
			}

			if (ItemIndex == (ULONG) -1) {
				EDITBALLOONTIP BalloonTip;

				BalloonTip.cbStruct	= sizeof(BalloonTip);
				BalloonTip.pszTitle	= L"Invalid Item Number";
				BalloonTip.pszText	= L"The item number you entered was either out of range "
									  L"or not displayed by the current set of filters you have "
									  L"selected.";
				BalloonTip.ttiIcon	= TTI_NONE;

				Edit_ShowBalloonTip(GetDlgItem(Window, edt1), &BalloonTip);
			} else {
				SelectListViewItemByIndex(ItemIndex);
				EndDialog(Window, IDOK);
			}
		} else if (ControlId == IDCANCEL) {
			PostMessage(Window, WM_CLOSE, 0, 0);
		}
	} else if (Message == WM_CLOSE) {
		EndDialog(Window, IDCANCEL);
	} else {
		return FALSE;
	}

	return TRUE;
}