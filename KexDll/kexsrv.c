///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsrv.c
//
// Abstract:
//
//     Routines for communicating with KexSrv.
//
// Author:
//
//     vxiiduu (17-Oct-2022)
//
// Revision History:
//
//     vxiiduu              17-Oct-2022  Initial creation.
//     vxiiduu				08-Nov-2022  Add bidirectional support.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI KexSrvOpenChannel(
	OUT	PHANDLE	ChannelHandle) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING PipeName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	if (!ChannelHandle) {
		return STATUS_INVALID_PARAMETER;
	}

	RtlInitConstantUnicodeString(&PipeName, KEXSRV_IPC_CHANNEL_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &PipeName, 0, NULL, NULL);

	Status = KexNtOpenFile(
		ChannelHandle,
		GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if (!NT_SUCCESS(Status)) {
		*ChannelHandle = NULL;
	}

	return Status;
} PROTECTED_FUNCTION_END_NOLOG

KEXAPI NTSTATUS NTAPI KexSrvSendMessage(
	IN	HANDLE				ChannelHandle,
	IN	PKEX_IPC_MESSAGE	Message) PROTECTED_FUNCTION
{
	IO_STATUS_BLOCK IoStatusBlock;

	if (!ChannelHandle) {
		return STATUS_PORT_DISCONNECTED;
	}

	if (!Message) {
		return STATUS_INVALID_PARAMETER;
	}

	return KexNtWriteFile(
		ChannelHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		Message,
		sizeof(KEX_IPC_MESSAGE) + Message->AuxiliaryDataBlockSize,
		NULL,
		NULL);
} PROTECTED_FUNCTION_END_NOLOG

KEXAPI NTSTATUS NTAPI KexSrvSendReceiveMessage(
	IN	HANDLE				ChannelHandle,
	IN	PKEX_IPC_MESSAGE	MessageToSend,
	OUT	PKEX_IPC_MESSAGE	MessageToReceive) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;

	if (!ChannelHandle) {
		return STATUS_PORT_DISCONNECTED;
	}

	if (!MessageToSend || !MessageToReceive) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = NtWriteFile(
		ChannelHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		MessageToSend,
		sizeof(KEX_IPC_MESSAGE) + MessageToSend->AuxiliaryDataBlockSize,
		NULL,
		NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	return NtReadFile(
		ChannelHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		MessageToReceive,
		sizeof(KEX_IPC_MESSAGE) + MessageToReceive->AuxiliaryDataBlockSize,
		NULL,
		NULL);
} PROTECTED_FUNCTION_END

KEXAPI NTSTATUS NTAPI KexSrvNotifyProcessStart(
	IN	HANDLE				ChannelHandle,
	IN	PCUNICODE_STRING	ApplicationName) PROTECTED_FUNCTION
{
	PKEX_IPC_MESSAGE Message;
	UNICODE_STRING DestinationString;

	if (!ChannelHandle) {
		return STATUS_PORT_DISCONNECTED;
	}

	if (!ApplicationName || !ApplicationName->Length || !ApplicationName->Buffer) {
		return STATUS_INVALID_PARAMETER;
	}

	Message = (PKEX_IPC_MESSAGE) StackAlloc(BYTE, sizeof(KEX_IPC_MESSAGE) + ApplicationName->Length);
	Message->MessageId = KexIpcKexProcessStart;
	Message->AuxiliaryDataBlockSize = ApplicationName->Length;
	Message->ProcessStartedInformation.ApplicationNameLength = KexRtlUnicodeStringCch(ApplicationName);

	RtlInitEmptyUnicodeString(
		&DestinationString,
		(PWCHAR) Message->AuxiliaryDataBlock,
		Message->AuxiliaryDataBlockSize);

	RtlCopyUnicodeString(&DestinationString, ApplicationName);

	return KexSrvSendMessage(ChannelHandle, Message);
} PROTECTED_FUNCTION_END