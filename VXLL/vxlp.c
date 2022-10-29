///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     opnclose.c
//
// Abstract:
//
//     Contains miscellaneous private routines.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              15-Oct-2022  Convert to v2 format.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

VXLSTATUS VxlpLogFileEntryToLogEntry(
	IN	VXLHANDLE			LogHandle,
	IN	PVXLLOGFILEENTRY	FileEntry,
	OUT	PPVXLLOGENTRY		Entry)
{
	PVXLLOGENTRY LogEntry;
	BOOLEAN Success;
	FILETIME LocalFileTime;

	ASSERT (LogHandle != NULL);
	ASSERT (FileEntry != NULL);
	ASSERT (Entry != NULL);

	*Entry = NULL;

	LogEntry = SafeAlloc(VXLLOGENTRY, 1);
	if (!LogEntry) {
		return VXL_OUT_OF_MEMORY;
	}

	//
	// Fill out VXLLOGENTRY structure using VXLLOGFILEENTRY structure.
	//

	// Convert time stored in the file to local time
	Success = FileTimeToLocalFileTime(&FileEntry->Time, &LocalFileTime);
	if (!Success) {
		// meh
		CopyMemory(&LocalFileTime, &FileEntry->Time, sizeof(LocalFileTime));
	}

	Success = FileTimeToSystemTime(&LocalFileTime, &LogEntry->Time);
	if (!Success) {
		// non-critical error - don't abort
		ZeroMemory(&LogEntry->Time, sizeof(LogEntry->Time));
	}

	LogEntry->Severity = FileEntry->Severity;
	LogEntry->SourceComponent = LogHandle->Header.SourceComponents[FileEntry->SourceComponentIndex];
	LogEntry->SourceLine = FileEntry->SourceLine;
	LogEntry->SourceFile = (PCWSTR) (((PBYTE) FileEntry) + sizeof(VXLLOGFILEENTRY));
	LogEntry->SourceFunction = LogEntry->SourceFile + FileEntry->SourceFileLength;
	LogEntry->TextHeader = LogEntry->SourceFunction + FileEntry->SourceFunctionLength;

	if (FileEntry->TextLength > 0) {
		LogEntry->Text = LogEntry->TextHeader + FileEntry->TextHeaderLength + 1;
		ASSERT (FileEntry->TextLength == wcslen(LogEntry->Text) + 1);
	} else {
		LogEntry->Text = L"";
	}

	ASSERT (FileEntry->SourceFileLength == wcslen(LogEntry->SourceFile) + 1);
	ASSERT (FileEntry->SourceFunctionLength == wcslen(LogEntry->SourceFunction) + 1);
	ASSERT (FileEntry->TextHeaderLength == wcslen(LogEntry->TextHeader) + 1);

	*Entry = LogEntry;
	return VXL_SUCCESS;
}

VXLSTATUS VxlpAcquireFileLock(
	IN	VXLHANDLE			LogHandle)
{
	ASSERT (LogHandle != NULL);
	EnterCriticalSection(&LogHandle->Lock);
	return VXL_SUCCESS;
}

VXLSTATUS VxlpReleaseFileLock(
	IN	VXLHANDLE			LogHandle)
{
	ASSERT (LogHandle != NULL);
	VxlpWriteLogFileHeader(LogHandle);
	LeaveCriticalSection(&LogHandle->Lock);
	return VXL_SUCCESS;
}

