#include "vxlview.h"
#include "resource.h"

NTSTATUS NTAPI EntryPoint(
	IN	PVOID	Parameter)
{
	HACCEL Accelerators;
	MSG Message;
	INITCOMMONCONTROLSEX InitComctl;

	InitComctl.dwSize = sizeof(InitComctl);
	InitComctl.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitComctl);

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;

	Accelerators = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_ACCELERATORS));
	CreateDialog(NULL, MAKEINTRESOURCE(IDD_MAINWND), NULL, MainWndProc);

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (TranslateAccelerator(MainWindow, Accelerators, &Message)) {
			continue;
		}

		if (!IsDialogMessage(MainWindow, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	LdrShutdownProcess();
	return NtTerminateProcess(NtCurrentProcess(), STATUS_SUCCESS);
}

INT_PTR CALLBACK MainWndProc(
	IN	HWND	_MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		BOOLEAN Success;
		PWSTR CommandLine;

		UNCONST (HWND) MainWindow = _MainWindow;
		UNCONST (HWND) StatusBarWindow = GetDlgItem(MainWindow, IDC_STATUSBAR);

		KexgApplicationMainWindow = MainWindow;
		SetWindowIcon(MainWindow, IDI_APPICON);

		InitializeBackend();
		InitializeListView();
		InitializeFilterControls();
		InitializeDetailsWindow();
		
		UpdateMainMenu();
		RestoreWindowPlacement();
		RestoreListViewColumns();

		CommandLine = GetCommandLineWithoutImageName();
		if (CommandLine[0] == '\0') {
			if (PROMPT_FOR_FILE_ON_STARTUP) {
				Success = OpenLogFileWithPrompt();
			} else {
				Success = TRUE;
			}
		} else {
			if (CommandLine[0] == '"') {
				PathUnquoteSpaces(CommandLine);
			}

			Success = OpenLogFile(CommandLine);
		}

		if (!Success) {
			ExitProcess(0);
		}
	} else if (Message == WM_CLOSE) {
		SaveListViewColumns();
		SaveWindowPlacement();
		CleanupBackend();
		ExitProcess(0);
	} else if (Message == WM_SIZE && (WParam == SIZENORMAL || WParam == SIZEFULLSCREEN)) {
		USHORT MainWndNewWidth = LOWORD(LParam);

		// DON'T change the order of the resize commands. They depend on each other.
		ResizeStatusBar(MainWndNewWidth);
		ResizeFilterControls();
		ResizeDetailsWindow();
		ResizeListView(MainWndNewWidth);
	} else if (Message == WM_GETMINMAXINFO) {
		PMINMAXINFO MinMaxInfo = (PMINMAXINFO) LParam;

		// Don't allow user to shrink the window too much.
		MinMaxInfo->ptMinTrackSize.x = DpiScaleX(800);
		MinMaxInfo->ptMinTrackSize.y = DpiScaleY(600);
	} else if (Message == WM_SETCURSOR) {
		HCURSOR Cursor;

		Cursor = (HCURSOR) GetClassLongPtr(MainWindow, GCLP_HCURSOR);

		// if the cursor is default, keep the default behavior - otherwise,
		// override the cursor. (intended for busy cursor)
		if (Cursor != LoadCursor(NULL, IDC_ARROW)) {
			SetCursor((HCURSOR) GetClassLongPtr(MainWindow, GCLP_HCURSOR));
			SetWindowLongPtr(MainWindow, DWLP_MSGRESULT, TRUE);
		} else {
			return FALSE;
		}
	} else if (Message == WM_COMMAND) {
		USHORT MessageSource;
		HWND FocusedWindow;
		WCHAR ClassName[32];

		MessageSource = LOWORD(WParam);

		switch (MessageSource) {
		case M_EXIT:
			PostMessage(MainWindow, WM_CLOSE, 0, 0);
			break;
		case M_OPEN:
			OpenLogFileWithPrompt();
			break;
		case M_EXPORT:
			ExportLogWithPrompt();
			break;
		case M_FIND:
			FocusedWindow = GetDlgItem(FilterWindow, IDC_SEARCHBOX);
			Edit_SetSel(FocusedWindow, 0, INT_MAX);
			SetFocus(FocusedWindow);
			break;
		case M_GOTORAW:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_GOTORAW), MainWindow, GotoRawDlgProc);
			break;
		case M_SELECTALL:
			FocusedWindow = GetFocus();
			GetClassName(FocusedWindow, ClassName, ARRAYSIZE(ClassName));

			if (StringEqual(ClassName, L"Edit")) {
				Edit_SetSel(FocusedWindow, 0, INT_MAX);
			}
			
			break;
		case M_ABOUT:
			DialogBox(NULL, MAKEINTRESOURCE(IDD_ABOUT), MainWindow, AboutWndProc);
			break;
		default:
			return FALSE;
		}
	} else if (Message == WM_NOTIFY) {
		LPNMHDR Notification;

		Notification = (LPNMHDR) LParam;

		if (Notification->idFrom == IDC_LISTVIEW) {
			if (Notification->code == LVN_GETDISPINFO) {
				LPLVITEM Item;

				Item = &((NMLVDISPINFO *) LParam)->item;
				PopulateListViewItem(Item);
			} else if (Notification->code == LVN_ITEMCHANGED) {
				LPNMLISTVIEW ChangedItemInfo;

				ChangedItemInfo = (LPNMLISTVIEW) LParam;
				if (ChangedItemInfo->uNewState & LVIS_FOCUSED) {
					SetWindowTextF(StatusBarWindow, L"Entry #%d selected.", 
								   GetLogEntryRawIndex(ChangedItemInfo->iItem) + 1);
					PopulateDetailsWindow(ChangedItemInfo->iItem);
				}
			} else {
				return FALSE;
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_CONTEXTMENU) {
		HWND Window;
		POINT ClickPoint;

		Window = (HWND) WParam;
		ClickPoint.x = GET_X_LPARAM(LParam);
		ClickPoint.y = GET_Y_LPARAM(LParam);

		if (Window == ListViewWindow) {
			HandleListViewContextMenu(&ClickPoint);
		}
	} else {
		// message not handled
		return FALSE;
	}

	return TRUE;
}

//
// Enable or disable menu items as necessary.
//
VOID UpdateMainMenu(
	VOID)
{
	HMENU MainMenu;

	MainMenu = GetMenu(MainWindow);

	if (IsLogFileOpened()) {
		EnableMenuItem(MainMenu, M_EXPORT, MF_ENABLED);
	} else {
		EnableMenuItem(MainMenu, M_EXPORT, MF_GRAYED);
	}
}