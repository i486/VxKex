///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     getcfg.c
//
// Abstract:
//
//     Contains functions for querying and enumerating VxKex configuration.
//
// Author:
//
//     vxiiduu (02-Feb-2024)
//
// Environment:
//
//     Win32 mode. This code must be able to run without KexDll, as it is used
//     in KexSetup. This code must function properly when run under WOW64.
//
// Revision History:
//
//     vxiiduu              02-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>
#include <KexW32ML.h>

//
// Retrieve VxKex configuration for a particular program.
// Returns TRUE on success and FALSE on failure. Call GetLastError() to obtain
// more information.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgGetConfiguration(
	IN	PCWSTR							ExeFullPath,
	OUT	PKXCFG_PROGRAM_CONFIGURATION	Configuration)
{
	NTSTATUS Status;
	ULONG ErrorCode;
	HKEY KeyHandle;
	UNICODE_STRING ExeFullPathUS;

	WCHAR VerifierDlls[256];
	ULONG GlobalFlag;
	ULONG KEX_DisableForChild;
	ULONG KEX_DisableAppSpecific;
	ULONG KEX_WinVerSpoof;
	ULONG KEX_StrongVersionSpoof;

	ASSERT (ExeFullPath != NULL);
	ASSERT (ExeFullPath[0] != '\0');
	ASSERT (Configuration != NULL);

	RtlZeroMemory(Configuration, sizeof(*Configuration));

	//
	// Open the IFEO key for this program.
	//

	RtlInitUnicodeString(&ExeFullPathUS, ExeFullPath);

	Status = LdrOpenImageFileOptionsKey(
		&ExeFullPathUS,
		FALSE,
		(PHANDLE) &KeyHandle);

	if (!NT_SUCCESS(Status)) {
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
	}

	//
	// Read the values of IFEO registry keys relevant to configuration.
	//

	ErrorCode = RegReadString(
		KeyHandle,
		NULL,
		L"VerifierDlls",
		VerifierDlls,
		ARRAYSIZE(VerifierDlls));

	if (ErrorCode != ERROR_SUCCESS) {
		VerifierDlls[0] = '\0';
	}

	RegReadI32(KeyHandle, NULL, L"GlobalFlag", &GlobalFlag);
	RegReadI32(KeyHandle, NULL, L"KEX_DisableForChild", &KEX_DisableForChild);
	RegReadI32(KeyHandle, NULL, L"KEX_DisableAppSpecific", &KEX_DisableAppSpecific);
	RegReadI32(KeyHandle, NULL, L"KEX_WinVerSpoof", &KEX_WinVerSpoof);
	RegReadI32(KeyHandle, NULL, L"KEX_StrongVersionSpoof", &KEX_StrongVersionSpoof);

	RegCloseKey(KeyHandle);

	//
	// Parse GlobalFlag and VerifierDlls to determine whether VxKex is enabled.
	//

	if (GlobalFlag & FLG_APPLICATION_VERIFIER) {
		if (StringSearchI(VerifierDlls, L"KexDll.dll")) {
			Configuration->Enabled = TRUE;
		}
	}

	//
	// Fill out the remaining members of the structure.
	//

	Configuration->DisableForChild = !!KEX_DisableForChild;
	Configuration->DisableAppSpecificHacks = !!KEX_DisableAppSpecific;
	Configuration->WinVerSpoof = (KEX_WIN_VER_SPOOF) KEX_WinVerSpoof;
	Configuration->StrongSpoofOptions = KEX_StrongVersionSpoof;

	return TRUE;
}

