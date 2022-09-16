#include <Windows.h>
#include <stdarg.h>
#include <strsafe.h>

// Some bullshit warning about a "deprecated" function in intrin.h
// get rid of the stupid warning
#pragma warning(push)
#pragma warning(disable:4995)
#include <intrin.h>
#pragma warning(pop)

#include "VXLL.h"
#include "VXLP.h"

VXLSTATUS VXLAPI VxlLogEx(
	IN		VXLHANDLE		LogHandle,
	IN		PCWSTR			SourceFile OPTIONAL,
	IN		ULONG			SourceLine,
	IN		PCWSTR			SourceFunction OPTIONAL,
	IN		VXLSEVERITY		Severity,
	IN		PCWSTR			Format, ...)
{
	VXLSTATUS Status;
	BOOLEAN Success;
	HRESULT Result;
	WCHAR SourceFunctionUnspecified[20];
	VXLLOGFILEENTRY LogFileEntry;
	PWSTR Text;
	LARGE_INTEGER Zero;
	LARGE_INTEGER OldEndOfFile;
	ULONG BytesWritten;
	va_list ArgList;

	//
	// Check if we are even allowed to write to the log.
	//
	if (LogHandle->OpenFlags & VXL_OPEN_READ_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	//
	// param validation
	//
	if (!LogHandle || !Format) {
		return VXL_INVALID_PARAMETER;
	}

	if (Severity >= LogSeverityMaximumValue) {
		return VXL_NOT_IMPLEMENTED;
	}

	if (!SourceFile) {
		SourceFile = L"(unspecified)";
	}

	if (!SourceFunction) {
		// look up the return address of this function call. Developer
		// will need to look in his symbol file for what function that
		// translates to.
		Result = StringCchPrintf(SourceFunctionUnspecified, ARRAYSIZE(SourceFunctionUnspecified),
								 L"0x%p", _ReturnAddress());

		if (SUCCEEDED(Result)) {
			SourceFunction = SourceFunctionUnspecified;
		} else {
			SourceFunction = L"(unknown)";
		}
	}

	if (wcslen(SourceFile) > USHRT_MAX || wcslen(SourceFunction) > USHRT_MAX) {
		return VXL_INVALID_PARAMETER_LENGTH;
	}

	//
	// Format the string to be placed in the log file and
	// fill out the members of the LogFileEntry structure.
	//
	va_start(ArgList, Format);
	LogFileEntry.TextLength = _vscwprintf(Format, ArgList);
	Text = (PWSTR) HeapAlloc(GetProcessHeap(), 0, (LogFileEntry.TextLength + 1) * sizeof(WCHAR));
	
	if (!Text) {
		va_end(ArgList);
		return VXL_OUT_OF_MEMORY;
	}

	Result = StringCchVPrintf(Text, (LogFileEntry.TextLength + 1), Format, ArgList);
	va_end(ArgList);

	if (FAILED(Result)) {
		Status = VXL_INSUFFICIENT_BUFFER;
		goto Error;
	}

	GetSystemTimeAsFileTime(&LogFileEntry.Time);
	LogFileEntry.Severity					= Severity;
	LogFileEntry.SourceFileLength			= (USHORT) wcslen(SourceFile);
	LogFileEntry.SourceFunctionLength		= (USHORT) wcslen(SourceFunction);
	LogFileEntry.SourceLine					= SourceLine;
	LogFileEntry.SourceComponentIndex		= LogHandle->SourceComponentIndex;

	Status = VxlpAcquireFileLock(LogHandle);
	if (VXL_FAILED(Status)) {
		goto Error;
	}

	//
	// If this is a critical log entry, cache it in the context structure.
	//
	if (Severity == LogSeverityCritical) {
		PVXLLOGENTRY LastCriticalEntry = &LogHandle->LastCriticalEntry;
		
		//
		// Free old entry (if present)
		//
		HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->SourceFile);
		HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->SourceFunction);
		HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->Text);

		//
		// Create new entry
		//
		Success = FileTimeToSystemTime(&LogFileEntry.Time, &LastCriticalEntry->Time);
		if (!Success) {
			ZeroMemory(&LastCriticalEntry->Time, sizeof(LastCriticalEntry->Time));
		}

		LastCriticalEntry->Severity				= LogFileEntry.Severity;
		LastCriticalEntry->SourceComponent		= LogHandle->Header.SourceComponents[LogFileEntry.SourceComponentIndex];
		LastCriticalEntry->SourceLine			= LogFileEntry.SourceLine;
		LastCriticalEntry->SourceFile			= (PWSTR) HeapAlloc(GetProcessHeap(), 0, (LogFileEntry.SourceFileLength + 1) * sizeof(WCHAR));
		LastCriticalEntry->SourceFunction		= (PWSTR) HeapAlloc(GetProcessHeap(), 0, (LogFileEntry.SourceFunctionLength + 1) * sizeof(WCHAR));
		LastCriticalEntry->Text					= (PWSTR) HeapAlloc(GetProcessHeap(), 0, (LogFileEntry.TextLength + 1) * sizeof(WCHAR));

		if (!LastCriticalEntry->SourceFile || !LastCriticalEntry->SourceFunction || !LastCriticalEntry->Text) {
			// Failure
			HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->SourceFile);
			HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->SourceFunction);
			HeapFree(GetProcessHeap(), 0, (PVOID) LastCriticalEntry->Text);
			ZeroMemory(LastCriticalEntry, sizeof(*LastCriticalEntry));
		} else {
			StringCchCopy((PWSTR) LastCriticalEntry->SourceFile, LogFileEntry.SourceFileLength + 1, SourceFile);
			StringCchCopy((PWSTR) LastCriticalEntry->SourceFunction, LogFileEntry.SourceFunctionLength + 1, SourceFunction);
			StringCchCopy((PWSTR) LastCriticalEntry->Text, LogFileEntry.TextLength + 1, Text);
		}
	}

	//
	// Write the log entry to the end of the log file.
	//
	// Data is written to the end of the log file in the following order:
	//   1. VXLLOGFILEENTRY structure
	//   2. Source file (as raw string, not null terminated)
	//   3. Source function (as raw string, not null terminated)
	//   4. Log text (as raw string, not null terminated)
	//

	// Move file pointer to end of file
	Zero.QuadPart = 0;
	Success = SetFilePointerEx(LogHandle->FileHandle, Zero, &OldEndOfFile, FILE_END);
	if (!Success) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	// Write VXLLOGFILEENTRY structure
	Success = WriteFile(
		LogHandle->FileHandle,
		&LogFileEntry,
		sizeof(LogFileEntry),
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != sizeof(LogFileEntry)) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	// Write source file name
	Success = WriteFile(
		LogHandle->FileHandle,
		SourceFile,
		LogFileEntry.SourceFileLength * sizeof(WCHAR),
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != LogFileEntry.SourceFileLength * sizeof(WCHAR)) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}
	
	// Write source function name
	Success = WriteFile(
		LogHandle->FileHandle,
		SourceFunction,
		LogFileEntry.SourceFunctionLength * sizeof(WCHAR),
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != LogFileEntry.SourceFunctionLength * sizeof(WCHAR)) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	// Write log text
	Success = WriteFile(
		LogHandle->FileHandle,
		Text,
		LogFileEntry.TextLength * sizeof(WCHAR),
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != LogFileEntry.TextLength * sizeof(WCHAR)) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	//
	// Update the log file header to reflect the number of entries.
	//
	LogHandle->Header.EventSeverityTypeCount[Severity]++;
	Status = VxlpWriteLogFileHeader(LogHandle);
	if (VXL_FAILED(Status)) {
		goto Error;
	}

	Status = VXL_SUCCESS;
	goto NoError;

Error:
	// rollback file pointer so our potentially corrupted log entry isn't
	// in the file. "Hopefully" the header wasn't corrupted, but hey, IO errors
	// aren't really something we can "fix".
	SetFilePointerEx(LogHandle->FileHandle, OldEndOfFile, NULL, FILE_BEGIN);
	SetEndOfFile(LogHandle->FileHandle);

NoError:
	VxlpReleaseFileLock(LogHandle);
	return Status;
}