#include "vxlview.h"

//
// Functions to load and save configuration information from the registry,
// such as the size and position of the list-view columns.
//

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