//
// Enumerate all programs with VxKex enabled and call a function for each one.
// This function works for legacy configuration as well.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnumerateConfiguration(
	IN	PKXCFG_ENUMERATE_CONFIGURATION_CALLBACK	ConfigurationCallback,
	IN	PVOID									CallbackExtraParameter)
{
	NTSTATUS Status;
	HKEY IfeoBaseKey;
	ULONG Index;
	ULONG ErrorCode;

	ASSERT (ConfigurationCallback != NULL);

	//
	// Open the IFEO base key.
	// Note that we should not close this key.
	//

	Status = LdrOpenImageFileOptionsKey(
		NULL,
		FALSE,
		(PHANDLE) &IfeoBaseKey);

	if (!NT_SUCCESS(Status)) {
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
	}

	//
	// Enumerate all the EXE keys.
	//

	Index = 0;

	while (TRUE) {
		BOOLEAN ContinueEnumeration;
		HKEY IfeoExeKey;
		WCHAR ExeBaseName[MAX_PATH];
		ULONG ExeBaseNameCch;
		ULONG UseFilter;
		WCHAR Debugger[MAX_PATH];
		ULONG SubkeyIndex;

		ExeBaseNameCch = ARRAYSIZE(ExeBaseName);
		ErrorCode = RegEnumKeyEx(
			IfeoBaseKey,
			Index++,
			ExeBaseName,
			&ExeBaseNameCch,
			NULL,
			NULL,
			NULL,
			NULL);

		if (ErrorCode == ERROR_NO_MORE_ITEMS) {
			break;
		}

		if (ErrorCode != ERROR_SUCCESS) {
			continue;
		}

		//
		// If not present, we Look for "Debugger" value and scan for "VxKexLdr.exe"
		// to check for legacy configuration.
		//

		ErrorCode = RegReadString(
			IfeoBaseKey,
			ExeBaseName,
			L"Debugger",
			Debugger,
			ARRAYSIZE(Debugger));
		
		if (ErrorCode != ERROR_SUCCESS) {
			goto NoLegacyConfigurationFound;
		}

		if (!StringSearchI(Debugger, L"VxKexLdr.exe")) {
			goto NoLegacyConfigurationFound;
		}

		// Legacy configuration has been found - call the callback
		ContinueEnumeration = ConfigurationCallback(ExeBaseName, TRUE, CallbackExtraParameter);
		if (!ContinueEnumeration) {
			SetLastError(ERROR_SUCCESS);
			return FALSE;
		}

NoLegacyConfigurationFound:
		//
		// Check for "UseFilter" value. If so, we will need to enum subkeys.
		//

		ErrorCode = RegReadI32(IfeoBaseKey, ExeBaseName, L"UseFilter", &UseFilter);
		if (ErrorCode != ERROR_SUCCESS || UseFilter == 0) {
			continue;
		}

		ErrorCode = RegOpenKeyEx(
			IfeoBaseKey,
			ExeBaseName,
			0,
			KEY_READ,
			&IfeoExeKey);

		if (ErrorCode != ERROR_SUCCESS) {
			continue;
		}

		SubkeyIndex = 0;

		while (TRUE) {
			WCHAR SubkeyName[32];
			ULONG SubkeyNameCch;
			WCHAR FilterFullPath[MAX_PATH];
			WCHAR VerifierDlls[256];
			ULONG GlobalFlag;

			SubkeyNameCch = ARRAYSIZE(SubkeyName);
			ErrorCode = RegEnumKeyEx(
				IfeoExeKey,
				SubkeyIndex++,
				SubkeyName,
				&SubkeyNameCch,
				NULL,
				NULL,
				NULL,
				NULL);

			if (ErrorCode == ERROR_NO_MORE_ITEMS) {
				break;
			}

			if (ErrorCode != ERROR_SUCCESS) {
				continue;
			}

			RegReadI32(IfeoExeKey, SubkeyName, L"GlobalFlag", &GlobalFlag);

			if ((GlobalFlag & FLG_APPLICATION_VERIFIER) == 0) {
				continue;
			}

			RegReadString(
				IfeoExeKey,
				SubkeyName,
				L"FilterFullPath",
				FilterFullPath,
				ARRAYSIZE(FilterFullPath));

			if (FilterFullPath[0] == '\0') {
				continue;
			}

			RegReadString(
				IfeoExeKey,
				SubkeyName,
				L"VerifierDlls",
				VerifierDlls,
				ARRAYSIZE(VerifierDlls));

			if (!StringSearchI(VerifierDlls, L"KexDll.dll")) {
				continue;
			}

			//
			// VxKex configuration has been found, call the callback.
			//

			ContinueEnumeration = ConfigurationCallback(FilterFullPath, FALSE, CallbackExtraParameter);

			if (!ContinueEnumeration) {
				RegCloseKey(IfeoExeKey);
				SetLastError(ERROR_SUCCESS);
				return FALSE;
			}
		}

		RegCloseKey(IfeoExeKey);
	}

	return TRUE;
}