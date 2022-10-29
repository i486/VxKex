///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     export.c
//
// Abstract:
//
//     Contains routines for exporting log file entries or entire log files
//     to a plain text format.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu              30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

//
// Export entire log as a .txt file.
//
VXLSTATUS VXLAPI VxlExportLogToText(
	IN		VXLHANDLE		LogHandle,
	IN		PCWSTR			TextFilePath,
	IN		BOOLEAN			LongForm)
{
	VXLSTATUS Status;
	HRESULT Result;
	ULONG EntryIndex;
	HANDLE OutputFileHandle;
	PVXLLOGENTRY LogEntry;
	PCWSTR IntroTextFormat = L"VXLL Log Export from application: %s\r\n\r\n";
	PWSTR IntroText;
	SIZE_T IntroTextCch;
	PWSTR Text;
	ULONG TextCch;
	ULONG TextMaxCch;
	ULONG BytesWritten;

	ASSERT (LogHandle != NULL);
	ASSERT (TextFilePath != NULL);

	// param validation
	if (!LogHandle || !TextFilePath) {
		return VXL_INVALID_PARAMETER;
	}

	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		return VXL_FILE_WRONG_MODE;
	}

	//
	// Open our output file.
	//
	OutputFileHandle = CreateFile(
		TextFilePath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (OutputFileHandle == INVALID_HANDLE_VALUE) {
		return VxlTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	//
	// Write a line of information regarding the source application and
	// VXLL library.
	//

	Result = StringCchPrintfBufferLength(&IntroTextCch, IntroTextFormat, LogHandle->Header.SourceApplication);
	if (FAILED(Result)) {
		return VXL_FAILURE;
	}

	if (IntroTextCch > ULONG_MAX) {
		return VXL_ENTRY_TEXT_TOO_LONG;
	}
	
	IntroText = StackAlloc(WCHAR, IntroTextCch);
	Result = StringCchPrintf(IntroText, IntroTextCch, IntroTextFormat, LogHandle->Header.SourceApplication);

	if (SUCCEEDED(Result)) {
		WriteFile(
			OutputFileHandle,
			IntroText,
			((ULONG) IntroTextCch - 1) * sizeof(WCHAR),
			&BytesWritten,
			NULL);
	}

	//
	// Keep fetching new log entries and exporting until there are no more.
	//
	EntryIndex = 0;
	LogEntry = NULL;
	Text = NULL;

	while (TRUE) {
		VxlFreeLogEntry(&LogEntry);
		SafeFree(Text);
		Text = NULL;

		Status = VxlReadLogEntry(
			LogHandle,
			EntryIndex++,
			&LogEntry);

		if (Status == VXL_ENTRY_OUT_OF_RANGE) {
			// No more entries - quit
			break;
		}

		if (VXL_FAILED(Status)) {
			continue;
		}

		Status = VxlConvertLogEntryToText(
			LogEntry,
			NULL,
			&TextCch,
			LongForm);

		if (Status != VXL_INSUFFICIENT_BUFFER) {
			continue;
		}

		//
		// we need to append a newline to the string
		//
		TextCch += 2;
		TextMaxCch = TextCch;

		Text = SafeAlloc(WCHAR, TextMaxCch);
		if (!Text) {
			continue;
		}

		Status = VxlConvertLogEntryToText(
			LogEntry,
			Text,
			&TextCch,
			LongForm);

		if (VXL_FAILED(Status)) {
			continue;
		}

		//
		// append newline
		//
		StringCchCat(Text, TextMaxCch, L"\r\n");

		//
		// write the converted string to the file
		//
		WriteFile(
			OutputFileHandle,
			Text,
			(TextCch + 2 - 1) * sizeof(WCHAR),
			&BytesWritten,
			NULL);
	}

	//
	// Close output file
	//
	CloseHandle(OutputFileHandle);
	return VXL_SUCCESS;
}

//
// Convert a VXLLOGENTRY structure to a form that can be displayed as text.
// If LongForm is TRUE, use a multi-line display format. Otherwise use a
// condensed format.
//
// If Text is NULL or if *TextCch is zero or too small, the function will
// return VXL_INSUFFICIENT_BUFFER and *TextCch will contain the correct size
// of the buffer in characters including the null terminator.
//
// If the function returns successfully, *TextCch will contain the length of
// the string in the buffer pointed to by Text, including the terminating
// null character.
//
VXLSTATUS VXLAPI VxlConvertLogEntryToText(
	IN		PVXLLOGENTRY	Entry,
	OUT		PWSTR			Text OPTIONAL,
	IN OUT	PULONG			TextCch,
	IN		BOOLEAN			LongForm)
{
	VXLSTATUS Status;
	HRESULT Result;
	WCHAR DateString[64];
	WCHAR TimeString[64];
	ULONG DateStringLength;
	ULONG TimeStringLength;
	SIZE_T OutputStringCch;
	ULONG OriginalTextCch;
	PCWSTR ShortFormFormatString = L"[%s\\%s:%lu %s %s] %s: %s";
	PCWSTR LongFormFormatString = L"Severity: %s (%s)\r\n"
								  L"Origin:   %s (%s, line %lu)\r\n"
								  L"Date:     %s %s\r\n"
								  L"Message:  %s\r\n";

	ASSERT (Entry != NULL);
	ASSERT (TextCch != NULL);

	// param validation
	if (!Entry || !TextCch) {
		return VXL_INVALID_PARAMETER;
	}

	OriginalTextCch = *TextCch;

	//
	// Format the date string.
	//
	DateStringLength = GetDateFormat(
		LOCALE_USER_DEFAULT,
		LongForm ? DATE_LONGDATE : DATE_SHORTDATE,
		&Entry->Time,
		NULL,
		DateString,
		ARRAYSIZE(DateString));

	if (!DateStringLength) {
		Status = VxlTranslateWin32Error(GetLastError(), VXL_FAILURE);
		goto Error;
	}

	//
	// Format time string.
	//
	TimeStringLength = GetTimeFormat(
		LOCALE_USER_DEFAULT,
		0,
		&Entry->Time,
		NULL,
		TimeString,
		ARRAYSIZE(TimeString));

	if (!TimeStringLength) {
		Status = VxlTranslateWin32Error(GetLastError(), VXL_FAILURE);
		goto Error;
	}

	//
	// Find the buffer size required and compare against what the user specified.
	//
	if (LongForm) {
		Result = StringCchPrintfBufferLength(
			&OutputStringCch,
			LongFormFormatString,
			VxlSeverityLookup(Entry->Severity, FALSE),
			VxlSeverityLookup(Entry->Severity, TRUE),
			Entry->SourceComponent,
			Entry->SourceFile,
			Entry->SourceLine,
			DateString,
			TimeString,
			Entry->Text);
	} else {
		Result = StringCchPrintfBufferLength(
			&OutputStringCch,
			ShortFormFormatString,
			Entry->SourceComponent,
			Entry->SourceFile,
			Entry->SourceLine,
			DateString,
			TimeString,
			VxlSeverityLookup(Entry->Severity, FALSE),
			Entry->Text);
	}

	if (FAILED(Result)) {
		Status = VXL_FAILURE;
		goto Error;
	}

	if (OutputStringCch > ULONG_MAX) {
		Status = VXL_ENTRY_TEXT_TOO_LONG;
		goto Error;
	}
	
	*TextCch = (ULONG) OutputStringCch;

	if (!Text || *TextCch < OutputStringCch) {
		Status = VXL_INSUFFICIENT_BUFFER;
		goto Error;
	}

	//
	// Actually format the text into the caller-supplied buffer
	//
	if (LongForm) {
		Result = StringCchPrintf(
			Text,
			OriginalTextCch,
			LongFormFormatString,
			VxlSeverityLookup(Entry->Severity, FALSE),
			VxlSeverityLookup(Entry->Severity, TRUE),
			Entry->SourceComponent,
			Entry->SourceFile,
			Entry->SourceLine,
			DateString,
			TimeString,
			Entry->Text);
	} else {
		Result = StringCchPrintf(
			Text,
			OriginalTextCch,
			ShortFormFormatString,
			Entry->SourceComponent,
			Entry->SourceFile,
			Entry->SourceLine,
			DateString,
			TimeString,
			VxlSeverityLookup(Entry->Severity, FALSE),
			Entry->Text);
	}

	if (FAILED(Result)) {
		Status = VXL_FAILURE;
		goto Error;
	}

	Status = VXL_SUCCESS;

Error:
	if (VXL_FAILED(Status)) {
		if (Text && OriginalTextCch) {
			*Text = '\0';
		}
	}

	return Status;
}