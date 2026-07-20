///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     gui.c
//
// Abstract:
//
//     Implements the main GUI of KexCfg.
//
// Author:
//
//     vxiiduu (09-Feb-2024)
//
// Environment:
//
//     Win32 mode. This part of the program is not run as SYSTEM.
//
// Revision History:
//
//     vxiiduu              09-Feb-2024  Initial creation.
//     vxiiduu              09-May-2026  Alt+double click on list view now opens
//                                       item properties.
//     vxiiduu              19-May-2026  Make KexCfg window resizable.
//     vxiiduu              30-Jun-2026  Prevent Explorer or MSIEXEC from showing
//                                       in the list of VxKex-enabled applications.
//     vxiiduu              04-Jul-2026  Prevent infinite loop of re-launching
//                                       if user is running as non-admin and UAC
//                                       is disabled.
//     vxiiduu              08-Jul-2026  Prevent error message or assertion dialog
//                                       from appearing when the user double-clicks
//                                       in an empty area of the list view.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexcfg.h"
#include "resource.h"
#include <KexGui.h>
#include <ShlObj.h>

STATIC HWND MainWindow = NULL;
STATIC HWND ListViewWindow = NULL;
STATIC BOOLEAN UnsavedChanges;
STATIC BOOLEAN SortOrderIsDescendingForColumn[2] = {0};

STATIC VOID KexCfgGuiPopulateGlobalConfiguration(
	VOID)
{
	BOOLEAN ExtendedContextMenu;
	BOOLEAN LoggingEnabled;
	WCHAR LogDir[MAX_PATH];

	//
	// Populate the Logging control group
	//

	KxCfgQueryLoggingSettings(TRUE, &LoggingEnabled, LogDir, ARRAYSIZE(LogDir), NULL);
	CheckDlgButton(MainWindow, IDC_ENABLELOGGING, LoggingEnabled);
	SetDlgItemText(MainWindow, IDC_LOGDIR, LogDir);

	//
	// Populate the System Integration control group
	//

	CheckDlgButton(MainWindow, IDC_ENABLEFORMSI, KxCfgQueryVxKexEnabledForMsiexec());
	CheckDlgButton(MainWindow, IDC_CPIWBYPA, KxCfgQueryExplorerCpiwBypass());

	if (KxCfgQueryShellContextMenuEntries(&ExtendedContextMenu)) {
		CheckDlgButton(MainWindow, IDC_ADDTOMENU, TRUE);
		ComboBox_SetCurSel(GetDlgItem(MainWindow, IDC_WHICHCONTEXTMENU), ExtendedContextMenu ? 0 : 1);
	} else {
		CheckDlgButton(MainWindow, IDC_ADDTOMENU, FALSE);
	}
}

