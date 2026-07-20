///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsalert.c
//
// Abstract:
//
//     Functions to deal with TLS alerts.
//
// Author:
//
//     vxiiduu (26-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               26-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Parses an incoming alert from an IoInformation buffer and stores the
// result in the context. Maps the alert to the appropriate SECURITY_STATUS.
//
// Do not include the TLS record header in the input buffer.
//
SECURITY_STATUS TlspProcessIncomingAlert(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	UCHAR AlertLevel;
	UCHAR AlertDescription;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	SecStatus = IoRead8(IoInformation, &AlertLevel);
	SecStatus = IoRead8(IoInformation, &AlertDescription);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	KexLogWarningEvent(
		L"Received incoming alert\r\n\r\n"
		L"Level: %hu\r\n"
		L"Code: %hu",
		AlertLevel,
		AlertDescription);

	//
	// Fatal alerts will cause an immediate connection close.
	// Close notify also causes a connection close.
	//

	if (AlertLevel == TLS_ALERT_LEVEL_FATAL || AlertDescription == TLS_ALERT_CLOSE_NOTIFY) {
		Context->State = CONTEXTSTATE_CONNECTION_CLOSED;
	}

	//
	// Map common alert descriptions to SSPI error codes.
	// https://learn.microsoft.com/en-us/windows/win32/SecAuthN/schannel-error-codes-for-tls-and-ssl-alerts
	//

	switch (AlertDescription) {
	case TLS_ALERT_CLOSE_NOTIFY:
		return SEC_I_CONTEXT_EXPIRED;

	case TLS_ALERT_BAD_RECORD_MAC:
	case TLS_ALERT_DECOMPRESSION_FAILURE:
		return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);

	case TLS_ALERT_DECRYPT_ERROR:
		return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);

	case TLS_ALERT_BAD_CERTIFICATE:
	case TLS_ALERT_UNSUPPORTED_CERT:
	case TLS_ALERT_CERT_UNKNOWN:
		return SP_LOG_RESULT(SEC_E_CERT_UNKNOWN);

	case TLS_ALERT_CERT_REVOKED:
		return CRYPT_E_REVOKED;

	case TLS_ALERT_CERT_EXPIRED:
		return SP_LOG_RESULT(SEC_E_CERT_EXPIRED);

	case TLS_ALERT_UNKNOWN_CA:
		return SP_LOG_RESULT(SEC_E_UNTRUSTED_ROOT);

	case TLS_ALERT_ACCESS_DENIED:
		return SP_LOG_RESULT(SEC_E_LOGON_DENIED);

	case TLS_ALERT_PROTOCOL_VERSION:
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);

	case TLS_ALERT_INSUFFICIENT_SECURITY:
		return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);

	case TLS_ALERT_INTERNAL_ERROR:
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);

	case TLS_ALERT_USER_CANCELED:
		return SP_LOG_RESULT(SEC_E_UNFINISHED_CONTEXT_DELETED);

	default:
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}
}