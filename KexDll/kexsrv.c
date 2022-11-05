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
//     vxiiduu				
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"
#include <KexLog.h>

KEXAPI NTSTATUS NTAPI KexSrvOpenChannel(
	OUT	PHANDLE	ChannelHandle) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING PipeName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	RtlInitConstantUnicodeString(&PipeName, KEXSRV_IPC_CHANNEL_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &PipeName, 0, NULL, NULL);

	Status = KexNtOpenFile(
		ChannelHandle,
		GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_WRITE,
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

KEXAPI NTSTATUS NTAPI KexSrvNotifyProcessStart(
	IN	HANDLE				ChannelHandle,
	IN	PCUNICODE_STRING	ApplicationName) PROTECTED_FUNCTION
{
	PKEX_IPC_MESSAGE Message;
	UNICODE_STRING DestinationString;

	if (!ChannelHandle) {
		return STATUS_PORT_DISCONNECTED;
	}

	if (!ApplicationName->Buffer) {
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

KEXAPI NTSTATUS CDECL KexSrvLogEventEx(
	IN	HANDLE		ChannelHandle,
	IN	VXLSEVERITY	Severity,
	IN	ULONG		SourceLine,
	IN	PCWSTR		SourceComponent,
	IN	PCWSTR		SourceFile,
	IN	PCWSTR		SourceFunction,
	IN	PCWSTR		Format,
	IN	...) PROTECTED_FUNCTION
{
	PKEX_IPC_MESSAGE Message;

	HRESULT Result;
	ARGLIST ArgList;
	ULONG MessageBufferCb;

	ULONG SourceComponentCch;
	ULONG SourceFileCch;
	ULONG SourceFunctionCch;
	SIZE_T TextCch;

	PWSTR SourceComponentOut;
	PWSTR SourceFileOut;
	PWSTR SourceFunctionOut;
	PWSTR TextOut;

	if (!ChannelHandle) {
		return STATUS_PORT_DISCONNECTED;
	}

	//
	// Validate pointer parameters
	//

	if (!SourceComponent) {
		return STATUS_INVALID_PARAMETER_4;
	}

	if (!SourceFile) {
		return STATUS_INVALID_PARAMETER_5;
	}

	if (!SourceFunction) {
		return STATUS_INVALID_PARAMETER_6;
	}

	if (!Format) {
		return STATUS_INVALID_PARAMETER_7;
	}

	va_start(ArgList, Format);

	//
	// Find out how many bytes the message buffer has to be, and then
	// allocate the correct sized buffer.
	//

	Result = StringCchVPrintfBufferLength(
		&TextCch,
		Format,
		ArgList);

	if (FAILED(Result)) {
		return STATUS_UNSUCCESSFUL;
	}

	SourceComponentCch = (ULONG) wcslen(SourceComponent);
	SourceFileCch = (ULONG) wcslen(SourceFile);
	SourceFunctionCch = (ULONG) wcslen(SourceFunction);

	MessageBufferCb = sizeof(KEX_IPC_MESSAGE);
	MessageBufferCb += SourceComponentCch * sizeof(WCHAR);
	MessageBufferCb += SourceFileCch * sizeof(WCHAR);
	MessageBufferCb += SourceFunctionCch * sizeof(WCHAR);
	MessageBufferCb += (ULONG) TextCch * sizeof(WCHAR);

	Message = (PKEX_IPC_MESSAGE) StackAlloc(BYTE, MessageBufferCb);

	//
	// Fill out the KEX_IPC_MESSAGE structure.
	//
	
	Message->MessageId = KexIpcLogEvent;
	Message->AuxiliaryDataBlockSize = (USHORT) (MessageBufferCb - sizeof(KEX_IPC_MESSAGE));
	Message->LogEventInformation.Severity = Severity;
	Message->LogEventInformation.SourceLine = SourceLine;
	Message->LogEventInformation.SourceComponentLength = (USHORT) SourceComponentCch;
	Message->LogEventInformation.SourceFileLength = (USHORT) SourceFileCch;
	Message->LogEventInformation.SourceFunctionLength = (USHORT) SourceFunctionCch;
	Message->LogEventInformation.TextLength = (USHORT) TextCch;

	//
	// Copy or format the strings into the buffer.
	//

	SourceComponentOut = (PWSTR) Message->AuxiliaryDataBlock;
	SourceFileOut = SourceComponentOut + SourceComponentCch;
	SourceFunctionOut = SourceFileOut + SourceFileCch;
	TextOut = SourceFunctionOut + SourceFunctionCch;

	RtlCopyMemory(SourceComponentOut, SourceComponent, SourceComponentCch * sizeof(WCHAR));
	RtlCopyMemory(SourceFileOut, SourceFile, SourceFileCch * sizeof(WCHAR));
	RtlCopyMemory(SourceFunctionOut, SourceFunction, SourceFunctionCch * sizeof(WCHAR));
	
	Result = StringCchVPrintf(
		TextOut,
		TextCch,
		Format,
		ArgList);

	if (FAILED(Result)) {
		return STATUS_UNSUCCESSFUL;
	}

	va_end(ArgList);

	//
	// Send the message.
	//

	return KexSrvSendMessage(ChannelHandle, Message);
} PROTECTED_FUNCTION_END_NOLOG