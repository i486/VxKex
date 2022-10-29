///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsrvp.h
//
// Abstract:
//
//     Private header for KexSrv internal functions.
//
// Author:
//
//     vxiiduu (02-Oct-2022)
//
// Environment:
//
//     KexSrv only.
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexLog.h>

#ifdef _DEBUG
#  define FRIENDLYAPPNAME L"VxKex Local Server (DEBUG BUILD)"
#else
#  define FRIENDLYAPPNAME L"VxKex Local Server"
#endif

#define KexSrvGlobalLog(Severity, ...) VxlLog(GlobalLogHandle, L"KexSrv", Severity, __VA_ARGS__)

typedef struct _KEXSRV_PER_CLIENT_INFO {
	VXLHANDLE LogHandle;
	ULONG ProcessId;
	WCHAR LogFilePath[MAX_PATH];
	WCHAR ApplicationName[MAX_PATH];

	union {
		BYTE MessageBuffer[0xFFFF];
		KEX_IPC_MESSAGE Message;
	};
} KEXSRV_PER_CLIENT_INFO, *PKEXSRV_PER_CLIENT_INFO;

extern VXLHANDLE GlobalLogHandle;

NTSTATUS NTAPI KexSrvHandleClientThreadProc(
	IN	HANDLE	PipeHandle);

NTSTATUS KexSrvDispatchMessage(
	IN OUT	PKEXSRV_PER_CLIENT_INFO ClientInfo);

VXLHANDLE KexSrvOpenLogFile(
	IN	PCWSTR	KexProgramName OPTIONAL,
	IN	ULONG	LogFilePathBufferCch OPTIONAL,
	OUT	PWSTR	LogFilePathOut OPTIONAL);

VOID KexSrvDumpMessageToLog(
	IN	PKEXSRV_PER_CLIENT_INFO	ClientInfo);

VOID KexSrvpHardError(
	IN	PKEXSRV_PER_CLIENT_INFO	ClientInfo,
	IN	NTSTATUS				Status,
	IN	ULONG					UlongParameter,
	IN	PCWSTR					StringParameter1,
	IN	PCWSTR					StringParameter2);