#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI SupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	BOOLEAN Success;
	HANDLE ExistingTransaction;

	ExistingTransaction = RtlGetCurrentTransaction();

	if (ExistingTransaction) {
		RtlSetCurrentTransaction(NULL);
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

	if (ExistingTransaction) {
		RtlSetCurrentTransaction(ExistingTransaction);
	}

	return Success;
}