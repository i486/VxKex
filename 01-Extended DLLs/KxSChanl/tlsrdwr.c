///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsrdwr.c
//
// Abstract:
//
//     TLS-specific IO information functions.
//
// Author:
//
//     vxiiduu (16-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               16-May-2026  Initial creation.
//     vxiiduu               02-Jun-2026  Move Io* functions into ioinfo.c
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Reads and parses a TLS record header from the given IO information buffer, but
// does not consume any data.
//
// This function also validates the record header. An appropriate error code will
// be returned if the record header is not valid.
//
SECURITY_STATUS TlspPeekRecordHeader(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PTLS_RECORD_HEADER			RecordHeader)
{
	SECURITY_STATUS SecStatus;
	PCTLS_RECORD_HEADER RecordHeaderPointer;

	SecStatus = IoPeek(
		IoInformation,
		(PPCVOID) &RecordHeaderPointer,
		sizeof(TLS_RECORD_HEADER));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	TlsDeserializeRecordHeader(RecordHeaderPointer, RecordHeader);
	SecStatus = TlspValidateRecordHeader(Context, RecordHeader);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}

//
// "Atomically" reads a 4-byte message (not record) header, returning the message
// type and length of data that follows.
//
SECURITY_STATUS TlspReadMessageHeader(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType,
	OUT		PULONG						MessageCb)
{
	SECURITY_STATUS SecStatus;
	PCUCHAR MessageHeader;

	SecStatus = IoRead(IoInformation, (PPCVOID) &MessageHeader, 4);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	*MessageType = MessageHeader[0];
	KexRtlCopyMemory(MessageCb, &MessageHeader[1], 3);
	*MessageCb = ByteSwap24(*MessageCb);

	return SEC_E_OK;
}