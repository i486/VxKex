///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsenc.c
//
// Abstract:
//
//     Contains functions which encrypt data to send to the peer.
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
//     vxiiduu               02-Jul-2026  Change to SEC_E_ENCRYPT_FAILURE for
//                                        sequence number overflow
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

STATIC SECURITY_STATUS TlspEncrypt12(
	IN		PCKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						ContentType)
{
	SECURITY_STATUS SecStatus;
	PVOID OutBuffer;
	ULONG OutBufferCb;
	ULONG OutBufferWrittenCb;
	BOOLEAN NeedToUseTlspWrite;

	ASSERT (Context->Credential->SslProvider != 0);

	//
	// Optimization: if the output buffer is empty and isn't going to be
	// dynamically allocated, then we can pass it directly to SslEncryptPacket.
	// This is likely to be the case after the handshake (when EncryptMessage is
	// being called).
	//
	// Otherwise, we'll have to use a temporary buffer and use IoWrite.
	//

	if (IoInformation->OutBufferMustAllocate == FALSE) {
		ASSERT (IoInformation->OutBuffer != NULL);
		ASSERT (IoInformation->OutBufferCb > 0);

		OutBuffer = RVA_TO_VA(
			IoInformation->OutBuffer,
			IoInformation->OutBufferWrittenCb);

		OutBufferCb = IoInformation->OutBufferCb - IoInformation->OutBufferWrittenCb;

		NeedToUseTlspWrite = FALSE;
	} else {
		// 2048 is the max. permissible increase in size from encryption in TLS 1.2.
		// Also add 5 to account for the fact that SslEncryptPacket also writes the
		// record header.
		OutBufferCb = IoInformation->OutBufferCb -
					  IoInformation->OutBufferWrittenCb +
					  2048 + sizeof(TLS_RECORD_HEADER);

		OutBuffer = StackAlloc(BYTE, OutBufferCb);
		NeedToUseTlspWrite = TRUE;
	}

	SecStatus = SslEncryptPacket(
		Context->Credential->SslProvider,
		Context->WriteKey12,
		(PBYTE) IoInformation->InBuffer + IoInformation->InBufferReadCb,
		IoInformation->InBufferCb - IoInformation->InBufferReadCb,
		(PBYTE) OutBuffer,
		OutBufferCb,
		&OutBufferWrittenCb,
		Context->WriteSequenceNumber,
		ContentType,
		0);

	if (FAILED(SecStatus)) {
		// Prevent NTE_* error codes from returning to the caller.
		switch (SecStatus) {
		case NTE_NO_MEMORY:
			SecStatus = SEC_E_INSUFFICIENT_MEMORY;
			break;
		case NTE_BUFFER_TOO_SMALL:
			// Shouldn't have happened with our stack allocation size.
			ASSERT (NeedToUseTlspWrite == FALSE);
			SecStatus = SEC_E_BUFFER_TOO_SMALL;
			break;
		case NTE_BAD_FLAGS:
		case NTE_INVALID_PARAMETER:
		case NTE_INVALID_HANDLE:
		case NTE_NOT_SUPPORTED:
		case NTE_INTERNAL_ERROR:
		case NTE_FAIL:
			// These errors probably mean that our code is at fault.
			ASSERT (FALSE);
			// Fall Through
		default:
			SecStatus = SEC_E_ENCRYPT_FAILURE;
			break;
		}
	}

	if (SUCCEEDED(SecStatus)) {
		// SslEncryptPacket *should* consume all available data.
		IoInformation->InBufferReadCb = IoInformation->InBufferCb;

		if (NeedToUseTlspWrite) {
			// OutBuffer was stack-allocated earlier.
			SecStatus = IoWrite(IoInformation, OutBuffer, OutBufferWrittenCb);
		} else {
			// Still need to give the caller the number of bytes written.
			IoInformation->OutBufferWrittenCb += OutBufferWrittenCb;
		}
	}

	return SecStatus;
}

