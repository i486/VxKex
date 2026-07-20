#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI FileExists(
	IN	PCWSTR	Path)
{
	HANDLE FindHandle;
	WIN32_FIND_DATA FindData;

	FindHandle = FindFirstFile(Path, &FindData);

	if (FindHandle == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	SafeFindClose(FindHandle);
	return TRUE;
}

//
// This function is slow and inefficient and doesn't really work properly
// for all edge cases such as relative input paths
// Hence why it is left as static
// It is only really meant for KexSetup to use it through SupersedeFile
//
STATIC BOOLEAN CreateDirectoryHierarchy(
	IN	PCWSTR	DirectoryPath,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	ULONG Iterations;
	WCHAR CreateDirectoryPath[MAX_PATH];

	ASSERT (!PathIsRelative(DirectoryPath));

	Iterations = 0;

DoAgain:
	StringCchCopy(CreateDirectoryPath, ARRAYSIZE(CreateDirectoryPath), DirectoryPath);

	until (PathCchIsRoot(CreateDirectoryPath)) {
		BOOL Success;

		if (TransactionHandle) {
			Success = CreateDirectoryTransacted(
				NULL,
				CreateDirectoryPath,
				NULL,
				TransactionHandle);
		} else {
			Success = CreateDirectory(
				CreateDirectoryPath,
				NULL);
		}

		if (Success) {
			break;
		}

		switch (GetLastError()) {
		case ERROR_ALREADY_EXISTS:
			goto OutOfLoop;
		case ERROR_PATH_NOT_FOUND:
			break;
		default:
			ASSERT (FALSE);
			return FALSE;
		}

		PathCchRemoveFileSpec(CreateDirectoryPath, ARRAYSIZE(CreateDirectoryPath));
	}

OutOfLoop:
	if (FileExists(DirectoryPath)) {
		if (PathIsDirectory(DirectoryPath)) {
			return TRUE;
		} else {
			SetLastError(ERROR_DIRECTORY);
			return FALSE;
		}
	} else {
		++Iterations;

		if (Iterations > 100) {
			return FALSE;
		}

		goto DoAgain;
	}
}

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI SupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	BOOLEAN Success;

	if (PathIsDirectory(SourceFile)) {
		HANDLE FindHandle;
		WCHAR FindPath[MAX_PATH];
		WIN32_FIND_DATA FindData;
		ULONG LastError;

		//
		// This code was added to work around what I believe is a bug or deficiency
		// in Transactional NTFS.
		//
		// 1. KexSetup creates a file somewhere under a transaction.
		// 2. The transaction gets either committed or rolled back and KexSetup exits.
		// 3. Everything is OK.
		// 4. Some app memory-maps one of the files that USED to be transacted, for
		//    example, a DLL or BDI.
		// 5. For some reason this causes the TxF behavior to resurface and all the
		//    directory components leading up to that file become non-renameable.
		//
		// Even though as far as I can tell the transaction is no longer active, some
		// remnant of the transaction remains until the computer is restarted.
		//
		// I weighed up whether to do this or to just remove transaction support from
		// KexSetup altogether but ended up doing this. Basically, transactions are
		// important so that VxKex is either "fully installed" or "not installed"
		// without any weird half-measures that can break the user's system due to
		// e.g. IFEO entries remaining while VxKex is in an unusable state.
		//
		// What we'll do instead of renaming directories is we'll just recurse into
		// the source directory and call SupersedeFile on all the child items.
		//
		// 11-Jul-2026: This code path was changed so that it also works for when the
		// source and target are on different drives.
		//

		// create a path that is %SourceFile%\*
		StringCchCopy(FindPath, ARRAYSIZE(FindPath), SourceFile);
		PathCchAppend(FindPath, ARRAYSIZE(FindPath), L"*");

		if (TransactionHandle) {
			FindHandle = FindFirstFileTransacted(
				FindPath,
				FindExInfoBasic,
				&FindData,
				FindExSearchNameMatch,
				NULL,
				0,
				TransactionHandle);
		} else {
			FindHandle = FindFirstFileEx(
				FindPath,
				FindExInfoBasic,
				&FindData,
				FindExSearchNameMatch,
				NULL,
				0);
		}

		if (FindHandle == INVALID_HANDLE_VALUE) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) {
				// No files in the directory, so everything is OK
				return TRUE;
			}

			return FALSE;
		}

		do {
			WCHAR SourcePath[MAX_PATH];
			WCHAR TargetPath[MAX_PATH];

			if (StringEqual(FindData.cFileName, L".") || StringEqual(FindData.cFileName, L"..")) {
				continue;
			}

			StringCchCopy(SourcePath, ARRAYSIZE(SourcePath), SourceFile);
			PathCchAppend(SourcePath, ARRAYSIZE(SourcePath), FindData.cFileName);

			StringCchCopy(TargetPath, ARRAYSIZE(TargetPath), TargetFile);
			PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), FindData.cFileName);

			if (!FileExists(TargetPath)) {
				WCHAR TargetDirectoryPath[MAX_PATH];

				StringCchCopy(TargetDirectoryPath, ARRAYSIZE(TargetDirectoryPath), TargetPath);
				PathCchRemoveFileSpec(TargetDirectoryPath, ARRAYSIZE(TargetDirectoryPath));
				CreateDirectoryHierarchy(TargetDirectoryPath, TransactionHandle);
			}
			
			Success = SupersedeFile(SourcePath, TargetPath, TransactionHandle);

			if (!Success) {
				break;
			}
		} until (!FindNextFile(FindHandle, &FindData));

		LastError = GetLastError();
		SafeFindClose(FindHandle);
		SetLastError(LastError);

		if (LastError != ERROR_NO_MORE_FILES) {
			return FALSE;
		}

		return TRUE;
	}

	if (TransactionHandle) {
		Success = MoveFileTransacted(
			SourceFile,
			TargetFile,
			NULL,
			NULL,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED,
			TransactionHandle);
	} else {
		Success = MoveFileEx(
			SourceFile,
			TargetFile,
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
	}

	if (!Success) {
		ULONG RandomIdentifier;
		WCHAR ExistingTargetNewName[MAX_PATH];

		// try to rename the old file to <file>.old_xxxx
		RandomIdentifier = GetTickCount();

		StringCchPrintf(
			ExistingTargetNewName,
			ARRAYSIZE(ExistingTargetNewName),
			L"%s.old_%04u", TargetFile, RandomIdentifier);

		if (TransactionHandle) {
			Success = MoveFileTransacted(
				TargetFile,
				ExistingTargetNewName,
				NULL,
				NULL,
				0,
				TransactionHandle);
		} else {
			Success = MoveFile(TargetFile, ExistingTargetNewName);
		}

		if (Success) {
			// schedule the old file to be deleted later

			if (TransactionHandle) {
				MoveFileTransacted(
					ExistingTargetNewName,
					NULL,
					NULL,
					NULL,
					MOVEFILE_DELAY_UNTIL_REBOOT,
					TransactionHandle);
			} else {
				MoveFileEx(
					ExistingTargetNewName,
					NULL,
					MOVEFILE_DELAY_UNTIL_REBOOT);
			}

			// move the new file into the old position
			if (TransactionHandle) {
				Success = MoveFileTransacted(
					SourceFile,
					TargetFile,
					NULL,
					NULL,
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED,
					TransactionHandle);
			} else {
				Success = MoveFileEx(
					SourceFile,
					TargetFile,
					MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
			}
		}
	}

	return Success;
}