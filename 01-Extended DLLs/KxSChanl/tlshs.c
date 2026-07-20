///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlshs.c
//
// Abstract:
//
//     Contains functions which drive a TLS handshake.
//     The code in this file is shared between TLS 1.2 and 1.3 and helps to
//     reduce code duplication between the two protocols.
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
// Handles Change Cipher Spec records for both TLS 1.2 and 1.3.
// The IoInformation buffer should describe only the contents of the record,
// without including the record header.
//
STATIC SECURITY_STATUS TlspProcessChangeCipherSpec(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	UCHAR Value;

	if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
		// TLS 1.3 spec says that we should accept change cipher spec records
		// at any point after sending the Client Hello and before receiving
		// the server's Finished handshake message.
		if (!TlspIsHandshakeInProgress(Context)) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	} else {
		// TLS 1.2 change cipher spec is only accepted at a specific point in
		// the handshake.
		if (Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_CHANGE_CIPHER_SPEC) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	}

	// The message payload must always be a single byte 0x01.
	SecStatus = IoRead8(IoInformation, &Value);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (Value != 1) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_FINISHED;
		Context->ReadKeyEnabled = TRUE;
		ASSERT (Context->ReadSequenceNumber == 0);
		break;
	case TLS1_3_PROTOCOL_VERSION:
		// Do nothing. Change Cipher Spec messages during the handshake
		// are ignored in TLS 1.3.
		ASSERT (TlspIsHandshakeInProgress(Context));
		break;
	default:
		NOT_REACHED;
	}

	return SEC_E_OK;
}

//
// Generates Change Cipher Spec record, including header, for both TLS 1.2 and 1.3.
// The format of the Change Cipher Spec record is very simple and is identical
// for both protocol versions.
//
STATIC SECURITY_STATUS TlspGenerateChangeCipherSpec(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	TLS_RECORD_HEADER RecordHeader;

	ASSERT (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC ||
			Context->State == CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC);

	TlspSerializeRecordHeader(
		&RecordHeader,
		CT_CHANGE_CIPHER_SPEC,
		TLS1_2_PROTOCOL_VERSION,
		1);

	SecStatus = IoWrite(IoInformation, &RecordHeader, sizeof(RecordHeader));
	SecStatus = IoWrite8(IoInformation, 1);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	if (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC) {
		Context->State = CONTEXTSTATE_TLS1_2_SENDING_FINISHED;
		Context->WriteKeyEnabled = TRUE;
		ASSERT (Context->WriteSequenceNumber == 0);
	} else if (Context->State == CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC) {
		Context->State = CONTEXTSTATE_TLS1_3_SENDING_FINISHED;
	} else {
		NOT_REACHED;
	}

	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspHashHandshakeData(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN		PCVOID						Data,
	IN		ULONG						DataCb)
{
	//
	// Cannot hash any handshake data before the protocol version is determined.
	//

	ASSERT (Context->ProtocolVersion != 0);
	ASSERT ((Data != NULL && DataCb > 0) ||
			(Data == NULL && DataCb == 0));

	if (Data == NULL || DataCb == 0) {
		// Nothing to do.
		return SEC_E_OK;
	}

	if (Context->State >= CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		// Any post-handshake CT_HANDSHAKE messages do not need to be hashed.
		// Doing so would waste resources by causing a hash object to be re-created.
		return SEC_E_OK;
	}

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		return TlspHashHandshakeData12(Context, Data, DataCb);
	case TLS1_3_PROTOCOL_VERSION:
		return TlspHashHandshakeData13(Context, Data, DataCb);
	default:
		NOT_REACHED;
	}
}

