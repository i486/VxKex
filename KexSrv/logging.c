///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     logging.c
//
// Abstract:
//
//     Deals with VXL log files.
//
// Author:
//
//     vxiiduu (14-Oct-2022)
//
// Environment:
//
//     Runs as a normal win32 program. Not as a "service" or anything
//     like that.
//
// Revision History:
//
//     vxiiduu               14-Oct-2022  Initial creation.
//     vxiiduu               24-Oct-2022  Add ability to write out log path.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexLog.h>
#include <ShlObj.h>
#include "kexsrvp.h"

//
// KexProgramName can be NULL, in which case the global log file for
// KexSrv will be opened.
//
VXLHANDLE KexSrvOpenLogFile(
	IN	PCWSTR	KexProgramName OPTIONAL,
	IN	ULONG	LogFilePathBufferCch OPTIONAL,
	OUT	PWSTR	LogFilePathOut OPTIONAL)
{
	ULONG Win32Error;
	WCHAR LogFilePath[MAX_PATH];
	WCHAR LogFileDir[MAX_PATH];
	ULONG LogFileDirCb;
	HRESULT Result;
	VXLSTATUS Status;
	VXLHANDLE LogHandle;

	LogFileDirCb = sizeof(LogFilePath);

	Win32Error = RegGetValue(
		HKEY_CURRENT_USER,
		L"SOFTWARE\\VXsoft\\VxKex",
		L"LogDir",
		RRF_RT_REG_SZ,
		NULL,
		LogFileDir,
		&LogFileDirCb);

	ASSERT (Win32Error == ERROR_SUCCESS);
	if (Win32Error) {
		return NULL;
	}

	// make sure logdir exists
	Win32Error = SHCreateDirectory(NULL, LogFileDir);
	ASSERT (Win32Error == ERROR_SUCCESS ||
			Win32Error == ERROR_ALREADY_EXISTS ||
			Win32Error == ERROR_FILE_EXISTS);

	if (!KexProgramName) {
		Result = StringCchPrintf(
			LogFilePath,
			ARRAYSIZE(LogFilePath),
			L"%s\\KexSrv.vxl",
			LogFileDir);
	} else {
		WCHAR KexProgramNameClean[MAX_PATH];
		ULONGLONG SystemTime64;

		// make sure no illegal chars in the program name
		StringCchCopy(KexProgramNameClean, ARRAYSIZE(KexProgramNameClean), KexProgramName);
		PathReplaceIllegalCharacters(KexProgramNameClean, '_', FALSE);

		SystemTime64 = *(PULONGLONG) &SharedUserData->SystemTime;
		Result = StringCchPrintf(
			LogFilePath,
			ARRAYSIZE(LogFilePath),
			L"%s\\%s-%020I64d.vxl",
			LogFileDir,
			KexProgramNameClean,
			SystemTime64);
	}

	ASSERT (SUCCEEDED(Result));
	if (FAILED(Result)) {
		return NULL;
	}

	// StringCchCopy will do param validation for us
	StringCchCopy(LogFilePathOut, LogFilePathBufferCch, LogFilePath);

	Status = VxlOpenLogFile(
		LogFilePath,
		&LogHandle,
		L"VxKex");

	ASSERT (VXL_SUCCEEDED(Status));
	if (VXL_FAILED(Status)) {
		// this will only succeed if we have already opened the global
		// log file - if not, it will have no effect.
		KexSrvGlobalLog(
			LogSeverityError,
			L"Failed to open log file for %s\r\n\r\n"
			L"Full file name: %s\r\n"
			L"VXLSTATUS error code: %s",
			KexProgramName,
			LogFilePath,
			VxlErrorLookup(Status));

		return NULL;
	}

	ASSERT (LogHandle != NULL);
	return LogHandle;
}