STATIC VOID KexCfgGuiApplyGlobalConfiguration(
	VOID)
{
	BOOLEAN Success;
	HANDLE TransactionHandle;
	BOOLEAN EnableLogging;
	BOOLEAN EnableForMsiexec;
	BOOLEAN EnableCpiwbypa;
	BOOLEAN EnableContextMenu;
	BOOLEAN ExtendedContextMenu;
	WCHAR LogDir[MAX_PATH];

	//
	// Gather inputs
	//

	EnableForMsiexec = IsDlgButtonChecked(MainWindow, IDC_ENABLEFORMSI);
	EnableCpiwbypa = IsDlgButtonChecked(MainWindow, IDC_CPIWBYPA);
	EnableContextMenu = IsDlgButtonChecked(MainWindow, IDC_ADDTOMENU);
	ExtendedContextMenu = (ComboBox_GetCurSel(GetDlgItem(MainWindow, IDC_WHICHCONTEXTMENU)) == 0);
	EnableLogging = IsDlgButtonChecked(MainWindow, IDC_ENABLELOGGING);

	GetDlgItemText(MainWindow, IDC_LOGDIR, LogDir, ARRAYSIZE(LogDir));

	//
	// Create transaction for the operation
	//

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		ErrorBoxF(
			_(L"A transaction for this operation could not be created. %s"),
			GetLastErrorAsString());

		return;
	}

	//
	// Apply Logging settings
	//

	Success = KxCfgConfigureLoggingSettings(
		TRUE,
		EnableLogging,
		LogDir,
		TransactionHandle);

	//
	// Apply system integration settings
	//

	Success = KxCfgEnableVxKexForMsiexec(EnableForMsiexec, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	Success = KxCfgEnableExplorerCpiwBypass(EnableCpiwbypa, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	Success = KxCfgConfigureShellContextMenuEntries(EnableContextMenu, ExtendedContextMenu, TransactionHandle);
	if (!Success) {
		goto Fail;
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	UnsavedChanges = FALSE;
	return;

Fail:
	NtRollbackTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	ErrorBoxF(_(L"The settings could not be applied. %s"), GetLastErrorAsString());
}

BOOLEAN CALLBACK ConfigurationCallback(
	IN	PCWSTR							ExeFullPathOrBaseName,
	IN	BOOLEAN							IsLegacyConfiguration,
	IN	PVOID							ExtraParameter)
{
	WCHAR ExeContainingFolder[MAX_PATH];
	PWSTR ExeBaseName;
	LVITEM ListViewItem;
	SHFILEINFO ShellFileInfo;
	ULONG_PTR Success;

	if (IsLegacyConfiguration) {
		// The installer should've removed all this crap
		ASSERT (!IsLegacyConfiguration);
		return TRUE;
	}

	StringCchCopy(
		ExeContainingFolder,
		ARRAYSIZE(ExeContainingFolder),
		ExeFullPathOrBaseName);

	PathCchRemoveFileSpec(ExeContainingFolder, ARRAYSIZE(ExeContainingFolder));
	ExeBaseName = PathFindFileName(ExeFullPathOrBaseName);

	if (PathIsPrefix(SharedUserData->NtSystemRoot, ExeFullPathOrBaseName)) {
		// This exe is in the Windows folder.
		// It could either be MSIEXEC, Explorer, or any other specially-allowed
		// Windows EXE. These are represented by checkboxes in the system integration
		// group, so we won't show them here.
		return TRUE;
	}

	//
	// Get icon of file. SHGetFileInfo works for all types of files including
	// MSI files, and gives us the same icon that would be displayed in Windows
	// Explorer.
	//

	Success = SHGetFileInfo(
		ExeFullPathOrBaseName,
		0,
		&ShellFileInfo,
		sizeof(ShellFileInfo),
		SHGFI_ICON | SHGFI_SMALLICON | SHGFI_SYSICONINDEX);

	RtlZeroMemory(&ListViewItem, sizeof(ListViewItem));

	if (Success) {
		ListViewItem.mask |= LVIF_IMAGE;
		ListViewItem.iImage = ShellFileInfo.iIcon;
		DestroyIcon(ShellFileInfo.hIcon);
	}

	ListViewItem.mask	   |= LVIF_TEXT;
	ListViewItem.iItem		= INT_MAX;
	ListViewItem.pszText	= ExeBaseName;

	//
	// Insert the item and set text for all columns.
	//

	ListViewItem.iItem = ListView_InsertItem(ListViewWindow, &ListViewItem);
	ListView_SetItemText(ListViewWindow, ListViewItem.iItem, 1, ExeContainingFolder);

	return TRUE;
}

STATIC VOID KexCfgGuiPopulateApplicationList(
	VOID)
{
	SetWindowRedraw(ListViewWindow, FALSE);

	ListView_DeleteAllItems(ListViewWindow);
	KxCfgEnumerateConfiguration(ConfigurationCallback, NULL);

	if (ListView_GetItemCount(ListViewWindow) != 0) {
		// enable Clean button
		EnableWindow(GetDlgItem(MainWindow, IDC_CLEANAPPS), TRUE);

		// reflow column widths
		ListView_SetColumnWidth(ListViewWindow, 0, LVSCW_AUTOSIZE);
		ListView_SetColumnWidth(ListViewWindow, 1, LVSCW_AUTOSIZE_USEHEADER);
	} else {
		// disable Clean button
		EnableWindow(GetDlgItem(MainWindow, IDC_CLEANAPPS), FALSE);
	}

	SetWindowRedraw(ListViewWindow, TRUE);
}

STATIC PCWSTR GetProgramFullPathFromListViewIndex(
	IN	ULONG	ItemIndex)
{
	STATIC WCHAR FilePath[MAX_PATH];
	WCHAR FileName[MAX_PATH];

	ListView_GetItemText(ListViewWindow, ItemIndex, 0, FileName, ARRAYSIZE(FileName));
	ListView_GetItemText(ListViewWindow, ItemIndex, 1, FilePath, ARRAYSIZE(FilePath));
	PathCchAppend(FilePath, ARRAYSIZE(FilePath), FileName);

	return FilePath;
}

STATIC VOID RemoveSelectedPrograms(
	VOID)
{
	ULONG ItemIndex;
	HANDLE TransactionHandle;

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		ErrorBoxF(
			_(L"A transaction for this operation could not be created. %s"),
			GetLastErrorAsString());

		return;
	}

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVNI_SELECTED);

	while (ItemIndex != -1) {
		PCWSTR ExeFullPath;
		BOOLEAN Success;

		ExeFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

		Success = KxCfgDeleteConfiguration(ExeFullPath, TransactionHandle);
		if (!Success) {
			ErrorBoxF(
				_(L"There was an error applying settings for \"%s\". %s"),
				ExeFullPath, GetLastErrorAsString());

			NtRollbackTransaction(TransactionHandle, TRUE);
			SafeClose(TransactionHandle);
			return;
		}

		ItemIndex = ListView_GetNextItem(ListViewWindow, ItemIndex, LVNI_SELECTED);
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	// refresh the list of enabled applications (since we removed some)
	KexCfgGuiPopulateApplicationList();
}

STATIC VOID AddProgram(
	VOID)
{
	WCHAR FileNames[1024];
	WCHAR FileFullPath[MAX_PATH];
	WCHAR KexDir[MAX_PATH];
	PCWSTR DirectoryName;
	PCWSTR FileName;
	ULONG DirectoryNameCch;
	OPENFILENAME OpenFileInfo;
	KXCFG_PROGRAM_CONFIGURATION Configuration;
	HANDLE TransactionHandle;

	//
	// Get a list of one or more programs from the user.
	//

	FileNames[0] = '\0';

	RtlZeroMemory(&OpenFileInfo, sizeof(OpenFileInfo));
	OpenFileInfo.lStructSize		= sizeof(OpenFileInfo);
	OpenFileInfo.hwndOwner			= MainWindow;
	OpenFileInfo.lpstrFilter		= L"Programs (*.exe, *.msi)\0*.exe;*.msi\0";
	OpenFileInfo.lpstrFile			= FileNames;
	OpenFileInfo.nMaxFile			= ARRAYSIZE(FileNames);
	OpenFileInfo.lpstrTitle			= _(L"Select Program(s)");
	OpenFileInfo.Flags				= OFN_EXPLORER | OFN_ALLOWMULTISELECT |
									  OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	OpenFileInfo.lpstrDefExt		= L"exe";

	if (GetOpenFileName(&OpenFileInfo) == FALSE) {
		ASSERT (CommDlgExtendedError() == 0);
		return;
	}

	ASSERT (FileNames[0] != '\0');

	DirectoryName = FileNames;
	DirectoryNameCch = (ULONG) wcslen(DirectoryName);
	ASSERT (DirectoryNameCch != 0);
	ASSERT (DirectoryNameCch < MAX_PATH);

	if (StringBeginsWithI(DirectoryName, SharedUserData->NtSystemRoot)) {
		// program(s) are in the Windows directory - do not allow
		ErrorBoxF(_(L"You cannot enable VxKex for programs in the Windows directory."));
		return;
	}

	KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));

	if (StringBeginsWithI(DirectoryName, KexDir)) {
		ErrorBoxF(_(L"You cannot enable VxKex for programs in the VxKex installation directory."));
		return;
	}

	if (OpenFileInfo.nFileOffset >= DirectoryNameCch) {
		// User selected more than one file.
		RtlCopyMemory(FileFullPath, DirectoryName, DirectoryNameCch * sizeof(WCHAR));
		FileFullPath[DirectoryNameCch] = '\\';
		FileFullPath[DirectoryNameCch + 1] = '\0';
		++DirectoryNameCch;

		FileName = FileNames + DirectoryNameCch;
	} else {
		// User selected only one file.
		FileName = FileNames;
		FileNames[DirectoryNameCch + 1] = '\0';
		DirectoryNameCch = 0;
	}

	ASSERT (FileName[0] != '\0');

	KexRtlZeroMemory(&Configuration, sizeof(Configuration));
	Configuration.Enabled = 1;

	//
	// Create transaction for the operation.
	//

	TransactionHandle = CreateSimpleTransaction(L"VxKex Configuration Tool (GUI) Transaction");

	if (!TransactionHandle) {
		ErrorBoxF(
			_(L"A transaction for this operation could not be created. %s"),
			GetLastErrorAsString());

		return;
	}

	//
	// Enable VxKex for each of the selected programs.
	//

	until (FileName[0] == '\0') {
		BOOLEAN Success;

		StringCchCopy(
			FileFullPath + DirectoryNameCch,
			ARRAYSIZE(FileFullPath) - DirectoryNameCch,
			FileName);

		Success = KxCfgSetConfiguration(
			FileFullPath,
			&Configuration,
			TransactionHandle);

		if (!Success) {
			ErrorBoxF(
				_(L"There was an error applying settings for \"%s\". %s"),
				FileFullPath, GetLastErrorAsString());

			NtRollbackTransaction(TransactionHandle, TRUE);
			SafeClose(TransactionHandle);
			return;
		}

		FileName = FileName + wcslen(FileName) + 1;
	}

	NtCommitTransaction(TransactionHandle, TRUE);
	SafeClose(TransactionHandle);

	KexCfgGuiPopulateApplicationList();
}

