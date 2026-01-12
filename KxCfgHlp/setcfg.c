///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     setcfg.c
//
// Abstract:
//
//     Contains functions for setting VxKex configuration.
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
// Configure VxKex for a particular program according to the configuration data
// structure.
// Returns TRUE on success and FALSE on failure. Call GetLastError() to obtain
// more information.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgSetConfiguration(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration,
	IN	HANDLE							TransactionHandle OPTIONAL)
{
	NTSTATUS Status;
	ULONG ErrorCode;
	BOOLEAN Success;
	HKEY KeyHandle;
	UNICODE_STRING ExeFullPathUS;

	WCHAR VerifierDlls[256];
	ULONG GlobalFlag;
	ULONG VerifierFlags;
	ULONG KEX_DisableForChild;
	ULONG KEX_DisableAppSpecific;
	ULONG KEX_WinVerSpoof;
	ULONG KEX_StrongVersionSpoof;

	ASSERT (ExeFullPath != NULL);
	ASSERT (ExeFullPath[0] != '\0');
	ASSERT (Configuration != NULL);

	if (KxCfgpElevationRequired()) {
		// Transactions cannot be applied cross-process. (Well, they can, but
		// we don't implement that kind of functionality because it's a huge pain
		// to get the transaction handles duplicated into elevated processes.
		// Plus, we don't need to do it.)
		ASSERT (TransactionHandle == NULL);

		return KxCfgpElevatedSetConfiguration(ExeFullPath, Configuration);
	}

	VerifierDlls[0] = '\0';

	//
	// The first thing to do is see whether we are doing the special case of
	// removing all the configuration values. A Configuration structure consisting
	// of all zeroes means deleting the VxKex configuration for a program.
	//

	if (Configuration->Enabled == FALSE &&
		Configuration->DisableForChild == FALSE &&
		Configuration->DisableAppSpecificHacks == FALSE &&
		Configuration->WinVerSpoof == WinVerSpoofNone &&
		Configuration->StrongSpoofOptions == 0) {

		return KxCfgDeleteConfiguration(ExeFullPath, TransactionHandle);
	}

	//
	// Step 1. Open or create the IFEO key for this program.
	//

	RtlInitUnicodeString(&ExeFullPathUS, ExeFullPath);

	Status = LdrOpenImageFileOptionsKey(
		&ExeFullPathUS,
		FALSE,
		(PHANDLE) &KeyHandle);

	if (!NT_SUCCESS(Status)) {
		// Probably there is simply no IFEO key for this program.
		// So we will call a helper function that creates one for us.
		Success = KxCfgpCreateIfeoKeyForProgram(
			ExeFullPath,
			&KeyHandle,
			TransactionHandle);

		if (!Success) {
			return FALSE;
		}
	} else {
		//
		// The key returned directly from LdrOpenImageFileOptionsKey might be
		// one that doesn't use FilterFullPath. In this case, we will still have
		// to call the helper function to create a proper one for us.
		//

		// Check whether that's the case by querying the existence of FilterFullPath
		// key under the IFEO subkey for the program.
		ErrorCode = RegGetValue(
			KeyHandle,
			NULL,
			L"FilterFullPath",
			RRF_RT_REG_SZ,
			NULL,
			NULL,
			NULL);

		if (ErrorCode == ERROR_SUCCESS) {
			// All good.
			// The key is opened read only by LdrOpenImageFileOptionsKey, and is
			// not transacted. Reopen read-write.

			Success = RegReOpenKey(&KeyHandle, KEY_READ | KEY_WRITE, TransactionHandle);
			if (!Success) {
				RegCloseKey(KeyHandle);
				return FALSE;
			}
		} else {
			// We need to create a proper IFEO subkey that uses a filter.
			// Do this by calling a helper function:
			RegCloseKey(KeyHandle);

			Success = KxCfgpCreateIfeoKeyForProgram(
				ExeFullPath,
				&KeyHandle,
				TransactionHandle);
			
			if (!Success) {
				return FALSE;
			}
		}
	}

	//
	// Step 2. Set registry values.
	//

	RegReadI32(KeyHandle, NULL, L"GlobalFlag", &GlobalFlag);
	RegReadI32(KeyHandle, NULL, L"VerifierFlags", &VerifierFlags);
	
	RegReadString(
		KeyHandle,
		NULL,
		L"VerifierDlls",
		VerifierDlls,
		ARRAYSIZE(VerifierDlls));

	if (Configuration->Enabled == FALSE) {
		BOOLEAN KexDllRemovedFromVerifierDlls;

		//
		// 1. Remove KexDll from the list of VerifierDlls.
		// 2. If there are no more VerifierDlls, turn off Application Verifier.
		//

		KexDllRemovedFromVerifierDlls = KxCfgpRemoveKexDllFromVerifierDlls(VerifierDlls);

		if (KexDllRemovedFromVerifierDlls && VerifierDlls[0] == '\0') {
			GlobalFlag &= ~FLG_APPLICATION_VERIFIER;
			VerifierFlags = 0;
		}
	} else {
		//
		// 1. Add KexDll to the list of VerifierDlls.
		// 2. Turn on Application Verifier and set VerifierFlags to 0x80000000.
		//

		if (!StringSearchI(VerifierDlls, L"kexdll.dll")) {
			StringCchCat(
				VerifierDlls,
				ARRAYSIZE(VerifierDlls),
				VerifierDlls[0] == '\0' ? L"kexdll.dll" : L" kexdll.dll");
		}

		GlobalFlag |= FLG_APPLICATION_VERIFIER;
		VerifierFlags = 0x80000000;
	}

	KEX_DisableForChild		= Configuration->DisableForChild;
	KEX_DisableAppSpecific	= Configuration->DisableAppSpecificHacks;
	KEX_WinVerSpoof			= Configuration->WinVerSpoof;
	KEX_StrongVersionSpoof	= Configuration->StrongSpoofOptions;

	try {
		ErrorCode = RegWriteI32(KeyHandle, NULL, L"KEX_DisableForChild", KEX_DisableForChild);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteI32(KeyHandle, NULL, L"KEX_DisableAppSpecific", KEX_DisableAppSpecific);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteI32(KeyHandle, NULL, L"KEX_WinVerSpoof", KEX_WinVerSpoof);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteI32(KeyHandle, NULL, L"KEX_StrongVersionSpoof", KEX_StrongVersionSpoof);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteI32(KeyHandle, NULL, L"GlobalFlag", GlobalFlag);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteI32(KeyHandle, NULL, L"VerifierFlags", VerifierFlags);
		if (ErrorCode) {
			return FALSE;
		}

		ErrorCode = RegWriteString(
			KeyHandle,
			NULL,
			L"VerifierDlls",
			VerifierDlls);

		if (ErrorCode) {
			return FALSE;
		}
	} finally {
		SetLastError(ErrorCode);
		RegCloseKey(KeyHandle);
	}

	return TRUE;
}