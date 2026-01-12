#include "vxlview.h"

//
// Functions to load and save configuration information from the registry,
// such as the size and position of the main window.
//

STATIC PCWSTR APP_REG_KEY = L"SOFTWARE\\VXsoft\\VxlView";

VOID SaveWindowPlacement(
	VOID)
{
	WINDOWPLACEMENT WindowPlacement;

	GetWindowPlacement(MainWindow, &WindowPlacement);

	RegWriteI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndLeft", WindowPlacement.rcNormalPosition.left);
	RegWriteI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndTop", WindowPlacement.rcNormalPosition.top);
	RegWriteI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndRight", WindowPlacement.rcNormalPosition.right);
	RegWriteI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndBottom", WindowPlacement.rcNormalPosition.bottom);
}

VOID RestoreWindowPlacement(
	VOID)
{
	ULONG Error;
	WINDOWPLACEMENT WindowPlacement;

	// required for first startup
	SetWindowPos(MainWindow, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	GetWindowPlacement(MainWindow, &WindowPlacement);

	Error = 0;
	Error += RegReadI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndLeft", (PULONG) &WindowPlacement.rcNormalPosition.left);
	Error += RegReadI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndTop", (PULONG) &WindowPlacement.rcNormalPosition.top);
	Error += RegReadI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndRight", (PULONG) &WindowPlacement.rcNormalPosition.right);
	Error += RegReadI32(HKEY_CURRENT_USER, APP_REG_KEY, L"WndBottom", (PULONG) &WindowPlacement.rcNormalPosition.bottom);

	SetWindowPlacement(MainWindow, &WindowPlacement);

	if (Error) {
		// typically occurs on first startup
		CenterWindow(MainWindow, HWND_DESKTOP);
	}
}

VOID SaveListViewColumns(
	VOID)
{
	USHORT ColumnWidths[ColumnMaxValue];
	INT ColumnOrder[ColumnMaxValue];
	ULONG Index;

	ListView_GetColumnOrderArray(ListViewWindow, ARRAYSIZE(ColumnOrder), ColumnOrder);

	for (Index = 0; Index < ColumnMaxValue; Index++) {
		ColumnWidths[Index] = ListView_GetColumnWidth(ListViewWindow, Index);
	}

	RegSetKeyValue(
		HKEY_CURRENT_USER,
		APP_REG_KEY,
		L"ColumnWidths",
		REG_BINARY,
		ColumnWidths,
		sizeof(ColumnWidths));

	RegSetKeyValue(
		HKEY_CURRENT_USER,
		APP_REG_KEY,
		L"ColumnOrder",
		REG_BINARY,
		ColumnOrder,
		sizeof(ColumnOrder));
}

VOID RestoreListViewColumns(
	VOID)
{
	USHORT ColumnWidths[ColumnMaxValue];
	INT ColumnOrder[ColumnMaxValue];
	ULONG DataSize;
	ULONG Index;
	LSTATUS LStatus;

	DataSize = sizeof(ColumnOrder);
	LStatus = RegGetValue(
		HKEY_CURRENT_USER,
		APP_REG_KEY,
		L"ColumnOrder",
		RRF_RT_REG_BINARY,
		NULL,
		ColumnOrder,
		&DataSize);

	if (LStatus == ERROR_SUCCESS && DataSize == sizeof(ColumnOrder)) {
		ListView_SetColumnOrderArray(ListViewWindow, ARRAYSIZE(ColumnOrder), ColumnOrder);
	}

	DataSize = sizeof(ColumnWidths);
	LStatus = RegGetValue(
		HKEY_CURRENT_USER,
		APP_REG_KEY,
		L"ColumnWidths",
		RRF_RT_REG_BINARY,
		NULL,
		ColumnWidths,
		&DataSize);

	if (LStatus == ERROR_SUCCESS && DataSize == sizeof(ColumnWidths)) {
		for (Index = 0; Index < ARRAYSIZE(ColumnWidths); Index++) {
			ListView_SetColumnWidth(ListViewWindow, Index, ColumnWidths[Index]);
		}
	}
}