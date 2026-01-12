#include "buildcfg.h"
#include <KxCfgHlp.h>

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryShellContextMenuEntries(
	OUT	PBOOLEAN	ExtendedMenu OPTIONAL)
{
	HKEY KeyHandle;
	ULONG ErrorCode;

	ErrorCode = RegOpenKeyEx(
		HKEY_CLASSES_ROOT,
		L"exefile\\shell\\open_vxkex",
		0,
		KEY_READ,
		&KeyHandle);

	ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

	if (ErrorCode == ERROR_SUCCESS) {
		if (ExtendedMenu) {
			WCHAR DummyBuffer[4];

			ErrorCode = RegReadString(KeyHandle, NULL, L"Extended", DummyBuffer, ARRAYSIZE(DummyBuffer));
			ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

			if (ErrorCode == ERROR_SUCCESS) {
				*ExtendedMenu = TRUE;
			} else {
				*ExtendedMenu = FALSE;
			}
		}

		SafeClose(KeyHandle);
		return TRUE;
	} else {
		return FALSE;
	}
}

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgConfigureShellContextMenuEntries(
	IN	BOOLEAN	Enable,
	IN	BOOLEAN	ExtendedMenu,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	ULONG ErrorCode;

	if (Enable) {
		BOOLEAN Success;
		HKEY KeyHandle;
		HKEY CommandKeyHandle;
		WCHAR KexDir[MAX_PATH];
		WCHAR Command[MAX_PATH + 16];
		PCWSTR KeyName;
		BOOLEAN AlreadyEnabledForMsi;
		BOOLEAN AlreadyEnabled;
		BOOLEAN AlreadyExtended;

		//
		// See if already enabled
		//

		AlreadyEnabled = KxCfgQueryShellContextMenuEntries(&AlreadyExtended);

		if (AlreadyEnabled) {
			if (AlreadyExtended != ExtendedMenu) {
				// we're changing which menu, so disable and re-enable
				KxCfgConfigureShellContextMenuEntries(FALSE, FALSE, TransactionHandle);
			} else {
				// configuration is already as requested
				return TRUE;
			}
		}

		//
		// We are enabling the VxKex context menu entries.
		// Assemble the command line for direct invocation of a VxKex-enabled program.
		//

		Success = KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));
		ASSERT (Success);
		if (!Success) {
			return FALSE;
		}

		StringCchPrintf(
			Command,
			ARRAYSIZE(Command),
			L"\"%s\\VxKexLdr.exe\" \"%%1\"",
			KexDir);

		KeyName = L"exefile\\shell\\open_vxkex";
		AlreadyEnabledForMsi = FALSE;
		KeyHandle = NULL;
		CommandKeyHandle = NULL;

		//
		// Write settings to registry.
		//
		
DoAgain:
		try {
			KeyHandle = KxCfgpCreateKey(
				HKEY_CLASSES_ROOT,
				KeyName,
				KEY_READ | KEY_WRITE,
				TransactionHandle);

			ASSERT (KeyHandle != NULL);

			if (!KeyHandle) {
				return FALSE;
			}

			CommandKeyHandle = KxCfgpCreateKey(
				KeyHandle,
				L"command",
				KEY_READ | KEY_WRITE,
				TransactionHandle);

			ASSERT (CommandKeyHandle != NULL);

			if (!CommandKeyHandle) {
				return FALSE;
			}

			ErrorCode = RegWriteString(KeyHandle, NULL, NULL, L"Run with VxKex enabled");
			ASSERT (ErrorCode == ERROR_SUCCESS);

			if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}

			ErrorCode = RegWriteString(CommandKeyHandle, NULL, NULL, Command);
			ASSERT (ErrorCode == ERROR_SUCCESS);

			if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}

			if (ExtendedMenu) {
				ErrorCode = RegWriteString(KeyHandle, NULL, L"Extended", L"");
				ASSERT (ErrorCode == ERROR_SUCCESS);

				if (ErrorCode != ERROR_SUCCESS) {
					SetLastError(ErrorCode);
					return FALSE;
				}
			}
		} finally {
			SafeClose(KeyHandle);
			SafeClose(CommandKeyHandle);
		}

		if (!AlreadyEnabledForMsi) {
			KeyName = L"Msi.Package\\shell\\open_vxkex";
			AlreadyEnabledForMsi = TRUE;
			goto DoAgain;
		}
	} else {
		//
		// We are disabling the extended context menu.
		//

		ErrorCode = KxCfgpDeleteKey(
			NULL,
			L"\\Registry\\Machine\\Software\\Classes\\exefile\\shell\\open_vxkex\\command",
			TransactionHandle);

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		ErrorCode = KxCfgpDeleteKey(
			NULL,
			L"\\Registry\\Machine\\Software\\Classes\\exefile\\shell\\open_vxkex",
			TransactionHandle);

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		ErrorCode = KxCfgpDeleteKey(
			NULL,
			L"\\Registry\\Machine\\Software\\Classes\\Msi.Package\\shell\\open_vxkex\\command",
			TransactionHandle);

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		ErrorCode = KxCfgpDeleteKey(
			NULL,
			L"\\Registry\\Machine\\Software\\Classes\\Msi.Package\\shell\\open_vxkex",
			TransactionHandle);

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			SetLastError(ErrorCode);
			return FALSE;
		}
	}

	return TRUE;
}