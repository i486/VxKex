///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     util.c
//
// Abstract:
//
//     This file contains miscellaneous utility functions for VxKex.
//
// Author:
//
//     vxiiduu (02-Feb-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               02-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "kexsetup.h"
#include <AclAPI.h>

BOOLEAN IsWow64(
	VOID)
{
	if (KexIs64BitBuild) {
		return FALSE;
	} else {
		BOOLEAN Success;
		BOOL CurrentProcessIsWow64Process;

		Success = IsWow64Process(GetCurrentProcess(), &CurrentProcessIsWow64Process);

		if (Success) {
			return CurrentProcessIsWow64Process;
		} else {
			return FALSE;
		}
	}
}

// InstallationPath must be a pointer to a buffer of size MAX_PATH characters.
VOID GetDefaultInstallationLocation(
	IN	PWSTR	InstallationPath)
{
	PCWSTR FailsafeDefault = L"C:\\Program Files\\VxKex";
	DWORD EnvironmentStringLength;

	EnvironmentStringLength = ExpandEnvironmentStrings(L"%ProgramW6432%", InstallationPath, MAX_PATH);
	if (EnvironmentStringLength == 0) {
		EnvironmentStringLength = ExpandEnvironmentStrings(L"%ProgramFiles%", InstallationPath, MAX_PATH);

		if (EnvironmentStringLength == 0) {
			goto FailSafe;
		}
	}

	if (PathIsRelative(InstallationPath)) {
		goto FailSafe;
	}

	PathCchAppend(InstallationPath, MAX_PATH, L"VxKex");
	return;

FailSafe:
	StringCchCopy(InstallationPath, MAX_PATH, FailsafeDefault);
	return;
}

