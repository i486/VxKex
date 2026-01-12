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
//   Keep in mind that the loader search path described above is only applicable
//   to process initialization AND when LdrLoadDll is called with NULL as the
//   first parameter (the DllPath parameter).
//
//   When DLLs are loaded using Kernel32/KernelBase functions such as
//   LoadLibrary, then the BASE dlls supply their own DllPath parameter to
//   LdrLoadDll which is passed down throughout the entire DLL load sequence
//   including the resolution of static imports. Therefore, the default loader
//   DllPath which is accessible through the PEB is not applicable after the
//   end of process initialization (except for the unlikely event of someone
//   calling LdrLoadDll with NULL as the first parameter).
//
// Author:
//
//     vxiiduu (30-Oct-2022)
//
// Revision History:
//
//     vxiiduu              30-Oct-2022  Initial creation.
//     vxiiduu              31-Oct-2022  Fix bugs and finalize implementation.
//     vxiiduu              18-Mar-2024  Fix a bug where forward slashes in the
//                                       DllPath could impede the ability of
//                                       KexpShrinkDllPathLength to do its job.
//     vxiiduu              05-Apr-2024  Correct more bugs.
//     vxiiduu              16-May-2025  Major rewrite of KexpShrinkDllPathLength
//                                       to enable it to detect duplicate path
//                                       elements at the end of the DLL path, stop
//                                       it from adding a spurious null character
//                                       to the end of the DLL path, and add
//                                       comments to clarify what the code is doing.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC INLINE NTSTATUS KexpShrinkDllPathLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			TargetLength);

STATIC INLINE NTSTATUS KexpPadDllPathToOriginalLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			OriginalLength);

STATIC INLINE VOID KexpNormalizeDllPathBackslashes(
	IN OUT	PUNICODE_STRING	DllPath);

STATIC INLINE VOID KexpReplaceDllPathEmbeddedNulls(
	IN OUT	PUNICODE_STRING	DllPath);

NTSTATUS KexpAddKex3264ToDllPath(
	VOID)
{
	NTSTATUS Status;
	PUNICODE_STRING DllPath;
	UNICODE_STRING NewDllPath;
	USHORT DllPathOriginalLength;

	ASSERT (VALID_UNICODE_STRING(&KexData->Kex3264DirPath));
	ASSERT (KexData->Kex3264DirPath.Length != 0);

	DllPath = &NtCurrentPeb()->ProcessParameters->DllPath;
	DllPathOriginalLength = DllPath->Length;

	ASSERT (VALID_UNICODE_STRING(DllPath));

	//
	// Remove any embedded nulls in the path. The presence of embedded null
	// characters will cause issues with logging, if it's enabled, and this
	// will result in an assertion failure on debug builds.
	//

	KexpReplaceDllPathEmbeddedNulls(DllPath);

	KexLogInformationEvent(
		L"Shrinking default loader DLL path\r\n\r\n"
		L"The original DLL path is: \"%wZ\"\r\n"
		L"The Kex3264Dir path is: \"%wZ\"",
		DllPath,
		&KexData->Kex3264DirPath);

	//
	// Convert all forward slashes in the DllPath to backslashes.
	// At least one real world case has been observed where a user's computer
	// had the Path environment variable contain forward slashes instead of
	// backslashes for some reason.
	//
	// Without normalizing the path separators, KexpShrinkDllPathLength would
	// fail. This would cause VxKex to not work.
	//

	KexpNormalizeDllPathBackslashes(DllPath);

	//
	// Call a helper function to shrink DllPath by *at least* the length
	// of our prepend string.
	//

	if (DllPath->Length <= KexData->Kex3264DirPath.Length) {
		// The DLL path is too small to add Kex3264DirPath to it.
		return STATUS_PATH_TOO_SHORT;
	}

	Status = KexpShrinkDllPathLength(DllPath, DllPath->Length - KexData->Kex3264DirPath.Length);
	ASSERT (NT_SUCCESS(Status));
	ASSERT (DllPath->Length < DllPathOriginalLength);
	ASSERT (DllPath->Length <= DllPathOriginalLength - KexData->Kex3264DirPath.Length);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Create a temporary buffer to hold the new DllPath.
	//

	NewDllPath.Length = 0;
	NewDllPath.MaximumLength = DllPath->Length + KexData->Kex3264DirPath.Length;
	NewDllPath.Buffer = StackAlloc(WCHAR, KexRtlUnicodeStringBufferCch(&NewDllPath));

	//
	// Build the new DllPath.
	//

	Status = RtlAppendUnicodeStringToString(&NewDllPath, &KexData->Kex3264DirPath);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = RtlAppendUnicodeStringToString(&NewDllPath, DllPath);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Copy new DllPath to old DllPath and finally pad to original length.
	//

	RtlCopyUnicodeString(DllPath, &NewDllPath);

	Status = KexpPadDllPathToOriginalLength(DllPath, DllPathOriginalLength);
	ASSERT (NT_SUCCESS(Status));

	ASSERT (!KexRtlUnicodeStringContainsEmbeddedNull(DllPath));

	return Status;
}

