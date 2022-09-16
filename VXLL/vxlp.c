#include <Windows.h>
#include <Shlwapi.h>
#include <strsafe.h>
#include "VXLL.h"
#include "VXLP.h"

BOOLEAN VxlpSeverityFiltersMatchSeverity(
	IN	ULONG				SeverityFilters,
	IN	VXLSEVERITY			Severity)
{
	if (SeverityFilters & VXL_FILTER_SEVERITY_CRITICAL && Severity == LogSeverityCritical) return TRUE;
	if (SeverityFilters & VXL_FILTER_SEVERITY_ERROR && Severity == LogSeverityError) return TRUE;
	if (SeverityFilters & VXL_FILTER_SEVERITY_WARNING && Severity == LogSeverityWarning) return TRUE;
	if (SeverityFilters & VXL_FILTER_SEVERITY_INFORMATION && Severity == LogSeverityInformation) return TRUE;
	if (SeverityFilters & VXL_FILTER_SEVERITY_DETAIL && Severity == LogSeverityDetail) return TRUE;
	if (SeverityFilters & VXL_FILTER_SEVERITY_DEBUG && Severity == LogSeverityDebug) return TRUE;

	return FALSE;
}

//
// Conditions:
//   - the file pointer of LogHandle->FileHandle must be immediately after the
//     relevant VXLLOGFILEENTRY structure in the file.
//
VXLSTATUS VxlpLogFileEntryToLogEntry(
	IN	VXLHANDLE			LogHandle,
	IN	PVXLLOGFILEENTRY	FileEntry,
	OUT	PPVXLLOGENTRY		Entry)
{
	PVXLLOGENTRY LogEntry;
	ULONG BytesRead;
	BOOLEAN Success;

	if (!LogHandle || !Entry) {
		return VXL_INVALID_PARAMETER;
	}

	*Entry = NULL;
	LogEntry = (PVXLLOGENTRY) HeapAlloc(GetProcessHeap(), 0, sizeof(*Entry));

	if (!LogEntry) {
		return VXL_OUT_OF_MEMORY;
	}

	//
	// Fill out VXLLOGENTRY structure using VXLLOGFILEENTRY structure
	//

	Success = FileTimeToSystemTime(&FileEntry->Time, &LogEntry->Time);
	if (!Success) {
		// non-critical error - don't abort
		ZeroMemory(&LogEntry->Time, sizeof(LogEntry->Time));
	}

	LogEntry->Severity = FileEntry->Severity;
	LogEntry->SourceComponent = LogHandle->Header.SourceComponents[LogHandle->SourceComponentIndex];
	LogEntry->SourceLine = FileEntry->SourceLine;
	LogEntry->SourceFile = (PWSTR) HeapAlloc(GetProcessHeap(), 0, (FileEntry->SourceFileLength + 1) * sizeof(WCHAR));
	LogEntry->SourceFunction = (PWSTR) HeapAlloc(GetProcessHeap(), 0, (FileEntry->SourceFunctionLength + 1) * sizeof(WCHAR));
	LogEntry->Text = (PWSTR) HeapAlloc(GetProcessHeap(), 0, (FileEntry->TextLength + 1) * sizeof(WCHAR));

	if (!LogEntry->SourceFile || !LogEntry->SourceFunction || !LogEntry->Text) {
		VxlFreeLogEntry(Entry);
		return VXL_OUT_OF_MEMORY;
	}

	//
	// Read source file name
	//
	Success = ReadFile(
		LogHandle->FileHandle,
		(PVOID) LogEntry->SourceFile,
		FileEntry->SourceFileLength * sizeof(WCHAR),
		&BytesRead,
		NULL);
	((PWSTR) LogEntry->SourceFile)[BytesRead] = '\0';
			
	//
	// Read source function name
	//
	Success = ReadFile(
		LogHandle->FileHandle,
		(PVOID) LogEntry->SourceFunction,
		FileEntry->SourceFunctionLength * sizeof(WCHAR),
		&BytesRead,
		NULL);
	((PWSTR) LogEntry->SourceFunction)[BytesRead] = '\0';

	//
	// Read the log entry text
	//
	Success = ReadFile(
		LogHandle->FileHandle,
		(PVOID) LogEntry->Text,
		FileEntry->TextLength * sizeof(WCHAR),
		&BytesRead,
		NULL);
	((PWSTR) LogEntry->Text)[BytesRead] = '\0';

	*Entry = LogEntry;
	return VXL_SUCCESS;
}

