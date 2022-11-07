///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllpath.c
//
// Abstract:
//
//     Contains routines for adding the Kex32/64 directory to the loader
//     search path, so that the process can load our extended DLLs. The
//     loader search path is a UNICODE_STRING structure, contained within
//     Peb->ProcessParameters->DllPath. This string contains semicolon-
//     separated Win32 paths, which is the same format as used in the %Path%
//     environment variable.
//
//     This is a relatively complex problem with a number of constraints.
//
//       1. We need to add KexDir\Kex32 or KexDir\Kex64 to the loader search
//          path, based on whether this is a 32 bit or 64 bit process.
//
//       2. The old length of the search path must be maintained, as the loader
//          caches the length value. Therefore, the search path cannot be
//          extended or contracted.
//
//       3. We must not disturb the presence or the order of existing search
//          path entries at all.
//
//     Fortunately, a number of factors make this possible in most cases:
//
//       1. The loader does not cache the actual data inside DllPath.Buffer,
//          so we can modify this buffer and influence the loader.
//
//   2 & 3. We can contract the length of the search path by removing
//          duplicate path entries. For example, %WinDir%\system32,
//          %WinDir%\system, and %WinDir% are all almost always duplicated
//          (since the system places them at the beginning, and then pastes
//          the %Path% environment variable after them, which contains
//          identical entries).
//
//          Sometimes, it is possible to collapse the length by removing
//          duplicate semicolons; however, this is much less likely to produce
//          a useful result and the current code implementation does not use
//          this method.
//
//          We can also pad the length of the search path out to the original
//          length by simply filling unused space with semicolons.
//
// Author:
//
//     vxiiduu (30-Oct-2022)
//
// Revision History:
//
//     vxiiduu              30-Oct-2022  Initial creation.
//     vxiiduu              31-Oct-2022  Fix bugs and finalize implementation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC NTSTATUS KexpShrinkDllPathLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			TargetLength);

STATIC NTSTATUS KexpPadDllPathToOriginalLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			OriginalLength);

NTSTATUS KexpAddKex3264ToDllPath(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PUNICODE_STRING DllPath;
	UNICODE_STRING NewDllPath;
	UNICODE_STRING Prepend;
	USHORT DllPathOriginalLength;

	DllPath = &NtCurrentPeb()->ProcessParameters->DllPath;
	DllPathOriginalLength = DllPath->Length;

	//
	// Create a scratch buffer for the data we will prepend to DllPath.
	//

	RtlInitEmptyUnicodeStringFromTeb(&Prepend);

	//
	// Build up the prepend string (KexDir + \Kex32; or \Kex64;)
	//

	Status = RtlAppendUnicodeStringToString(&Prepend, &KexData->KexDir);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (KexIs64BitBuild) {
		Status = RtlAppendUnicodeToString(&Prepend, L"\\Kex64;");
	} else {
		Status = RtlAppendUnicodeToString(&Prepend, L"\\Kex32;");
	}

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Call a helper function to shrink DllPath by *at least* the length
	// of our prepend string.
	//

	Status = KexpShrinkDllPathLength(DllPath, DllPath->Length - Prepend.Length);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Create a temporary buffer to hold the new DllPath.
	//

	NewDllPath.Length = 0;
	NewDllPath.MaximumLength = DllPath->Length + Prepend.Length;
	NewDllPath.Buffer = StackAlloc(WCHAR, KexRtlUnicodeStringBufferCch(&NewDllPath));

	//
	// Build the new DllPath.
	//

	Status = RtlAppendUnicodeStringToString(&NewDllPath, &Prepend);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = RtlAppendUnicodeStringToString(&NewDllPath, DllPath);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Copy new DllPath to old DllPath and finally pad to original length.
	//

	RtlCopyUnicodeString(DllPath, &NewDllPath);
	
	return KexpPadDllPathToOriginalLength(DllPath, DllPathOriginalLength);
} PROTECTED_FUNCTION_END

