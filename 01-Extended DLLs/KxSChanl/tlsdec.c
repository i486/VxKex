///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsdec.c
//
// Abstract:
//
//     Contains functions which decrypt data from the peer.
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
//     vxiiduu               02-Jul-2026  Change to SEC_E_DECRYPT_FAILURE for
//                                        sequence number overflow
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

STATIC SECURITY_STATUS TlspDecrypt12(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCTLS_RECORD_HEADER			RecordHeader,
	OUT		PUCHAR						ContentType)
{
	SECURITY_STATUS SecStatus;
	ULONG OutBufferWrittenCb;

	ASSERT (Context->Credential->SslProvider != 0);
	ASSERT (IoInformation->InBufferReadCb == 0);
	ASSERT (IoInformation->OutBufferMustAllocate == FALSE);

	SecStatus = SslDecryptPacket(
		Context->Credential->SslProvider,
		Context->ReadKey12,
		(PBYTE) IoInformation->InBuffer + IoInformation->InBufferReadCb,
		IoInformation->InBufferCb - IoInformation->InBufferReadCb,
		(PBYTE) IoInformation->OutBuffer + IoInformation->OutBufferWrittenCb,
		IoInformation->OutBufferCb - IoInformation->OutBufferWrittenCb,
		&OutBufferWrittenCb,
		Context->ReadSequenceNumber,
		0);

	// Map NTE_* status to SEC_* status.
	if (FAILED(SecStatus)) {
		switch (SecStatus) {
		case NTE_BUFFER_TOO_SMALL:
			return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
		case NTE_BUFFERS_OVERLAP:
		case NTE_BAD_FLAGS:
		case NTE_INVALID_PARAMETER:
		case NTE_INVALID_HANDLE:
		case NTE_NOT_SUPPORTED:
		case NTE_INTERNAL_ERROR:
		case NTE_FAIL:
			// These errors probably mean that our code is at fault.
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		case NTE_BAD_DATA:
		case NTE_DECRYPTION_FAILURE:
		default:
			return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
			break;
		}
	}

	IoInformation->OutBufferWrittenCb += OutBufferWrittenCb;
	*ContentType = RecordHeader->Type;

	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspDecrypt13(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCTLS_RECORD_HEADER			RecordHeader,
	OUT		PUCHAR						ContentType)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	ULONG OutBufferWrittenCb;
	BYTE Nonce[12];
	PCVOID Ciphertext;
	ULONG CiphertextCb;
	PCVOID Tag;
	ULONG TagCb;
	PBYTE Plaintext;
	BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AeadInfo;
	PCTLS_RECORD_HEADER RawRecordHeader;

	ASSERT (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION);
	ASSERT (Context->ReadKey13 != NULL);
	ASSERT (Context->ReadKeyEnabled);

	//
	// Skip past the record header.
	// We'll keep a pointer to the raw (big endian) record header for later.
	//

	SecStatus = IoRead(IoInformation, (PPCVOID) &RawRecordHeader, sizeof(TLS_RECORD_HEADER));
	ASSERT (SUCCEEDED(SecStatus));

	//
	// Build the NONCE data, which is the sequence number XOR'd with the read key
	// IV. The sequence number is converted to big endian.
	//

	KexRtlZeroMemory(Nonce, sizeof(Nonce));
	
	*((PULONGLONG) &Nonce[sizeof(Nonce) - sizeof(ULONGLONG)]) = ByteSwap64(
		Context->ReadSequenceNumber);

	*(PULONG) &Nonce[0] ^= *(PULONG) &Context->ReadKeyIv13[0];
	*(PULONG) &Nonce[4] ^= *(PULONG) &Context->ReadKeyIv13[4];
	*(PULONG) &Nonce[8] ^= *(PULONG) &Context->ReadKeyIv13[8];

	//
	// The ciphertext is the first N-16 bytes of record data, where N is
	// the length of the record payload.
	// If the record payload is less than 16 bytes, it's invalid.
	//

	if (RecordHeader->DataCb < 16) {
		return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
	}

	CiphertextCb = RecordHeader->DataCb - 16;

	SecStatus = IoRead(IoInformation, &Ciphertext, CiphertextCb);
	ASSERT (SUCCEEDED(SecStatus));

	//
	// The tag is the last 16 bytes of the record data, for AES cipher suites.
	//

	TagCb = 16;
	
	SecStatus = IoRead(IoInformation, &Tag, TagCb);
	ASSERT (SUCCEEDED(SecStatus));
	ASSERT (IoInformation->InBufferReadCb == IoInformation->InBufferCb);

	//
	// Decrypt.
	// The "additional data" (AD in AEAD) is the *raw* record header (that has
	// not been byte swapped).
	//

	BCRYPT_INIT_AUTH_MODE_INFO(AeadInfo);
	AeadInfo.pbNonce = Nonce;
	AeadInfo.cbNonce = sizeof(Nonce);
	AeadInfo.pbTag = (PUCHAR) Tag;
	AeadInfo.cbTag = TagCb;
	AeadInfo.pbAuthData = (PBYTE) RawRecordHeader;
	AeadInfo.cbAuthData = sizeof(TLS_RECORD_HEADER);

	Plaintext = (PBYTE) RVA_TO_VA(
		IoInformation->OutBuffer,
		IoInformation->OutBufferWrittenCb);

	Status = BCryptDecrypt(
		Context->ReadKey13,
		(PUCHAR) Ciphertext,
		CiphertextCb,
		&AeadInfo,
		NULL,
		0,
		Plaintext,
		IoInformation->OutBufferCb - IoInformation->OutBufferWrittenCb,
		&OutBufferWrittenCb,
		0);
	
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		switch (Status) {
		case STATUS_BUFFER_TOO_SMALL:
			return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
		case STATUS_AUTH_TAG_MISMATCH:
			return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
		case STATUS_NOT_SUPPORTED:
		case STATUS_INVALID_PARAMETER:
		case STATUS_INVALID_HANDLE:
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		case STATUS_INVALID_BUFFER_SIZE:
		default:
			return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
		}
	}

	//
	// Remove the padding and the content-type byte. Pass the content-type to
	// the caller. Shrink OutBufferWrittenCb as appropriate - at the end of the
	// loop, OutBufferWrittenCb should not include the content-type or any
	// padding.
	//
	// The padding is optional. If present, it will take the form of a number
	// of zeroes at the end of the plaintext.
	//
	// The content type byte (CT_*) precedes the padding.
	//

	while (TRUE) {
		if (OutBufferWrittenCb == 0) {
			// Entire buffer consisted of zeroes. Invalid data.
			return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
		}

		--OutBufferWrittenCb;

		if (Plaintext[OutBufferWrittenCb] != 0x00) {
			// We found the content type byte.
			*ContentType = Plaintext[OutBufferWrittenCb];
			break;
		}
	}

	//
	// Validate the inner content-type byte. Only CT_APPLICATIONDATA, CT_ALERT,
	// and CT_HANDSHAKE may be sent as encrypted TLS 1.3 records.
	//

	switch (*ContentType) {
	case CT_APPLICATIONDATA:
	case CT_ALERT:
	case CT_HANDSHAKE:
		// OK
		break;
	default:
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Tell caller how much output data was produced.
	//

	IoInformation->OutBufferWrittenCb += OutBufferWrittenCb;
	
	return SEC_E_OK;
}

