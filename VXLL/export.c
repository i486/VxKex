#include <Windows.h>
#include <strsafe.h>
#include "VXLL.h"
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
	ULONG IntroTextCch;
	PWSTR Text;
	ULONG TextCch;
	ULONG BytesWritten;

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
		return VxlpTranslateWin32Error(GetLastError(), VXL_FILE_IO_ERROR);
	}

	//
	// Write a line of information regarding the source application and
	// VXLL library.
	//

	IntroTextCch = _scwprintf(IntroTextFormat, LogHandle->Header.SourceApplication) + 1;
	IntroText = (PWSTR) HeapAlloc(GetProcessHeap(), 0, IntroTextCch * sizeof(WCHAR));
	
	if (IntroText) {
		Result = StringCchPrintf(IntroText, IntroTextCch, IntroTextFormat, LogHandle->Header.SourceApplication);

		if (SUCCEEDED(Result)) {
			WriteFile(
				OutputFileHandle,
				IntroText,
				(IntroTextCch - 1) * sizeof(WCHAR),
				&BytesWritten,
				NULL);
		}

		HeapFree(GetProcessHeap(), 0, IntroText);
	}

	//
	// Keep fetching new log entries and exporting until there are no more.
	//
	EntryIndex = 0;
	LogEntry = NULL;
	Text = NULL;

	while (TRUE) {
		VxlFreeLogEntry(&LogEntry);
		HeapFree(GetProcessHeap(), 0, (PVOID) Text);

		Status = VxlReadLogEntry(
			LogHandle,
			0,
			FALSE,
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

		Text = (PWSTR) HeapAlloc(GetProcessHeap(), 0, TextCch * sizeof(WCHAR));

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
		StringCchCat(Text, TextCch, L"\r\n");

		//
		// write the converted string to the file
		//
		WriteFile(
			OutputFileHandle,
			Text,
			TextCch * sizeof(WCHAR),
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
	PWSTR DateString = NULL;
	PWSTR TimeString = NULL;
	ULONG DateStringLength;
	ULONG TimeStringLength;
	ULONG OutputStringLength;
	ULONG OriginalTextCch;
	PCWSTR ShortFormFormatString = L"[%s\\%s:%lu %s %s] %s: %s";
	PCWSTR LongFormFormatString = L"Severity: %s (%s)\r\n"
								  L"Origin:   %s (%s, line %lu)\r\n"
								  L"Date:     %s %s\r\n"
								  L"Message:  %s\r\n";

	// param validation
	if (!TextCch) {
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
		NULL,
		0);

	if (!DateStringLength) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FAILURE);
		goto Error;
	}

	DateString = (PWSTR) HeapAlloc(GetProcessHeap(), 0, DateStringLength * sizeof(WCHAR));

	if (!DateString) {
		Status = VXL_OUT_OF_MEMORY;
		goto Error;
	}

	DateStringLength = GetDateFormat(
		LOCALE_USER_DEFAULT,
		LongForm ? DATE_LONGDATE : DATE_SHORTDATE,
		&Entry->Time,
		NULL,
		DateString,
		DateStringLength);

	if (!DateStringLength) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FAILURE);
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
		NULL,
		0);

	if (!TimeStringLength) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FAILURE);
		goto Error;
	}

	TimeString = (PWSTR) HeapAlloc(GetProcessHeap(), 0, TimeStringLength * sizeof(WCHAR));

	if (!TimeString) {
		Status = VXL_OUT_OF_MEMORY;
		goto Error;
	}

	TimeStringLength = GetTimeFormat(
		LOCALE_USER_DEFAULT,
		0,
		&Entry->Time,
		NULL,
		TimeString,
		TimeStringLength);

	if (!TimeStringLength) {
		Status = VxlpTranslateWin32Error(GetLastError(), VXL_FAILURE);
		goto Error;
	}

	//
	// Find the buffer size required and compare against what the user specified.
	//
	if (LongForm) {
		OutputStringLength = _scwprintf(
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
		OutputStringLength = _scwprintf(
			ShortFormFormatString,
			Entry->SourceComponent,
			Entry->SourceFile,
			Entry->SourceLine,
			DateString,
			TimeString,
			VxlSeverityLookup(Entry->Severity, FALSE),
			Entry->Text);
	}

	OutputStringLength++; // for null terminator
	*TextCch = OutputStringLength;

	if (!Text || *TextCch < OutputStringLength) {
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
	if (DateString) {
		HeapFree(GetProcessHeap(), 0, DateString);
	}

	if (TimeString) {
		HeapFree(GetProcessHeap(), 0, TimeString);
	}

	if (VXL_FAILED(Status)) {
		if (Text && OriginalTextCch) {
			*Text = '\0';
		}
	}

	return Status;
}