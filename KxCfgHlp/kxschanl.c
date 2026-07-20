///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxschanl.c
//
// Abstract:
//
//     Contains functions for registering and unregistering the KxSChanl SSP
//     in the legacy way which globally registered it.
//
//     This registration method is no longer used by VxKex. This code only exists
//     in order to allow for unregistration of KxSChanl when upgrading from
//     beta/pre-release versions.
//
//     HKLM\SYSTEM\CurrentControlSet\Control\SecurityProviders\SecurityProviders
//     is a REG_SZ which contains a COMMA-separated list of DLL base names.
//
//     These DLLs are SSPs (Security support providers) which just get loaded
//     from the default DLL search path (Kex3264Dir, system32, %PATH%...)
//
// Author:
//
//     vxiiduu (17-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               17-May-2026  Initial creation.
//     vxiiduu               23-Jun-2026  Update function names to indicate that
//                                        this is the legacy registration method.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KxCfgHlp.h>

#define KXSCHANL_DLL_NAME L"KxSChanl.dll"
#define KXSCHANL_DLL_CCH StringLiteralLength(KXSCHANL_DLL_NAME)

// returns TRUE if KxSChanl SSP is registered
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryLegacyKxSChanlSsp(
	VOID)
{
	ULONG ErrorCode;
	WCHAR SecurityProviders[MAX_PATH];

	ErrorCode = RegReadString(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\SecurityProviders",
		L"SecurityProviders",
		SecurityProviders,
		ARRAYSIZE(SecurityProviders));

	if (ErrorCode != ERROR_SUCCESS) {
		return FALSE;
	}

	return StringSearchI(SecurityProviders, KXSCHANL_DLL_NAME);
}

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnableLegacyKxSChanlSsp(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	ULONG ErrorCode;
	HKEY KeyHandle;

	KeyHandle = KxCfgpCreateKey(
		HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Control\\SecurityProviders",
		KEY_READ | KEY_WRITE,
		TransactionHandle);

	ASSERT (VALID_HANDLE(KeyHandle));

	if (!KeyHandle) {
		return FALSE;
	}

	try {
		HRESULT Result;
		PWCHAR KxSChanlStringLocation;
		WCHAR SecurityProviders[MAX_PATH];

		ErrorCode = RegReadString(
			KeyHandle,
			NULL,
			L"SecurityProviders",
			SecurityProviders,
			ARRAYSIZE(SecurityProviders));

		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			// That's OK, we'll just go with a blank string.
			SecurityProviders[0] = '\0';
		} else if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		//
		// Find the location of any existing KxSChanl.dll entry.
		//

		KxSChanlStringLocation = (PWCHAR) StringFindI(SecurityProviders, KXSCHANL_DLL_NAME);
		
		//
		// Enable or disable by adding or removing KxSChanl.dll.
		//

		if (Enable) {
			if (KxSChanlStringLocation) {
				// Already enabled.
				return TRUE;
			}

			if (SecurityProviders[0] != '\0') {
				// string not empty, we need to add a comma
				Result = StringCchCat(
					SecurityProviders,
					ARRAYSIZE(SecurityProviders),
					L",");

				ASSERT (SUCCEEDED(Result));

				if (FAILED(Result)) {
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return FALSE;
				}
			}

			Result = StringCchCat(
				SecurityProviders,
				ARRAYSIZE(SecurityProviders),
				KXSCHANL_DLL_NAME);

			ASSERT (SUCCEEDED(Result));

			if (FAILED(Result)) {
				// Most probable failure cause is a lack of buffer space
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return FALSE;
			}
		} else {
			ULONG SecurityProvidersCch;
			ULONG KxSChanlStringCch;
			ULONG CchPrecedingKxSChanlString;

			if (!KxSChanlStringLocation) {
				// Already disabled.
				return TRUE;
			}
			
			KxSChanlStringCch = KXSCHANL_DLL_CCH;

			if (KxSChanlStringLocation[KxSChanlStringCch] == ',') {
				// we want to remove that trailing comma as well
				++KxSChanlStringCch;
			} else if (KxSChanlStringLocation != &SecurityProviders[0] &&
					   KxSChanlStringLocation[-1] == ',') {

				// remove preceding comma as well
				--KxSChanlStringLocation;
				++KxSChanlStringCch;
			}

			SecurityProvidersCch = (ULONG) wcslen(SecurityProviders);
			CchPrecedingKxSChanlString = (ULONG) (KxSChanlStringLocation - &SecurityProviders[0]);

			// shift the contents of the string backwards
			RtlMoveMemory(
				KxSChanlStringLocation,
				KxSChanlStringLocation + KxSChanlStringCch,
				(SecurityProvidersCch -
				 CchPrecedingKxSChanlString -
				 KxSChanlStringCch) * sizeof(WCHAR));

			SecurityProvidersCch -= KxSChanlStringCch;

			// null terminate the resulting string
			SecurityProviders[SecurityProvidersCch] = '\0';

			// we don't want double commas - alert about this in debug builds
			ASSERT (!StringSearch(SecurityProviders, L",,"));
		}

		ErrorCode = RegWriteString(
			KeyHandle,
			NULL,
			L"SecurityProviders",
			SecurityProviders);

		ASSERT (ErrorCode == ERROR_SUCCESS);

		if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}
	} finally {
		SafeClose(KeyHandle);
	}

	return TRUE;
}