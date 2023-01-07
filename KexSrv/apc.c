///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     apc.c
//
// Abstract:
//
//     Contains I/O APC functions, called for every read and write operation
//     on the server named pipe.
//
// Author:
//
//     vxiiduu (05-Jan-2023)
//
// Revision History:
//
//     vxiiduu               05-Jan-2023  Initial creation
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsrvp.h"

VOID NTAPI CompletedReadApc(
	IN	PVOID				ApcContext,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				Reserved)
{
}

VOID NTAPI CompletedWriteApc(
	IN	PVOID				ApcContext,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				Reserved)
{
	NTSTATUS Status;
	PKEXSRV_PER_CLIENT_PROCESS_DATA Process;

	ASSERT (IoStatusBlock != NULL);

	Process = CONTAINING_RECORD(IoStatusBlock, KEXSRV_PER_CLIENT_PROCESS_DATA, IoStatusBlock);

	if (!NT_SUCCESS(IoStatusBlock->Status)) {
		KexLogErrorEvent(
			L"I/O error on a completed write to pipe for PID %ld (%s)",
			Process->ProcessId,
			KexRtlNtStatusToString(IoStatusBlock->Status));
		
		DisconnectProcess(Process);
		return;
	}

	Status = NtReadFile(
		Process->PipeHandle,
		NULL,
		CompletedReadApc,
		NULL,
		IoStatusBlock,
		Process->IncomingMessageBuffer,
		ARRAYSIZE(Process->IncomingMessageBuffer),
		NULL,
		NULL);

	if (Status != STATUS_PENDING && !NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"NtReadFile error on pipe for PID %ld (%s)",
			Process->ProcessId,
			KexRtlNtStatusToString(IoStatusBlock->Status));

		DisconnectProcess(Process);
		return;
	}
}