STATIC VOID ProgramNotFoundUserPrompt(
	IN	PCWSTR	ProgramFullPath)
{
	INT UserResponse;

	UserResponse = MessageBoxF(
		TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
		NULL,
		FRIENDLYAPPNAME,
		_(L"The selected program cannot be found"),
		_(L"The program \"%s\" was moved or deleted. "
		  L"Would you like to remove this entry from the list of VxKex-enabled applications?"),
		ProgramFullPath);

	if (UserResponse == IDYES) {
		RemoveSelectedPrograms();
	}
}

STATIC VOID OpenSelectedItemLocation(
	VOID)
{
	HRESULT Result;
	ULONG ItemIndex;
	PCWSTR ProgramFullPath;

	ASSERT (IsWindow(ListViewWindow));
	ASSERT (ListView_GetSelectedCount(ListViewWindow) == 1);

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	Result = OpenFileLocation(ProgramFullPath, 0);

	if (FAILED(Result)) {
		if (Result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
			//
			// Special case for file not found. We will ask the user whether he wants
			// to remove this entry since the EXE doesn't exist anymore.
			//

			ProgramNotFoundUserPrompt(ProgramFullPath);
		} else {
			// display generic error box
			ErrorBoxF(_(L"Couldn't open file location. %s"), Win32ErrorAsString(Result));
		}
	}
}

STATIC VOID OpenSelectedItemProperties(
	VOID)
{
	BOOLEAN Success;
	PCWSTR ProgramFullPath;
	ULONG ItemIndex;

	ASSERT (ListView_GetSelectedCount(ListViewWindow) == 1);

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	Success = ShowPropertiesDialog(ProgramFullPath, SW_SHOW);
	if (!Success) {
		ULONG ErrorCode;

		ErrorCode = GetLastError();

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			ProgramNotFoundUserPrompt(ProgramFullPath);
		} else {
			ErrorBoxF(_(L"Couldn't open file properties. %s"), Win32ErrorAsString(ErrorCode));
		}
	}
}

