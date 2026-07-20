///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     encdec.c
//
// Abstract:
//
//     Contains the implementation of EncryptMessage and DecryptMessage.
//
// Author:
//
//     vxiiduu (10-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               10-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

SECURITY_STATUS SppEncryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN		ULONG			QualityOfProtectionFlags,
	IN OUT	PSecBufferDesc	Message)
{
	SECURITY_STATUS SecStatus;
	PKXSCHANL_CONTEXT Context;
	KXSCHANL_IO_INFORMATION IoInformation;
	PSecBuffer HeaderSecBuffer;
	PSecBuffer DataSecBuffer;
	PSecBuffer TrailerSecBuffer;
	ULONG Index;
	BOOLEAN BuffersContiguous;

	ASSERT (Message != NULL);

	Context = SppReadContextHandle(ContextHandle);

	if (Context->State != CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
	}

	//
	// The Message parameter should contain 4 buffers, 3 of which we are going
	// to use at the moment.
	//   - SECBUFFER_STREAM_HEADER, uninitialized buffer with size >= 5 for the
	//     TLS header.
	//   - SECBUFFER_DATA, initialized buffer containing the data to be encrypted.
	//   - SECBUFFER_STREAM_TRAILER, uninitialized buffer with a size according
	//     to the trailer size of the current cipher suits, for the padding and
	//     the MAC code.
	//   - SECBUFFER_EMPTY, uninitialized buffer which we don't use. Apparently
	//     Schannel uses it for renegotiation but we don't support renegotiation.
	//
	// Here is some information for what Schannel does when you pass buffers that
	// don't match the above description.
	//
	//   - If you don't have SECBUFFER_DATA (e.g. it's a SECBUFFER_EMPTY), then
	//     you get SEC_E_INVALID_TOKEN.
	//   - If you don't have a SECBUFFER_STREAM_TRAILER, or if you don't have a
	//     SECBUFFER_STREAM_HEADER, then you get SEC_E_ENCRYPT_FAILURE.
	//   - If you don't have a SECBUFFER_EMPTY, then there is no error.
	//   - If you have a SECBUFFER_TOKEN buffer, then it gets used as either a
	//     header or trailer buffer, depending on whether there was already a
	//     header or trailer buffer that came before it.
	//   - If your SECBUFFER_DATA buffer is more than 16384 bytes, then you get
	//     SEC_E_ENCRYPT_FAILURE.
	//   - If your SECBUFFER_STREAM_HEADER is less than the required header size,
	//     then you get SEC_E_ENCRYPT_FAILURE.
	//   - If your SECBUFFER_STREAM_TRAILER is less than the required trailer size,
	//     then you also get SEC_E_ENCRYPT_FAILURE.
	//
	// Schannel appears to use the same kind of for-loop buffer finding logic
	// as InitializeSecurityContext, where the *last* buffer of a given type is
	// used. So for example if you have two SECBUFFER_STREAM_HEADERs, the first
	// of which is valid and the second of which is invalid (e.g. has wrong size),
	// then it uses the second invalid one and gives you SEC_E_ENCRYPT_FAILURE.
	//

	HeaderSecBuffer = NULL;
	DataSecBuffer = NULL;
	TrailerSecBuffer = NULL;

	for (Index = 0; Index < Message->cBuffers; ++Index) {
		switch (Message->pBuffers[Index].BufferType) {
		case SECBUFFER_STREAM_HEADER:
			HeaderSecBuffer = &Message->pBuffers[Index];
			break;
		case SECBUFFER_DATA:
			DataSecBuffer = &Message->pBuffers[Index];
			break;
		case SECBUFFER_STREAM_TRAILER:
			TrailerSecBuffer = &Message->pBuffers[Index];
			break;
		case SECBUFFER_TOKEN:
			if (HeaderSecBuffer == NULL) {
				HeaderSecBuffer = &Message->pBuffers[Index];
			} else if (TrailerSecBuffer == NULL) {
				TrailerSecBuffer = &Message->pBuffers[Index];
			}
			break;
		}
	}

	if (!HeaderSecBuffer || !TrailerSecBuffer) {
		return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
	}

	if (!DataSecBuffer) {
		return SP_LOG_RESULT(SEC_E_INVALID_TOKEN);
	}

	if (HeaderSecBuffer->cbBuffer < Context->HeaderSize) {
		return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
	}

	if (DataSecBuffer->cbBuffer > TLS_MAX_PLAINTEXT_CB) {
		return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
	}

	if (TrailerSecBuffer->cbBuffer < Context->TrailerSize) {
		return SP_LOG_RESULT(SEC_E_ENCRYPT_FAILURE);
	}

	//
	// Find out if the caller-supplied buffers are contiguous. If so,
	// then we don't need to allocate any temporary memory, which is great.
	// Microsoft's documentation recommends that the buffers are contiguous.
	//
	// Note: if the SECBUFFER_STREAM_HEADER size is not exactly the needed
	// size then we have to consider the buffers discontiguous because
	// otherwise there'll be a gap.
	//

	if (RVA_TO_VA(HeaderSecBuffer->pvBuffer, HeaderSecBuffer->cbBuffer) == DataSecBuffer->pvBuffer &&
		RVA_TO_VA(DataSecBuffer->pvBuffer, DataSecBuffer->cbBuffer) == TrailerSecBuffer->pvBuffer) {

		BuffersContiguous = TRUE;
	} else {
		BuffersContiguous = FALSE;
	}

	//
	// Set up IoInformation structure for TlsEncrypt.
	//

	KexRtlZeroMemory(&IoInformation, sizeof(IoInformation));

	IoInformation.InBuffer = DataSecBuffer->pvBuffer;
	IoInformation.InBufferCb = DataSecBuffer->cbBuffer;

	// We use our own header size value and not the caller provided value
	// in case the caller buffer is larger than what we need - in which
	// case we will later have to shift the buffer.
	IoInformation.OutBufferCb = Context->HeaderSize +
								DataSecBuffer->cbBuffer +
								TrailerSecBuffer->cbBuffer;

	if (BuffersContiguous) {
		IoInformation.OutBuffer = HeaderSecBuffer->pvBuffer;
	} else {
		IoInformation.OutBuffer = StackAlloc(BYTE, IoInformation.OutBufferCb);
	}

	SecStatus = TlsEncrypt(
		Context,
		&IoInformation,
		CT_APPLICATIONDATA,
		NULL,
		0,
		NULL);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Update header size and trailer size based on the actual amount of encrypted
	// data generated by the TLS component.
	//

	HeaderSecBuffer->cbBuffer = Context->HeaderSize;

	TrailerSecBuffer->cbBuffer = IoInformation.OutBufferWrittenCb -
								 (Context->HeaderSize + DataSecBuffer->cbBuffer);

	//
	// If we allocated a temporary buffer due to discontiguity of the caller's
	// supplied buffers, then we now need to copy all the data back into the
	// caller's buffers into the right places.
	//

	if (!BuffersContiguous) {
		PBYTE Header;
		PBYTE Data;
		PBYTE Trailer;

		Header = (PBYTE) IoInformation.OutBuffer;
		Data = Header + Context->HeaderSize;
		Trailer = Data + DataSecBuffer->cbBuffer;

		KexRtlCopyMemory(
			HeaderSecBuffer->pvBuffer,
			Header,
			HeaderSecBuffer->cbBuffer);

		KexRtlCopyMemory(
			DataSecBuffer->pvBuffer,
			Data,
			DataSecBuffer->cbBuffer);

		KexRtlCopyMemory(
			TrailerSecBuffer->pvBuffer,
			Trailer,
			TrailerSecBuffer->cbBuffer);
	}

	//
	// However, if we directly wrote into the caller's buffers and the caller's
	// supplied HeaderSecBuffer size is larger than what we need, we need to shift
	// all the contents of the buffer...
	//

	if (BuffersContiguous && HeaderSecBuffer->cbBuffer > Context->HeaderSize) {
		KexLogDebugEvent(L"Caller supplied an oversized SECBUFFER_STREAM_HEADER");
		KexDebugCheckpoint();
		RtlMoveMemory(
			DataSecBuffer->pvBuffer,
			RVA_TO_VA(HeaderSecBuffer->pvBuffer, Context->HeaderSize),
			DataSecBuffer->cbBuffer + TrailerSecBuffer->cbBuffer);
	}

	return SEC_E_OK;
}