//
// Process a complete incoming handshake message in the IO information buffer.
// The buffer should not include the 4-byte message header.
//
STATIC SECURITY_STATUS TlspProcessHandshakeMessage(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						MessageType,
	IN		ULONG						MessageCb)
{
	SECURITY_STATUS SecStatus;
	PCVOID MessageData;
	UCHAR OriginalMessageHeader[4];
	KXSCHANL_IO_INFORMATION HeaderIoInformation;

	KexLogDebugEvent(
		L"Received message %hu with length %lu",
		MessageType,
		MessageCb);

	// Get pointer to start of message data for hashing.
	SecStatus = IoPeek(IoInformation, &MessageData, MessageCb);
	ASSERT (SUCCEEDED(SecStatus));

	//
	// Call the appropriate sub-function for the current protocol version.
	//

	switch (Context->ProtocolVersion) {
	case 0:
		ASSERT (Context->State == CONTEXTSTATE_EXPECTING_SERVER_HELLO);

		// Indeterminate version. Only Server Hello is allowed at this stage.
		if (MessageType != TLS_HSMSG_SERVER_HELLO) {
			SecStatus = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
			break;
		}

		SecStatus = TlspHandleServerHello(Context, IoInformation);
		break;
	case TLS1_2_PROTOCOL_VERSION:
		// Message handlers take a Context parameter, a MessageType parameter,
		// and an IO information containing just the message contents (no
		// message header).
		SecStatus = TlspProcessHandshakeMessage12(Context, IoInformation, MessageType);
		break;
	case TLS1_3_PROTOCOL_VERSION:
		SecStatus = TlspProcessHandshakeMessage13(Context, IoInformation, MessageType);
		break;
	default:
		NOT_REACHED;
	}

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// No data should be left unprocessed by the message handlers - may
	// indicate a bug or junk data sent by the server.
	//

	if (IoInformation->InBufferReadCb != IoInformation->InBufferCb) {
		KexLogErrorEvent(
			L"%lu bytes of trailing data after TLS handshake message with type %hs",
			IoInformation->InBufferCb - IoInformation->InBufferReadCb,
			MessageType);

		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Update the running hash of the handshake message.
	//

	// If there is a saved Client Hello message in the context, we need to hash
	// that first.
	if (Context->SavedClientHello) {
		ASSERT (Context->SavedClientHelloCb > 4);
		ASSERT (MessageType == TLS_HSMSG_SERVER_HELLO);

		SecStatus = TlspHashHandshakeData(
			Context,
			Context->SavedClientHello,
			Context->SavedClientHelloCb);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		// clean up no longer needed context members
		SafeFree(Context->SavedClientHello);
		Context->SavedClientHelloCb = 0;
	}

	//
	// Our IoInformation only contains the actual contents of the message but the
	// hash needs to include also the 4-byte message header. We can reconstruct
	// the original value of the message header from MessageType and MessageCb.
	//

	KexRtlZeroMemory(&HeaderIoInformation, sizeof(HeaderIoInformation));
	HeaderIoInformation.OutBuffer = OriginalMessageHeader;
	HeaderIoInformation.OutBufferCb = sizeof(OriginalMessageHeader);

	SecStatus = IoWrite8(&HeaderIoInformation, MessageType);
	SecStatus = IoWriteSwap24(&HeaderIoInformation, MessageCb);
	ASSERT (SUCCEEDED(SecStatus));

	// Hash only the header for now.
	SecStatus = TlspHashHandshakeData(
		Context,
		OriginalMessageHeader,
		sizeof(OriginalMessageHeader));

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	// Hash the actual handshake data, which excludes the header.
	SecStatus = TlspHashHandshakeData(
		Context,
		MessageData,
		MessageCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// For TLS 1.3, we need to derive the handshake keys after processing
	// Server Hello.
	// This needs to be done after the Client Hello and Server Hello messages
	// are hashed, hence why we do it here.
	//

	if (Context->State == CONTEXTSTATE_TLS1_3_EXPECTING_ENCRYPTED_EXTENSIONS) {
		SecStatus = TlspDeriveHandshakeKeys13(Context);
		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}
	}

	return SecStatus;
}

//
// This function is responsible for buffering and reassembling incomplete
// handshake message fragments (if necessary), and processes the entire
// message once it is available.
//
// The IO information input buffer should describe data which is contained
// inside TLS handshake records. It is not necessary for the data to begin
// at a handshake message boundary.
//
STATIC SECURITY_STATUS TlspProcessHandshakeMessageFragment(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION MessageIoInformation;
	UCHAR MessageType;
	ULONG MessageCb;
	ULONG IoAvailableCb;

	//
	// This entire if-else block's purpose is to set MessageType and MessageCb.
	// If that can't be done because we don't have enough data then execution
	// will not proceed past the if-else block.
	//

	if (Context->FragmentedMessageHeaderCb > 0) {
		ULONG HeaderMissingCb;

		//
		// If we previously read *part* of a handshake message header, then we
		// need to continue reading that header.
		//

		ASSERT (Context->FragmentedMessage == NULL);
		ASSERT (Context->FragmentedMessageType == 0);
		ASSERT (Context->FragmentedMessageBufferCb == 0);
		ASSERT (Context->FragmentedMessageCb == 0);
		ASSERT (Context->FragmentedMessageHeaderCb < sizeof(Context->FragmentedMessageHeader));

		HeaderMissingCb = sizeof(Context->FragmentedMessageHeader) -
						  Context->FragmentedMessageHeaderCb;

		IoAvailableCb = IoInformation->InBufferCb - IoInformation->InBufferReadCb;

		SecStatus = IoReadCopy(
			IoInformation,
			&Context->FragmentedMessageHeader[Context->FragmentedMessageHeaderCb],
			min(HeaderMissingCb, IoAvailableCb));

		ASSERT (SUCCEEDED(SecStatus));

		Context->FragmentedMessageHeaderCb += (UCHAR) min(HeaderMissingCb, IoAvailableCb);

		if (Context->FragmentedMessageHeaderCb < sizeof(Context->FragmentedMessageHeader)) {
			// Still not enough.
			return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
		}

		//
		// We now have a complete message header. Get the message type and length.
		//

		KexRtlCopyMemory(
			&MessageCb,
			&Context->FragmentedMessageHeader[1],
			3);

		MessageType = Context->FragmentedMessageHeader[0];
		MessageCb = ByteSwap24(MessageCb);
	} else if (Context->FragmentedMessage) {
		MessageType = Context->FragmentedMessageType;
		MessageCb = Context->FragmentedMessageBufferCb;
	} else {
		//
		// We're at the start of a message boundary.
		// Just read the message type and length from the IO information buffer.
		//

		SecStatus = TlspReadMessageHeader(IoInformation, &MessageType, &MessageCb);

		if (FAILED(SecStatus)) {
			ULONG RemainingCb;

			ASSERT (SecStatus == SEC_E_INCOMPLETE_MESSAGE);

			//
			// Less than 4 bytes remaining in the buffer.
			// If we still have 3, 2, or 1 byte(s) left than those bytes need to be
			// saved into the context.
			//

			RemainingCb = IoInformation->InBufferCb - IoInformation->InBufferReadCb;
			ASSERT (RemainingCb < sizeof(Context->FragmentedMessageHeader));

			SecStatus = IoReadCopy(
				IoInformation,
				Context->FragmentedMessageHeader,
				RemainingCb);

			return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
		}
	}

	// Clean up the fragmented header context fields since we don't need them anymore.
	// Even if the message is still fragmented we will be saving the header information
	// into proper context fields FragmentedMessageType and FragmentedMessageBufferCb.
	KexRtlZeroMemory(
		Context->FragmentedMessageHeader,
		sizeof(Context->FragmentedMessageHeader));

	Context->FragmentedMessageHeaderCb = 0;

	// We will impose a limit of 256KB on the size of any invididual handshake message.
	// The spec does not say anything about a maximum limit but it's unreasonable to
	// send an extremely large handshake message.

	if (MessageCb > (256 * 1024)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// The purpose of the following section of code is to set up MessageIoInformation
	// with the entire contents of the message (excluding the header). If we don't
	// have enough data to do that, execution will not proceed past this section.
	//

	KexRtlZeroMemory(&MessageIoInformation, sizeof(MessageIoInformation));

	IoAvailableCb = IoInformation->InBufferCb - IoInformation->InBufferReadCb;

	if (Context->FragmentedMessage) {
		ULONG MessageMissingCb;
		ULONG CbAddedToFragmentedMessage;

		ASSERT (Context->FragmentedMessageBufferCb > Context->FragmentedMessageCb);

		//
		// A previous call to this function saved an incomplete fragment of a
		// handshake message. We need to continue assembling that fragment.
		//

		MessageMissingCb = Context->FragmentedMessageBufferCb - Context->FragmentedMessageCb;
		CbAddedToFragmentedMessage = min(MessageMissingCb, IoAvailableCb);

		SecStatus = IoReadCopy(
			IoInformation,
			&Context->FragmentedMessage[Context->FragmentedMessageCb],
			CbAddedToFragmentedMessage);

		ASSERT (SUCCEEDED(SecStatus));

		Context->FragmentedMessageCb += CbAddedToFragmentedMessage;

		if (MessageMissingCb > IoAvailableCb) {
			// We still don't have a full handshake message.
			return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
		}

		// We now have a full handshake message in the FragmentedMessage buffer.
		ASSERT (Context->FragmentedMessageBufferCb == Context->FragmentedMessageCb);

		MessageIoInformation.InBuffer = Context->FragmentedMessage;
		MessageIoInformation.InBufferCb = Context->FragmentedMessageCb;
	} else {
		if (MessageCb > IoAvailableCb) {
			//
			// This message is fragmented. We have to allocate a buffer in the context
			// structure to hold it and then save the part that we have.
			//

			Context->FragmentedMessage = SafeAlloc(BYTE, MessageCb);

			if (Context->FragmentedMessage == NULL) {
				return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
			}

			SecStatus = IoReadCopy(
				IoInformation,
				Context->FragmentedMessage,
				IoAvailableCb);

			ASSERT (SUCCEEDED(SecStatus));

			Context->FragmentedMessageType = MessageType;
			Context->FragmentedMessageBufferCb = MessageCb;
			Context->FragmentedMessageCb = IoAvailableCb;

			return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
		}

		//
		// We have the entire message available in the input IoInformation.
		//

		MessageIoInformation.InBufferCb = MessageCb;
		
		SecStatus = IoRead(
			IoInformation,
			&MessageIoInformation.InBuffer,
			MessageIoInformation.InBufferCb);

		ASSERT (SUCCEEDED(SecStatus));
	}

	//
	// Now we have a complete message in MessageIoInformation.
	// Pass it to the complete message handler.
	//

	SecStatus = TlspProcessHandshakeMessage(
		Context,
		&MessageIoInformation,
		MessageType,
		MessageCb);

	// Clean up any context fields for fragmented messages that may have been set
	// since we've now consumed it.
	SafeFree(Context->FragmentedMessage);
	Context->FragmentedMessageCb = 0;
	Context->FragmentedMessageBufferCb = 0;
	Context->FragmentedMessageType = 0;

	return SecStatus;
}

//
// Process CT_HANDSHAKE message contents which appear after the handshake
// is already complete.
//
SECURITY_STATUS TlsProcessPostHandshakeHandshakeMessageFragment(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	ASSERT (Context->State == CONTEXTSTATE_HANDSHAKE_COMPLETE);
	return TlspProcessHandshakeMessageFragment(Context, IoInformation);
}

//
// Process a single complete record and all the handshake messages contained
// inside, if any.
//
STATIC SECURITY_STATUS TlspProcessIncomingRecord(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	TLS_RECORD_HEADER RecordHeader;
	UCHAR ContentType; // CT_*
	KXSCHANL_IO_INFORMATION RecordIoInformation;

	//
	// IO information structure which describes the record payload.
	//

	KexRtlZeroMemory(&RecordIoInformation, sizeof(RecordIoInformation));

	//
	// Peek at and validate the record header.
	//

	SecStatus = TlspPeekRecordHeader(Context, IoInformation, &RecordHeader);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	if (Context->ReadKeyEnabled && RecordHeader.Type != CT_CHANGE_CIPHER_SPEC) {
		PBYTE DecryptedRecord;
		ULONG DecryptedRecordCb;

		//
		// We expect the message to be encrypted, so decrypt it to a temporary stack
		// buffer.
		// The decrypted message cannot be larger than the encrypted message, so we
		// use RecordHeader.DataCb as the allocation size.
		//

		DecryptedRecordCb = RecordHeader.DataCb;
		DecryptedRecord = StackAlloc(BYTE, DecryptedRecordCb);

		SecStatus = TlsDecrypt(
			Context,
			IoInformation,
			&ContentType,
			DecryptedRecord,
			DecryptedRecordCb,
			&DecryptedRecordCb);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		RecordIoInformation.InBuffer = DecryptedRecord;
		RecordIoInformation.InBufferCb = DecryptedRecordCb;
	} else {
		//
		// Unencrypted message. Read the record and skip past the header.
		//

		RecordIoInformation.InBufferCb = sizeof(TLS_RECORD_HEADER) + RecordHeader.DataCb;

		SecStatus = IoRead(
			IoInformation,
			&RecordIoInformation.InBuffer,
			RecordIoInformation.InBufferCb);

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		// Skip past header.
		RecordIoInformation.InBufferReadCb += sizeof(TLS_RECORD_HEADER);

		// For unencrypted records the header type is always the real content
		// type.
		ContentType = RecordHeader.Type;
	}

	//
	// We have the contents of a TLS record in RecordIoInformation.
	// Read messages out of it.
	//

	switch (ContentType) {
	case CT_ALERT:
		return TlspProcessIncomingAlert(Context, &RecordIoInformation);
	case CT_CHANGE_CIPHER_SPEC:
		return TlspProcessChangeCipherSpec(Context, &RecordIoInformation);
	case CT_HANDSHAKE:
		// Check that we haven't received a zero-length handshake message. This is
		// prohibited in both TLS 1.2 and 1.3.

		if (RecordIoInformation.InBufferCb - RecordIoInformation.InBufferReadCb == 0) {
			goto UnexpectedMessage;
		}

		do {
			SecStatus = TlspProcessHandshakeMessageFragment(Context, &RecordIoInformation);

			if (FAILED(SecStatus)) {
				break;
			}
		} until (RecordIoInformation.InBufferReadCb == RecordIoInformation.InBufferCb);

		// Unless there was a problem with the data (in which SecStatus will reflect it),
		// we should consume all data in a record.
		ASSERT (FAILED(SecStatus) ||
				RecordIoInformation.InBufferReadCb == RecordIoInformation.InBufferCb);

		break;
	default:
	UnexpectedMessage:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	return SecStatus;
}

STATIC SECURITY_STATUS TlspGenerateHandshakeMessage(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION MessageHeaderIoInformation;
	ULONG OriginalOutBufferWrittenCb;
	UCHAR MessageType;
	ULONG MessageCb;

	// Save the buffer offset. This is so we know where to overwrite the
	// message type and length later on. We can't save a pointer directly
	// because the OutBuffer value can change (if OutBufferMustAllocate).
	OriginalOutBufferWrittenCb = IoInformation->OutBufferWrittenCb;

	// Write a placeholder 4-byte value which we will later fill out with
	// the message type and length.
	SecStatus = IoWriteSwap32(IoInformation, 0);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Call the appropriate version-specific message generator.
	// Message generators only write the message contents into the IO information buffer
	// in order to simplify their implementation.
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		SecStatus = TlspGenerateHandshakeMessage12(Context, IoInformation, &MessageType);
		break;
	case TLS1_3_PROTOCOL_VERSION:
		SecStatus = TlspGenerateHandshakeMessage13(Context, IoInformation, &MessageType);
		break;
	default:
		NOT_REACHED;
	}

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Update the message type and length.
	//

	// -4 for message header, which was written after we saved the original
	// OutBufferWrittenCb.
	MessageCb = IoInformation->OutBufferWrittenCb - OriginalOutBufferWrittenCb - 4;

	KexRtlZeroMemory(&MessageHeaderIoInformation, sizeof(MessageHeaderIoInformation));

	MessageHeaderIoInformation.OutBuffer = RVA_TO_VA(
		IoInformation->OutBuffer,
		OriginalOutBufferWrittenCb);

	MessageHeaderIoInformation.OutBufferCb = 4;

	SecStatus = IoWrite8(&MessageHeaderIoInformation, MessageType);
	SecStatus = IoWriteSwap24(&MessageHeaderIoInformation, MessageCb);
	ASSERT (SUCCEEDED(SecStatus));

	KexLogDebugEvent(
		L"Generated message %hu with length %lu",
		MessageType,
		MessageCb);

	return SecStatus;
}

//
// Generate a single output record. May coalesce several handshake messages
// into a single record.
//
// This function automatically handles the encryption of outgoing records.
//
STATIC SECURITY_STATUS TlspGenerateOutgoingRecord(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	BYTE OutgoingRecordBuffer[TLS_MAX_PLAINTEXT_CB];
	KXSCHANL_IO_INFORMATION RecordIoInformation;

	if (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC ||
		Context->State == CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC) {

		//
		// Special case: Change Cipher Spec has a different content type and is
		// never encrypted.
		//

		return TlspGenerateChangeCipherSpec(Context, IoInformation);
	}

	//
	// Set up an IO information to describe the plaintext contents of the
	// record.
	//

	KexRtlZeroMemory(&RecordIoInformation, sizeof(RecordIoInformation));
	RecordIoInformation.OutBuffer = OutgoingRecordBuffer;
	RecordIoInformation.OutBufferCb = sizeof(OutgoingRecordBuffer);

	//
	// Keep generating handshake messages until we need more data from the server.
	// Currently, we will assume that all handshake messages will fit in a single
	// record. This is only guaranteed to be true because we don't support client
	// certificates for now.
	//

	while (TlspIsResponseState(Context)) {
		SecStatus = TlspGenerateHandshakeMessage(Context, &RecordIoInformation);

		if (FAILED(SecStatus)) {
			ASSERT (SecStatus != SEC_E_BUFFER_TOO_SMALL);
			return SecStatus;
		}

		if (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC ||
			Context->State == CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC) {

			// This is not a handshake message anymore so break out.
			break;
		}
	}

	//
	// Add the entire contents of handshake records to the handshake hash.
	//

	SecStatus = TlspHashHandshakeData(
		Context,
		RecordIoInformation.OutBuffer,
		RecordIoInformation.OutBufferWrittenCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Encrypt the record if necessary.
	// Otherwise, just write the record header directly to the output, followed by
	// record plaintext data.
	//

	if (Context->WriteKeyEnabled) {
		ULONG PlaintextConsumedCb;

		SecStatus = TlsEncrypt(
			Context,
			IoInformation,
			CT_HANDSHAKE,
			RecordIoInformation.OutBuffer,
			RecordIoInformation.OutBufferWrittenCb,
			&PlaintextConsumedCb);

		ASSERT (SUCCEEDED(SecStatus));
		ASSERT (PlaintextConsumedCb == RecordIoInformation.OutBufferWrittenCb);
	} else {
		TLS_RECORD_HEADER RecordHeader;

		ASSERT (RecordIoInformation.OutBufferWrittenCb < TLS_MAX_PLAINTEXT_CB);

		TlspSerializeRecordHeader(
			&RecordHeader,
			CT_HANDSHAKE,
			TLS1_2_PROTOCOL_VERSION,
			(USHORT) RecordIoInformation.OutBufferWrittenCb);

		SecStatus = IoWrite(IoInformation, &RecordHeader, sizeof(RecordHeader));

		SecStatus = IoWrite(
			IoInformation,
			RecordIoInformation.OutBuffer,
			RecordIoInformation.OutBufferWrittenCb);
	}

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// After encrypting that handshake record, if the handshake is now done and we
	// are using TLS 1.3, we need to derive the new read and write keys.
	//

	if (Context->State == CONTEXTSTATE_HANDSHAKE_COMPLETE &&
		Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {

		SecStatus = TlspDeriveApplicationKeys13(Context);

		if (FAILED(SecStatus)) {
			return SecStatus;
		}
	}

	return SEC_E_OK;
}

//
// TlsConnect - Drive a TLS handshake forward.
//
// This function is called with:
//   - a context handle which is still in one of the handshake states.
//   - an IO information buffer which contains one or more complete TLS records,
//     and has an appropriate amount of output buffer space to contain up to
//     KXSCHANL_PACKAGE_INFO_CB_MAX_TOKEN bytes of data.
//
// This function is responsible for:
//   - sending ClientHello and receiving ServerHello.
//   - decrypting incoming records, if necessary.
//   - processing incoming alerts during the handshake.
//   - providing complete handshake messages to the protocol version-specific
//     handshake message handlers (buffering fragmented messages if necessary).
//

SECURITY_STATUS TlsConnect(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	ULONG OriginalOutBufferWrittenCb;

	ASSERT (Context != NULL);
	ASSERT (TlspIsHandshakeInProgress(Context) == TRUE);
	ASSERT (IoInformation != NULL);

	OriginalOutBufferWrittenCb = IoInformation->OutBufferWrittenCb;

	//
	// Special case: sending the client hello message on first call.
	//

	if (Context->State == CONTEXTSTATE_SENDING_CLIENT_HELLO) {
		// TlspWriteClientHello writes the TLS record header and updates the
		// context state, so we can just call it and return straight away.
		return TlspWriteClientHello(Context, IoInformation);
	}

	//
	// Process as many input records as we can.
	//

	while (TlspIsInputState(Context)) {
		SecStatus = TlspProcessIncomingRecord(Context, IoInformation);

		if (FAILED(SecStatus)) {
			goto FunctionExit;
		}

		if (IoInformation->InBufferReadCb >= IoInformation->InBufferCb) {
			// No more records in input buffer.
			break;
		}
	}

	//
	// Generate as many response records as possible.
	//

	while (TlspIsResponseState(Context)) {
		SecStatus = TlspGenerateOutgoingRecord(Context, IoInformation);

		if (FAILED(SecStatus)) {
			goto FunctionExit;
		}
	}

FunctionExit:

	if (SecStatus == SEC_E_INCOMPLETE_MESSAGE) {
		// We should always return SEC_I_CONTINUE_NEEDED if we encounter a situation where
		// we can't read enough data. This is because we're guaranteed to be called with
		// more than one complete record in the IO information input buffer.
		SecStatus = SEC_I_CONTINUE_NEEDED;
	} else if (SecStatus == SEC_E_OK && TlspIsHandshakeInProgress(Context)) {
		// Cannot return SEC_E_OK until the handshake is complete.
		SecStatus = SEC_I_CONTINUE_NEEDED;
	}
	
	return SecStatus;
}