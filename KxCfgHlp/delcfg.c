///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     delcfg.c
//
// Abstract:
//
//     Contains functions for deleting VxKex configuration for a program.
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
// Delete VxKex configuration for a particular program.
// Returns TRUE on success and FALSE on failure. Call GetLastError() to obtain
// more information.
// Note: This function only handles post-rewrite VxKex configuration and not
// legacy configuration. There's a separate function to handle legacy
// configuration.
//
KXCFGDECLSPEC BOOLEAN KxCfgDeleteConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	NTSTATUS Status;
	ULONG ErrorCode;
	BOOLEAN Success;
	HKEY IfeoKeyHandle;
	ULONG Index;
	UNICODE_STRING ExeFullPathUS;
	WCHAR VerifierDlls[256];
	BOOLEAN KexDllWasRemoved;
	ULONG NumberOfValues;
	ULONG NumberOfSubkeys;
	
	if (KxCfgpElevationRequired()) {
		ASSERT (TransactionHandle == NULL);
		return KxCfgpElevatedDeleteConfiguration(ExeFullPath);
	}

	//
	// 1. Open the IFEO key for the program. If there is no IFEO key for this
	//    program, then return TRUE, because there is already no VxKex config
	//    for this program.
	//
	// 2. Unconditionally delete all VxKex-specific values (KEX_*).
	//
	// 3. Unconditionally remove KexDll.dll from VerifierDlls (if
	//    VerifierDlls is present). If KexDll.dll was the only entry in
	//    VerifierDlls, then delete VerifierDlls.
	//
	// 4. If KexDll.dll was the only entry in VerifierDlls, turn off
	//    FLG_APPLICATION_VERIFIER in GlobalFlag. If GlobalFlag is now
	//    zero, delete GlobalFlag.
	//
	// 5. If FLG_APPLICATION_VERIFIER was removed from GlobalFlag, then
	//    delete VerifierFlags.
	//
	// 6. If there are no more values in the IFEO key for the program (besides
	//    FilterFullPath), delete the IFEO key for the program.
	//
	// 7. If there are no more subkeys under the IFEO EXE key (the key that
	//    has UseFilter in it), delete the IFEO EXE key.
	//

	//
	// Step 1. Open the IFEO key for the program.
	//

	RtlInitUnicodeString(&ExeFullPathUS, ExeFullPath);

	Status = LdrOpenImageFileOptionsKey(
		&ExeFullPathUS,
		FALSE,
		(PHANDLE) &IfeoKeyHandle);

	if (Status == STATUS_OBJECT_NAME_NOT_FOUND) {
		return TRUE;
	} else {
		if (!NT_SUCCESS(Status)) {
			SetLastError(RtlNtStatusToDosError(Status));
			return FALSE;
		}
	}

	// Note that the LdrOpenImageFileOptionsKey opens the key read-only.
	// We will have to reopen it read-write.
	Success = RegReOpenKey(
		&IfeoKeyHandle,
		KEY_READ | KEY_WRITE | DELETE,
		TransactionHandle);

	if (!Success) {
		return FALSE;
	}

	try {
		//
		// Step 2. Unconditionally delete all VxKex-specific values.
		// In order to reduce the maintenance requirements for this code, we will
		// enumerate the values of the key and simply delete everything that has
		// a name starting with "KEX_" (rather than hard-coding the names that we
		// want to delete).
		//

		Index = 0;

		while (TRUE) {
			WCHAR ValueName[128];
			ULONG ValueNameCch;

			ValueNameCch = ARRAYSIZE(ValueName);

			ErrorCode = RegEnumValue(
				IfeoKeyHandle,
				Index++,
				ValueName,
				&ValueNameCch,
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

			// remember, registry key and value names are case insensitive
			if (StringBeginsWithI(ValueName, L"KEX_")) {
				ErrorCode = RegDeleteValue(IfeoKeyHandle, ValueName);
				ASSERT (ErrorCode == ERROR_SUCCESS);

				//
				// This is required so that RegEnumValue doesn't skip certain values.
				// It's a bit of a hack. The documentation for RegEnumValue says:
				//
				//   While using RegEnumValue, an application should not call any
				//   registry functions that might change the key being queried.
				//
				// The quick and dirty solution which follows the documentation is to
				// simply restart the enumeration from 0, but I've found that simply
				// decrementing the index (remember, we incremented it after the call
				// to RegEnumValue) works just as well.
				//

				--Index;
			}
		}

		//
		// Step 3. Unconditionally remove KexDll.dll from VerifierDlls.
		// If there's a key called VerifierDlls under the IfeoKeyHandle, we will call
		// a helper function to remove KexDll.dll from the list.
		//

		ErrorCode = RegReadString(
			IfeoKeyHandle,
			NULL,
			L"VerifierDlls",
			VerifierDlls,
			ARRAYSIZE(VerifierDlls));

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			// VxKex isn't enabled.
			return TRUE;
		} else if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		KexDllWasRemoved = KxCfgpRemoveKexDllFromVerifierDlls(VerifierDlls);

		if (KexDllWasRemoved) {
			if (VerifierDlls[0] == '\0') {
				// VerifierDlls is now empty because we removed KexDll.dll from it.
				// Because of that we will now delete the VerifierDlls value.
				ErrorCode = RegDeleteValue(IfeoKeyHandle, L"VerifierDlls");
			} else {
				// VerifierDlls was modified but it's not empty. We will write it
				// back to the registry.

				ErrorCode = RegWriteString(
					IfeoKeyHandle,
					NULL,
					L"VerifierDlls",
					VerifierDlls);
			}

			if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}
		}

		//
		// Step 4. If VerifierDlls is now empty, we will also remove FLG_APPLICATION_VERIFIER
		// from the global flags. We will also remove FLG_SHOW_LDR_SNAPS since it is a flag
		// that, at this point, we can infer only VxKex would have set.
		//

		if (KexDllWasRemoved && VerifierDlls[0] == '\0') {
			ULONG GlobalFlag;

			ErrorCode = RegReadI32(IfeoKeyHandle, NULL, L"GlobalFlag", &GlobalFlag);

			if (ErrorCode == ERROR_FILE_NOT_FOUND) {
				return TRUE;
			} else if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}

			if (GlobalFlag & (FLG_APPLICATION_VERIFIER | FLG_SHOW_LDR_SNAPS)) {

				GlobalFlag &= ~(FLG_APPLICATION_VERIFIER | FLG_SHOW_LDR_SNAPS);

				if (GlobalFlag == 0) {
					// GlobalFlag is now empty, so delete it.
					ErrorCode = RegDeleteValue(IfeoKeyHandle, L"GlobalFlag");
				} else {
					// GlobalFlag still contains stuff. Write it back to registry.
					ErrorCode = RegWriteI32(IfeoKeyHandle, NULL, L"GlobalFlag", GlobalFlag);
				}

				if (ErrorCode != ERROR_SUCCESS) {
					SetLastError(ErrorCode);
					return FALSE;
				}

				//
				// Step 5. If FLG_APPLICATION_VERIFIER was removed from GlobalFlag, then
				// delete VerifierFlags.
				//

				RegDeleteValue(IfeoKeyHandle, L"VerifierFlags");
			}
		}

		//
		// Step 6. If there are no more values in the IFEO key for the program (besides
		// FilterFullPath), delete the IFEO key for the program.
		//

		ErrorCode = RegQueryInfoKey(
			IfeoKeyHandle,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			NULL,
			&NumberOfValues,
			NULL,
			NULL,
			NULL,
			NULL);

		if (ErrorCode == ERROR_SUCCESS) {
			if (NumberOfValues == 1) {
				// One value under this key - check if its name is FilterFullPath.
				// If so, set number of values to 0 (ignore the presence of FilterFullPath).

				ErrorCode = RegGetValue(
					IfeoKeyHandle,
					NULL,
					L"FilterFullPath",
					RRF_RT_ANY,
					NULL,
					NULL,
					NULL);

				if (ErrorCode == ERROR_SUCCESS) {
					NumberOfValues = 0;
				}
			}

			if (NumberOfValues == 0) {
				// No values we care about here, so delete the key.
				RegDeleteTree(IfeoKeyHandle, NULL);
				NtDeleteKey(IfeoKeyHandle);
				RegCloseKey(IfeoKeyHandle);
				IfeoKeyHandle = NULL;

				//
				// Step 7. If there are no more subkeys under the IFEO EXE key (the key that
				// has UseFilter in it), delete the IFEO EXE key.
				//

				{
					PCWSTR ExeBaseName;

					ExeBaseName = PathFindFileName(ExeFullPath);

					Status = LdrOpenImageFileOptionsKey(
						NULL,
						FALSE,
						(PHANDLE) &IfeoKeyHandle);

					ASSERT (NT_SUCCESS(Status));

					IfeoKeyHandle = KxCfgpOpenKey(
						IfeoKeyHandle,
						ExeBaseName,
						KEY_READ | KEY_WRITE | DELETE,
						TransactionHandle);

					ASSERT (IfeoKeyHandle != NULL);

					if (!IfeoKeyHandle) {
						// ignore the error, it's not critical
						return TRUE;
					}
				}

				ErrorCode = RegQueryInfoKey(
					IfeoKeyHandle,
					NULL,
					NULL,
					NULL,
					&NumberOfSubkeys,
					NULL,
					NULL,
					&NumberOfValues,
					NULL,
					NULL,
					NULL,
					NULL);

				ASSERT (ErrorCode == ERROR_SUCCESS);

				if (ErrorCode != ERROR_SUCCESS) {
					return TRUE;
				}
				
				if (NumberOfSubkeys == 0 && NumberOfValues == 1) {
					// Check if the value is UseFilter. If so, ignore it.
					ErrorCode = RegGetValue(
						IfeoKeyHandle,
						NULL,
						L"UseFilter",
						RRF_RT_ANY,
						NULL,
						NULL,
						NULL);

					if (ErrorCode == ERROR_SUCCESS) {
						NumberOfValues = 0;
					}
				}

				if (NumberOfSubkeys == 0 && NumberOfValues == 0) {
					RegDeleteTree(IfeoKeyHandle, NULL);
					NtDeleteKey(IfeoKeyHandle);
				}
			}
		}
	} finally {
		RegCloseKey(IfeoKeyHandle);
	}

	return TRUE;
}