//
// This function reads and performs basic validation checks on the header of
// an opened VXL file. The header is placed in the buffer indicated by the
// LogHeader parameter.
//
// This function assumes that there is a header in the file. If you are creating
// a new log file, this function will always fail.
//
VXLSTATUS VxlpReadLogFileHeader(
	IN	VXLHANDLE			LogHandle)
{
	BOOLEAN Success;
	ULONG BytesRead;
	LARGE_INTEGER Zero;

	ASSERT (LogHandle != NULL);

	//
	// Move the file pointer to the beginning of the file, where the header
	// is located.
	//

	Zero.QuadPart = 0;
	Success = SetFilePointerEx(
		LogHandle->FileHandle,
		Zero,
		NULL,
		FILE_BEGIN);

	if (!Success) {
		return VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	//
	// Read the log file header.
	//

	Success = ReadFile(
		LogHandle->FileHandle,
		&LogHandle->Header,
		sizeof(VXLLOGFILEHEADER),
		&BytesRead,
		NULL);

	if (!Success) {
		return VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	if (BytesRead != sizeof(VXLLOGFILEHEADER)) {
		// Usually this means we have reached end of file. Which means
		// the file is not a valid VXL file e.g. an empty file, or some
		// other random kind of file.
		return VXL_FILE_INVALID;
	}

	//
	// Verify the log file header makes sense and is a valid VXL file.
	// First, check the magic value - which should be "VXLL"
	//
	if (!(LogHandle->Header.Magic[0] == 'V' &&
		  LogHandle->Header.Magic[1] == 'X' &&
		  LogHandle->Header.Magic[2] == 'L' &&
		  LogHandle->Header.Magic[3] == 'L')) {

		return VXL_FILE_INVALID;
	}

	//
	// From here on, any errors in the header means a corrupt file, not
	// an invalid one. The distinction is important because we can still
	// try to read a corrupt file, but we will refuse to read an invalid
	// file.
	//
	// Version number for VXLL starts at 1. So 0 is not valid.
	//
	if (LogHandle->Header.Version == 0) {
		if (!(LogHandle->OpenFlags & VXL_OPEN_IGNORE_CORRUPTION)) {
			return VXL_FILE_CORRUPT;
		}
	}

	// Check that the file version matches the library version.
	if (LogHandle->Header.Version != VXLL_VERSION) {
		return VXL_VERSION_MISMATCH;
	}

	return VXL_SUCCESS;
}

//
// Initialize the fields of a VXLLOGFILEHEADER structure for the first
// time.
//
VXLSTATUS VxlpInitializeLogFileHeader(
	OUT	PVXLLOGFILEHEADER	LogHeader,
	IN	PCWSTR				SourceApplication)
{
	HRESULT Result;

	ASSERT (LogHeader != NULL);
	ASSERT (SourceApplication != NULL);

	if (!LogHeader || !SourceApplication) {
		return VXL_INVALID_PARAMETER;
	}

	// initialize static fields
	ZeroMemory(LogHeader, sizeof(*LogHeader));
	LogHeader->Magic[0] = 'V';
	LogHeader->Magic[1] = 'X';
	LogHeader->Magic[2] = 'L';
	LogHeader->Magic[3] = 'L';
	LogHeader->Version = VXLL_VERSION;
	LogHeader->Dirty = TRUE;

	// initialize string fields
	Result = StringCchCopy(LogHeader->SourceApplication, ARRAYSIZE(LogHeader->SourceApplication), SourceApplication);
	if (FAILED(Result)) {
		return VXL_INVALID_PARAMETER_LENGTH;
	}

	return VXL_SUCCESS;
}

//
// Write a log file header to the beginning of a log file.
// This function updates the log header in the file using the in-memory
// copy contained within the context structure. No consistency checking
// is done to the header in memory, so don't pass any garbage values.
//
VXLSTATUS VxlpWriteLogFileHeader(
	IN	VXLHANDLE			LogHandle)
{
	BOOLEAN Success;
	LARGE_INTEGER Zero;
	ULONG BytesWritten;

	ASSERT (LogHandle != NULL);
	ASSERT (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY);

	//
	// Move file pointer to beginning of file
	//

	Zero.QuadPart = 0;
	Success = SetFilePointerEx(
		LogHandle->FileHandle,
		Zero,
		NULL,
		FILE_BEGIN);

	if (!Success) {
		return VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	//
	// Overwrite whatever is there with the updated header.
	//

	Success = WriteFile(
		LogHandle->FileHandle,
		&LogHandle->Header,
		sizeof(VXLLOGFILEHEADER),
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != sizeof(VXLLOGFILEHEADER)) {
		return VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	return VXL_SUCCESS;
}

//
// Find the index into the header SourceComponents array based on a source
// component string. Place the index into the SourceComponentIndex pointer
// parameter. If SourceComponentIndex is NULL, update the provided context.
//
VXLSTATUS VxlpFindSourceComponentIndex(
	IN OUT	VXLHANDLE	LogHandle,
	IN		PCWSTR		SourceComponent,
	OUT		PUSHORT		SourceComponentIndex)
{
	USHORT Index;

	ASSERT (LogHandle != NULL);
	ASSERT (SourceComponent != NULL);
	ASSERT (SourceComponentIndex != NULL);

	*SourceComponentIndex = 0;

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header.SourceComponents); ++Index) {
		if (StringEqual(SourceComponent, LogHandle->Header.SourceComponents[Index])) {
			*SourceComponentIndex = Index;
			return VXL_SUCCESS;
		}
	}

	return VXL_FILE_SOURCE_COMPONENT_NOT_FOUND;
}

VXLSTATUS VxlpFindOrCreateSourceComponentIndex(
	IN OUT	VXLHANDLE	LogHandle,
	IN		PCWSTR		SourceComponent,
	OUT		PUSHORT		SourceComponentIndex)
{
	VXLSTATUS Status;
	USHORT Index;

	ASSERT (LogHandle != NULL);
	ASSERT (SourceComponent != NULL);
	ASSERT (SourceComponentIndex != NULL);

	*SourceComponentIndex = 0;

	Status = VxlpFindSourceComponentIndex(LogHandle, SourceComponent, SourceComponentIndex);
	if (VXL_FAILED(Status)) {
		if (Status != VXL_FILE_SOURCE_COMPONENT_NOT_FOUND) {
			return Status;
		}
	} else {
		return Status;
	}

	//
	// Source component was not found. We need to allocate a new one.
	//
	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header.SourceComponents); ++Index) {
		if (LogHandle->Header.SourceComponents[Index][0] == '\0') {
			HRESULT Result;

			Result = StringCchCopy(LogHandle->Header.SourceComponents[Index],
								   ARRAYSIZE(LogHandle->Header.SourceComponents[Index]),
								   SourceComponent);

			if (FAILED(Result)) {
				return VXL_INVALID_PARAMETER_LENGTH;
			}

			*SourceComponentIndex = Index;
			return VXL_SUCCESS;
		}
	}

	return VXL_FILE_SOURCE_COMPONENT_LIMIT_EXCEEDED;
}

