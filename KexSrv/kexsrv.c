///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsrv.c
//
// Abstract:
//
//     Main file for the VxKex server program.
//
//     KexSrv runs as a normal background program, accepts connections from
//     VxKex enabled processes and is responsible for performing logging and
//     displaying hard error messages.
//
//     In the future we may explore bidirectional communication with the
//     client. However, for the time being, communication between client and
//     server is unidirectional.
//
// Author:
//
//     vxiiduu (02-Oct-2022)
//
// Environment:
//
//     Runs as a normal win32 program. Not as a "service" or anything
//     like that.
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexLog.h>
#include "kexsrvp.h"

VXLHANDLE GlobalLogHandle;

NORETURN VOID EntryPoint(
	VOID)
{
	NTSTATUS Status;
	HANDLE PipeHandle;
	UNICODE_STRING PipeName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	LARGE_INTEGER DefaultTimeout;
	LARGE_INTEGER FailureDelay;

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;

	RtlInitConstantUnicodeString(&PipeName, KEXSRV_IPC_CHANNEL_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &PipeName, 0, NULL, NULL);

	FailureDelay.QuadPart = -(10 * 1000 * 10000); // 10 s
	DefaultTimeout.QuadPart = -(50 * 10000); // 50 ms

	//
	// See if another instance of KexSrv is already running. If so, quit.
	//
	Status = NtOpenFile(
		&PipeHandle,
		GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_WRITE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if (NT_SUCCESS(Status)) {
		// kexsrv is already running, since we were able to open the pipe which
		// we didn't create yet.
#ifdef _DEBUG
		ErrorBoxF(L"Another instance of KexSrv is already running.");
#endif
		ExitProcess(0);
	}

	//
	// Open global log file.
	//
	GlobalLogHandle = KexSrvOpenLogFile(NULL, 0, NULL);
	KexSrvGlobalLog(
		LogSeverityInformation,
		L"KexSrv started\r\n\r\n"
		L"VxKex version: %s\r\n"
		L"Build timestamp: %s %s\r\n"
		L"Source last modified: %s",
		_L(KEX_VERSION_STR),
		__DATEW__,
		__TIMEW__,
		__TIMESTAMPW__);

	while (TRUE) {
		//
		// Create an instance of KexSrv's named pipe.
		//

		Status = NtCreateNamedPipeFile(
			&PipeHandle,
			GENERIC_READ | SYNCHRONIZE | FILE_CREATE_PIPE_INSTANCE,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_WRITE,
			FILE_OPEN_IF,
			FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
			FILE_PIPE_MESSAGE_TYPE,
			FILE_PIPE_MESSAGE_MODE,
			FILE_PIPE_QUEUE_OPERATION,
			256, // max instances
			1024,
			1024,
			&DefaultTimeout);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			// wait a while and then try again
			NtDelayExecution(FALSE, &FailureDelay);
			continue;
		}

		//
		// Listen for an incoming client.
		//

		Status = NtFsControlFile(
			PipeHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			FSCTL_PIPE_LISTEN,
			NULL,
			0,
			NULL,
			0);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			NtDelayExecution(FALSE, &FailureDelay);
			NtClose(PipeHandle);
			continue;
		}

		//
		// Create a thread to handle this client.
		// The parameter to the thread is the pipe handle.
		// The thread is responsible for closing the handle when finished.
		//

		Status = RtlCreateUserThread(
			NtCurrentProcess(),
			NULL,
			FALSE,
			0,
			0,
			0,
			KexSrvHandleClientThreadProc,
			PipeHandle,
			NULL,
			NULL);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			NtDelayExecution(FALSE, &FailureDelay);
			NtClose(PipeHandle);
			continue;
		}
	}
}