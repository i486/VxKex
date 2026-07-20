///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsshut.c
//
// Abstract:
//
//     Contains functions which drive a TLS shutdown.
//
// Author:
//
//     vxiiduu (24-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               24-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Build a Close Notify alert record and put it in the IoInformation.
// Makes no use of the input buffer.
//
SECURITY_STATUS TlsShutdown(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	TLS_ALERT_PAYLOAD AlertRecordPayload;

	ASSERT (Context != NULL);
	ASSERT (Context->State == CONTEXTSTATE_SENDING_CLOSE_NOTIFY);

	AlertRecordPayload.Level = TLS_ALERT_LEVEL_FATAL;
	AlertRecordPayload.Code = TLS_ALERT_CLOSE_NOTIFY;

	if (Context->WriteKeyEnabled) {
		SecStatus = TlsEncrypt(
			Context,
			IoInformation,
			CT_ALERT,
			&AlertRecordPayload,
			sizeof(AlertRecordPayload),
			NULL);
	} else {
		TLS_RECORD_HEADER RecordHeader;

		TlspSerializeRecordHeader(
			&RecordHeader,
			CT_ALERT,
			TLS1_2_PROTOCOL_VERSION,
			sizeof(AlertRecordPayload));

		SecStatus = IoWrite(IoInformation, &RecordHeader, sizeof(RecordHeader));
		SecStatus = IoWrite(IoInformation, &AlertRecordPayload, sizeof(AlertRecordPayload));
	}

	ASSERT (SUCCEEDED(SecStatus));

	Context->State = CONTEXTSTATE_CONNECTION_CLOSED;

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}