VXLSTATUS VxlpValidateFileOpenFlags(
	IN	ULONG		Flags)
{
	if (Flags & VXL_OPEN_READ_ONLY &&
		Flags & VXL_OPEN_WRITE_ONLY) {
		// Cannot open for reading and writing at the same time.
		return VXL_INVALID_PARAMETER_COMBINATION;
	}

	if (Flags & VXL_OPEN_READ_ONLY) {
		// Any write-specific parameters are invalid.
		if (Flags & VXL_OPEN_CREATE_IF_NOT_EXISTS ||
			Flags & VXL_OPEN_APPEND_IF_EXISTS ||
			Flags & VXL_OPEN_OVERWRITE_IF_EXISTS) {

			return VXL_INVALID_PARAMETER_COMBINATION;
		}
	} else if (Flags & VXL_OPEN_WRITE_ONLY) {
		// Only one "if exists" action may be specified.
		if (Flags & VXL_OPEN_APPEND_IF_EXISTS &&
			Flags & VXL_OPEN_OVERWRITE_IF_EXISTS) {

			return VXL_INVALID_PARAMETER_COMBINATION;
		}

		// If we don't create a file when it doesn't exist, caller must
		// specify what happens if it does exist.
		if (!(Flags & VXL_OPEN_CREATE_IF_NOT_EXISTS)) {
			if (!(Flags & VXL_OPEN_APPEND_IF_EXISTS) &&
				!(Flags & VXL_OPEN_OVERWRITE_IF_EXISTS)) {
				
				return VXL_INVALID_PARAMETER_COMBINATION;
			}
		}
	} else {
		// Must open for either reading or writing.
		return VXL_INVALID_PARAMETER;
	}

	return VXL_SUCCESS;
}

VXLSTATUS VxlpTranslateFileOpenFlags(
	IN	ULONG		VxlFlags,
	OUT	PULONG		DesiredAccess,
	OUT	PULONG		FlagsAndAttributes,
	OUT	PULONG		CreationDisposition)
{
	ASSERT (VxlFlags != 0);
	ASSERT (DesiredAccess != NULL);
	ASSERT (FlagsAndAttributes != NULL);
	ASSERT (CreationDisposition != NULL);

	if (VxlFlags & VXL_OPEN_READ_ONLY) {
		*DesiredAccess = GENERIC_READ;
		*FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
		*CreationDisposition = OPEN_EXISTING;
	} else {
		*DesiredAccess = GENERIC_READ | GENERIC_WRITE;
		*FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

		if (VxlFlags & VXL_OPEN_CREATE_IF_NOT_EXISTS) {
			if (VxlFlags & VXL_OPEN_APPEND_IF_EXISTS) {
				*CreationDisposition = OPEN_ALWAYS;
			} else if (VxlFlags & VXL_OPEN_OVERWRITE_IF_EXISTS) {
				*CreationDisposition = CREATE_ALWAYS;
			} else {
				*CreationDisposition = CREATE_NEW;
			}
		} else {
			if (VxlFlags & VXL_OPEN_APPEND_IF_EXISTS) {
				*CreationDisposition = OPEN_EXISTING;
			} else if (VxlFlags & VXL_OPEN_OVERWRITE_IF_EXISTS) {
				*CreationDisposition = TRUNCATE_EXISTING;
			}
		}
	}

	return VXL_SUCCESS;
}

//
// Return TRUE if the file specified by FileHandle is 0-bytes in
// size. Or FALSE if the file has some contents.
//
BOOLEAN VxlpFileIsEmpty(
	IN	HANDLE	FileHandle)
{
	LARGE_INTEGER FileSize;

	ASSERT (FileHandle != NULL);
	ASSERT (FileHandle != INVALID_HANDLE_VALUE);

	if (!GetFileSizeEx(FileHandle, &FileSize)) {
		return TRUE;
	} else {
		if (FileSize.QuadPart == 0) {
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

// VXLL can be compiled both as a .lib and as a .dll, so we need this
// preprocessor condition
#ifdef KEX_TARGET_TYPE_DLL
BOOL WINAPI DllMain(
	IN	HMODULE		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(DllBase);
	}

	return TRUE;
}
#endif