STATIC VOID RunSelectedProgram(
	VOID)
{
	PCWSTR ProgramFullPath;
	PCWSTR ErrorMessage;
	ULONG_PTR ErrorCode;
	ULONG ItemIndex;

	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);
	ProgramFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

	//
	// Enable CPIW bypass so that we have the ability to run programs with a
	// subsystem version higher than 6.1.
	//

	KexPatchCpiwSubsystemVersionCheck();

	// why ShellExecute and not CreateProcess? because we can have .msi files too.
	ErrorCode = (ULONG_PTR) ShellExecute(
		MainWindow,
		L"open",
		ProgramFullPath,
		NULL,
		NULL,
		SW_SHOWDEFAULT);

	ErrorMessage = NULL;

	if (ErrorCode <= 32) {
		switch (ErrorCode) {
		case SE_ERR_FNF:
		case SE_ERR_PNF:
			ProgramNotFoundUserPrompt(ProgramFullPath);
			break;
		case SE_ERR_ACCESSDENIED:
			ErrorMessage = _(L"Access was denied or the executable file format is invalid.");
			break;
		case SE_ERR_OOM:
			ErrorMessage = _(L"There was not enough memory to complete the operation.");
			break;
		case SE_ERR_SHARE:
			ErrorMessage = _(L"A sharing violation occurred.");
			break;
		case SE_ERR_ASSOCINCOMPLETE:
			ErrorMessage = L"SE_ERR_ASSOCINCOMPLETE";
			break;
		case SE_ERR_DDETIMEOUT:
			ErrorMessage = L"SE_ERR_DDETIMEOUT";
			break;
		case SE_ERR_DDEFAIL:
			ErrorMessage = L"SE_ERR_DDEFAIL";
			break;
		case SE_ERR_DDEBUSY:
			ErrorMessage = L"SE_ERR_DDEBUSY";
			break;
		case SE_ERR_NOASSOC:
			ErrorMessage = L"SE_ERR_NOASSOC";
			break;
		case SE_ERR_DLLNOTFOUND:
			ErrorMessage = _(L"A DLL was not found.");
			break;
		default:
			ErrorMessage = _(L"An unknown error has occurred.");
			break;
		}
	}

	if (ErrorMessage) {
		ErrorBoxF(L"\"%s\": %s", ProgramFullPath, ErrorMessage);
	}
}

STATIC BOOLEAN ShouldCleanProgramConfiguration(
	IN	PCWSTR	ExeFullPath)
{
	WCHAR Root[MAX_PATH];
	HRESULT Result;

	if (FileExists(ExeFullPath)) {
		return FALSE;
	}

	//
	// The file doesn't currently exist. Determine what to do based
	// on the path type and drive type.
	//

	if (PathIsNetworkPath(ExeFullPath)) {
		// It's a network path. Don't remove the configuration because
		// the network path might just be disconnected.
		return FALSE;
	}

	StringCchCopy(Root, ARRAYSIZE(Root), ExeFullPath);
	Result = PathCchStripToRoot(Root, ARRAYSIZE(Root));
	ASSERT (SUCCEEDED(Result));

	if (SUCCEEDED(Result) && GetDriveType(Root) == DRIVE_FIXED) {
		// The drive is a fixed, connected drive (e.g. C:) but the file
		// does not exist. Open and shut case sherlock, we need to delete
		// the configuration for this nonexistent program.
		return TRUE;
	}

	return FALSE;
}

STATIC VOID CleanPrograms(
	VOID)
{
	ULONG ItemIndex;
	ULONG NumberOfItemsDeleted;

	NumberOfItemsDeleted = 0;
	ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVNI_ALL);

	while (ItemIndex != -1) {
		PCWSTR ExeFullPath;

		ExeFullPath = GetProgramFullPathFromListViewIndex(ItemIndex);

		if (ShouldCleanProgramConfiguration(ExeFullPath)) {
			BOOLEAN Success;

			Success = KxCfgDeleteConfiguration(ExeFullPath, NULL);

			if (Success) {
				ListView_DeleteItem(ListViewWindow, ItemIndex);
				++NumberOfItemsDeleted;
				--ItemIndex;
			}
		}

		ItemIndex = ListView_GetNextItem(ListViewWindow, ItemIndex, LVNI_ALL);
	}

	switch (NumberOfItemsDeleted) {
	case 0:
		InfoBoxF(_(L"No programs require cleaning from the list of VxKex-enabled applications."));
		break;
	case 1:
		InfoBoxF(_(L"1 program was cleaned from the list of VxKex-enabled applications."));
		break;
	default:
		InfoBoxF(
			_(L"%lu programs were cleaned from the list of VxKex-enabled applications."),
			NumberOfItemsDeleted);
		break;
	}
}