SECURITY_STATUS SppDecryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN OUT	PSecBufferDesc	Message)
{
	SECURITY_STATUS SecStatus;
	PKXSCHANL_CONTEXT Context;
	KXSCHANL_IO_INFORMATION IoInformation;
	UCHAR ContentType;
	ULONG Index;
	PSecBuffer CiphertextSecBuffer;
	PSecBuffer HeaderSecBuffer;
	PSecBuffer PlaintextSecBuffer;
	PSecBuffer TrailerSecBuffer;
	PSecBuffer ExtraSecBuffer;
	TLS_RECORD_HEADER TlsHeader;
	ULONG TlsDataLengthExcludingIV;
	
	Context = SppReadContextHandle(ContextHandle);

	if (Context->State != CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
	}

	//
	// The caller passes in 4 buffers. One of them is SECBUFFER_DATA (the ciphertext)
	// and the other 3 are SECBUFFER_EMPTY.
	//
	// If we get less than 4 buffers, we must return SEC_E_INVALID_TOKEN. This is
	// what Schannel does.
	//
	// If we don't have a SECBUFFER_DATA, we must also return SEC_E_INVALID_TOKEN.
	//
	// If the SECBUFFER_DATA was too small, we return SEC_E_INCOMPLETE_MESSAGE,
	// and set both the plaintext buffer and the header buffer to SECBUFFER_MISSING
	// (with both cbBuffer's set to the number of additional missing bytes).
	//
	// Upon a successful return, the buffer that was SECBUFFER_DATA becomes a
	// SECBUFFER_STREAM_HEADER, the first available SECBUFFER_EMPTY becomes a
	// SECBUFFER_DATA (containing the plaintext), and the next available
	// SECBUFFER_EMPTY becomes a SECBUFFER_STREAM_TRAILER.
	//
	// The final available SECBUFFER_EMPTY becomes a SECBUFFER_EXTRA, only IF the input
	// ciphertext was too large and we didn't use all of it.
	//
	// The cbBuffer members are all updated as well of course.
	//

	CiphertextSecBuffer = NULL;
	HeaderSecBuffer = NULL;
	PlaintextSecBuffer = NULL;
	TrailerSecBuffer = NULL;
	ExtraSecBuffer = NULL;

	for (Index = 0; Index < Message->cBuffers; ++Index) {
		switch (Message->pBuffers[Index].BufferType) {
		case SECBUFFER_DATA:
			CiphertextSecBuffer = &Message->pBuffers[Index];
			HeaderSecBuffer = &Message->pBuffers[Index];
			break;
		case SECBUFFER_EMPTY:
			if (PlaintextSecBuffer == NULL) {
				PlaintextSecBuffer = &Message->pBuffers[Index];
			} else if (TrailerSecBuffer == NULL) {
				TrailerSecBuffer = &Message->pBuffers[Index];
			} else if (ExtraSecBuffer == NULL) {
				ExtraSecBuffer = &Message->pBuffers[Index];
			}

			break;
		}
	}

	if (!HeaderSecBuffer || !PlaintextSecBuffer || !TrailerSecBuffer || !ExtraSecBuffer) {
		return SP_LOG_RESULT(SEC_E_INVALID_TOKEN);
	}

	ASSERT (CiphertextSecBuffer != NULL);

	//
	// Read the TLS record header to determine whether we have extra data or
	// missing data.
	//

	if (CiphertextSecBuffer->cbBuffer < Context->HeaderSize) {
		ULONG MissingCb;

		// We don't even have a full TLS record header.
		// Return SEC_E_INCOMPLETE_MESSAGE and pass back two SECBUFFER_MISSING,
		// both set to the number of bytes required to obtain a full TLS header.

		MissingCb = Context->HeaderSize - CiphertextSecBuffer->cbBuffer;

		HeaderSecBuffer->BufferType = SECBUFFER_MISSING;
		HeaderSecBuffer->cbBuffer = MissingCb;
		PlaintextSecBuffer->BufferType = SECBUFFER_MISSING;
		PlaintextSecBuffer->cbBuffer = MissingCb;

		// pvBuffer is deliberately not set to NULL, even though we did it for
		// the SECBUFFER_MISSING in InitializeSecurityContext. This is to reflect
		// that Schannel code does not set it to NULL, for whatever reason.
		// In both WinXP and Win10 this is the case.

		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	// Remember, the TlsHeader is server-controlled data, so we have to be careful
	// when working with it. Do not trust TlsHeader.DataCb too much.
	TlsDeserializeRecordHeader(
		(PCTLS_RECORD_HEADER) HeaderSecBuffer->pvBuffer,
		&TlsHeader);
	
	if (TlsHeader.DataCb + sizeof(TLS_RECORD_HEADER) < Context->HeaderSize) {
		return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
	}

	TlsDataLengthExcludingIV = sizeof(TLS_RECORD_HEADER) +
							   TlsHeader.DataCb -
							   Context->HeaderSize;

	if (TlsDataLengthExcludingIV > TLS_MAX_PLAINTEXT_CB + 512) {
		// TLS data is unreasonably large.
		KexLogDebugEvent(L"TLS data is unreasonably large");
		return SP_LOG_RESULT(SEC_E_DECRYPT_FAILURE);
	}

	if (TlsDataLengthExcludingIV + Context->HeaderSize > CiphertextSecBuffer->cbBuffer) {
		ULONG MissingCb;

		MissingCb = TlsDataLengthExcludingIV + Context->HeaderSize - 
					CiphertextSecBuffer->cbBuffer;

		// Not enough data. Need to return SEC_E_INCOMPLETE_MESSAGE and pass back
		// two SECBUFFER_MISSING.

		HeaderSecBuffer->BufferType = SECBUFFER_MISSING;
		HeaderSecBuffer->cbBuffer = MissingCb;
		PlaintextSecBuffer->BufferType = SECBUFFER_MISSING;
		PlaintextSecBuffer->cbBuffer = MissingCb;

		return SEC_E_INCOMPLETE_MESSAGE;
	} else if (CiphertextSecBuffer->cbBuffer > TlsDataLengthExcludingIV + Context->HeaderSize) {
		// Too much data. Put the amount of extra data in SECBUFFER_EXTRA.
		ExtraSecBuffer->BufferType = SECBUFFER_EXTRA;

		ExtraSecBuffer->cbBuffer = CiphertextSecBuffer->cbBuffer -
								   (TlsDataLengthExcludingIV + Context->HeaderSize);

		ExtraSecBuffer->pvBuffer = RVA_TO_VA(
			CiphertextSecBuffer->pvBuffer,
			TlsDataLengthExcludingIV + Context->HeaderSize);
	}

	//
	// Set up buffers.
	//

	// We'll leave pvBuffer as is - remember, HeaderSecBuffer points to the same thing
	// as CiphertextSecBuffer, so it's ok.
	HeaderSecBuffer->BufferType = SECBUFFER_STREAM_HEADER;
	HeaderSecBuffer->cbBuffer = Context->HeaderSize;

	PlaintextSecBuffer->BufferType = SECBUFFER_DATA;
	PlaintextSecBuffer->pvBuffer = RVA_TO_VA(
		HeaderSecBuffer->pvBuffer,
		HeaderSecBuffer->cbBuffer);

	// This is just an upper bound. It may end up becoming smaller later.
	PlaintextSecBuffer->cbBuffer = TlsDataLengthExcludingIV;

	TrailerSecBuffer->BufferType = SECBUFFER_STREAM_TRAILER;

	//
	// Set up IoInformation.
	// We'll only let TlsDecrypt read a single TLS record.
	//

	KexRtlZeroMemory(&IoInformation, sizeof(IoInformation));
	IoInformation.InBuffer = CiphertextSecBuffer->pvBuffer;
	IoInformation.InBufferCb = TlsDataLengthExcludingIV + Context->HeaderSize;
	IoInformation.OutBuffer = PlaintextSecBuffer->pvBuffer;
	IoInformation.OutBufferCb = PlaintextSecBuffer->cbBuffer;

	//
	// Decrypt the incoming record.
	//

	SecStatus = TlsDecrypt(
		Context,
		&IoInformation,
		&ContentType,
		NULL,
		0,
		NULL);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Do not allow anything other than application data to be given to the caller.
	//

	if (ContentType != CT_APPLICATIONDATA) {
		KXSCHANL_IO_INFORMATION MessageIoInformation;
		ULONG DecryptedMessageCb;

		// Save the length of the decrypted message and then zero it to prevent
		// the application from seeing any non-CT_APPLICATIONDATA stuff.
		DecryptedMessageCb = IoInformation.OutBufferWrittenCb;
		IoInformation.OutBufferWrittenCb = 0;

		if (ContentType == CT_HANDSHAKE) {
			//
			// Process any post-handshake CT_HANDSHAKE messages.
			//

			KexRtlZeroMemory(&MessageIoInformation, sizeof(MessageIoInformation));
			MessageIoInformation.InBuffer = IoInformation.OutBuffer;
			MessageIoInformation.InBufferCb = DecryptedMessageCb;

			do {
				SecStatus = TlsProcessPostHandshakeHandshakeMessageFragment(
					Context,
					&MessageIoInformation);

				if (FAILED(SecStatus)) {
					break;
				}
			} until (MessageIoInformation.InBufferReadCb == MessageIoInformation.InBufferCb);

			if (FAILED(SecStatus)) {
				Context->State = CONTEXTSTATE_CONNECTION_CLOSED;
				return SecStatus;
			}
		}
	}

	//
	// Update buffer sizes and pointers.
	// NB: CiphertextSecBuffer should not be used from now on because there is no
	// more ciphertext anymore.
	//

	PlaintextSecBuffer->cbBuffer = IoInformation.OutBufferWrittenCb;

	TrailerSecBuffer->pvBuffer = RVA_TO_VA(
		PlaintextSecBuffer->pvBuffer,
		PlaintextSecBuffer->cbBuffer);

	TrailerSecBuffer->cbBuffer = TlsDataLengthExcludingIV - PlaintextSecBuffer->cbBuffer;

	return SecStatus;
}