//
// Try to shrink the DllPath length to equal or less than "Length" by
// removing duplicate entries.
//
STATIC INLINE NTSTATUS KexpShrinkDllPathLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			TargetLength)
{
	NTSTATUS Status;
	UNICODE_STRING Semicolon;
	UNICODE_STRING DllPathAfterCurrentEntry;

	RtlInitConstantUnicodeString(&Semicolon, L";");

	//
	// Append a semicolon to DllPath to simplify the code which removes duplicate
	// path elements. This is always possible because of the null terminator at
	// the end of the DllPath string.
	//
	// In order to maintain compatibility with any badly written code elsewhere
	// which depends on this null terminator, we will have to remember to add the
	// null terminator back on.
	//

	Status = RtlAppendUnicodeStringToString(DllPath, &Semicolon);
	ASSERT (NT_SUCCESS(Status));

	DllPathAfterCurrentEntry = *DllPath;

	until (DllPath->Length <= TargetLength) {
		UNICODE_STRING CurrentEntry;
		UNICODE_STRING DllPathAfterDuplicateEntry;

		//
		// Fetch a path entry.
		//

		while (RtlPrefixUnicodeString(&Semicolon, &DllPathAfterCurrentEntry, FALSE)) {
			// Skip past semicolons.
			KexRtlAdvanceUnicodeString(&DllPathAfterCurrentEntry, sizeof(WCHAR));
		}

		Status = RtlFindCharInUnicodeString(
			0,
			&DllPathAfterCurrentEntry,
			&Semicolon,
			&CurrentEntry.Length);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		// Set up CurrentPathEntry so that it contains a single path entry with no
		// leading or trailing semicolons.
		CurrentEntry.Buffer = DllPathAfterCurrentEntry.Buffer;
		CurrentEntry.MaximumLength = CurrentEntry.Length;
		CurrentEntry.Length -= sizeof(WCHAR); // it includes the semicolon - get rid of it

		// Set up DllPathAfterCurrentEntry so that it begins right after CurrentPathEntry.
		KexRtlAdvanceUnicodeString(&DllPathAfterCurrentEntry, CurrentEntry.Length);
		ASSERT (DllPathAfterCurrentEntry.Buffer == KexRtlEndOfUnicodeString(&CurrentEntry));

		if (KexRtlUnicodeStringCch(&CurrentEntry) == 0) {
			// Empty path entry. Skip past it so that the code to remove duplicates
			// doesn't have to handle empty path entries.
			continue;
		}

		//
		// Find duplicates of the current path entry and remove them.
		//

		while (TRUE) {
			UNICODE_STRING StringToSearchFor;
			UNICODE_STRING DuplicateEntry;
			USHORT CchRemoved;

			//
			// Find another instance of CurrentEntry. We want the path entry to be
			// complete, so we will search for the current entry both preceded and
			// followed by a semicolon.
			//

			RtlInitEmptyUnicodeStringFromTeb(&StringToSearchFor);

			Status = STATUS_SUCCESS;

			Status |= RtlAppendUnicodeStringToString(&StringToSearchFor, &Semicolon);
			ASSERT (NT_SUCCESS(Status));
			
			Status |= RtlAppendUnicodeStringToString(&StringToSearchFor, &CurrentEntry);
			ASSERT (NT_SUCCESS(Status));

			Status |= RtlAppendUnicodeStringToString(&StringToSearchFor, &Semicolon);
			ASSERT (NT_SUCCESS(Status));

			if (Status != STATUS_SUCCESS) {
				// If any of the three appends failed (e.g. because CurrentEntry is a
				// super long string introduced through an environment variable or
				// something) then just skip this.
				break;
			}

			DuplicateEntry.Buffer = KexRtlFindUnicodeSubstring(
				&DllPathAfterCurrentEntry,
				&StringToSearchFor,
				TRUE);

			if (DuplicateEntry.Buffer == NULL) {
				break;
			}

			//
			// Set up Length and MaximumLength.
			//

			DuplicateEntry.Length = StringToSearchFor.Length - sizeof(WCHAR);
			DuplicateEntry.MaximumLength = StringToSearchFor.Length;

			Status = KexRtlSetUnicodeStringBufferEnd(
				&DuplicateEntry,
				KexRtlEndOfUnicodeString(DllPath));

			ASSERT (NT_SUCCESS(Status));
			ASSERT (!KexRtlUnicodeStringEndsWith(&DuplicateEntry, &Semicolon, FALSE));
			ASSERT (DuplicateEntry.Buffer != CurrentEntry.Buffer);
			ASSERT (KexRtlEndOfUnicodeStringBuffer(&DuplicateEntry) == KexRtlEndOfUnicodeString(DllPath));

			//
			// Set up DllPathAfterDuplicateEntry.
			//

			DllPathAfterDuplicateEntry = DuplicateEntry;
			DllPathAfterDuplicateEntry.Length = DllPathAfterDuplicateEntry.MaximumLength;
			KexRtlAdvanceUnicodeString(&DllPathAfterDuplicateEntry, DuplicateEntry.Length);

			ASSERT (DllPathAfterDuplicateEntry.Buffer == KexRtlEndOfUnicodeString(&DuplicateEntry));

			//
			// Copy DllPathAfterDuplicateEntry back on top of DuplicateEntry. This reduces
			// the length of the DLL path, so we will first record how many characters we
			// have shortened the DllPath string by so we can appropriately update length
			// values.
			//

			CchRemoved = KexRtlUnicodeStringCch(&DuplicateEntry);

			RtlCopyUnicodeString(&DuplicateEntry, &DllPathAfterDuplicateEntry);
			ASSERT (DuplicateEntry.Length == DllPathAfterDuplicateEntry.Length);

			DllPath->Length -= CchRemoved * sizeof(WCHAR);
			DllPathAfterCurrentEntry.Length -= CchRemoved * sizeof(WCHAR);

			// Remember that RtlCopyUnicodeString will null terminate the resulting buffer.
			// This is just a quick debug check to make sure the length calculation wasn't
			// fucked up somehow.
			ASSERT (wcslen(DllPath->Buffer) == KexRtlUnicodeStringCch(DllPath));
		}
	}

	//
	// Remove trailing semicolon. We added one at the beginning of this function.
	// This is so we have space to re-add the null terminator.
	//

	ASSERT (KexRtlUnicodeStringEndsWith(DllPath, &Semicolon, FALSE));
	DllPath->Length -= sizeof(WCHAR);

	//
	// Add the null terminator (which we removed at the beginning of the function)
	// back on to DllPath. Failure is not critical here.
	//

	Status = KexRtlNullTerminateUnicodeString(DllPath);
	ASSERT (NT_SUCCESS(Status));

	return STATUS_SUCCESS;
}

