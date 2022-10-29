///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     client.c
//
// Abstract:
//
//     Contains the thread procedure which is invoked per client.
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

NTSTATUS NTAPI KexSrvHandleClientThreadProc(
	IN	HANDLE	PipeHandle)
{
	KEXSRV_PER_CLIENT_INFO ClientInfo;
	NTSTATUS Status;
	ULONG ExpectedBytes;
	IO_STATUS_BLOCK IoStatusBlock;
	USHORT LastStep;

	ASSERT (PipeHandle != NULL);
	ASSERT (PipeHandle != INVALID_HANDLE_VALUE);

	ZeroMemory(&ClientInfo, sizeof(ClientInfo));

	//
	// Figure out which process is connected to this instance.
	//

	LastStep = 0;
	Status = NtFsControlFile(
		PipeHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
		(PVOID) "ClientProcessId",
		sizeof("ClientProcessId"),
		&ClientInfo.ProcessId,
		sizeof(ClientInfo.ProcessId));
	ASSERT (NT_SUCCESS(Status));
	CHECKED (NT_SUCCESS(Status));

	KexSrvGlobalLog(
		LogSeverityInformation,
		L"Client (PID %lu) has connected to KexSrv",
		ClientInfo.ProcessId);

	while (TRUE) {
		//
		// Read messages from the client.
		//

		LastStep = 1;
		Status = NtReadFile(
			PipeHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			ClientInfo.MessageBuffer,
			sizeof(ClientInfo.MessageBuffer),
			NULL,
			NULL);

		if (Status == STATUS_PIPE_BROKEN) {
			// The client has disconnected or exited.
			// This is not an error.
			goto Finish;
		}

		ASSERT (NT_SUCCESS(Status));
		CHECKED (NT_SUCCESS(Status));

		//
		// Verify that the length of the received message matches
		// what the client sent. If anything is weird, bail out because
		// clearly this client is loony.
		//

		LastStep = 2;
		ExpectedBytes = sizeof(KEX_IPC_MESSAGE) + ClientInfo.Message.AuxiliaryDataBlockSize;

		if (IoStatusBlock.Information != ExpectedBytes) {
			KexSrvGlobalLog(
				LogSeverityWarning,
				L"Client (PID %lu) has sent an invalid message\r\n\r\n"
				L"Expected %lu bytes, received %Iu",
				ClientInfo.ProcessId,
				ExpectedBytes,
				IoStatusBlock.Information);

			Status = STATUS_INFO_LENGTH_MISMATCH;
			goto Error;
		}

		//
		// Pass the message to the dispatch routine for further action.
		//

		LastStep = 3;
		Status = KexSrvDispatchMessage(&ClientInfo);
		ASSERT (NT_SUCCESS(Status));
		CHECKED (NT_SUCCESS(Status));
	}

Error:
Finish:
	{
		VXLSEVERITY LogSeverity;
		
		if (NT_SUCCESS(Status) || Status == STATUS_PIPE_BROKEN) {
			LogSeverity = LogSeverityInformation;
		} else {
			LogSeverity = LogSeverityError;
		}

		KexSrvGlobalLog(
			LogSeverity,
			L"Disconnecting from client (PID %lu)\r\n\r\n"
			L"Last step: %hu\r\n"
			L"NTSTATUS code: 0x%08lx\r\n"
			L"%s",
			ClientInfo.ProcessId,
			LastStep,
			Status,
			NtStatusAsString(Status));
	}

	VxlCloseLogFile(&ClientInfo.LogHandle);
	NtClose(PipeHandle);
	return Status;
}