STATIC VOID HandleListViewContextMenu(
	IN	PPOINT	ClickPoint)
{
	HWND HeaderWindow;
	RECT HeaderWindowRect;
	LVHITTESTINFO HitTestInfo;
	ULONG ItemIndex;
	USHORT MenuId;
	INT DefaultMenuItem;
	ULONG MenuSelection;
	ULONG ListViewSelectedCount;

	HeaderWindow = ListView_GetHeader(ListViewWindow);
	GetWindowRect(HeaderWindow, &HeaderWindowRect);
	
	if (PtInRect(&HeaderWindowRect, *ClickPoint)) {
		// The user has right clicked on the list view header.
		return;
	}

	HitTestInfo.pt = *ClickPoint;
	ScreenToClient(ListViewWindow, &HitTestInfo.pt);
	ItemIndex = ListView_HitTest(ListViewWindow, &HitTestInfo);
	ListViewSelectedCount = ListView_GetSelectedCount(ListViewWindow);
	DefaultMenuItem = -1;

	if (ItemIndex == -1) {
		// The user has right clicked on a blank space in the list view.
		MenuId = IDM_LISTVIEWBLANKSPACEMENU;
	} else {
		// The user has right clicked on one or more items.
		if (ListViewSelectedCount == 1) {
			MenuId = IDM_LISTVIEWITEMMENU;
			DefaultMenuItem = M_OPENFILELOCATION;
		} else if (ListViewSelectedCount > 1) {
			// more than 1 item
			MenuId = IDM_LISTVIEWMULTIITEMMENU;
		} else {
			// can happen if Ctrl key is held down.
			// in this situation, programs like Windows Explorer will display
			// the same menu as for right clicking on blank space.
			MenuId = IDM_LISTVIEWBLANKSPACEMENU;
		}
	}

	MenuSelection = ContextMenuEx(ListViewWindow, MenuId, ClickPoint, DefaultMenuItem);
	if (!MenuSelection) {
		return;
	}
	
	if (MenuSelection == M_OPENFILELOCATION) {
		ASSERT (ListViewSelectedCount == 1);
		OpenSelectedItemLocation();
	} else if (MenuSelection == M_RUNPROGRAM) {
		ASSERT (ListViewSelectedCount == 1);
		RunSelectedProgram();
	} else if (MenuSelection == M_PROPERTIES) {
		ASSERT (ListViewSelectedCount == 1);
		OpenSelectedItemProperties();
	} else if (MenuSelection == M_REMOVE) {
		RemoveSelectedPrograms();
	} else if (MenuSelection == M_ADDPROGRAM) {
		AddProgram();
	} else if (MenuSelection == M_CLEANPROGRAMS) {
		CleanPrograms();
	}
}

STATIC INT CALLBACK KexCfgGuiSortListViewItems(
	IN	LPARAM	LParam1,
	IN	LPARAM	LParam2,
	IN	LPARAM	ColumnIndex)
{
	WCHAR Text1[MAX_PATH];
	WCHAR Text2[MAX_PATH];
	ULONG Length1;
	ULONG Length2;
	INT ComparisonResult;
	
	ListView_GetItemTextEx(ListViewWindow, LParam1, (INT) ColumnIndex, Text1, ARRAYSIZE(Text1), &Length1);
	ListView_GetItemTextEx(ListViewWindow, LParam2, (INT) ColumnIndex, Text2, ARRAYSIZE(Text2), &Length2);

	ComparisonResult = CompareString(
		LOCALE_USER_DEFAULT,
		0,
		Text1,
		Length1,
		Text2,
		Length2);

	ASSERT (ComparisonResult != 0);
	ComparisonResult -= 2;

	ASSERT (ColumnIndex < ARRAYSIZE(SortOrderIsDescendingForColumn));

	if (SortOrderIsDescendingForColumn[ColumnIndex]) {
		ComparisonResult = -ComparisonResult;
	}

	return ComparisonResult;
}