STATIC SECURITY_STATUS TlspEncrypt13(
    IN		PCKXSCHANL_CONTEXT			Context,
    IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
    IN		UCHAR						ContentType)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BYTE Nonce[12];
	PCVOID Plaintext;
	ULONG PlaintextCb;
	PBYTE InnerPlaintext;
	ULONG InnerPlaintextCb;
	PBYTE Ciphertext;
	ULONG CiphertextCb;
	ULONG CiphertextWrittenCb;
	TLS_RECORD_HEADER RecordHeader;
	BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO AeadInfo;

	ASSERT (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION);
	ASSERT (Context->WriteKey13 != NULL);
	ASSERT (Context->WriteKeyEnabled);

	//
	// Read all available plaintext from the input buffer.
	// TlspEncrypt clamps this to TLS_MAX_PLAINTEXT_CB.
	//

	ASSERT (IoInformation->InBufferCb - IoInformation->InBufferReadCb <=
			TLS_MAX_PLAINTEXT_CB);

	SecStatus = IoRead(
		IoInformation,
		&Plaintext,
		IoInformation->InBufferCb - IoInformation->InBufferReadCb);

	ASSERT (SUCCEEDED(SecStatus));

	PlaintextCb = IoInformation->InBufferReadCb;

	//
	// Build the inner plaintext by appending a content type byte to the input.
	// No zero padding is added.
	//

	InnerPlaintextCb = PlaintextCb + 1;
	InnerPlaintext = StackAlloc(BYTE, InnerPlaintextCb);

	if (PlaintextCb > 0) {
		KexRtlCopyMemory(InnerPlaintext, Plaintext, PlaintextCb);
	}

	InnerPlaintext[InnerPlaintextCb - 1] = ContentType;

	//
	// Build the nonce: WriteKeyIv13 XOR big-endian sequence number.
	// The sequence number is left-padded to 12 bytes with zeroes.
	//

	KexRtlZeroMemory(Nonce, sizeof(Nonce));

	*((PULONGLONG) &Nonce[sizeof(Nonce) - sizeof(ULONGLONG)]) = ByteSwap64(
		Context->WriteSequenceNumber);

	*(PULONG) &Nonce[0] ^= *(PULONG) &Context->WriteKeyIv13[0];
	*(PULONG) &Nonce[4] ^= *(PULONG) &Context->WriteKeyIv13[4];
	*(PULONG) &Nonce[8] ^= *(PULONG) &Context->WriteKeyIv13[8];

	//
	// The output buffer holds the ciphertext followed by the 16-byte tag.
	// We point pbTag to the last 16 bytes so the tag is written directly
	// after the ciphertext, making the whole buffer contiguous.
	//

	CiphertextCb = InnerPlaintextCb + 16;
	Ciphertext = StackAlloc(BYTE, CiphertextCb);

	//
	// Build the record header.
	//

	TlspSerializeRecordHeader(
		&RecordHeader,
		CT_APPLICATIONDATA,
		TLS1_2_PROTOCOL_VERSION,
		(USHORT) CiphertextCb);

	//
	// Set up the AEAD info. AuthData is the raw big-endian record header.
	// pbTag points to the last 16 bytes of the Ciphertext buffer so the
	// final output is [ciphertext][tag] in one contiguous block.
	//

	BCRYPT_INIT_AUTH_MODE_INFO(AeadInfo);
	AeadInfo.pbNonce		= Nonce;
	AeadInfo.cbNonce		= sizeof(Nonce);
	AeadInfo.pbTag			= Ciphertext + InnerPlaintextCb;
	AeadInfo.cbTag			= 16;
	AeadInfo.pbAuthData		= (PBYTE) &RecordHeader;
	AeadInfo.cbAuthData		= sizeof(RecordHeader);

	//
	// Encrypt. BCrypt writes the ciphertext to pbOutput and the 16-byte
	// authentication tag to pbTag (which is the tail of the same buffer).
	//

	Status = BCryptEncrypt(
		Context->WriteKey13,
		(PUCHAR) InnerPlaintext,
		InnerPlaintextCb,
		&AeadInfo,
		NULL,
		0,
		Ciphertext,
		InnerPlaintextCb,
		&CiphertextWrittenCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (CiphertextWrittenCb == InnerPlaintextCb);

	if (!NT_SUCCESS(Status)) {
		switch (Status) {
		case STATUS_BUFFER_TOO_SMALL:
			return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
		case STATUS_NOT_SUPPORTED:
		case STATUS_INVALID_PARAMETER:
		case STATUS_INVALID_HANDLE:
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		case STATUS_INVALID_BUFFER_SIZE:
		default:
			return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
		}
	}

	//
	// Write the record header and the ciphertext+tag to the output buffer.
	// CiphertextCb covers both: InnerPlaintextCb bytes of ciphertext plus
	// 16 bytes of tag that BCrypt wrote to pbTag.
	//

	SecStatus = IoWrite(IoInformation, &RecordHeader, sizeof(RecordHeader));
	SecStatus = IoWrite(IoInformation, Ciphertext, CiphertextCb);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}