#ifdef _DEBUG
VOID KexSrvDumpMessageToLog(
	IN	PKEXSRV_PER_CLIENT_INFO	ClientInfo)
{
	PKEX_IPC_MESSAGE Message;

	ASSERT (ClientInfo != NULL);

	Message = &ClientInfo->Message;

	//
	// PNZWCH = a string that is not null terminated
	//

	if (Message->MessageId == KexIpcKexProcessStart) {
		PKEX_IPC_MESSAGE_DATA_PROCESS_STARTED ProcessStartedInfo;
		PNZWCH ApplicationName;

		ProcessStartedInfo = &Message->ProcessStartedInformation;
		ApplicationName = (PNZWCH) Message->AuxiliaryDataBlock;

		KexSrvGlobalLog(
			LogSeverityDebug,
			L"Received message from client (PID %lu): KexIpcKexProcessStart\r\n\r\n"
			L"MessageId = %d\r\n"
			L"AuxiliaryDataBlockSize = %hu\r\n"
			L"ApplicationName = \"%.*s\"",
			ClientInfo->ProcessId,
			Message->MessageId,
			Message->AuxiliaryDataBlockSize,
			ProcessStartedInfo->ApplicationNameLength, ApplicationName);
	} else if (Message->MessageId == KexIpcHardError) {
		PKEX_IPC_MESSAGE_DATA_HARD_ERROR HardErrorInfo;
		PNZWCH StringParameter1;
		PNZWCH StringParameter2;

		HardErrorInfo = &Message->HardErrorInformation;
		StringParameter1 = (PNZWCH) Message->AuxiliaryDataBlock;
		StringParameter2 = StringParameter1 + HardErrorInfo->StringParameter1Length;

		KexSrvGlobalLog(
			LogSeverityDebug,
			L"Received message from client (PID %lu): KexIpcHardError\r\n\r\n"
			L"MessageId = %d\r\n"
			L"AuxiliaryDataBlockSize = %hu\r\n"
			L"Status = 0x%08lx\r\n"
			L"UlongParameter = %lu\r\n"
			L"StringParameter1 = \"%.*s\"\r\n"
			L"StringParameter2 = \"%.*s\"",
			ClientInfo->ProcessId,
			Message->MessageId,
			Message->AuxiliaryDataBlockSize,
			HardErrorInfo->Status,
			HardErrorInfo->UlongParameter,
			HardErrorInfo->StringParameter1Length, StringParameter1,
			HardErrorInfo->StringParameter2Length, StringParameter2);
	} else if (Message->MessageId == KexIpcLogEvent) {
		PKEX_IPC_MESSAGE_DATA_LOG_EVENT LogEventInfo;
		PNZWCH SourceComponent;
		PNZWCH SourceFile;
		PNZWCH SourceFunction;
		PNZWCH Text;

		LogEventInfo = &Message->LogEventInformation;
		SourceComponent = (PNZWCH) Message->AuxiliaryDataBlock;
		SourceFile = SourceComponent + LogEventInfo->SourceComponentLength;
		SourceFunction = SourceFile + LogEventInfo->SourceFileLength;
		Text = SourceFunction + LogEventInfo->SourceFunctionLength;

		KexSrvGlobalLog(
			LogSeverityDebug,
			L"Received message from client (PID %lu): KexIpcLogEvent\r\n\r\n"
			L"MessageId = %d\r\n"
			L"AuxiliaryDataBlockSize = %hu\r\n"
			L"Severity = %s\r\n"
			L"SourceLine = %lu\r\n"
			L"SourceComponent = %.*s\r\n"
			L"SourceFile = %.*s\r\n"
			L"SourceFunction = \"%.*s\"\r\n"
			L"Text = \"%.*s\"",
			ClientInfo->ProcessId,
			Message->MessageId,
			Message->AuxiliaryDataBlockSize,
			VxlSeverityLookup((VXLSEVERITY) LogEventInfo->Severity, FALSE),
			LogEventInfo->SourceLine,
			LogEventInfo->SourceComponentLength, SourceComponent,
			LogEventInfo->SourceFileLength, SourceFile,
			LogEventInfo->SourceFunctionLength, SourceFunction,
			LogEventInfo->TextLength, Text);
	}
}
#endif