//
// Check if a particular program has configuration for VxKex versions equal to
// or older than 0.0.0.3, and if so, delete it.
// Returns TRUE if legacy configuration was deleted, or FALSE if not.
//
// Legacy versions of VxKex do not use the Win7+ IFEO filter mechanism.
// Therefore, all we need to do is see if the IFEO base key has a subkey
// matching the EXE base name, and if so, check to see if it has a Debugger
// string value that contains "VxKexLdr.exe" somewhere in it. If so, delete
// the Debugger value.
//
// Legacy versions stored all auxiliary configuration such as WinVerSpoof etc.
// within the old "VxKexLdr" *HKCU* key, which we won't bother to touch.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgDeleteLegacyConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	NTSTATUS Status;
	ULONG ErrorCode;
	HKEY IfeoKey;
	PCWSTR ExeBaseName;
	WCHAR Debugger[MAX_PATH];
	ULONG NumberOfValues;
	ULONG NumberOfSubkeys;

	ASSERT (ExeFullPath != NULL);
	ASSERT (ExeFullPath[0] != '\0');

	ExeBaseName = PathFindFileName(ExeFullPath);
	if (!ExeBaseName) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	Status = LdrOpenImageFileOptionsKey(
		NULL,
		FALSE,
		(PHANDLE) &IfeoKey);

	if (!NT_SUCCESS(Status)) {
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
	}

	IfeoKey = KxCfgpOpenKey(
		IfeoKey,
		ExeBaseName,
		KEY_READ | KEY_WRITE | DELETE,
		TransactionHandle);

	if (!IfeoKey) {
		ErrorCode = GetLastError();

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			// no legacy config to delete
			return TRUE;
		} else {
			SetLastError(ErrorCode);
			return FALSE;
		}
	}

	try {
		ErrorCode = RegReadString(
			IfeoKey,
			NULL,
			L"Debugger",
			Debugger,
			ARRAYSIZE(Debugger));

		if (ErrorCode == ERROR_FILE_NOT_FOUND) {
			// No legacy configuration to delete.
			return TRUE;
		} else if (ErrorCode != ERROR_SUCCESS) {
			SetLastError(ErrorCode);
			return FALSE;
		}

		if (StringSearchI(Debugger, L"VxKexLdr.exe")) {
			// We have legacy configuration. Delete the Debugger value.
			ErrorCode = RegDeleteValue(IfeoKey, L"Debugger");

			if (ErrorCode != ERROR_SUCCESS) {
				SetLastError(ErrorCode);
				return FALSE;
			}
		}

		//
		// If there are no more values or subkeys inside the IFEO EXE key,
		// then delete the IFEO EXE key.
		//

		ErrorCode = RegQueryInfoKey(
			IfeoKey,
			NULL,
			NULL,
			NULL,
			&NumberOfSubkeys,
			NULL,
			NULL,
			&NumberOfValues,
			NULL,
			NULL,
			NULL,
			NULL);

		if (ErrorCode == ERROR_SUCCESS) {
			if (NumberOfSubkeys == 0 && NumberOfValues == 0) {
				RegDeleteTree(IfeoKey, NULL);
				NtDeleteKey(IfeoKey);
			}
		}
	} finally {
		RegCloseKey(IfeoKey);
	}

	return TRUE;
}