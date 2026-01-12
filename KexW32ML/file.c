#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI SupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile)
{
	BOOLEAN Success;

	Success = MoveFileEx(
		SourceFile,
		TargetFile,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);

	if (!Success) {
		ULONG RandomIdentifier;
		WCHAR ExistingTargetNewName[MAX_PATH];

		// try to rename the old file to <file>.old_xxxx
		RandomIdentifier = GetTickCount();

		StringCchPrintf(
			ExistingTargetNewName,
			ARRAYSIZE(ExistingTargetNewName),
			L"%s.old_%04u", TargetFile, RandomIdentifier);

		Success = MoveFileEx(
			TargetFile,
			ExistingTargetNewName,
			MOVEFILE_REPLACE_EXISTING);

		if (Success) {
			// schedule the old file to be deleted later
			MoveFileEx(
				ExistingTargetNewName,
				NULL,
				MOVEFILE_DELAY_UNTIL_REBOOT);

			// move the new file into the old position
			Success = MoveFileEx(
				SourceFile,
				TargetFile,
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
		}
	}

	return Success;
}