VXLSTATUS VxlpAcquireFileLock(
	IN	VXLHANDLE			LogHandle)
{
	VXLSTATUS Status;
	BOOLEAN Success;
	OVERLAPPED Overlapped;

	EnterCriticalSection(&LogHandle->Lock);

	ZeroMemory(&Overlapped, sizeof(Overlapped));
	Overlapped.Offset = 0;
	Overlapped.OffsetHigh = 0;
	Success = LockFileEx(
		LogHandle->FileHandle,
		LOCKFILE_EXCLUSIVE_LOCK,
		0,
		sizeof(VXLLOGFILEHEADER),
		0,
		&Overlapped);

	if (!Success) {
		LeaveCriticalSection(&LogHandle->Lock);
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_SYNCHRONIZATION_ERROR);
	}

	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		//
		// Update the log header in the context structure so that it
		// matches what's in the file.
		//
		Status = VxlpReadLogFileHeader(LogHandle);
		if (VXL_FAILED(Status)) {
			VxlpReleaseFileLock(LogHandle);
			return Status;
		}
	}

	return VXL_SUCCESS;
}

VXLSTATUS VxlpReleaseFileLock(
	IN	VXLHANDLE			LogHandle)
{
	BOOLEAN Success;

	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		//
		// Update the log file header with the context structure.
		// Don't care if it fails - we tried.
		//
		VxlpWriteLogFileHeader(LogHandle);
	}

	Success = UnlockFile(LogHandle->FileHandle, 0, 0, sizeof(VXLLOGFILEHEADER), 0);
	if (!Success) {
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_SYNCHRONIZATION_ERROR);
	}

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

	// param validation
	if (!LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

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
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
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
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
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
		return VXL_FILE_CORRUPT;
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

	// initialize static fields
	ZeroMemory(LogHeader, sizeof(*LogHeader));
	LogHeader->Magic[0] = 'V';
	LogHeader->Magic[1] = 'X';
	LogHeader->Magic[2] = 'L';
	LogHeader->Magic[3] = 'L';
	LogHeader->Version = 1;
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

	// validation
	if (!LogHandle) {
		return VXL_INVALID_PARAMETER;
	}

	if (LogHandle->OpenFlags & VXL_OPEN_READ_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

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
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
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
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
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
	OUT		PUSHORT		SourceComponentIndex OPTIONAL)
{
	USHORT Index;

	if (!SourceComponentIndex) {
		SourceComponentIndex = &LogHandle->SourceComponentIndex;
	}

	*SourceComponentIndex = 0;

	// param validation
	if (!SourceComponent) {
		return VXL_INVALID_PARAMETER;
	}

	for (Index = 0; Index < ARRAYSIZE(LogHandle->Header.SourceComponents); ++Index) {
		if (!StrCmpI(SourceComponent, LogHandle->Header.SourceComponents[Index])) {
			*SourceComponentIndex = Index;
			return VXL_SUCCESS;
		}
	}

	return VXL_FILE_SOURCE_COMPONENT_NOT_FOUND;
}

VXLSTATUS VxlpFindOrCreateSourceComponentIndex(
	IN OUT	VXLHANDLE	LogHandle,
	IN		PCWSTR		SourceComponent,
	OUT		PUSHORT		SourceComponentIndex OPTIONAL)
{
	VXLSTATUS Status;
	USHORT Index;

	//
	// No param validation because VxlpFindSourceComponentIndex already does that.
	//

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
	if (!VxlFlags || !DesiredAccess || !FlagsAndAttributes || !CreationDisposition) {
		return VXL_INVALID_PARAMETER;
	}

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

VXLSTATUS VxlpTranslateWin32Error(
	IN	ULONG		Win32Error,
	IN	VXLSTATUS	Default OPTIONAL)
{
	switch (GetLastError()) {
	case ERROR_SUCCESS:
		return VXL_SUCCESS;
	case ERROR_INVALID_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_PARAMETER:
	case ERROR_INVALID_FLAGS:
		return VXL_INVALID_PARAMETER;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_DRIVE:
		return VXL_FILE_NOT_FOUND;
	case ERROR_FILE_EXISTS:
		return VXL_FILE_ALREADY_EXISTS;
	case ERROR_ACCESS_DENIED:
		return VXL_INSUFFICIENT_CREDENTIALS;
	case ERROR_NOT_ENOUGH_MEMORY:
		return VXL_OUT_OF_MEMORY;
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return VXL_FILE_ALREADY_OPENED;
	case ERROR_OPEN_FAILED:
	case ERROR_CANNOT_MAKE:
		return VXL_FILE_CANNOT_OPEN;
	case ERROR_WRITE_PROTECT:
	case ERROR_READ_FAULT:
	case ERROR_WRITE_FAULT:
		return VXL_FILE_IO_ERROR;
	case ERROR_INSUFFICIENT_BUFFER:
		return VXL_INSUFFICIENT_BUFFER;
	default:
		if (Default) {
			return Default;
		} else {
			return VXL_FAILURE;
		}
	}
}