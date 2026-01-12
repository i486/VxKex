///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexexprt.c
//
// Abstract:
//
//     DLL export dumper for creating .def files or #pragma directives.
//     The differences from the utility of the same name in previous versions
//     of VxKex include:
//       - minor GUI enhancements
//       - ability to operate on DLL files of any bitness, no matter the bitness
//         of the tool
//
// Author:
//
//     vxiiduu (29-Oct-2022)
//
// Revision History:
//
//     vxiiduu               29-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include "kexexprt.h"
#include "resource.h"

VOID EntryPoint(
	VOID)
{
	HWND MainWindow;
	HACCEL Accelerators;
	MSG Message;

	KexgApplicationFriendlyName = L"DLL Export Dumper";

	Accelerators = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_ACCELERATORS));
	MainWindow = CreateDialog(NULL, MAKEINTRESOURCE(IDD_MAINWND), NULL, MainWndProc);

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (TranslateAccelerator(MainWindow, Accelerators, &Message)) {
			continue;
		}

		if (!IsDialogMessage(MainWindow, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}
	
	ExitProcess(0);
}

INT_PTR CALLBACK MainWndProc(
	IN	HWND	MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		HDC DeviceContext;
		UINT TabStops;

		KexgApplicationMainWindow = MainWindow;
		SetWindowText(MainWindow, KexgApplicationFriendlyName);
		CheckDlgButton(MainWindow, IDC_RBDEF, TRUE);
		DragAcceptFiles(MainWindow, TRUE);

		// Set monospace font for the results edit control
		DeviceContext = GetDC(GetDlgItem(MainWindow, IDC_RESULT));

		SendMessage(
			GetDlgItem(MainWindow, IDC_RESULT),
			WM_SETFONT,
			(WPARAM) CreateFont(-MulDiv(8, GetDeviceCaps(DeviceContext, LOGPIXELSY), 72),
								0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
								L"Consolas"),
			MAKELPARAM(TRUE, 0));

		ReleaseDC(GetDlgItem(MainWindow, IDC_RESULT), DeviceContext);

		TabStops = 16;
		Edit_SetTabStops(GetDlgItem(MainWindow, IDC_RESULT), 1, &TabStops);

		ToolTip(MainWindow, IDC_RBDEF, L"Generate module-definition (.def) file statements.");
		ToolTip(MainWindow, IDC_RBPRAGMA, L"Generate #pragma preprocessor directives suitable "
										  L"for embedding in a C/C++ header file.");
	} else if (Message == WM_CLOSE) {
		EndDialog(MainWindow, 0);
		PostQuitMessage(0);
	} else if (Message == WM_COMMAND) {
		USHORT ControlId = LOWORD(WParam);

		if (ControlId == IDCANCEL) {
			PostMessage(MainWindow, WM_CLOSE, 0, 0);
		} else if (ControlId == IDC_BROWSE) {
			WCHAR DllPath[MAX_PATH];
			WCHAR System32Path[MAX_PATH];
			OPENFILENAME OpenFileInfo;

			StringCchPrintf(System32Path, ARRAYSIZE(System32Path), L"%s\\system32",
							SharedUserData->NtSystemRoot);

			DllPath[0] = '\0';

			ZeroMemory(&OpenFileInfo, sizeof(OpenFileInfo));
			OpenFileInfo.lStructSize				= sizeof(OpenFileInfo);
			OpenFileInfo.hwndOwner					= MainWindow;
			OpenFileInfo.lpstrFilter				= L"DLL Files (*.dll, *.exe, *.sys)\0*.dll;*.exe;*.sys\0All Files (*.*)\0*.*\0";
			OpenFileInfo.lpstrFile					= DllPath;
			OpenFileInfo.nMaxFile					= ARRAYSIZE(DllPath);
			OpenFileInfo.lpstrInitialDir			= System32Path;
			OpenFileInfo.lpstrTitle					= L"Select a DLL...";
			OpenFileInfo.Flags						= OFN_FILEMUSTEXIST | OFN_FORCESHOWHIDDEN | OFN_HIDEREADONLY;
			OpenFileInfo.lpstrDefExt				= L"dll";

			if (GetOpenFileName(&OpenFileInfo)) {
				SetDlgItemText(MainWindow, IDC_FILEPATH, DllPath);
			}
		} else if (ControlId == IDOK) {
			KEXEXPRT_GENERATE_STYLE Style;
			WCHAR DllPath[MAX_PATH];
			HWND FilePathWindow;

			FilePathWindow = GetDlgItem(MainWindow, IDC_FILEPATH);

			if (GetWindowTextLength(FilePathWindow) == 0) {
				EDITBALLOONTIP BalloonTip;

				BalloonTip.cbStruct				= sizeof(BalloonTip);
				BalloonTip.pszTitle				= L"Specify a path";
				BalloonTip.pszText				= L"You must specify the full path to a Portable Executable image file, e.g. a DLL.";
				BalloonTip.ttiIcon				= TTI_ERROR;
				Edit_ShowBalloonTip(FilePathWindow, &BalloonTip);

				return TRUE;
			}

			if (IsDlgButtonChecked(MainWindow, IDC_RBDEF)) {
				Style = GenerateStyleDef;
			} else if (IsDlgButtonChecked(MainWindow, IDC_RBPRAGMA)) {
				Style = GenerateStylePragma;
			} else {
				NOT_REACHED;
			}

			GetDlgItemText(MainWindow, IDC_FILEPATH, DllPath, ARRAYSIZE(DllPath));
			DumpExports(MainWindow, DllPath, Style, IsDlgButtonChecked(MainWindow, IDC_INCLUDEORDINALS));
		} else if (ControlId == M_SELECTALL) {
			HWND FocusedWindow;
			WCHAR ClassName[16];

			FocusedWindow = GetFocus();
			GetClassName(FocusedWindow, ClassName, ARRAYSIZE(ClassName));

			if (StringEqual(ClassName, L"Edit")) {
				Edit_SetSel(FocusedWindow, 0, INT_MAX);
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_DROPFILES) {
		HDROP DroppedItem;
		WCHAR DroppedFilePath[MAX_PATH];

		DroppedItem = (HDROP) WParam;

		DragQueryFile(DroppedItem, 0, DroppedFilePath, ARRAYSIZE(DroppedFilePath));
		SetDlgItemText(MainWindow, IDC_FILEPATH, DroppedFilePath);
		DragFinish(DroppedItem);

		PostMessage(MainWindow, WM_COMMAND, IDOK, 0);
	} else if (Message == WM_CTLCOLORSTATIC) {
		HWND Control;

		//
		// Make the read-only results edit control have a white background.
		// By default, read-only edit controls are greyed out.
		//

		Control = (HWND) LParam;

		if (GetWindowLongPtr(Control, GWLP_ID) == IDC_RESULT) {
			return (INT_PTR) GetStockObject(WHITE_BRUSH);
		} else {
			return FALSE;
		}
	} else {
		// unhandled message
		return FALSE;
	}

	return TRUE;
}