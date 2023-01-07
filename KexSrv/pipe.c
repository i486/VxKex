///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     pipe.c
//
// Abstract:
//
//     Miscellaneous functions for dealing with named pipes.
//
// Author:
//
//     vxiiduu (05-Jan-2023)
//
// Revision History:
//
//     vxiiduu               05-Jan-2023  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsrvp.h"

NTSTATUS CreateConnectPipeInstance(
	OUT	PHANDLE				PipeHandle,
	IN	HANDLE				ConnectEventHandle,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock)
{
	NTSTATUS Status;
	LONGLONG DefaultTimeout;

	ASSERT (PipeHandle != NULL);
	ASSERT (VALID_HANDLE(ConnectEventHandle));
	ASSERT (ObjectAttributes != NULL);
	ASSERT (IoStatusBlock != NULL);

	//
	// Default timeout - set to infinite
	//

	DefaultTimeout = 0x8000000000000000;

	//
	// Create an instance of the server's named pipe.
	//

	Status = NtCreateNamedPipeFile(
		PipeHandle,
		GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE | FILE_CREATE_PIPE_INSTANCE,
		ObjectAttributes,
		IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		0,
		FILE_PIPE_MESSAGE_TYPE,
		FILE_PIPE_MESSAGE_MODE,
		FILE_PIPE_QUEUE_OPERATION,
		256,
		1024,
		1024,
		&DefaultTimeout);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Listen for clients trying to connect.
	//

	Status = NtFsControlFile(
		*PipeHandle,
		ConnectEventHandle,
		NULL,
		NULL,
		IoStatusBlock,
		FSCTL_PIPE_LISTEN,
		NULL,
		0,
		NULL,
		0);

	if (Status == STATUS_PIPE_CONNECTED) {
		Status = NtSetEvent(ConnectEventHandle, NULL);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Status = STATUS_SUCCESS;
	}

	return Status;
}