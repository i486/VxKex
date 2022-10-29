///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexSrv.h
//
// Abstract:
//
//     Contains structure, enum and type definitions useful for sending
//     messages to the VxKex server process.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Environment:
//
//     Any.
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexTypes.h>

#define KEXSRV_IPC_CHANNEL_NAME L"\\??\\pipe\\KexSrvIpcChannel"

typedef enum _KEX_IPC_MESSAGE_ID {
	KexIpcKexProcessStart,
	KexIpcHardError,
	KexIpcLogEvent,
	KexIpcMaximumMessageId
} KEX_IPC_MESSAGE_ID;

typedef struct _KEX_IPC_MESSAGE_DATA_KEX_PROCESS_STARTED {
	USHORT		ApplicationNameLength;
} TYPEDEF_TYPE_NAME(KEX_IPC_MESSAGE_DATA_PROCESS_STARTED);

typedef struct _KEX_IPC_MESSAGE_DATA_HARD_ERROR {
	NTSTATUS	Status;
	ULONG		UlongParameter;
	USHORT		StringParameter1Length;
	USHORT		StringParameter2Length;
} TYPEDEF_TYPE_NAME(KEX_IPC_MESSAGE_DATA_HARD_ERROR);

typedef struct _KEX_IPC_MESSAGE_DATA_LOG_EVENT {
	ULONG		Severity;
	ULONG		SourceLine;
	USHORT		SourceComponentLength;
	USHORT		SourceFileLength;
	USHORT		SourceFunctionLength;
	USHORT		TextLength;
} TYPEDEF_TYPE_NAME(KEX_IPC_MESSAGE_DATA_LOG_EVENT);

//
// The maximum size of a message is 64KB.
//
typedef struct _KEX_IPC_MESSAGE {
	KEX_IPC_MESSAGE_ID		MessageId;
	USHORT					AuxiliaryDataBlockSize;

	union {
		// Client->Server Messages
		KEX_IPC_MESSAGE_DATA_PROCESS_STARTED	ProcessStartedInformation;
		KEX_IPC_MESSAGE_DATA_HARD_ERROR			HardErrorInformation;
		KEX_IPC_MESSAGE_DATA_LOG_EVENT			LogEventInformation;
	};

	BYTE					AuxiliaryDataBlock[];
} TYPEDEF_TYPE_NAME(KEX_IPC_MESSAGE);