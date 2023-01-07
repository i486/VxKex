///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsrvp.h
//
// Abstract:
//
//     Private header file for KexSrv.
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

#pragma once

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

extern PKEX_PROCESS_DATA KexData;
extern PRTL_DYNAMIC_HASH_TABLE ProcessThreadTable;

//
// Data-Type Definitions
//

typedef struct _KEXSRV_PER_CLIENT_THREAD_DATA **PPKEXSRV_PER_CLIENT_THREAD_DATA;

typedef struct _KEXSRV_PER_CLIENT_PROCESS_DATA {
	BOOLEAN							IsProcess;
	RTL_DYNAMIC_HASH_TABLE_ENTRY	HashTableEntry;
	ULONG							ProcessId;

	// Thread table. Contains an array of all the threads in this
	// process.
	ULONG							NumberOfThreads;
	PPKEXSRV_PER_CLIENT_THREAD_DATA	Threads;
	
	HANDLE							PipeHandle;
	IO_STATUS_BLOCK					IoStatusBlock;

	union {
		KEX_IPC_MESSAGE				IncomingMessage;
		BYTE						IncomingMessageBuffer[0xFFFF];
	};

	union {
		KEX_IPC_MESSAGE				OutgoingMessage;
		BYTE						OutgoingMessageBuffer[0xFFFF];
	};
} TYPEDEF_TYPE_NAME(KEXSRV_PER_CLIENT_PROCESS_DATA);

typedef struct _KEXSRV_PER_CLIENT_THREAD_DATA {
	BOOLEAN							IsProcess;
	RTL_DYNAMIC_HASH_TABLE_ENTRY	HashTableEntry;
	ULONG							ThreadId;

	// Index into the parent process's thread table, such that:
	// (this.ParentProcess->Threads[this.ThreadTableIndex] == &this)
	ULONG							ThreadTableIndex;
	
	// Pointer to the parent process's data.
	PKEXSRV_PER_CLIENT_PROCESS_DATA	ParentProcess;

	UNICODE_STRING					ThreadDescription;
} TYPEDEF_TYPE_NAME(KEXSRV_PER_CLIENT_THREAD_DATA);

//
// apc.c
//

VOID NTAPI CompletedReadApc(
	IN	PVOID				ApcContext,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				Reserved);

VOID NTAPI CompletedWriteApc(
	IN	PVOID				ApcContext,
	IN	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				Reserved);

//
// logging.c
//

NTSTATUS OpenServerLogFile(
	OUT	PVXLHANDLE	LogHandle);

//
// pipe.c
//

NTSTATUS CreateConnectPipeInstance(
	OUT	PHANDLE				PipeHandle,
	IN	HANDLE				ConnectEventHandle,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock);

//
// procthrd.c
//

NTSTATUS AcceptConnectProcess(
	OUT	PPKEXSRV_PER_CLIENT_PROCESS_DATA	ProcessDataOut OPTIONAL,
	IN	HANDLE								PipeHandle);

NTSTATUS DisconnectProcess(
	IN OUT	PKEXSRV_PER_CLIENT_PROCESS_DATA	Process);