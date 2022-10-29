///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dispatch.c
//
// Abstract:
//
//     Dispatches messages arriving from the client.
//
// Author:
//
//     vxiiduu (03-Oct-2022)
//
// Revision History:
//
//     vxiiduu               03-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include "kexsrvp.h"

NTSTATUS KexSrvDispatchMessage(
	IN OUT	PKEXSRV_PER_CLIENT_INFO	ClientInfo)
{
	PKEX_IPC_MESSAGE Message;

	ASSERT (ClientInfo != NULL);

	Message = &ClientInfo->Message;

#ifdef _DEBUG
	KexSrvDumpMessageToLog(ClientInfo);
#endif

	if (Message->MessageId == KexIpcKexProcessStart) {
		PKEX_IPC_MESSAGE_DATA_PROCESS_STARTED ProcessStartedInfo;
		PNZWCH ApplicationName;

		//
		// Can't send this message twice - if we have already stored a
		// client name, something is wrong.
		//
		if (ClientInfo->ApplicationName[0] != '\0' || ClientInfo->LogHandle != NULL) {
			KexSrvGlobalLog(
				LogSeverityWarning,
				L"Client (PID %lu) has sent a duplicate KexIpcKexProcessStart message",
				ClientInfo->ProcessId);
			return STATUS_ACCESS_DENIED;
		}

		ProcessStartedInfo = &Message->ProcessStartedInformation;

		//
		// Store the client-provided client name into the client info structure.
		//
		ApplicationName = (PNZWCH) Message->AuxiliaryDataBlock;
		
		StringCchCopyN(
			ClientInfo->ApplicationName,
			ARRAYSIZE(ClientInfo->ApplicationName),
			ApplicationName,
			ProcessStartedInfo->ApplicationNameLength);

		//
		// Open the log file and place the handle in the ClientInfo structure.
		//		
		ClientInfo->LogHandle = KexSrvOpenLogFile(
			ClientInfo->ApplicationName,
			ARRAYSIZE(ClientInfo->LogFilePath),
			ClientInfo->LogFilePath);

	} else if (Message->MessageId == KexIpcHardError) {
		ULONG StringParameter1Length;
		ULONG StringParameter2Length;
		PWSTR StringParameter1;
		PWSTR StringParameter2;

		// Include space for null terminators
		StringParameter1Length = Message->HardErrorInformation.StringParameter1Length + 1;
		StringParameter2Length = Message->HardErrorInformation.StringParameter2Length + 1;

		if (StringParameter1Length > 1) {
			StringParameter1 = StackAlloc(WCHAR, StringParameter1Length);

			StringCchCopyN(
				StringParameter1,
				StringParameter1Length,
				(PCWSTR) &Message->AuxiliaryDataBlock[0],
				StringParameter1Length - 1);
		} else {
			StringParameter1 = L"";
		}

		if (StringParameter2Length > 1) {
			StringParameter2 = StackAlloc(WCHAR, StringParameter2Length);

			StringCchCopyN(
				StringParameter2,
				StringParameter2Length,
				&((PCWSTR) (Message->AuxiliaryDataBlock))[StringParameter1Length - 1],
				StringParameter2Length - 1);
		} else {
			StringParameter2 = L"";
		}

		//
		// Display the custom Hard Error task dialog.
		// KexSrvpHardErrorDialog will perform all user input processing.
		//

		KexSrvpHardError(
			ClientInfo,
			Message->HardErrorInformation.Status,
			Message->HardErrorInformation.UlongParameter,
			StringParameter1,
			StringParameter2);

	} else if (Message->MessageId == KexIpcLogEvent) {
		PKEX_IPC_MESSAGE_DATA_LOG_EVENT LogEventInfo;
		PNZWCH SourceComponent;
		PNZWCH SourceFile;
		PNZWCH SourceFunction;
		PNZWCH Text;
		PWSTR SourceComponentZ;
		PWSTR SourceFileZ;
		PWSTR SourceFunctionZ;
		VXLSTATUS Status;

		ASSERT (ClientInfo->LogHandle != NULL);

		LogEventInfo = &Message->LogEventInformation;
		SourceComponent = (PNZWCH) Message->AuxiliaryDataBlock;
		SourceFile = SourceComponent + LogEventInfo->SourceComponentLength;
		SourceFunction = SourceFile + LogEventInfo->SourceFileLength;
		Text = SourceFunction + LogEventInfo->SourceFunctionLength;

		//
		// allocate memory for null-terminated versions of source parameters.
		// We don't need this for the log text itself because we can use the
		// printf specifier %.*s to limit the length.
		//
		SourceComponentZ = StackAlloc(WCHAR, LogEventInfo->SourceComponentLength + 1);
		SourceFileZ = StackAlloc(WCHAR, LogEventInfo->SourceFileLength + 1);
		SourceFunctionZ = StackAlloc(WCHAR, LogEventInfo->SourceFunctionLength + 1);

		StringCchCopyN(
			SourceComponentZ,
			LogEventInfo->SourceComponentLength + 1,
			SourceComponent,
			LogEventInfo->SourceComponentLength);

		StringCchCopyN(
			SourceFileZ,
			LogEventInfo->SourceFileLength + 1,
			SourceFile,
			LogEventInfo->SourceFileLength);

		StringCchCopyN(
			SourceFunctionZ,
			LogEventInfo->SourceFunctionLength + 1,
			SourceFunction,
			LogEventInfo->SourceFunctionLength);

		ASSERT (wcslen(SourceComponentZ) == LogEventInfo->SourceComponentLength);
		ASSERT (wcslen(SourceFileZ) == LogEventInfo->SourceFileLength);
		ASSERT (wcslen(SourceFunctionZ) == LogEventInfo->SourceFunctionLength);

		Status = VxlLogEx(
			ClientInfo->LogHandle,
			SourceComponentZ,
			SourceFileZ,
			LogEventInfo->SourceLine,
			SourceFunctionZ,
			(VXLSEVERITY) LogEventInfo->Severity,
			L"%.*s",
			LogEventInfo->TextLength,
			Text);

		ASSERT (VXL_SUCCEEDED(Status));
		if (VXL_FAILED(Status)) {
			KexSrvGlobalLog(
				LogSeverityError,
				L"Failed to process a log event request from client (PID %lu)\r\n\r\n"
				L"VXLSTATUS error code: %d (%s)",
				ClientInfo->ProcessId,
				Status,
				VxlErrorLookup(Status));
		}
	} else {
		KexSrvGlobalLog(
			LogSeverityWarning,
			L"Client (PID %lu) has sent a message with an invalid message ID\r\n\r\n"
			L"Message ID: %d",
			ClientInfo->ProcessId,
			ClientInfo->Message.MessageId);

		return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}