VOID KexSetupCreateKey(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	SubKey,
	IN	REGSAM	DesiredAccess,
	OUT	PHKEY	KeyHandleOut)
{
	ULONG ErrorCode;

	ASSERT (KexSetupTransactionHandle != NULL);
	ASSERT (KexSetupTransactionHandle != INVALID_HANDLE_VALUE);
	ASSERT (KeyHandle != NULL);
	ASSERT (KeyHandle != INVALID_HANDLE_VALUE);
	ASSERT (SubKey != NULL);
	ASSERT (DesiredAccess != 0);
	ASSERT (KeyHandleOut != NULL);

	ErrorCode = RegCreateKeyTransacted(
		KeyHandle,
		SubKey,
		0,
		NULL,
		0,
		DesiredAccess | KEY_WOW64_64KEY,
		NULL,
		KeyHandleOut,
		NULL,
		KexSetupTransactionHandle,
		NULL);

	if (ErrorCode != ERROR_SUCCESS) {
		PCWSTR BaseKeyName;

		switch ((ULONG_PTR) KeyHandle) {
		case HKEY_LOCAL_MACHINE:
			BaseKeyName = L"HKEY_LOCAL_MACHINE";
			break;
		case HKEY_CURRENT_USER:
			BaseKeyName = L"HKEY_CURRENT_USER";
			break;
		default:
			BaseKeyName = L"UNKNOWN";
			break;
		}

		ErrorBoxF(
			L"Failed to create or open registry key \"%s\\%s\". %s",
			BaseKeyName,
			SubKey,
			Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

VOID KexSetupDeleteKey(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	Subkey OPTIONAL)
{
	HKEY NewKeyHandle;
	NTSTATUS Status;
	ULONG ErrorCode;

	ASSERT (KeyHandle != NULL);
	ASSERT (KeyHandle != INVALID_HANDLE_VALUE);

	ErrorCode = RegOpenKeyTransacted(
		KeyHandle,
		Subkey,
		0,
		KEY_READ | KEY_WRITE | DELETE | KEY_WOW64_64KEY,
		&NewKeyHandle,
		KexSetupTransactionHandle,
		NULL);

	if (ErrorCode == ERROR_FILE_NOT_FOUND) {
		// No problem - key is already gone.
		return;
	}

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(L"Failed to open registry key for deletion. %s", Win32ErrorAsString(ErrorCode));
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	ErrorCode = RegDeleteTree(NewKeyHandle, NULL);
	if (ErrorCode != ERROR_SUCCESS) {
		RegCloseKey(NewKeyHandle);
		ErrorBoxF(L"Failed to delete keys and subkeys. %s", Win32ErrorAsString(ErrorCode));
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	Status = NtDeleteKey(NewKeyHandle);
	ErrorCode = RtlNtStatusToDosError(Status);

	SafeClose(NewKeyHandle);

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(L"Failed to delete key. %s", Win32ErrorAsString(ErrorCode));
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

VOID KexSetupRegWriteI32(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	IN	ULONG	Data)
{
	ULONG ErrorCode;

	ErrorCode = RegWriteI32(KeyHandle, NULL, ValueName, Data);

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(
			L"Setup was unable to write to the registry value %s. %s",
			ValueName, Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

VOID KexSetupRegWriteString(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	IN	PCWSTR	Data)
{
	ULONG ErrorCode;

	ErrorCode = RegWriteString(KeyHandle, NULL, ValueName, Data);

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(
			L"Setup was unable to write to the registry value %s. %s",
			ValueName, Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

VOID KexSetupRegReadString(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	OUT	PWSTR	Buffer,
	IN	ULONG	BufferCch)
{
	ULONG ErrorCode;

	ErrorCode = RegReadString(KeyHandle, NULL, ValueName, Buffer, BufferCch);

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(
			L"Setup was unable to read the registry value %s. %s",
			ValueName, Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

// This function applies the ACL from KexDir to the specified file.
VOID KexSetupApplyAclToFile(
	IN	PCWSTR	FilePath)
{
	ULONG ErrorCode;
	STATIC BOOLEAN AlreadyInitialized = FALSE;
	STATIC PSECURITY_DESCRIPTOR KexDirSecurity = NULL;
	STATIC PACL KexDirDacl = NULL;

	if (!AlreadyInitialized) {
		ErrorCode = GetNamedSecurityInfo(
			KexDir,
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			&KexDirDacl,
			NULL,
			&KexDirSecurity);

		ASSERT (ErrorCode == ERROR_SUCCESS);

		if (ErrorCode == ERROR_SUCCESS) {
			AlreadyInitialized = TRUE;
		} else {
			return;
		}
	}

	ErrorCode = SetNamedSecurityInfo(
		(PWSTR) FilePath,
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		KexDirDacl,
		NULL);

	ASSERT (ErrorCode == ERROR_SUCCESS);
}

VOID KexSetupSupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile)
{
	BOOLEAN Success;

	Success = SupersedeFile(SourceFile, TargetFile, KexSetupTransactionHandle);

	if (!Success) {
		ErrorBoxF(
			L"Failed to move \"%s\" to \"%s\". %s",
			SourceFile,
			TargetFile,
			GetLastErrorAsString());

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	//
	// Apply ACLs from the target directory to the target file.
	//

	KexSetupApplyAclToFile(TargetFile);
}

// Buffer must have a size of at least MAX_PATH characters.
VOID KexSetupFormatPath(
	OUT	PWSTR	Buffer,
	IN	PCWSTR	Format,
	IN	...)
{
	HRESULT Result;
	ARGLIST ArgList;

	va_start(ArgList, Format);

	Result = StringCchVPrintf(
		Buffer,
		MAX_PATH,
		Format,
		ArgList);

	va_end(ArgList);

	if (FAILED(Result)) {
		ErrorBoxF(
			L"A path is too long. Ensure the folder you have chosen to install VxKex into "
			L"is not nested too deeply.");

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

VOID KexSetupMoveFileSpecToDirectory(
	IN	PCWSTR	FileSpec,
	IN	PCWSTR	TargetDirectoryPath)
{
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;
	WCHAR SourceDirectoryPath[MAX_PATH];
	WCHAR SourcePath[MAX_PATH];
	WCHAR TargetPath[MAX_PATH];

	ASSERT (FileSpec != NULL);
	ASSERT (TargetDirectoryPath != NULL);

	StringCchCopy(SourceDirectoryPath, ARRAYSIZE(SourceDirectoryPath), FileSpec);
	PathCchRemoveFileSpec(SourceDirectoryPath, ARRAYSIZE(SourceDirectoryPath));

	FindHandle = FindFirstFileEx(
		FileSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		0);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(L"VxKex setup files could not be found. %s", GetLastErrorAsString());
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	SetLastError(ERROR_SUCCESS);

	do {
		// Check if the FindNextFile at the end of the loop reported some kind of error.
		if (GetLastError() != ERROR_SUCCESS) {
			ErrorBoxF(L"Failed to move VxKex files. %s", GetLastErrorAsString());
			RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
		}

		if (FindData.cFileName[0] == '.') {
			if (FindData.cFileName[1] == '\0') {
				continue;
			} else if (FindData.cFileName[1] == '.') {
				if (FindData.cFileName[2] == '\0') {
					continue;
				}
			}
		}

		KexSetupFormatPath(SourcePath, L"%s\\%s", SourceDirectoryPath, FindData.cFileName);
		KexSetupFormatPath(TargetPath, L"%s\\%s", TargetDirectoryPath, FindData.cFileName);
		KexSetupSupersedeFile(SourcePath, TargetPath);
		
		SetLastError(ERROR_SUCCESS);
	} until (!FindNextFile(FindHandle, &FindData) && GetLastError() == ERROR_NO_MORE_FILES);
}

VOID KexSetupCreateDirectory(
	IN	PCWSTR	DirectoryPath)
{
	BOOLEAN Success;

	ASSERT (DirectoryPath != NULL);

	Success = CreateDirectory(DirectoryPath, NULL);
	if (!Success) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			return;
		}

		ErrorBoxF(L"Failed to create directory: \"%s\". %s", DirectoryPath, GetLastErrorAsString());
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

BOOLEAN KexSetupDeleteFile(
	IN	PCWSTR	FilePath)
{
	BOOLEAN Success;

	Success = DeleteFile(FilePath);
	
	if (!Success) {
		ULONG RandomIdentifier;
		WCHAR NewFileName[MAX_PATH];

		//
		// Failed to delete the file.
		// Try to rename the file to something else and then schedule its deletion.
		//

		RandomIdentifier = GetTickCount();

		StringCchPrintf(
			NewFileName,
			ARRAYSIZE(NewFileName),
			L"%s.old_%04u", FilePath, RandomIdentifier);

		Success = MoveFile(FilePath, NewFileName);
		if (Success) {
			FilePath = NewFileName;
		}

		RtlSetCurrentTransaction(NULL);

		Success = MoveFileTransacted(
			FilePath,
			NULL,
			NULL,
			NULL,
			MOVEFILE_DELAY_UNTIL_REBOOT,
			KexSetupTransactionHandle);

		RtlSetCurrentTransaction(KexSetupTransactionHandle);

		if (!Success) {
			return FALSE;
		}
	}

	return TRUE;
}

BOOLEAN KexSetupRemoveDirectoryRecursive(
	IN	PCWSTR	DirectoryPath)
{
	ULONG DirectoryPathCch;
	ULONG DirectoryPathSpecCch;
	PWSTR DirectoryPathSpec;

	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;

	DirectoryPathCch = (ULONG) wcslen(DirectoryPath) + 1;
	DirectoryPathSpecCch = DirectoryPathCch + 2;
	DirectoryPathSpec = StackAlloc(WCHAR, DirectoryPathSpecCch);
	StringCchCopy(DirectoryPathSpec, DirectoryPathSpecCch, DirectoryPath);
	PathCchAppend(DirectoryPathSpec, DirectoryPathSpecCch, L"*");

	FindHandle = FindFirstFileEx(
		DirectoryPathSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		FIND_FIRST_EX_LARGE_FETCH);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	do {
		WCHAR FileFullPath[MAX_PATH];

		// skip . and .. directories
		if (FindData.cFileName[0] == '.') {
			if (FindData.cFileName[1] == '.') {
				if (FindData.cFileName[2] == '\0') {
					continue;
				}
			} else if (FindData.cFileName[1] == '\0') {
				continue;
			}
		}

		if (FindData.dwFileAttributes == FILE_ATTRIBUTE_SYSTEM) {
			// Better not delete this.
			continue;
		}

		StringCchCopy(FileFullPath, ARRAYSIZE(FileFullPath), DirectoryPath);
		PathCchAppend(FileFullPath, ARRAYSIZE(FileFullPath), FindData.cFileName);

		if (FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			// Recurse into directory.
			KexSetupRemoveDirectoryRecursive(FileFullPath);
			continue;
		}

		// Normal file - delete it
		KexSetupDeleteFile(FileFullPath);
	} until (!FindNextFile(FindHandle, &FindData) && GetLastError() == ERROR_NO_MORE_FILES);

	// Remove the directory itself, which should now hopefully be empty.
	return RemoveDirectory(DirectoryPath);
}

BOOLEAN KexSetupDeleteFilesBySpec(
	IN	PCWSTR	FileSpec)
{
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;
	BOOLEAN Success;

	FindHandle = FindFirstFileEx(
		FileSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		FIND_FIRST_EX_LARGE_FETCH);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	Success = 0xff;

	do {
		WCHAR FileFullPath[MAX_PATH];

		// skip . and .. directories
		if (FindData.cFileName[0] == '.') {
			if (FindData.cFileName[1] == '.') {
				if (FindData.cFileName[2] == '\0') {
					continue;
				}
			} else if (FindData.cFileName[1] == '\0') {
				continue;
			}
		}

		if (FindData.dwFileAttributes == FILE_ATTRIBUTE_SYSTEM) {
			// Better not delete this.
			Success = FALSE;
			continue;
		}

		StringCchCopy(FileFullPath, ARRAYSIZE(FileFullPath), FileSpec);
		PathCchRemoveFileSpec(FileFullPath, ARRAYSIZE(FileFullPath));
		PathCchAppend(FileFullPath, ARRAYSIZE(FileFullPath), FindData.cFileName);

		if (FindData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		// Normal file - delete it
		Success &= KexSetupDeleteFile(FileFullPath);
	} until (!FindNextFile(FindHandle, &FindData) && GetLastError() == ERROR_NO_MORE_FILES);

	return Success;
}