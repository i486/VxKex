///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     writelog.c
//
// Abstract:
//
//     Contains the public routine for writing log entries.
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

VXLSTATUS VXLAPI VxlLogEx(
	IN		VXLHANDLE		LogHandle,
	IN		PCWSTR			SourceComponent OPTIONAL,
	IN		PCWSTR			SourceFile OPTIONAL,
	IN		ULONG			SourceLine,
	IN		PCWSTR			SourceFunction OPTIONAL,
	IN		VXLSEVERITY		Severity,
	IN		PCWSTR			Format,
	IN		...)
{
	VXLSTATUS Status;
	BOOLEAN Success;
	HRESULT Result;
	LARGE_INTEGER Zero;
	LARGE_INTEGER OldEndOfFile;
	ULONG BytesWritten;

	SIZE_T UnifiedBufferCb;
	PBYTE UnifiedBuffer;

	SIZE_T SourceFileBufferCch;
	SIZE_T SourceFunctionBufferCch;
	SIZE_T TextHeaderBufferCch;
	SIZE_T TextBufferCch;
	PWSTR SourceFileBuffer;
	PWSTR SourceFunctionBuffer;
	PWSTR TextHeaderBuffer;
	PWSTR TextBuffer;
	
	ULONG Index;
	PVXLLOGFILEENTRY LogFileEntry;

	va_list ArgList;

	//
	// param validation
	//
	if (!LogHandle || !Format) {
		return VXL_INVALID_PARAMETER;
	}

	if (Severity < 0) {
		return VXL_INVALID_PARAMETER;
	}

	if (Severity >= LogSeverityMaximumValue) {
		return VXL_NOT_IMPLEMENTED;
	}

	//
	// Check if we are even allowed to write to the log.
	//
	if (LogHandle->OpenFlags & VXL_OPEN_READ_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	if (!SourceComponent) {
		SourceComponent = L"(unspecified)";
	}

	if (!SourceFile) {
		SourceFile = L"(unspecified)";
	}

	if (!SourceFunction) {
		SourceFunction = L"(unspecified)";
	}

	SourceFileBufferCch = (ULONG) wcslen(SourceFile) + 1;
	SourceFunctionBufferCch = (ULONG) wcslen(SourceFunction) + 1;

	if (SourceFileBufferCch > USHRT_MAX || SourceFunctionBufferCch > USHRT_MAX) {
		return VXL_INVALID_PARAMETER_LENGTH;
	}

	//
	// Figure out how long the formatted log text will be.
	//
	va_start(ArgList, Format);
	Result = StringCchVPrintfBufferLength(&TextHeaderBufferCch, Format, ArgList);
	if (FAILED(Result)) {
		return VXL_FAILURE;
	}

	if (TextHeaderBufferCch > USHRT_MAX) {
		return VXL_ENTRY_TEXT_TOO_LONG;
	}

	//
	// Allocate space for the LogFileEntry and all the strings as one continuous
	// stack allocation.
	//
	UnifiedBufferCb = sizeof(VXLLOGFILEENTRY) + 
					  SourceFileBufferCch * sizeof(WCHAR) +
					  SourceFunctionBufferCch * sizeof(WCHAR) +
					  TextHeaderBufferCch * sizeof(WCHAR);

	// Generally speaking we shouldn't allocate huge data on the stack.
	if (UnifiedBufferCb >= (1024 * 512)) { // 512kb
		return VXL_ENTRY_TEXT_TOO_LONG;
	}

	UnifiedBuffer = StackAlloc(BYTE, UnifiedBufferCb);
	LogFileEntry = (PVXLLOGFILEENTRY) UnifiedBuffer;
	SourceFileBuffer = (PWSTR) (UnifiedBuffer + sizeof(VXLLOGFILEENTRY));
	SourceFunctionBuffer = SourceFileBuffer + SourceFileBufferCch;
	TextHeaderBuffer = SourceFunctionBuffer + SourceFunctionBufferCch;

	//
	// Copy source file and function strings into buffer.
	//

	CopyMemory(SourceFileBuffer, SourceFile, SourceFileBufferCch * sizeof(WCHAR));
	CopyMemory(SourceFunctionBuffer, SourceFunction, SourceFunctionBufferCch * sizeof(WCHAR));

	//
	// Format log text.
	//

	Result = StringCchVPrintf(TextHeaderBuffer, TextHeaderBufferCch, Format, ArgList);
	va_end(ArgList);

	if (FAILED(Result)) {
		Status = VXL_INSUFFICIENT_BUFFER;
		goto Error;
	}

	//
	// Find the first newline (\r\n) in the log text (if any).
	// If the newline is the last character in the buffer, ignore it.
	//

	TextBuffer = NULL;
	TextBufferCch = 0;

	if (TextHeaderBufferCch > 5) {
		for (Index = 2; Index < (TextHeaderBufferCch - 2); ++Index) {
			if (TextHeaderBuffer[Index] == '\r' && TextHeaderBuffer[Index + 1] == '\n') {
				// Erase the newline so that the text header and the text form two
				// distinct strings.
				TextHeaderBuffer[Index] = '\0';
				TextHeaderBuffer[Index + 1] = '\0';

				TextBuffer = &TextHeaderBuffer[Index + 2];

				TextBufferCch = TextHeaderBufferCch - Index - 2;
				TextHeaderBufferCch = Index + 1;

				ASSERT (TextBufferCch == wcslen(TextBuffer) + 1);
				ASSERT (TextHeaderBufferCch == wcslen(TextHeaderBuffer) + 1);

				break;
			}
		}
	}

	//
	// Fill out VXLLOGFILEENTRY structure.
	//

	GetSystemTimeAsFileTime(&LogFileEntry->Time);
	LogFileEntry->Severity					= Severity;
	LogFileEntry->SourceFileLength			= (USHORT) SourceFileBufferCch;
	LogFileEntry->SourceFunctionLength		= (USHORT) SourceFunctionBufferCch;
	LogFileEntry->SourceLine				= SourceLine;
	LogFileEntry->TextLength				= (USHORT) TextBufferCch;
	LogFileEntry->TextHeaderLength			= (USHORT) TextHeaderBufferCch;

	Status = VxlpFindOrCreateSourceComponentIndex(LogHandle, SourceComponent, &LogFileEntry->SourceComponentIndex);
	if (VXL_FAILED(Status)) {
		goto Error;
	}

	Status = VxlpAcquireFileLock(LogHandle);
	if (VXL_FAILED(Status)) {
		goto Error;
	}

	//
	// Write the log entry to the end of the log file.
	//
	// Data is written to the end of the log file in the following order:
	//   1. VXLLOGFILEENTRY structure
	//   2. Source file (null terminated)
	//   3. Source function (null terminated)
	//   4. First line of log text (null terminated)
	//   5. Remainder of log text (null terminated)
	//
	// If the log file consists of only one line, then LogFileEntry->TextLength will
	// be zero.
	//

	// Move file pointer to end of file
	Zero.QuadPart = 0;
	Success = SetFilePointerEx(LogHandle->FileHandle, Zero, &OldEndOfFile, FILE_END);
	if (!Success) {
		Status = VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	// Write VXLLOGFILEENTRY structure + all strings
	Success = WriteFile(
		LogHandle->FileHandle,
		UnifiedBuffer,
		(ULONG) UnifiedBufferCb,
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != UnifiedBufferCb) {
		Status = VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
		goto Error;
	}

	//
	// Update the log file header to reflect the number of entries.
	//
	LogHandle->Header.EventSeverityTypeCount[Severity]++;

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