//
// The encrypted data is always written to the output buffer of IoInformation
// using IoWrite. A complete TLS record is written, including the header.
//
// If InBuffer, InBufferCb and InBufferReadCb are specified, the plaintext
// data is read from that buffer, and the number of consumed bytes is written
// to InBufferReadCb.
//
// Otherwise, the plaintext is read from the input buffer of IoInformation using
// IoRead.
//
// A maximum of TLS_MAX_PLAINTEXT_CB bytes are read from the input buffer.
//
// The buffer override parameters are intended for internally generated data;
// in other words, for handshake data generated by the KxSChanl library during
// InitializeSecurityContext. To encrypt caller-provided data, specify NULL
// for InBuffer, and IoInformation->InBuffer will be used instead.
//
SECURITY_STATUS TlsEncrypt(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						ContentType,
	IN		PCVOID						InBuffer OPTIONAL,
	IN		ULONG						InBufferCb,
	OUT		PULONG						InBufferReadCb OPTIONAL)
{
	SECURITY_STATUS SecStatus;
	BOOLEAN InBufferOverride;
	KXSCHANL_IO_INFORMATION IoInformationTemp;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	IoInformationTemp = *IoInformation;

	//
	// Handle the buffer override: if input buffers are specified, we'll
	// put those buffers into IoInformationTemp.
	//

	if (InBuffer != NULL && InBufferCb > 0) {
		IoInformationTemp.InBuffer		= InBuffer;
		IoInformationTemp.InBufferCb	= InBufferCb;

		InBufferOverride = TRUE;
	} else {
		IoInformationTemp.InBuffer = RVA_TO_VA(
			IoInformation->InBuffer,
			IoInformation->InBufferReadCb);

		IoInformationTemp.InBufferCb = IoInformation->InBufferCb -
									   IoInformation->InBufferReadCb;

		InBufferOverride = FALSE;
	}

	IoInformationTemp.InBufferReadCb = 0;

	//
	// Clamp the input data length to the maximum TLS record payload size.
	//

	IoInformationTemp.InBufferCb = min(
		IoInformationTemp.InBufferCb,
		TLS_MAX_PLAINTEXT_CB);

	//
	// Call the appropriate sub-function depending on the negotiated TLS
	// protocol version.
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		SecStatus = TlspEncrypt12(Context, &IoInformationTemp, ContentType);
		break;
	case TLS1_3_PROTOCOL_VERSION:
		SecStatus = TlspEncrypt13(Context, &IoInformationTemp, ContentType);
		break;
	default:
		NOT_REACHED;
	}

	//
	// Update sequence number if successful.
	//

	if (SUCCEEDED(SecStatus)) {
		if (Context->WriteSequenceNumber >= 0x1000000000000LL) {
			// eliminate any possibility of sequence number wrap-over
			return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
		}

		++Context->WriteSequenceNumber;
	}

	//
	// Let the caller know how much data was read or written.
	//

	IoInformation->OutBuffer			= IoInformationTemp.OutBuffer;
	IoInformation->OutBufferCb			= IoInformationTemp.OutBufferCb;
	IoInformation->OutBufferWrittenCb	= IoInformationTemp.OutBufferWrittenCb;

	if (InBufferOverride) {
		if (InBufferReadCb) {
			*InBufferReadCb = IoInformationTemp.InBufferReadCb;
		}
	} else {
		IoInformation->InBufferReadCb += IoInformationTemp.InBufferReadCb;
	}

	return SecStatus;
}