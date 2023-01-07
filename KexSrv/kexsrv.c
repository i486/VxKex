///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsrv.c
//
// Abstract:
//
//     VxKex local server, main file
//
// Author:
//
//     vxiiduu (03-Jan-2023)
//
// Revision History:
//
//     vxiiduu               03-Jan-2023  Initial creation, rewrite original.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsrvp.h"

PKEX_PROCESS_DATA KexData = NULL;
RTL_DYNAMIC_HASH_TABLE _ProcessThreadTable;
PRTL_DYNAMIC_HASH_TABLE ProcessThreadTable = &_ProcessThreadTable;

NORETURN VOID NTAPI EntryPoint(
	IN	PVOID	Parameter)
{
	NTSTATUS Status;
	HANDLE ConnectEventHandle;
	HANDLE PipeHandle;
	UNICODE_STRING PipeName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	//
	// Try to open the KexSrv named pipe. If we succeed, that means that
	// another instance of KexSrv is already running. If another instance
	// of the server is already running, we can just quit here.
	//

	RtlInitConstantUnicodeString(&PipeName, KEXSRV_IPC_CHANNEL_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &PipeName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenFile(
		&PipeHandle,
		GENERIC_WRITE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0);

	if (NT_SUCCESS(Status)) {
		DbgPrint("Server pipe already exists\r\n");
		NtTerminateProcess(NtCurrentProcess(), STATUS_OBJECT_NAME_EXISTS);
	}

	//
	// Get a pointer to KexData.
	//

	Status = KexDataInitialize(&KexData);
	if (!NT_SUCCESS(Status)) {
		DbgPrint("Failed to initialize KexData (%ws)\r\n", KexRtlNtStatusToString(Status));
		NtTerminateProcess(NtCurrentProcess(), Status);
	}

	//
	// Initialize the process/thread table.
	//

	RtlCreateHashTable(&ProcessThreadTable, 0, 0);

	//
	// Open the server's log file. If this fails, we will not be able to
	// log anything, but it's otherwise a non-critical error.
	//

	OpenServerLogFile(&KexData->LogHandle);
	KexLogInformationEvent(L"Server process started.");

	//
	// Create an event. This event will be signaled when a client is attempting to
	// connect to the server.
	//

	Status = NtCreateEvent(
		&ConnectEventHandle,
		EVENT_ALL_ACCESS,
		NULL,
		NotificationEvent,
		TRUE);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to create the pipe-connect event.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		NtTerminateProcess(NtCurrentProcess(), Status);
	}

	//
	// Create and connect the first instance of the pipe.
	//

	Status = CreateConnectPipeInstance(
			&PipeHandle,
			ConnectEventHandle,
			&ObjectAttributes, 
			&IoStatusBlock);

	if (Status != STATUS_PENDING && !NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to create and connect the first instance of the server pipe.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		NtTerminateProcess(NtCurrentProcess(), Status);
	}

	//
	// Main loop
	//

	while (TRUE) {
		Status = NtWaitForSingleObject(
			ConnectEventHandle,
			TRUE,
			NULL);

		if (Status == STATUS_SUCCESS) {
			PKEXSRV_PER_CLIENT_PROCESS_DATA PerProcessData;

			//
			// This means that a client is connected.
			// Invoke a subroutine to allocate and register data structures, etc.
			//

			Status = AcceptConnectProcess(&PerProcessData, PipeHandle);
			if (NT_SUCCESS(Status)) {
				//
				// Initiate an asynchronous read of the new pipe.
				// This call will return immediately.
				//

				CompletedWriteApc(NULL, &PerProcessData->IoStatusBlock, 0);

				KexLogInformationEvent(
					L"New client process (PID %lu) has successfully connected.",
					PerProcessData->ProcessId);
			} else {
				KexLogErrorEvent(
					L"Failed to accept a connection from a client process. (%s)",
					KexRtlNtStatusToString(Status));
				NtClose(PipeHandle);
			}

			//
			// Create and connect another instance.
			//

			Status = CreateConnectPipeInstance(
				&PipeHandle,
				ConnectEventHandle,
				&ObjectAttributes,
				&IoStatusBlock);

			if (Status != STATUS_PENDING && !NT_SUCCESS(Status)) {
				NtTerminateProcess(NtCurrentProcess(), Status);
			}
		} else if (!NT_SUCCESS(Status)) {
			NtTerminateProcess(NtCurrentProcess(), Status);
		} else {
			//
			// - Thread was alerted (somehow)
			// - User APC was delivered (somehow)
			// - I/O APC was delivered
			//
			// In these cases we don't need to take any action here.
			// See functions in apc.c
			//

			NOTHING;
		}
	}

	NOT_REACHED;
}