STATIC INLINE NTSTATUS KexpPadDllPathToOriginalLength(
	IN	PUNICODE_STRING	DllPath,
	IN	USHORT			OriginalLength)
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
		*Pointer++ = ';';
	}

	return STATUS_SUCCESS;
}

//
// Convert all slashes in the specified DllPath to backslashes.
//
STATIC INLINE VOID KexpNormalizeDllPathBackslashes(
	IN OUT	PUNICODE_STRING	DllPath)
{
	ULONG Index;
	ULONG DllPathCch;

	ASSUME (VALID_UNICODE_STRING(DllPath));

	DllPathCch = KexRtlUnicodeStringCch(DllPath);
	ASSUME (DllPathCch > 0);

	for (Index = 0; Index < DllPathCch; ++Index) {
		if (DllPath->Buffer[Index] == '/') {
			DllPath->Buffer[Index] = '\\';
		}
	}
}

//
// Replace embedded null \0 characters with semicolons. They can cause
// problems with logging.
//
STATIC INLINE VOID KexpReplaceDllPathEmbeddedNulls(
	IN OUT	PUNICODE_STRING	DllPath)
{
	ULONG Index;

	for (Index = 0; Index < KexRtlUnicodeStringCch(DllPath); ++Index) {
		if (DllPath->Buffer[Index] == '\0') {
			KexDebugCheckpoint();
			DllPath->Buffer[Index] = ';';
		}
	}

	ASSERT (!KexRtlUnicodeStringContainsEmbeddedNull(DllPath));
}