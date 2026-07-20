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
//     vxiiduu               30-Apr-2026  Change KexSetupApplyAclToFile to stop
//                                        using Get/SetNamedSecurityInfo, which
//                                        do not support transactions
//     vxiiduu               04-Jul-2026  Change KexSetupApplyAclToFile to be
//                                        recursive.
//     vxiiduu               05-Jul-2026  Remove obsolete IsWow64 function.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsetup.h"
#include <AclAPI.h>

// InstallationPath must be a pointer to a buffer of size MAX_PATH characters.
VOID GetDefaultInstallationLocation(
	IN	PWSTR	InstallationPath)
{
	PCWSTR FailsafeDefault = L"C:\\Program Files\\VxKex";
	DWORD EnvironmentStringLength;

	EnvironmentStringLength = GetEnvironmentVariable(L"ProgramW6432", InstallationPath, MAX_PATH);
	if (EnvironmentStringLength == 0) {
		EnvironmentStringLength = GetEnvironmentVariable(L"ProgramFiles", InstallationPath, MAX_PATH);

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
			_(L"Failed to create or open registry key \"%s\\%s\". %s"),
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
		ErrorBoxF(
			_(L"Failed to open registry key for deletion. %s"),
			Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	ErrorCode = RegDeleteTree(NewKeyHandle, NULL);
	if (ErrorCode != ERROR_SUCCESS) {
		RegCloseKey(NewKeyHandle);

		ErrorBoxF(
			_(L"Failed to delete keys and subkeys. %s"),
			Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	Status = NtDeleteKey(NewKeyHandle);
	ErrorCode = RtlNtStatusToDosError(Status);

	SafeClose(NewKeyHandle);

	if (ErrorCode != ERROR_SUCCESS) {
		ErrorBoxF(_(L"Failed to delete key. %s"), Win32ErrorAsString(ErrorCode));
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
			_(L"Setup was unable to write to the registry value %s. %s"),
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
			_(L"Setup was unable to write to the registry value %s. %s"),
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
			_(L"Setup was unable to read the registry value %s. %s"),
			ValueName, Win32ErrorAsString(ErrorCode));

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

// This function applies the ACL from KexDir to the specified file or folder
// recursively.
VOID KexSetupApplyAclToFile(
	IN	PCWSTR	FilePath)
{
	BOOL Success;
	ULONG ErrorCode;
	HANDLE FileHandle;
	STATIC PACL Dacl = NULL;
	WIN32_FILE_ATTRIBUTE_DATA FileAttributes;

	if (!Dacl) {
		PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
		WCHAR KexDirParentDir[MAX_PATH];

		StringCchCopy(KexDirParentDir, ARRAYSIZE(KexDirParentDir), KexDir);
		PathCchRemoveFileSpec(KexDirParentDir, ARRAYSIZE(KexDirParentDir));

		FileHandle = CreateFileTransacted(
			KexDirParentDir,
			READ_CONTROL,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS,
			NULL,
			KexSetupTransactionHandle,
			NULL,
			NULL);

		ASSERT (VALID_HANDLE(FileHandle));

		if (FileHandle == INVALID_HANDLE_VALUE) {
			return;
		}

		ErrorCode = GetSecurityInfo(
			FileHandle,
			SE_FILE_OBJECT,
			DACL_SECURITY_INFORMATION,
			NULL,
			NULL,
			&Dacl,
			NULL,
			&SecurityDescriptor);

		// We deliberately do not free the security descriptor, because
		// the DACL is inside that allocation.

		ASSERT (ErrorCode == ERROR_SUCCESS);
		SafeClose(FileHandle);

		if (ErrorCode != ERROR_SUCCESS) {
			return;
		}
	}

	FileHandle = CreateFileTransacted(
		FilePath,
		READ_CONTROL | WRITE_DAC,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS,
		NULL,
		KexSetupTransactionHandle,
		NULL,
		NULL);

	ASSERT (VALID_HANDLE(FileHandle));

	if (FileHandle == INVALID_HANDLE_VALUE) {
		return;
	}

	ASSERT (Dacl != NULL);

	ErrorCode = SetSecurityInfo(
		FileHandle,
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION,
		NULL,
		NULL,
		Dacl,
		NULL);

	ASSERT (ErrorCode == ERROR_SUCCESS);
	SafeClose(FileHandle);

	//
	// If the file is a directory, recurse into it.
	//

	Success = GetFileAttributesTransacted(
		FilePath,
		GetFileExInfoStandard,
		&FileAttributes,
		KexSetupTransactionHandle);

	ASSERT (Success);

	if (Success && (FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		HRESULT Result;
		WCHAR FindPath[MAX_PATH];
		HANDLE FindHandle;
		WIN32_FIND_DATA FindData;

		Result = StringCchCopy(FindPath, ARRAYSIZE(FindPath), FilePath);
		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			return;
		}

		Result = PathCchAppend(FindPath, ARRAYSIZE(FindPath), L"*");
		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			return;
		}

		FindHandle = FindFirstFileTransacted(
			FindPath,
			FindExInfoBasic,
			&FindData,
			FindExSearchNameMatch,
			NULL,
			0,
			KexSetupTransactionHandle);

		ASSERT (FindHandle != INVALID_HANDLE_VALUE);

		if (FindHandle == INVALID_HANDLE_VALUE) {
			return;
		}

		do {
			WCHAR FullPath[MAX_PATH];

			if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
				(StringEqual(FindData.cFileName, L".") || StringEqual(FindData.cFileName, L".."))) {
				continue;
			}

			Result = StringCchCopy(FullPath, ARRAYSIZE(FullPath), FilePath);
			ASSERT (SUCCEEDED(Result));

			if (FAILED(Result)) {
				continue;
			}

			Result = PathCchAppend(FullPath, ARRAYSIZE(FullPath), FindData.cFileName);
			ASSERT (SUCCEEDED(Result));

			if (FAILED(Result)) {
				continue;
			}

			KexSetupApplyAclToFile(FullPath);
		} until (!FindNextFile(FindHandle, &FindData));

		SafeFindClose(FindHandle);
	}
}

BOOLEAN KexSetupFilesAreIdentical(
	IN	PCWSTR	File1,
	IN	PCWSTR	File2)
{
	BOOLEAN Identical;
	BOOLEAN Success;
	HANDLE FileHandle1;
	HANDLE FileHandle2;
	HANDLE SectionHandle1;
	HANDLE SectionHandle2;
	PVOID Data1;
	PVOID Data2;
	LARGE_INTEGER FileSize1;
	LARGE_INTEGER FileSize2;

	Identical = FALSE;
	FileHandle1 = NULL;
	FileHandle2 = NULL;
	SectionHandle1 = NULL;
	SectionHandle2 = NULL;
	Data1 = NULL;
	Data2 = NULL;

	//
	// Open the files.
	//

	FileHandle1 = CreateFileTransacted(
		File1,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL,
		KexSetupTransactionHandle,
		NULL,
		NULL);

	ASSERT (FileHandle1 != INVALID_HANDLE_VALUE);

	if (FileHandle1 == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	FileHandle2 = CreateFileTransacted(
		File2,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL,
		KexSetupTransactionHandle,
		NULL,
		NULL);

	ASSERT (FileHandle2 != INVALID_HANDLE_VALUE);

	if (FileHandle2 == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	//
	// Check file size is identical.
	//

	Success = GetFileSizeEx(FileHandle1, &FileSize1);
	ASSERT (Success);

	if (!Success) {
		goto Exit;
	}

	Success = GetFileSizeEx(FileHandle2, &FileSize2);
	ASSERT (Success);

	if (!Success) {
		goto Exit;
	}

	if (FileSize1.QuadPart != FileSize2.QuadPart) {
		// Files can't be the same if size is different
		goto Exit;
	}

	if (FileSize1.QuadPart == 0) {
		// Zero-length files are always identical
		Identical = TRUE;
		goto Exit;
	}

	//
	// Create section objects.
	//

	SectionHandle1 = CreateFileMapping(
		FileHandle1,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);

	ASSERT (SectionHandle1 != NULL);

	if (SectionHandle1 == NULL) {
		goto Exit;
	}

	SectionHandle2 = CreateFileMapping(
		FileHandle2,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);

	ASSERT (SectionHandle2 != NULL);

	if (SectionHandle2 == NULL) {
		goto Exit;
	}

	//
	// Map section objects to memory.
	//

	Data1 = MapViewOfFile(
		SectionHandle1,
		FILE_MAP_READ,
		0,
		0,
		0);

	ASSERT (Data1 != NULL);

	if (Data1 == NULL) {
		goto Exit;
	}

	Data2 = MapViewOfFile(
		SectionHandle2,
		FILE_MAP_READ,
		0,
		0,
		0);

	ASSERT (Data2 != NULL);

	if (Data2 == NULL) {
		goto Exit;
	}

	//
	// Check if the file contents are equal.
	//

	ASSERT (FileSize1.QuadPart == FileSize2.QuadPart);
	ASSERT (FileSize1.QuadPart > 0);
	Identical = RtlEqualMemory(Data1, Data2, (SIZE_T) FileSize1.QuadPart);

Exit:
	if (Data1) {
		Success = UnmapViewOfFile(Data1);
		ASSERT (Success);
	}

	if (Data2) {
		Success = UnmapViewOfFile(Data2);
		ASSERT (Success);
	}

	SafeClose(SectionHandle1);
	SafeClose(SectionHandle2);
	SafeClose(FileHandle1);
	SafeClose(FileHandle2);

	return Identical;
}

// NOTE: This function will return TRUE if Directory2 contains additional files that
// are not in Directory1. Be careful.
BOOLEAN KexSetupDirectoriesAreIdentical(
	IN	PCWSTR	Directory1,
	IN	PCWSTR	Directory2)
{
	BOOLEAN Identical;
	WCHAR FindPath[MAX_PATH];
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;
	ULONG NumberOfItemsInDirectory1;
	ULONG NumberOfItemsInDirectory2;

	Identical = FALSE;
	FindHandle = NULL;
	NumberOfItemsInDirectory1 = 0;
	NumberOfItemsInDirectory2 = 0;

	//
	// Use FindFirstFile & co. to loop through the contents of the first
	// directory.
	//

	StringCchCopy(FindPath, ARRAYSIZE(FindPath), Directory1);
	PathCchAppend(FindPath, ARRAYSIZE(FindPath), L"*");

	FindHandle = FindFirstFileTransacted(
		FindPath,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		0,
		KexSetupTransactionHandle);

	ASSERT (FindHandle != INVALID_HANDLE_VALUE);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		goto Exit;
	}

	do {
		WCHAR ThisFile[MAX_PATH];	// file from Directory1
		WCHAR OtherFile[MAX_PATH];	// file from Directory2

		if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
			(StringEqual(FindData.cFileName, L".") || StringEqual(FindData.cFileName, L".."))) {
			continue;
		}

		++NumberOfItemsInDirectory1;

		StringCchCopy(ThisFile, ARRAYSIZE(ThisFile), Directory1);
		PathCchAppend(ThisFile, ARRAYSIZE(ThisFile), FindData.cFileName);
		StringCchCopy(OtherFile, ARRAYSIZE(OtherFile), Directory2);
		PathCchAppend(OtherFile, ARRAYSIZE(OtherFile), FindData.cFileName);

		Identical = KexSetupFilesOrDirectoriesAreIdentical(ThisFile, OtherFile);

		if (!Identical) {
			goto Exit;
		}

		SetLastError(ERROR_SUCCESS);
	} until (!FindNextFile(FindHandle, &FindData));

	ASSERT (GetLastError() == ERROR_NO_MORE_FILES);

	Identical = TRUE;

Exit:
	SafeFindClose(FindHandle);

	return Identical;
}

// NOTE: This function will return TRUE if File2 is a directory which contains
// additional files that are not in File1. Be careful.
BOOLEAN KexSetupFilesOrDirectoriesAreIdentical(
	IN	PCWSTR	File1,
	IN	PCWSTR	File2)
{
	BOOLEAN Success;
	BOOLEAN Identical;
	WIN32_FILE_ATTRIBUTE_DATA AttributeData1;
	WIN32_FILE_ATTRIBUTE_DATA AttributeData2;
	BOOLEAN IsDirectory1;
	BOOLEAN IsDirectory2;

	Identical = FALSE;

	//
	// Figure out whether File1 and File2 are directories.
	// Note: If either File1 or File2 does not exist, then
	// GetFileAttributesTransacted will fail, and we'll correctly
	// return FALSE.
	//

	Success = GetFileAttributesTransacted(
		File1,
		GetFileExInfoStandard,
		&AttributeData1,
		KexSetupTransactionHandle);

	if (!Success) {
		goto Exit;
	}

	Success = GetFileAttributesTransacted(
		File2,
		GetFileExInfoStandard,
		&AttributeData2,
		KexSetupTransactionHandle);

	if (!Success) {
		goto Exit;
	}

	IsDirectory1 = !!(AttributeData1.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	IsDirectory2 = !!(AttributeData2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

	//
	// If both are files, we'll just call KexSetupFilesAreIdentical.
	// If one is a file and one is a directory, we'll return FALSE.
	// If both are directories, then we'll need to start recursing.
	//

	if (!IsDirectory1 && !IsDirectory2) {
		if ((AttributeData1.nFileSizeLow != AttributeData2.nFileSizeLow) ||
			(AttributeData1.nFileSizeHigh != AttributeData2.nFileSizeHigh)) {

			// file sizes different, no need to call comparison function
			goto Exit;
		}

		Identical = KexSetupFilesAreIdentical(File1, File2);
		goto Exit;
	}

	if (IsDirectory1 != IsDirectory2) {
		goto Exit;
	}

	ASSERT (IsDirectory1 && IsDirectory2);

	Identical = KexSetupDirectoriesAreIdentical(File1, File2);

Exit:
	return Identical;
}

VOID KexSetupSupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile)
{
	BOOLEAN Success;

	Success = MoveFileTransacted(
		SourceFile,
		TargetFile,
		NULL,
		NULL,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED,
		KexSetupTransactionHandle);

	if (!Success) {
		if (KexSetupFilesOrDirectoriesAreIdentical(SourceFile, TargetFile)) {
			// We couldn't move the files but it turns out that they're the same anyway.
			
			//
			// Fix the ACLs in the files even though we don't have to change the file
			// contents.
			// Some beta builds were distributed which failed to set ACLs on dictionary
			// files and ICU data files, etc. which means that applications would fail
			// if they tried to use these files while running as non-admin.
			//

			KexSetupApplyAclToFile(TargetFile);

			return;
		}

		// The target file is likely in use.
		// Call SupersedeFile, which renames the existing file and then
		// uses MOVEFILE_DELAY_UNTIL_REBOOT to schedule deletion later.
		Success = SupersedeFile(SourceFile, TargetFile, KexSetupTransactionHandle);

		if (!Success) {
			ErrorBoxF(
				_(L"Failed to move \"%s\" to \"%s\". %s"),
				SourceFile,
				TargetFile,
				GetLastErrorAsString());

			RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
		}
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
		ErrorBoxF(_(
			L"A path is too long. Ensure the folder you have chosen to install VxKex into "
			L"is not nested too deeply."));

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

	FindHandle = FindFirstFileTransacted(
		FileSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		0,
		KexSetupTransactionHandle);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(
			_(L"VxKex setup files could not be found. %s"),
			GetLastErrorAsString());

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	do {
		if (StringEqual(FindData.cFileName, L".") || StringEqual(FindData.cFileName, L"..")) {
			continue;
		}

		KexSetupFormatPath(SourcePath, L"%s\\%s", SourceDirectoryPath, FindData.cFileName);
		KexSetupFormatPath(TargetPath, L"%s\\%s", TargetDirectoryPath, FindData.cFileName);
		KexSetupSupersedeFile(SourcePath, TargetPath);
	} until (!FindNextFile(FindHandle, &FindData));

	ASSERT (GetLastError() == ERROR_NO_MORE_FILES);

	if (GetLastError() != ERROR_NO_MORE_FILES) {
		ErrorBoxF(_(L"Failed to move VxKex files. %s"), GetLastErrorAsString());
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	SafeFindClose(FindHandle);
}

VOID KexSetupCreateDirectory(
	IN	PCWSTR	DirectoryPath)
{
	BOOLEAN Success;

	ASSERT (DirectoryPath != NULL);

	Success = CreateDirectoryTransacted(
		NULL,
		DirectoryPath,
		NULL,
		KexSetupTransactionHandle);

	if (!Success) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			return;
		}

		ErrorBoxF(
			_(L"Failed to create directory: \"%s\". %s"),
			DirectoryPath,
			GetLastErrorAsString());

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}
}

BOOLEAN KexSetupDeleteFile(
	IN	PCWSTR	FilePath)
{
	BOOLEAN Success;

	Success = DeleteFileTransacted(FilePath, KexSetupTransactionHandle);

	if (!Success) {
		ULONG RandomIdentifier;
		WCHAR NewFileName[MAX_PATH];

		//
		// Failed to delete the file.
		// See if we've already tried to delete this file before (has .old_* extension).
		// If so, then we won't try to rename+delete it again, because that
		// can cause the .old_XXXX extensions to accumulate and slows down the
		// installer if you re-install many times in a row (e.g. for development).
		//

		if (StringSearchI(FilePath, L".old_")) {
			return FALSE;
		}

		//
		// Try to rename the file to something else and then schedule its deletion.
		//

		RandomIdentifier = GetTickCount();

		StringCchPrintf(
			NewFileName,
			ARRAYSIZE(NewFileName),
			L"%s.old_%04u", FilePath, RandomIdentifier);

		Success = MoveFileTransacted(
			FilePath,
			NewFileName,
			NULL,
			NULL,
			0,
			KexSetupTransactionHandle);

		if (Success) {
			FilePath = NewFileName;
		}

		Success = MoveFileTransacted(
			FilePath,
			NULL,
			NULL,
			NULL,
			MOVEFILE_DELAY_UNTIL_REBOOT,
			KexSetupTransactionHandle);

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

	FindHandle = FindFirstFileTransacted(
		DirectoryPathSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		0,
		KexSetupTransactionHandle);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	do {
		WCHAR FileFullPath[MAX_PATH];

		if (StringEqual(FindData.cFileName, L".") || StringEqual(FindData.cFileName, L"..")) {
			continue;
		}

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
			// Better not delete this.
			continue;
		}

		StringCchCopy(FileFullPath, ARRAYSIZE(FileFullPath), DirectoryPath);
		PathCchAppend(FileFullPath, ARRAYSIZE(FileFullPath), FindData.cFileName);

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			// Recurse into directory.
			KexSetupRemoveDirectoryRecursive(FileFullPath);
		} else {
			// Normal file - delete it
			KexSetupDeleteFile(FileFullPath);
		}
	} until (!FindNextFile(FindHandle, &FindData));

	ASSERT (GetLastError() == ERROR_NO_MORE_FILES);

	SafeFindClose(FindHandle);

	// Remove the directory itself, which should now hopefully be empty.
	return RemoveDirectoryTransacted(DirectoryPath, KexSetupTransactionHandle);
}

BOOLEAN KexSetupDeleteFilesBySpec(
	IN	PCWSTR	FileSpec)
{
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;
	BOOLEAN Success;

	FindHandle = FindFirstFileTransacted(
		FileSpec,
		FindExInfoBasic,
		&FindData,
		FindExSearchNameMatch,
		NULL,
		0,
		KexSetupTransactionHandle);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	Success = 0xff;

	do {
		WCHAR FileFullPath[MAX_PATH];

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
			// Better not delete this.
			Success = FALSE;
			continue;
		}

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}

		StringCchCopy(FileFullPath, ARRAYSIZE(FileFullPath), FileSpec);
		PathCchRemoveFileSpec(FileFullPath, ARRAYSIZE(FileFullPath));
		PathCchAppend(FileFullPath, ARRAYSIZE(FileFullPath), FindData.cFileName);

		// Normal file - delete it
		Success &= KexSetupDeleteFile(FileFullPath);
	} until (!FindNextFile(FindHandle, &FindData));

	ASSERT (GetLastError() == ERROR_NO_MORE_FILES);

	SafeFindClose(FindHandle);
	return Success;
}

//
// In version 1.1.6.x we changed logging to be disabled by default in new
// installations. As part of this change we changed the DisableLogging registry
// value to EnableLogging. This registry value exists in the VxKex system and
// user configuration keys
//
// We will read the DisableLogging value, invert it, and set EnableLogging as
// appropriate.
//
// If there's no DisableLogging value, we won't do anything. This means that
// users who never changed the logging setting will receive the new default,
// but users who have gone into KexCfg to change logging settings will keep
// their existing settings as expected.
//
VOID KexSetupChangeDisableLoggingToEnableLogging(
	IN	HKEY	KeyHandle)
{
	ULONG Win32Error;
	ULONG DisableLogging;

	Win32Error = RegReadI32(KeyHandle, NULL, L"DisableLogging", &DisableLogging);

	if (Win32Error == ERROR_SUCCESS) {
		RegDeleteValue(KeyHandle, L"DisableLogging");
		KexSetupRegWriteI32(KeyHandle, L"EnableLogging", !DisableLogging);
	}
}