//
// Encrypted data is always read from the input buffer of IoInformation.
// The output buffer is the output buffer of IoInformation by default, but it
// can be overridden by specifying the OutBuffer parameter.
//
// See the comment above TlsEncrypt - same idea.
//
// This function will consume a single whole record from the input buffer.
//
// If this function receives an alert from the server, the alert will be
// processed.
//
// The ContentType output parameter contains the "real" content type of the
// record. Callers must use this output parameter instead of manually reading
// the TLS record header because the record header in TLS 1.3 is fake.
//
SECURITY_STATUS TlsDecrypt(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						ContentType,
	OUT		PVOID						OutBuffer OPTIONAL,
	IN		ULONG						OutBufferCb,
	OUT		PULONG						OutBufferWrittenCb OPTIONAL)
{
	SECURITY_STATUS SecStatus;
	BOOLEAN OutBufferOverride;
	TLS_RECORD_HEADER RecordHeader;
	KXSCHANL_IO_INFORMATION IoInformationTemp;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);
	ASSERT (OutBuffer || IoInformation->OutBufferMustAllocate == FALSE);

	*ContentType = 0;

	if (OutBufferWrittenCb) {
		*OutBufferWrittenCb = 0;
	}

	//
	// Set up an IO information structure which describes a single complete
	// record.
	//

	SecStatus = TlspPeekRecordHeader(Context, IoInformation, &RecordHeader);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	IoInformationTemp = *IoInformation;
	IoInformationTemp.InBufferCb = sizeof(TLS_RECORD_HEADER) + RecordHeader.DataCb;
	IoInformationTemp.InBufferReadCb = 0;

	SecStatus = IoRead(
		IoInformation,
		&IoInformationTemp.InBuffer,
		IoInformationTemp.InBufferCb);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Handle buffer override.
	//

	if (OutBuffer != NULL && OutBufferCb > 0) {
		IoInformationTemp.OutBuffer				= OutBuffer;
		IoInformationTemp.OutBufferCb			= OutBufferCb;
		IoInformationTemp.OutBufferWrittenCb	= 0;
		IoInformationTemp.OutBufferMustAllocate	= FALSE;

		OutBufferOverride = TRUE;
	} else {
		OutBufferOverride = FALSE;
	}

	//
	// Call appropriate sub-function for the TLS protocol version in use.
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		SecStatus = TlspDecrypt12(
			Context,
			&IoInformationTemp,
			&RecordHeader,
			ContentType);

		break;
	case TLS1_3_PROTOCOL_VERSION:
		SecStatus = TlspDecrypt13(
			Context,
			&IoInformationTemp,
			&RecordHeader,
			ContentType);

		break;
	default:
		NOT_REACHED;
	}

	//
	// Update sequence number if successful.
	//

	if (SUCCEEDED(SecStatus)) {
		if (Context->ReadSequenceNumber >= 0x1000000000000LL) {
			// equivalent to over 4 million terabytes of data...
			// eliminate any possibility of sequence number wrap-over
			return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
		}

		++Context->ReadSequenceNumber;
	}

	//
	// If we got an alert, process the alert.
	//

	if (*ContentType == CT_ALERT) {
		KXSCHANL_IO_INFORMATION AlertIoInformation;

		KexRtlZeroMemory(&AlertIoInformation, sizeof(AlertIoInformation));
		AlertIoInformation.InBuffer = IoInformationTemp.OutBuffer;
		AlertIoInformation.InBufferCb = IoInformationTemp.OutBufferWrittenCb;

		return TlspProcessIncomingAlert(Context, &AlertIoInformation);
	}

	//
	// Inform the caller of how much was written to the output.
	//

	if (OutBufferOverride) {
		*OutBufferWrittenCb = IoInformationTemp.OutBufferWrittenCb;
	} else {
		IoInformation->OutBufferWrittenCb = IoInformationTemp.OutBufferWrittenCb;
	}

	return SecStatus;
}