//
// Try to shrink the DllPath length to equal or less than "Length" by
// removing duplicate entries.
//
STATIC NTSTATUS KexpShrinkDllPathLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			TargetLength) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING DllPathAfterCurrentEntry;
	UNICODE_STRING CurrentPathEntry;
	UNICODE_STRING DuplicatePathEntry;
	UNICODE_STRING Semicolon;

	RtlInitConstantUnicodeString(&Semicolon, L";");
	DllPathAfterCurrentEntry = *DllPath;

	until (DllPath->Length <= TargetLength) {

		//
		// Fetch a path entry
		//

		Status = RtlFindCharInUnicodeString(
			0,
			&DllPathAfterCurrentEntry,
			&Semicolon,
			&CurrentPathEntry.Length);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogErrorEvent(
				L"RtlFindCharInUnicodeString returned an error\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
			return Status;
		}

		CurrentPathEntry.Length -= sizeof(WCHAR); // it includes the semicolon - get rid of it
		CurrentPathEntry.Buffer = DllPathAfterCurrentEntry.Buffer;
		CurrentPathEntry.MaximumLength = CurrentPathEntry.Length;
		KexRtlAdvanceUnicodeString(&DllPathAfterCurrentEntry, CurrentPathEntry.Length + sizeof(WCHAR));

		//
		// Look for one or more duplicate entries later in the path.
		//

		do {
			DuplicatePathEntry.Buffer = KexRtlFindUnicodeSubstring(
				&DllPathAfterCurrentEntry,
				&CurrentPathEntry,
				TRUE);

			if (DuplicatePathEntry.Buffer) {
				UNICODE_STRING AfterDuplicate;

				DuplicatePathEntry.Length = CurrentPathEntry.Length;
				DuplicatePathEntry.MaximumLength = (USHORT)
					((KexRtlEndOfUnicodeString(DllPath) - DuplicatePathEntry.Buffer) * sizeof(WCHAR));

				//
				// We want only full path entries, i.e. surrounded by semicolons, or at
				// the end of the string.
				//

				// the -1 is safe because we know DuplicatePathEntry is always ahead of DllPath
				if (DuplicatePathEntry.Buffer[-1] != ';') {
					break;
				}

				if (KexRtlEndOfUnicodeString(&DuplicatePathEntry) != KexRtlEndOfUnicodeString(DllPath)) {
					if (*(KexRtlEndOfUnicodeString(&DuplicatePathEntry)) != L';') {
						break;
					}
				}

				//
				// We need to cut this path entry out of the original DllPath and update the
				// length field accordingly. To do this, we will copy all characters from
				// the end of the duplicate path entry over top of the beginning of the
				// duplicate entry.
				//

				AfterDuplicate.Buffer = KexRtlEndOfUnicodeString(&DuplicatePathEntry);
				AfterDuplicate.Length = (USHORT) ((KexRtlEndOfUnicodeString(DllPath) - AfterDuplicate.Buffer) * sizeof(WCHAR));
				AfterDuplicate.MaximumLength = AfterDuplicate.Length;

				DllPath->Length -= DuplicatePathEntry.Length;
				RtlCopyUnicodeString(&DuplicatePathEntry, &AfterDuplicate);
			}
		} while (DuplicatePathEntry.Buffer);
	}

	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

STATIC NTSTATUS KexpPadDllPathToOriginalLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			OriginalLength) PROTECTED_FUNCTION
{
	PWCHAR Pointer;

	if (DllPath->Length > OriginalLength) {
		return STATUS_INTERNAL_ERROR;
	}

	//
	// Add semicolons to the end until DllPath reaches the correct length.
	//

	Pointer = KexRtlEndOfUnicodeString(DllPath);
	DllPath->Length = OriginalLength;

	while (Pointer < KexRtlEndOfUnicodeString(DllPath)) {
		*Pointer++ = L';';
	}

	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END