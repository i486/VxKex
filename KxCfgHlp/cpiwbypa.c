#include "buildcfg.h"
#include <KxCfgHlp.h>
#include <KexW32ML.h>

// returns TRUE if cpiwbypa.dll BHO is registered
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryExplorerCpiwBypass(
	VOID)
{
	HKEY KeyHandle;
	ULONG ErrorCode;

	ErrorCode = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
		L"Browser Helper Objects\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
		0,
		STANDARD_RIGHTS_READ,
		&KeyHandle);

	ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

	if (ErrorCode == ERROR_SUCCESS) {
		SafeClose(KeyHandle);
		return TRUE;
	} else {
		return FALSE;
	}
}

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnableExplorerCpiwBypass(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	ULONG ErrorCode;

	if (Enable) {
		HKEY KeyHandle;
		
		KeyHandle = KxCfgpCreateKey(
			HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
			L"Browser Helper Objects\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
			KEY_READ | KEY_WRITE,
			TransactionHandle);

		if (!KeyHandle) {
			return FALSE;
		}

		RegWriteString(KeyHandle, NULL, NULL, L"VxKex CPIW Version Check Bypass");
		SafeClose(KeyHandle);

		if (KexRtlOperatingSystemBitness() == 64) {
			// Also create Wow64 registry keys/values

			KeyHandle = KxCfgpCreateKey(
				HKEY_LOCAL_MACHINE,
				L"Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
				L"Browser Helper Objects\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
				KEY_READ | KEY_WRITE,
				TransactionHandle);

			if (!KeyHandle) {
				return FALSE;
			}

			ErrorCode = RegWriteString(KeyHandle, NULL, NULL, L"VxKex CPIW Version Check Bypass");
			if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}

			SafeClose(KeyHandle);
		}

		//
		// Pre-disable the BHO in Internet Explorer to prevent annoying pop up.
		// "The add-on blah from an unknown publisher is ready to use."
		//

		KeyHandle = KxCfgpCreateKey(
			HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Ext\\"
			L"Settings\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
			KEY_READ | KEY_WRITE,
			TransactionHandle);

		ASSERT (KeyHandle != NULL);

		RegWriteI32(KeyHandle, NULL, L"Flags", 0x40);
		SafeClose(KeyHandle);
	} else {
		//
		// We are disabling the CpiwBypa BHO.
		//

		ErrorCode = KxCfgpDeleteKey(
			NULL,
			L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\"
			L"Explorer\\Browser Helper Objects\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
			TransactionHandle);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		if (KexRtlOperatingSystemBitness() == 64) {
			// delete the wow64 key as well

			ErrorCode = KxCfgpDeleteKey(
				NULL,
				L"\\Registry\\Machine\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\"
				L"Explorer\\Browser Helper Objects\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
				TransactionHandle);

			if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
				SetLastError(ErrorCode);
				return FALSE;
			}
		}

		//
		// Get rid of cached Internet Explorer BHO settings as well.
		// HKCU\Software is not redirected on WOW64, so no need to do all the
		// dancing around with Wow6432Node and whatever.
		//

		{
			HKEY CurrentUserKeyHandle;

			RtlOpenCurrentUser(KEY_READ, (PHANDLE) &CurrentUserKeyHandle);

			ErrorCode = KxCfgpDeleteKey(
				CurrentUserKeyHandle,
				L"Software\\Microsoft\\Windows\\CurrentVersion\\Ext\\"
				L"Settings\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}",
				TransactionHandle);

			SafeClose(CurrentUserKeyHandle);
		}

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);
	}

	return TRUE;
}