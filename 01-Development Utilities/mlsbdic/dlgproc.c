///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     Dialog procedure and other GUI-related code for when the BDI compiler is
//     running in GUI mode (as opposed to command line mode).
//
// Author:
//
//     vxiiduu (21-May-2025)
//
// Environment:
//
//     Win32 GUI
//
// Revision History:
//
//     vxiiduu              21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "mlsbdic.h"

INT_PTR CALLBACK BdicDialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		KexgApplicationMainWindow = Window;

		// Enable path auto-completion for edit controls.
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		SHAutoComplete(GetDlgItem(Window, IDC_INPUTFILEPATH), SHACF_FILESYS_ONLY | SHACF_USETAB);
		SHAutoComplete(GetDlgItem(Window, IDC_OUTPUTFILEPATH), SHACF_FILESYS_ONLY | SHACF_USETAB);

		// Limit maximum path length that can be typed into edit controls.
		Edit_LimitText(GetDlgItem(Window, IDC_INPUTFILEPATH), MAX_PATH - 1);
		Edit_LimitText(GetDlgItem(Window, IDC_OUTPUTFILEPATH), MAX_PATH - 1);
	} else if (Message == WM_COMMAND) {
		HWND ControlWindow;
		ULONG ControlId;
		ULONG NotificationCode;

		ControlWindow = (HWND) LParam;
		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDCANCEL) {
			// Cancel button or Esc key pressed
			EndDialog(Window, STATUS_CANCELLED);
		} else if (ControlId == IDOK) {
			WCHAR InputFilePath[MAX_PATH];
			WCHAR OutputFilePath[MAX_PATH];

			// IDOK is the "Compile/Decompile" button.
			
			BdicCompileBdiDic(InputFilePath, OutputFilePath);
		} else if (ControlId == IDC_INPUTBROWSE || ControlId == IDC_OUTPUTBROWSE) {
			WCHAR FilePath[MAX_PATH];
			OPENFILENAME OpenFileName;
			HWND EditControl;

			// "Browse..." was clicked for either the input file or output file

			EditControl = GetDlgItem(
				Window,
				ControlId == IDC_INPUTBROWSE ? IDC_INPUTFILEPATH : IDC_OUTPUTFILEPATH);

			GetWindowText(EditControl, FilePath, ARRAYSIZE(FilePath));

			RtlZeroMemory(&OpenFileName, sizeof(OpenFileName));
			OpenFileName.lStructSize	= sizeof(OpenFileName);
			OpenFileName.hwndOwner		= Window;
			OpenFileName.lpstrFilter	= L"Dictionary files (*.bdi, *.dic)\0*.bdi;*.dic\0";
			OpenFileName.lpstrFile		= FilePath;
			OpenFileName.nMaxFile		= ARRAYSIZE(FilePath);
			OpenFileName.Flags			= OFN_EXPLORER | OFN_HIDEREADONLY;

			if (ControlId == IDC_INPUTBROWSE) {
				OpenFileName.lpstrTitle	= L"Select Input File";
				OpenFileName.lpstrDefExt= L"dic";
				GetOpenFileName(&OpenFileName);
			} else if (ControlId == IDC_OUTPUTBROWSE) {
				OpenFileName.lpstrTitle	= L"Select Output File";
				OpenFileName.lpstrDefExt= L"bdi";
				GetSaveFileName(&OpenFileName);
			} else {
				ASSUME (FALSE);
			}

			SetWindowText(EditControl, FilePath);
		} else if (ControlId == IDC_CMDLINEHELP) {
			// "Command-line usage help" button clicked - show the same message as we
			// show for /? on the command line
			DisplayHelpMessage();
		} else {
			return FALSE;
		}
	} else if (Message == WM_DROPFILES) {
		HDROP DroppedItem;
		WCHAR DroppedFilePath[MAX_PATH];

		DroppedItem = (HDROP) WParam;

		DragQueryFile(DroppedItem, 0, DroppedFilePath, ARRAYSIZE(DroppedFilePath));
		SetDlgItemText(Window, IDC_INPUTFILEPATH, DroppedFilePath);
		DragFinish(DroppedItem);
	} else {
		return FALSE;
	}

	return TRUE;
}