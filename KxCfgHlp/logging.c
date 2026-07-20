#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryLoggingSettings(
	IN	BOOLEAN		PerUserSettings,
	OUT	PBOOLEAN	IsEnabled OPTIONAL,
	OUT	PWSTR		LogDir OPTIONAL,
	IN	ULONG		LogDirCch,
	IN	HANDLE		TransactionHandle OPTIONAL)
{
	HKEY VxKexKeyHandle;
	ULONG ErrorCode;

	ASSERT (!!LogDir == !!LogDirCch);

	if (IsEnabled != NULL) {
		*IsEnabled = FALSE;
	}

	if (LogDir != NULL) {
		LogDir[0] = '\0';
	}

	VxKexKeyHandle = KxCfgOpenVxKexRegistryKey(
		PerUserSettings,
		KEY_READ,
		TransactionHandle);

	ASSERT (VxKexKeyHandle != NULL);

	if (!VxKexKeyHandle) {
		return FALSE;
	}

	if (IsEnabled != NULL) {
		ULONG EnableLogging;

		ErrorCode = RegReadI32(VxKexKeyHandle, NULL, L"EnableLogging", &EnableLogging);
		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		*IsEnabled = !!EnableLogging;
	}

	if (LogDir != NULL) {
		ErrorCode = RegReadString(VxKexKeyHandle, NULL, L"LogDir", LogDir, LogDirCch);
		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);
	}

	SafeClose(VxKexKeyHandle);
	return TRUE;
}

//
// If LogDir is NULL, it will be set to "%localappdata%\vxkex\logs" for user
// settings, or "%programdata%\vxkex\logs" for system settings.
// Environment variables are expanded in LogDir.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgConfigureLoggingSettings(
	IN	BOOLEAN		PerUserSettings,
	IN	BOOLEAN		Enabled,
	IN	PCWSTR		LogDir OPTIONAL,
	IN	HANDLE		TransactionHandle OPTIONAL)
{
	HKEY VxKexUserKeyHandle;
	ULONG LogDirExpandedCch;
	WCHAR LogDirExpanded[MAX_PATH];
	ULONG ErrorCode;

	if (!LogDir || LogDir[0] == '\0') {
		if (PerUserSettings) {
			LogDir = L"%LOCALAPPDATA%\\VxKex\\Logs";
		} else {
			LogDir = L"%PROGRAMDATA%\\VxKex\\Logs";
		}
	}

	LogDirExpandedCch = ExpandEnvironmentStrings(
		LogDir,
		LogDirExpanded,
		ARRAYSIZE(LogDirExpanded));

	ASSERT (LogDirExpandedCch != 0);

	if (LogDirExpandedCch == 0) {
		return FALSE;
	}

	VxKexUserKeyHandle = KxCfgOpenVxKexRegistryKey(
		TRUE,
		KEY_READ | KEY_WRITE,
		TransactionHandle);

	ASSERT (VxKexUserKeyHandle != NULL);

	if (!VxKexUserKeyHandle) {
		return FALSE;
	}

	try {
		ErrorCode = RegWriteI32(VxKexUserKeyHandle, NULL, L"EnableLogging", Enabled);
		ASSERT (ErrorCode == ERROR_SUCCESS);

		if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		ErrorCode = RegWriteString(VxKexUserKeyHandle, NULL, L"LogDir", LogDirExpanded);
		ASSERT (ErrorCode == ERROR_SUCCESS);

		if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}
	} finally {
		SafeClose(VxKexUserKeyHandle);
	}

	//
	// Refresh the disk cleanup handler, since it depends on the location of
	// the log directory.
	//

	KxCfgInstallDiskCleanupHandler(TransactionHandle);

	return TRUE;
}