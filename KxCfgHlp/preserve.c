///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     preserve.c
//
// Abstract:
//
//     Contains functions for preserving VxKex program configuration to a
//     registry key other than the IFEO key, and restoring that configuration
//     back to IFEO.
//
//     This is used to implement enhanced support for the "Keep my compatibility
//     settings" checkbox in KexSetup.
//
// Author:
//
//     vxiiduu (25-Jun-2026)
//
// Environment:
//
//     Win32 mode. This code must be able to run without KexDll, as it is used
//     in KexSetup. This code must function properly when run under WOW64.
//
// Revision History:
//
//     vxiiduu              25-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KxCfgHlp.h>

#define KXCFG_PRESERVED_CONFIGURATION_VERSION 2

// Written directly to the registry as REG_BINARY.
// Should not contain any members which change size with bitness.
//
// When this structure is read from the registry, applications must check the
// Version member. If it does not match KXCFG_PRESERVED_CONFIGURATION_VERSION,
// the preserved configuration must be ignored.
typedef struct {
	// Set to KXCFG_PRESERVED_CONFIGURATION_VERSION.
	ULONG						Version;

	// Must be the last member in the structure. This is to account for future
	// size changes to KXCFG_PROGRAM_CONFIGURATION or KEX_IFEO_PARAMETERS.
	KXCFG_PROGRAM_CONFIGURATION	Configuration;
} TYPEDEF_TYPE_NAME(KXCFG_PRESERVED_CONFIGURATION);

#define PRESERVED_CONFIG_KEY_PATH L"Software\\VXsoft\\VxKex\\PreservedConfig"

//
// Save a single program's configuration from IFEO to VxKex HKLM key.
// The configuration is saved to HKLM\Software\VXsoft\VxKex\PreservedConfig.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgPreserveConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	BOOLEAN Success;
	KXCFG_PRESERVED_CONFIGURATION PreservedConfiguration;
	HKEY PreservedConfigKeyHandle;
	LSTATUS ErrorCode;

	//
	// Form a KXCFG_PRESERVED_CONFIGURATION structure for this program.
	//

	KexRtlZeroMemory(&PreservedConfiguration, sizeof(PreservedConfiguration));
	PreservedConfiguration.Version = KXCFG_PRESERVED_CONFIGURATION_VERSION;

	Success = KxCfgGetConfiguration(
		ExeFullPath,
		&PreservedConfiguration.Configuration);

	ASSERT (Success);

	if (!Success) {
		return FALSE;
	}

	//
	// Create or open the preserved config key and save the PreservedConfiguration
	// structure into a value with name equivalent to ExeFullPath.
	//

	PreservedConfigKeyHandle = KxCfgpCreateKey(
		HKEY_LOCAL_MACHINE,
		PRESERVED_CONFIG_KEY_PATH,
		KEY_SET_VALUE,
		TransactionHandle);

	ASSERT (PreservedConfigKeyHandle != NULL);

	if (PreservedConfigKeyHandle == NULL) {
		return FALSE;
	}

	// Can't use RegSetKeyValue because it's not transacted.
	ErrorCode = RegSetValueEx(
		PreservedConfigKeyHandle,
		ExeFullPath,
		0,
		REG_BINARY,
		(PCBYTE) &PreservedConfiguration,
		sizeof(PreservedConfiguration));

	ASSERT (ErrorCode == ERROR_SUCCESS);
	SafeClose(PreservedConfigKeyHandle);

	if (ErrorCode != ERROR_SUCCESS) {
		return FALSE;
	}

	return TRUE;
}

//
// Restore all preserved configuration, if any, from VxKex HKLM key to IFEO.
// Deletes the HKLM\Software\VXsoft\VxKex\PreservedConfig key afterwards.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgRestorePreservedConfiguration(
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	BOOLEAN Success;
	NTSTATUS Status;
	HKEY PreservedConfigKeyHandle;
	ULONG Index;

	//
	// Open the preserved config key.
	//

	PreservedConfigKeyHandle = KxCfgpOpenKey(
		HKEY_LOCAL_MACHINE,
		PRESERVED_CONFIG_KEY_PATH,
		KEY_QUERY_VALUE | DELETE,
		TransactionHandle);

	if (PreservedConfigKeyHandle == NULL) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			// Key not found is OK, means we have nothing to do.
			return TRUE;
		}

		return FALSE;
	}

	//
	// Enumerate contents and parse the KXCFG_PRESERVED_CONFIGURATION
	// structures.
	//

	Success = TRUE;
	Index = 0;

	while (TRUE) {
		BOOLEAN Success2;
		LSTATUS ErrorCode;
		WCHAR ExeFullPath[MAX_PATH];
		ULONG ExeFullPathCch;
		ULONG DataType;
		KXCFG_PRESERVED_CONFIGURATION PreservedConfiguration;
		ULONG PreservedConfigurationCb;

		ExeFullPathCch = ARRAYSIZE(ExeFullPath);
		PreservedConfigurationCb = sizeof(PreservedConfiguration);

		KexRtlZeroMemory(&PreservedConfiguration, sizeof(PreservedConfiguration));

		ErrorCode = RegEnumValue(
			PreservedConfigKeyHandle,
			Index,
			ExeFullPath,
			&ExeFullPathCch,
			NULL,
			&DataType,
			(PBYTE) &PreservedConfiguration,
			&PreservedConfigurationCb);

		++Index;

		if (ErrorCode == ERROR_NO_MORE_ITEMS) {
			break;
		} else if (ErrorCode == ERROR_MORE_DATA ||
				   ErrorCode == ERROR_INSUFFICIENT_BUFFER ||
				   ErrorCode == ERROR_BUFFER_OVERFLOW) {

			ASSERT (("Unexpected data size from RegEnumValue", FALSE));
			continue;
		} else if (ErrorCode != ERROR_SUCCESS) {
			ASSERT (("Unexpected error from RegEnumValue", FALSE));
			Success = FALSE;
			break;
		}

		if (DataType != REG_BINARY) {
			ASSERT (("Unexpected data type from RegEnumValue", FALSE));
			continue;
		}

		if (PreservedConfigurationCb != sizeof(PreservedConfiguration)) {
			ASSERT (("Unexpected data size from RegEnumValue", FALSE));
			continue;
		}

		if (PreservedConfiguration.Version == 0 ||
			PreservedConfiguration.Version != KXCFG_PRESERVED_CONFIGURATION_VERSION) {

			// Unsupported version. We can't handle this.
			ASSERT (("Unexpected PreservedConfiguration.Version", FALSE));
			continue;
		}

		//
		// We can now put this configuration into IFEO.
		//

		Success2 = KxCfgSetConfiguration(
			ExeFullPath,
			&PreservedConfiguration.Configuration,
			TransactionHandle);

		ASSERT (Success2);
	}

	//
	// Delete the preserved configuration registry key.
	//

	Status = NtDeleteKey(PreservedConfigKeyHandle);
	ASSERT (NT_SUCCESS(Status));

	SafeClose(PreservedConfigKeyHandle);

	return Success;
}