STATIC VOID HandleWindowResize(
	IN	HWND	Window,
	IN	USHORT	NewWidth,
	IN	USHORT	NewHeight)
{
	// Poor man's CSS!
	CONST ULONG ColumnButtonRightPadding = 7;
	CONST ULONG RowButtonBottomPadding = 11;
	CONST ULONG RowButtonSpacing = 2;
	CONST ULONG ListViewBottomPadding = RowButtonBottomPadding;
	CONST ULONG ListViewRightPadding = 7;

	HWND Control;
	RECT Rect;
	ULONG Index;
	ULONG PreviousButtonLeft;
	ULONG RightOfGroupBoxes;
	ULONG LeftOfColumnButtons;
	ULONG TopOfRowButtons;

	STATIC CONST INT GroupBoxIds[] = {
		IDC_GB_LOGGING,
		IDC_GB_INTEGRATION,
	};

	STATIC CONST INT GroupBoxEditControlIds[] = {
		IDC_LOGDIR,
	};

	STATIC CONST INT RowButtonIds[] = {
		// The order must be preserved for row button IDs
		IDC_APPLY,
		IDC_CANCEL,
		IDC_OK,
	};

	STATIC CONST INT ColumnButtonIds[] = {
		IDC_BROWSELOGDIR,
		IDC_NEWAPP,
		IDC_REMOVEAPPS,
		IDC_PROPERTIES,
		IDC_CLEANAPPS,
	};

	//
	// Resize group boxes.
	//

	for (Index = 0; Index < ARRAYSIZE(GroupBoxIds); ++Index) {
		ULONG NewGroupBoxWidth;

		Control = GetDlgItem(Window, GroupBoxIds[Index]);

		GetWindowRect(Control, &Rect);
		MapWindowRect(HWND_DESKTOP, Window, &Rect);

		// same padding on left and right
		NewGroupBoxWidth = NewWidth - (2 * Rect.left);

		SetWindowPos(
			Control,
			NULL,
			0,
			0,
			NewGroupBoxWidth,
			Rect.bottom - Rect.top,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

		RightOfGroupBoxes = Rect.left + NewGroupBoxWidth;

		// Hack to prevent flickering of the checkboxes.
		GetWindowRect(Control, &Rect);
		MapWindowRect(HWND_DESKTOP, Window, &Rect);
		Rect.left = DpiScaleX(450);
		Rect.right = NewWidth;

		if (Rect.left < Rect.right) {
			InvalidateRect(Window, &Rect, FALSE);
		}
	}

	//
	// Move column buttons so that their right edge is at an offset
	// from the right edge of the group boxes.
	//

	for (Index = 0; Index < ARRAYSIZE(ColumnButtonIds); ++Index) {
		ULONG NewButtonLeft; 
		ULONG NewButtonTop;

		Control = GetDlgItem(Window, ColumnButtonIds[Index]);

		GetWindowRect(Control, &Rect);
		MapWindowRect(HWND_DESKTOP, Window, &Rect);

		NewButtonLeft = RightOfGroupBoxes -
						(Rect.right - Rect.left) -
						DpiScaleX(ColumnButtonRightPadding);

		NewButtonTop = Rect.top;

		SetWindowPos(
			Control,
			NULL,
			NewButtonLeft,
			NewButtonTop,
			0,
			0,
			SWP_NOZORDER | SWP_NOSIZE);

		LeftOfColumnButtons = NewButtonLeft;
	}

	//
	// Move row buttons.
	//

	PreviousButtonLeft = 0;

	for (Index = 0; Index < ARRAYSIZE(RowButtonIds); ++Index) {
		ULONG NewButtonLeft;
		ULONG NewButtonTop;

		Control = GetDlgItem(Window, RowButtonIds[Index]);

		GetWindowRect(Control, &Rect);
		MapWindowRect(HWND_DESKTOP, Window, &Rect);

		if (PreviousButtonLeft == 0) {
			NewButtonLeft = LeftOfColumnButtons;
		} else {
			NewButtonLeft = PreviousButtonLeft -
							(Rect.right - Rect.left) -
							DpiScaleX(RowButtonSpacing);
		}

		NewButtonTop = NewHeight -
						(Rect.bottom - Rect.top) -
						DpiScaleY(RowButtonBottomPadding);

		SetWindowPos(
			Control,
			NULL,
			NewButtonLeft,
			NewButtonTop,
			0,
			0,
			SWP_NOZORDER | SWP_NOSIZE);

		// Because we move the buttons one after the other, they can
		// overwrite each other if the window is resized rapidly and
		// cause artifacts. In order to fix this we invalidate.
		InvalidateRect(Control, NULL, FALSE);

		PreviousButtonLeft = NewButtonLeft;
		TopOfRowButtons = NewButtonTop;
	}

	//
	// Resize edit controls.
	//

	for (Index = 0; Index < ARRAYSIZE(GroupBoxEditControlIds); ++Index) {
		Control = GetDlgItem(Window, GroupBoxEditControlIds[Index]);

		GetWindowRect(Control, &Rect);
		MapWindowRect(HWND_DESKTOP, Window, &Rect);

		SetWindowPos(
			Control,
			NULL,
			0,
			0,
			LeftOfColumnButtons - Rect.left - DpiScaleX(ColumnButtonRightPadding),
			Rect.bottom - Rect.top,
			SWP_NOMOVE | SWP_NOZORDER);
	}

	//
	// Resize the applications list view so that its right edge is at
	// an offset from the left edge of the column buttons, and its
	// bottom edge is at an offset from the row buttons.
	//

	GetWindowRect(ListViewWindow, &Rect);
	MapWindowRect(HWND_DESKTOP, Window, &Rect);

	SetWindowPos(
		ListViewWindow,
		NULL,
		0, 0,
		LeftOfColumnButtons - Rect.left - DpiScaleX(ListViewRightPadding),
		TopOfRowButtons - Rect.top - DpiScaleY(ListViewBottomPadding),
		SWP_NOZORDER | SWP_NOMOVE);
}

#define WNDPOS_REG_KEY L"SOFTWARE\\VXsoft\\KexCfg"

STATIC INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		BOOLEAN Success;
		HWND ComboBoxWindow;
		HWND LogDirWindow;
		LVCOLUMN Column;
		HIMAGELIST SystemSmallImageList = NULL;
		HIMAGELIST SystemLargeImageList = NULL;

		MainWindow = Window;
		KexgApplicationMainWindow = Window;
		SetWindowIcon(Window, IDI_APPICON);

		//
		// Translate static window text with MLS.
		//

		MlsgTranslateWindow(Window);

		//
		// Limit the max length of the log directory path to 200 characters.
		// Make the path edit box autocomplete paths.
		//

		LogDirWindow = GetDlgItem(Window, IDC_LOGDIR);
		Edit_LimitText(LogDirWindow, 200);
		SHAutoComplete(LogDirWindow, SHACF_FILESYS_DIRS | SHACF_USETAB);
		
		//
		// Initialize the context menu selection combo box.
		//

		ComboBoxWindow = GetDlgItem(Window, IDC_WHICHCONTEXTMENU);
		ComboBox_AddString(ComboBoxWindow, _(L"extended context menu"));
		ComboBox_AddString(ComboBoxWindow, _(L"normal context menu"));
		ComboBox_SetCurSel(ComboBoxWindow, 0);

		//
		// Initialize the application list view.
		//

		ListViewWindow = GetDlgItem(Window, IDC_LISTVIEW);
		SetWindowTheme(ListViewWindow, L"Explorer", NULL);

		Success = Shell_GetImageLists(&SystemLargeImageList, &SystemSmallImageList);
		ASSERT (Success);

		if (Success) {
			ListView_SetImageList(ListViewWindow, SystemSmallImageList, LVSIL_SMALL);
			ListView_SetImageList(ListViewWindow, SystemLargeImageList, LVSIL_NORMAL);
		}

		ListView_SetExtendedListViewStyle(
			ListViewWindow,
			LVS_EX_FULLROWSELECT |
			LVS_EX_LABELTIP |
			LVS_EX_DOUBLEBUFFER);

		RtlZeroMemory(&Column, sizeof(Column));
		Column.mask = LVCF_TEXT | LVCF_WIDTH;
		
		Column.pszText = (PWSTR) _(L"Application");
		Column.cx = DpiScaleX(100);
		ListView_InsertColumn(ListViewWindow, 0, &Column);

		Column.pszText = (PWSTR) _(L"Containing folder");
		ListView_InsertColumn(ListViewWindow, 1, &Column);
		ListView_SetColumnWidth(ListViewWindow, 1, LVSCW_AUTOSIZE_USEHEADER);

		//
		// Add tool tips.
		//

		ToolTip(Window, IDC_ENABLELOGGING, _(
			L"If you enable logging, VxKex will create log files in the specified "
			L"folder every time you run an application which has VxKex enabled."));
		ToolTip(Window, IDC_ENABLEFORMSI, _(
			L"This option allows VxKex to work with MSI installers.\r\n"
			L"If you encounter unexpected problems with MSI installers, try disabling this option."));
		ToolTip(Window, IDC_CPIWBYPA, _(
			L"This option causes a DLL to be loaded into Windows Explorer at startup in order "
			L"to remove the version check for certain programs.\r\n"
			L"If you encounter unexpected problems with Windows Explorer, try disabling this "
			L"option."));
		ToolTip(Window, IDC_ADDTOMENU, _(
			L"Add \"Run with VxKex\" options to the context menu for .exe and .msi files."));
		ToolTip(Window, IDC_WHICHCONTEXTMENU, _(
			L"Shift+Right Click on a .exe or .msi file opens the extended context menu.\r\n"
			L"Right clicking without holding the Shift key opens the normal context menu."));
		ToolTip(Window, IDC_CLEANAPPS, _(
			L"Applications which no longer exist may remain enabled in VxKex. Click this button "
			L"in order to remove deleted EXEs or MSIs from the list."));

		//
		// Populate the top section (global configuration) and the apps list.
		//

		KexCfgGuiPopulateGlobalConfiguration();
		KexCfgGuiPopulateApplicationList();

		//
		// Update the state of certain interdependent controls (i.e. controls
		// that are supposed to be enabled/disabled based on the state of a
		// checkbox)
		//

		DialogProc(Window, WM_COMMAND, IDC_ENABLELOGGING, 0);
		DialogProc(Window, WM_COMMAND, IDC_ADDTOMENU, 0);

		//
		// Restore window placement from the registry.
		//

		RestoreWindowPlacement(Window, WNDPOS_REG_KEY);

		UnsavedChanges = FALSE;
		EnableWindow(GetDlgItem(Window, IDC_APPLY), FALSE);
	} else if (Message == WM_COMMAND) {
		ULONG ControlId;
		ULONG NotificationCode;

		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDC_ENABLELOGGING ||
			ControlId == IDC_ENABLEFORMSI ||
			ControlId == IDC_CPIWBYPA ||
			ControlId == IDC_ADDTOMENU ||
			(ControlId == IDC_WHICHCONTEXTMENU && NotificationCode == CBN_SELCHANGE) ||
			(ControlId == IDC_LOGDIR && NotificationCode == EN_CHANGE)) {

			if (UnsavedChanges == FALSE) {
				UnsavedChanges = TRUE;
				EnableWindow(GetDlgItem(Window, IDC_APPLY), TRUE);
			}
		}

		if (ControlId == IDC_CANCEL || ControlId == M_EXIT) {
			if (UnsavedChanges) {
				INT UserResponse;

				UserResponse = MessageBoxF(
					TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
					0,
					_(FRIENDLYAPPNAME),
					_(L"Are you sure you want to exit?"),
					_(L"Changes that are not applied will be discarded."));

				if (UserResponse == IDYES) {
					PostQuitMessage(0);
				}
			} else {
				PostQuitMessage(0);
			}
		} else if (ControlId == IDC_OK) {
			if (UnsavedChanges) {
				KexCfgGuiApplyGlobalConfiguration();
			}

			unless (UnsavedChanges) {
				PostQuitMessage(0);
			}
		} else if (ControlId == IDC_APPLY) {
			if (UnsavedChanges) {
				KexCfgGuiApplyGlobalConfiguration();
			}

			unless (UnsavedChanges) {
				EnableWindow(GetDlgItem(Window, ControlId), FALSE);
			}
		} else if (ControlId == IDC_BROWSELOGDIR) {
			BOOLEAN Success;
			WCHAR NewLogDir[MAX_PATH];

			GetDlgItemText(Window, IDC_LOGDIR, NewLogDir, ARRAYSIZE(NewLogDir));
			Success = PickFolder(Window, NULL, 0, NewLogDir, ARRAYSIZE(NewLogDir));

			if (Success) {
				SetDlgItemText(Window, IDC_LOGDIR, NewLogDir);
			}
		} else if (ControlId == IDC_ENABLELOGGING) {
			BOOLEAN LoggingEnabled;

			LoggingEnabled = IsDlgButtonChecked(Window, ControlId);

			EnableWindow(GetDlgItem(Window, IDC_LOGDIR), LoggingEnabled);
			EnableWindow(GetDlgItem(Window, IDC_BROWSELOGDIR), LoggingEnabled);
		} else if (ControlId == IDC_ADDTOMENU) {
			EnableWindow(
				GetDlgItem(Window, IDC_WHICHCONTEXTMENU),
				IsDlgButtonChecked(Window, ControlId));
		} else if (ControlId == IDC_NEWAPP) {
			AddProgram();
		} else if (ControlId == IDC_REMOVEAPPS) {
			RemoveSelectedPrograms();
		} else if (ControlId == IDC_PROPERTIES) {
			ULONG ItemIndex;

			ItemIndex = ListView_GetNextItem(ListViewWindow, -1, LVIS_SELECTED);

			ShowPropertiesDialog(
				GetProgramFullPathFromListViewIndex(ItemIndex),
				SW_SHOWDEFAULT);
		} else if (ControlId == IDC_CLEANAPPS) {
			CleanPrograms();
		} else if (ControlId == M_SELECTALL) {
			HWND FocusedWindow;
			WCHAR ClassName[32];

			FocusedWindow = GetFocus();
			GetClassName(FocusedWindow, ClassName, ARRAYSIZE(ClassName));

			if (StringEqual(ClassName, L"SysListView32")) {
				ListView_SetItemState(FocusedWindow, -1, LVIS_SELECTED, LVIS_SELECTED);
			}
		} else if (ControlId == M_DELETE) {
			if (GetFocus() == ListViewWindow) {
				RemoveSelectedPrograms();
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_NOTIFY) {
		LPNMHDR Notification;

		Notification = (LPNMHDR) LParam;

		if (Notification->idFrom == IDC_LISTVIEW) {
			if (Notification->code == LVN_ITEMCHANGED) {
				ULONG ListViewSelectedCount;

				ListViewSelectedCount = ListView_GetSelectedCount(ListViewWindow);

				if (ListViewSelectedCount == 0) {
					// no items selected
					EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), FALSE);
					EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE);
				} else {
					// one or more items selected
					EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), TRUE);

					if (ListViewSelectedCount == 1) {
						EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), TRUE);
					} else {
						// no properties button for >1 item selected
						EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE);
					}
				}
			} else if (Notification->code == LVN_DELETEITEM) {
				// after items are deleted, no items will be selected anymore
				EnableWindow(GetDlgItem(Window, IDC_REMOVEAPPS), FALSE);
				EnableWindow(GetDlgItem(Window, IDC_PROPERTIES), FALSE); 
			} else if (Notification->code == NM_DBLCLK &&
					   ListView_GetSelectedCount(ListViewWindow) == 1) {
				//
				// User double clicked on a list-view item.
				// If Alt is held down, we will open the properties of the file,
				// just like in Windows Explorer.
				// Otherwise, we will open the location of that file.
				//

				if (GetKeyState(VK_MENU) & 0x8000) {
					OpenSelectedItemProperties();
				} else {
					OpenSelectedItemLocation();
				}
			} else if (Notification->code == LVN_COLUMNCLICK) {
				ULONG ColumnIndex;

				ColumnIndex = ((LPNMLISTVIEW) LParam)->iSubItem;

				// toggle sort order
				ASSERT (ColumnIndex < ARRAYSIZE(SortOrderIsDescendingForColumn));
				SortOrderIsDescendingForColumn[ColumnIndex] ^= 1;

				// User has clicked on a column header.
				// We are now going to sort that column.
				ListView_SortItemsEx(
					ListViewWindow,
					KexCfgGuiSortListViewItems,
					ColumnIndex);
			}
		} else {
			return FALSE;
		}
	} else if (Message == WM_CONTEXTMENU) {
		HWND ContextMenuWindow;

		ContextMenuWindow = (HWND) WParam;

		if (ContextMenuWindow == ListViewWindow) {
			POINT ClickPoint;
			ClickPoint.x = GET_X_LPARAM(LParam);
			ClickPoint.y = GET_Y_LPARAM(LParam);
			HandleListViewContextMenu(&ClickPoint);
		}
	} else if (Message == WM_GETMINMAXINFO) {
		PMINMAXINFO MinMaxInfo;

		// Set a minimum size limit for the window
		MinMaxInfo = (PMINMAXINFO) LParam;
		MinMaxInfo->ptMinTrackSize.x = DpiScaleX(496);
		MinMaxInfo->ptMinTrackSize.y = DpiScaleY(483);
	} else if (Message == WM_SIZE && (WParam == SIZENORMAL || WParam == SIZEFULLSCREEN)) {
		HandleWindowResize(
			Window,
			GET_X_LPARAM(LParam),
			GET_Y_LPARAM(LParam));
	} else if (Message == WM_CLOSE) {
		DialogProc(Window, WM_COMMAND, IDC_CANCEL, 0);
	} else {
		return FALSE;
	}

	return TRUE;
}

VOID KexCfgOpenGUI(
	VOID)
{
	HACCEL Accelerators;
	MSG Message;
	INITCOMMONCONTROLSEX InitComctl;

	KexgApplicationFriendlyName = _(FRIENDLYAPPNAME);

	if (!IsUserAnAdmin()) {
		CriticalErrorBoxF(
			_(L"This program needs to be run as administrator. Log in as an "
			  L"administrator and try again."));

		NOT_REACHED;
	}

	InitComctl.dwSize = sizeof(InitComctl);
	InitComctl.dwICC = ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitComctl);

	Accelerators = LoadAccelerators(NULL, MAKEINTRESOURCE(IDA_ACCELERATORS));
	CreateDialog(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DialogProc);

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (TranslateAccelerator(MainWindow, Accelerators, &Message)) {
			continue;
		}

		if (!IsDialogMessage(MainWindow, &Message)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	ASSERT (IsWindow(MainWindow));
	SaveWindowPlacement(MainWindow, WNDPOS_REG_KEY);
}