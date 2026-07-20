///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     secctx.c
//
// Abstract:
//
//     Contains the implementation of InitializeSecurityContext and
//     DeleteSecurityContext.
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
//     vxiiduu               30-Jun-2026  Ignore input buffer for ISC when the
//                                        client hello hasn't been sent yet.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

INLINE PKXSCHANL_CONTEXT SppReadContextHandle(
	IN	PCtxtHandle	ContextHandle)
{
	ASSERT (ContextHandle != NULL);
	ASSERT (SecIsValidHandle(ContextHandle));
	return (PKXSCHANL_CONTEXT) ContextHandle->dwUpper;
}

STATIC INLINE VOID SppWriteContextHandle(
	OUT	PCtxtHandle				ContextHandle,
	IN	PCKXSCHANL_CONTEXT		Context)
{
	ASSERT (Context != NULL);
	ContextHandle->dwUpper = (ULONG_PTR) Context;
}

// The domain name supplied to the library must be encoded in ASCII Punycode
// format, which we will convert using RtlIdnToAscii.
STATIC INLINE BOOLEAN SppApplyTargetName(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCWSTR				ServerName)
{
	NTSTATUS Status;
	WCHAR PunycodeUnicode[ARRAYSIZE(Context->TargetName)];
	ULONG Cch;

	ASSERT (ServerName != NULL);

	if (ServerName[0] == '\0') {
		return TRUE;
	}

	//
	// Convert domain name to Punycode
	//

	Cch = ARRAYSIZE(PunycodeUnicode);

	Status = RtlIdnToAscii(
		0,
		ServerName,
		-1,
		PunycodeUnicode,
		&Cch);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (Cch == wcslen(PunycodeUnicode) + 1);

	if (!NT_SUCCESS(Status)) {
		KexLogDebugEvent(
			L"RtlIdnToAscii failed\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status),
			Status);
		
		return FALSE;
	}

	//
	// Convert Punycode (in Unicode format) to ANSI and store it into the
	// context.
	//

	Context->TargetNameCch = WideCharToMultiByte(
		CP_ACP,
		WC_NO_BEST_FIT_CHARS,
		PunycodeUnicode,
		Cch,
		Context->TargetName,
		ARRAYSIZE(Context->TargetName),
		NULL,
		NULL);

	ASSERT (Context->TargetNameCch > 0);

	if (Context->TargetNameCch == 0) {
		KexLogDebugEvent(
			L"WideCharToMultiByte failed: Win32 error %lu",
			GetLastError());
		
		return FALSE;
	}

	//
	// Cch currently includes the null terminator. We don't want it to include
	// the null terminator.
	//

	--Context->TargetNameCch;
	ASSERT (Context->TargetNameCch > 0);
	ASSERT (Context->TargetNameCch == strlen(Context->TargetName));

	return TRUE;
}

// Returns NULL on failure.
STATIC INLINE SECURITY_STATUS SppCreateNewSecurityContext(
	OUT	PPKXSCHANL_CONTEXT		NewContext,
	IN	PKXSCHANL_CREDENTIAL	Credential)
{
	NTSTATUS Status;
	PKXSCHANL_CONTEXT Context;

	ASSERT (Credential != NULL);

	*NewContext = NULL;

	//
	// Allocate memory for KXSCHANL_CONTEXT structure
	//

	Context = SafeAllocEx(RtlProcessHeap(), HEAP_ZERO_MEMORY, KXSCHANL_CONTEXT, 1);
	ASSERT (Context != NULL);

	if (Context == NULL) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	// Store a pointer to the parent credential object
	Context->Credential = Credential;

	// The header size is set to 5 for now; it will be updated later.
	Context->HeaderSize = sizeof(TLS_RECORD_HEADER);

	// Client random (32 bytes).
	Status = KexRtlGenerateRandomData(
		Context->ClientRandom,
		sizeof(Context->ClientRandom));

	if (!NT_SUCCESS(Status)) {
		SafeFree(Context);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	*NewContext = Context;
	return SEC_E_OK;
}

//
// Fill out BlockSize, HeaderSize, and TrailerSize members of the context
// structure.
//
STATIC INLINE VOID SppGetHeaderBlockAndTrailerSize(
	IN OUT	PKXSCHANL_CONTEXT	Context)
{
	SECURITY_STATUS SecStatus;
	NCRYPT_SSL_CIPHER_SUITE CipherSuite;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion != 0);
	ASSERT (Context->State >= CONTEXTSTATE_HANDSHAKE_COMPLETE);
	ASSERT (Context->BlockSize == 0);
	ASSERT (Context->TrailerSize == 0);

	SecStatus = TlsLookupCipherSuiteInfo(
		Context->ProtocolVersion,
		Context->CipherSuite,
		Context->EccKeyType,
		&CipherSuite);

	// Can only fail when passed invalid parameters + zeroes output even
	// upon failure.
	ASSERT (SUCCEEDED(SecStatus));

	ASSERT (Context->BlockSize <= USHRT_MAX);
	Context->BlockSize = (USHORT) CipherSuite.dwCipherBlockLen;

	if (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION) {
		NCRYPT_SSL_CIPHER_LENGTHS CipherLengths;

		SecStatus = SslLookupCipherLengths(
			Context->Credential->SslProvider,
			Context->ProtocolVersion,
			TlspMapCipherSuiteForNcrypt(Context->CipherSuite),
			Context->EccKeyType,
			&CipherLengths,
			sizeof(CipherLengths),
			0);

		// SslLookupCipherLengths can only fail when passed invalid
		// parameters.
		ASSERT (SUCCEEDED(SecStatus));

		ASSERT (CipherLengths.dwHeaderLen <= UCHAR_MAX - sizeof(TLS_RECORD_HEADER));
		Context->HeaderSize = (UCHAR) (sizeof(TLS_RECORD_HEADER) + CipherLengths.dwHeaderLen);

		ASSERT (CipherLengths.dwFixedTrailerLen +
				CipherLengths.dwMaxVariableTrailerLen <= UCHAR_MAX);

		Context->TrailerSize = (UCHAR) (CipherLengths.dwFixedTrailerLen +
										CipherLengths.dwMaxVariableTrailerLen);
	} else if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
		Context->HeaderSize = sizeof(TLS_RECORD_HEADER);
		Context->TrailerSize = 16 + 1; // 16-byte tag + 1-byte inner content type
	} else {
		NOT_REACHED;
	}

	KexLogInformationEvent(
		L"Cipher suite finalized\r\n\r\n"
		L"Protocol version: 0x%04hx\r\n"
		L"Cipher suite:     %s\r\n"
		L"ECC key type:     0x%04hx",
		Context->ProtocolVersion,
		TlsMapCipherSuiteToIanaName(Context->CipherSuite),
		Context->EccKeyType);
}

//
// Get the length of all combined complete TLS records in the given buffer.
// If there are no complete records in the buffer, returns 0.
//
// If there are no complete records in the buffer, *FirstRecordCb is set to
// the length of the first TLS record. If not enough information is present
// to determine the length of the first TLS record, *FirstRecordCb will be
// set to 0.
//
STATIC INLINE ULONG SppGetCompleteTlsRecordsCb(
	IN	PKXSCHANL_CONTEXT	Context,
	IN	PCVOID				Buffer,
	IN	ULONG				BufferCb,
	OUT	PULONG				FirstRecordCb OPTIONAL)
{
	ULONG CompleteTlsRecordsCb;
	ULONG RemainingBufferCb;

	CompleteTlsRecordsCb = 0;
	RemainingBufferCb = BufferCb;

	if (FirstRecordCb) {
		*FirstRecordCb = 0;
	}

	until (RemainingBufferCb < sizeof(TLS_RECORD_HEADER)) {
		TLS_RECORD_HEADER TlsHeader;
		ULONG RecordCb;

		TlsDeserializeRecordHeader(
			(PCTLS_RECORD_HEADER) RVA_TO_VA(Buffer, CompleteTlsRecordsCb),
			&TlsHeader);

		RecordCb = sizeof(TLS_RECORD_HEADER) + TlsHeader.DataCb;

		if (RecordCb > RemainingBufferCb) {
			// Reached an incomplete record

			if (CompleteTlsRecordsCb == 0) {
				if (FirstRecordCb) {
					*FirstRecordCb = RecordCb;
				}
			}

			break;
		}

		CompleteTlsRecordsCb += RecordCb;
		RemainingBufferCb -= RecordCb;
	}

	return CompleteTlsRecordsCb;
}

STATIC INLINE SECURITY_STATUS SppSetApplicationProtocols(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PSecBuffer			AlpnSecBuffer)
{
	SECURITY_STATUS SecStatus;
	PSEC_APPLICATION_PROTOCOLS ApplicationProtocols;
	PSEC_APPLICATION_PROTOCOL_LIST ProtocolList;
	ULONG_PTR EndOfProtocolLists;
	ULONG ProtocolListsCb;
	USHORT ProtocolListDataCb;

	ASSERT (AlpnSecBuffer->BufferType == SECBUFFER_APPLICATION_PROTOCOLS);

	//
	// Validate the outer SEC_APPLICATION_PROTOCOLS structure.
	//

	if (AlpnSecBuffer->cbBuffer < sizeof(SEC_APPLICATION_PROTOCOLS) ||
		AlpnSecBuffer->pvBuffer == NULL) {

		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	ApplicationProtocols = (PSEC_APPLICATION_PROTOCOLS) AlpnSecBuffer->pvBuffer;
	ProtocolListsCb = ApplicationProtocols->ProtocolListsSize;

	if (ProtocolListsCb < sizeof(SEC_APPLICATION_PROTOCOL_LIST) ||
		ProtocolListsCb + sizeof(ULONG) < ProtocolListsCb || // overflow check
		AlpnSecBuffer->cbBuffer < ProtocolListsCb + sizeof(ULONG)) {

		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Iterate through the SEC_APPLICATION_PROTOCOL_LIST entries looking
	// for the ALPN one.
	//

	EndOfProtocolLists = (ULONG_PTR) RVA_TO_VA(
		ApplicationProtocols,
		ApplicationProtocols->ProtocolListsSize + sizeof(ULONG));

	ProtocolList = &ApplicationProtocols->ProtocolLists[0];

	until ((ULONG_PTR) ProtocolList >= EndOfProtocolLists) {
		ULONG ProtocolListStride;

		if (ProtocolList->ProtoNegoExt == SecApplicationProtocolNegotiationExt_ALPN) {
			break;
		}

		//
		// Advance to the next SEC_APPLICATION_PROTOCOL_LIST entry.
		// Each entry is padded to the natural alignment of the struct
		// (this is what Win10 Schannel does).
		//

		ProtocolListStride = (FIELD_OFFSET(SEC_APPLICATION_PROTOCOL_LIST, ProtocolList) +
							  ProtocolList->ProtocolListSize +
							  (sizeof(SEC_APPLICATION_PROTOCOL_LIST) - 1)) /
							 sizeof(SEC_APPLICATION_PROTOCOL_LIST);

		ProtocolList += (ULONG_PTR) ProtocolListStride;
	}

	if ((ULONG_PTR) ProtocolList >= EndOfProtocolLists) {
		// Couldn't find ALPN list.
		KexDebugCheckpoint();
		return SEC_E_OK;
	}

	//
	// We've got an ALPN protocol list in ProtocolList.
	// Call helper function to validate it.
	//

	ASSERT (ProtocolList->ProtoNegoExt == SecApplicationProtocolNegotiationExt_ALPN);

	ProtocolListDataCb = ProtocolList->ProtocolListSize;

	SecStatus = TlsValidateApplicationProtocolList(
		Context,
		ProtocolList->ProtocolList,
		ProtocolList->ProtocolListSize,
		TRUE);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}
		
	//
	// Copy the raw wire-format bytes into the context.
	// This data will be inserted verbatim into the ALPN TLS extension
	// in the ClientHello.
	//

	KexRtlCopyMemory(
		Context->ApplicationProtocols,
		ProtocolList->ProtocolList,
		ProtocolListDataCb);

	Context->ApplicationProtocolsCb = ProtocolListDataCb;

	// Found the ALPN entry, we're done.
	return SEC_E_OK;
}

//
// The application calls InitializeSecurityContext in order to connect to a server.
// When we need to send something to the server, we return SEC_I_CONTINUE_NEEDED.
// When we need to receive something from the server, we return SEC_E_INCOMPLETE_MESSAGE.
// If the server wants to renegotiate (for example, to obtain a client certificate),
// we return SEC_I_INCOMPLETE_CREDENTIALS.
// When the handshake is complete, we return SEC_E_OK.
//
// The application also calls InitializeSecurityContext in order to disconnect from
// the server. This is preceded by ApplyControlToken with SCHANNEL_SHUTDOWN.
//
SECURITY_STATUS SppInitializeSecurityContext(
	IN		PCredHandle		CredentialHandle OPTIONAL,
	IN		PVOID			RetAddr,
	IN		PCtxtHandle		ContextHandle OPTIONAL,
	IN		PWSTR			TargetName OPTIONAL,
	IN		ULONG			ContextRequirementsFlags,
	IN		PSecBufferDesc	Input OPTIONAL,
	IN OUT	PCtxtHandle		NewContextHandle,
	IN OUT	PSecBufferDesc	Output OPTIONAL,
	OUT		PULONG			ContextAttributesFlags OPTIONAL,
	OUT		PTimeStamp		Expiry OPTIONAL)
{
	PKXSCHANL_CONTEXT Context;
	SECURITY_STATUS SecStatus;
	BOOLEAN Success;
	BOOLEAN FirstCall;
	ULONG Index;
	KXSCHANL_IO_INFORMATION IoInformation;
	PSecBuffer InputSecBuffer;
	PSecBuffer OutputSecBuffer;
	PSecBuffer ExtraSecBuffer;
	PSecBuffer AlpnSecBuffer;
	ULONG FlagsOut;
	ULONG ContextFlagsClear;

	SecStatus = SEC_E_INTERNAL_ERROR;

	InputSecBuffer = NULL;
	OutputSecBuffer = NULL;
	ExtraSecBuffer = NULL;
	AlpnSecBuffer = NULL;

	ContextFlagsClear = 0;
	FlagsOut = ISC_RET_REPLAY_DETECT |
			   ISC_RET_SEQUENCE_DETECT |
			   ISC_RET_CONFIDENTIALITY |
			   ISC_RET_STREAM;

	//
	// Fixup EncryptMessage and DecryptMessage pointers.
	// See comment in sspihack.c for why this is necessary.
	//

	SppFixupFunctionPointers(CredentialHandle, RetAddr);

	//
	// Set up the output buffers.
	// The for-loops logic is taken from Windows code, yes it's weird, but it
	// is what Schannel does as far as I can tell.
	// And yes, Schannel does this before looking at the context handle.
	//

	KexRtlZeroMemory(&IoInformation, sizeof(IoInformation));

	if (Output == NULL) {
		return SP_LOG_RESULT(SEC_E_INVALID_TOKEN);
	}

	for (Index = 0; Index < Output->cBuffers; ++Index) {
		switch (Output->pBuffers[Index].BufferType & (~SECBUFFER_ATTRMASK)) {
		case SECBUFFER_EMPTY:
			if (OutputSecBuffer == NULL &&
				(ContextRequirementsFlags & ISC_REQ_ALLOCATE_MEMORY)) {

				OutputSecBuffer = &Output->pBuffers[Index];
			}

			break;
		case SECBUFFER_TOKEN:
			OutputSecBuffer = &Output->pBuffers[Index];
			break;
		}
	}

	if (!OutputSecBuffer) {
		return SP_LOG_RESULT(SEC_E_INVALID_TOKEN);
	}

	OutputSecBuffer->BufferType = SECBUFFER_TOKEN;

	if (ContextRequirementsFlags & ISC_REQ_ALLOCATE_MEMORY) {
		IoInformation.OutBufferMustAllocate = TRUE;
		FlagsOut |= ISC_RET_ALLOCATED_MEMORY;
	} else {
		if (OutputSecBuffer->pvBuffer == NULL) {
			return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
		}

		IoInformation.OutBuffer = OutputSecBuffer->pvBuffer;
		IoInformation.OutBufferCb = OutputSecBuffer->cbBuffer;
	}

	//
	// Handle flags.
	//

	if (ContextRequirementsFlags & ISC_REQ_EXTENDED_ERROR) {
		// No-op
		FlagsOut |= ISC_RET_EXTENDED_ERROR;
	}

	if (ContextRequirementsFlags & ISC_REQ_CONNECTION) {
		// Not supported, haven't found an app using it + requires different
		// buffer handling (I think)
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
	}

	if (ContextRequirementsFlags & ISC_REQ_MUTUAL_AUTH) {
		FlagsOut |= ISC_RET_MUTUAL_AUTH;

		if (ContextRequirementsFlags & ISC_REQ_MANUAL_CRED_VALIDATION) {
			// Mutually exclusive with ISC_REQ_MUTUAL_AUTH
			return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
		}

		ContextFlagsClear |= ISC_RET_MANUAL_CRED_VALIDATION;
	} else {
		if (ContextRequirementsFlags & ISC_REQ_MANUAL_CRED_VALIDATION) {
			// This means don't verify the peer certificate.
			FlagsOut |= ISC_RET_MANUAL_CRED_VALIDATION;
			ContextFlagsClear |= ISC_RET_MUTUAL_AUTH;
		}
	}

	if (ContextRequirementsFlags & ISC_REQ_USE_SUPPLIED_CREDS) {
		// Not supported, but pretend it succeeded.
		// Curl sets this flag but doesn't really need it, most of the time.
		FlagsOut |= ISC_RET_USED_SUPPLIED_CREDS;
	}

	if (ContextAttributesFlags) {
		*ContextAttributesFlags = FlagsOut;
	}

	//
	// Check if we're on the first call to this function.
	// If so, we need to create a new context; otherwise, we just need to use
	// the one supplied by the caller.
	//

	if (ContextHandle == NULL) {
		PKXSCHANL_CREDENTIAL Credential;

		//
		// We need to create a new context structure and pass it out to NewContextHandle.
		//

		Credential = SppReadCredentialHandle(CredentialHandle);
		SecStatus = SppCreateNewSecurityContext(&Context, Credential);
		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		//
		// Provide the TLS component with the name of our server, if the caller
		// has supplied one.
		//

		if (TargetName) {
			Success = SppApplyTargetName(Context, TargetName);

			if (!Success) {
				SppWriteContextHandle(NewContextHandle, Context);
				SppDeleteSecurityContext(NewContextHandle);
				return SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);
			}
		}

		//
		// Give the caller the new context handle.
		//

		SppWriteContextHandle(NewContextHandle, Context);

		KexLogInformationEvent(
			L"New TLS connection to \"%s\"\r\n\r\n",
			TargetName ? TargetName : L"<unspecified server>");

		FirstCall = TRUE;
	} else {
		Context = SppReadContextHandle(ContextHandle);

		if (Context->State >= CONTEXTSTATE_HANDSHAKE_COMPLETE &&
			Context->State != CONTEXTSTATE_SENDING_CLOSE_NOTIFY) {

			// InitializeSecurityContext can only be called to initiate or continue
			// a handshake, or initiate or continue a disconnection (shutdown).
			return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
		}

		FirstCall = FALSE;
	}

	Context->Flags |= FlagsOut;
	Context->Flags &= ~ContextFlagsClear;

	//
	// Set up input buffers.
	//
	// We will check the handshake state. There's at least one app in existence
	// which passes an invalid input buffer (pvBuffer is uninitialized) during
	// shutdown, which causes a crash, but not with Schannel because Schannel
	// doesn't access that pointer on shutdown.
	//
	// This buggy app that I keep mentioning is this test app on Github:
	// https://gist.github.com/mmozeiko/c0dfcc8fec527a90a02145d2cc0bfb6d
	//
	// I used this during testing but it is actually extremely buggy. It
	// discards the input buffer on receiving SEC_E_INCOMPLETE_MESSAGE (which
	// is not supposed to happen), it passes an uninitialized input buffer
	// during shutdown (which I guess is kind of understandable since there's
	// no data to be sent to the server during shutdown), and was generally
	// kind of a pain in the ass. Unfortunately other developers may have
	// used it as a guide...
	//
	// 30-Jun-2026: Check the context state and make sure it is greater than
	// CONTEXTSTATE_SENDING_CLIENT_HELLO. The .NET DLL, System.Net.Security calls
	// this function with a non-null input buffer at all times - even when the
	// handshake hasn't even begun.
	//

	if (Input != NULL &&
		Context->State > CONTEXTSTATE_SENDING_CLIENT_HELLO &&
		Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE) {

		for (Index = 0; Index < Input->cBuffers; ++Index) {
			switch (Input->pBuffers[Index].BufferType & (~SECBUFFER_ATTRMASK)) {
			case SECBUFFER_EMPTY:
				if (InputSecBuffer == NULL) {
					InputSecBuffer = &Input->pBuffers[Index];
				} else {
					ExtraSecBuffer = &Input->pBuffers[Index];
				}

				break;
			case SECBUFFER_TOKEN:
				InputSecBuffer = &Input->pBuffers[Index];
				break;
			case SECBUFFER_APPLICATION_PROTOCOLS:
				AlpnSecBuffer = &Input->pBuffers[Index];
				break;
			}
		}

		if (InputSecBuffer) {
			ULONG CompleteTlsRecordsCb;
			ULONG FirstRecordCb;

			IoInformation.InBuffer = InputSecBuffer->pvBuffer;
			IoInformation.InBufferCb = InputSecBuffer->cbBuffer;

			CompleteTlsRecordsCb = SppGetCompleteTlsRecordsCb(
				Context,
				IoInformation.InBuffer,
				IoInformation.InBufferCb,
				&FirstRecordCb);

			if (CompleteTlsRecordsCb == 0) {
				// No complete TLS records in input buffer. Tell the application
				// that it needs to give us more data.

				if (ExtraSecBuffer) {
					ASSERT (ExtraSecBuffer->BufferType == SECBUFFER_EMPTY);

					if (FirstRecordCb == 0) {
						FirstRecordCb = sizeof(TLS_RECORD_HEADER);
					}

					ASSERT (FirstRecordCb > InputSecBuffer->cbBuffer);

					ExtraSecBuffer->BufferType = SECBUFFER_MISSING;
					ExtraSecBuffer->cbBuffer = FirstRecordCb - InputSecBuffer->cbBuffer;
					ExtraSecBuffer->pvBuffer = NULL;
				}

				// Don't use SP_LOG_RESULT macro here, since it will erroneously log
				// SEC_E_INCOMPLETE_MESSAGE as an error.
				return SEC_E_INCOMPLETE_MESSAGE;
			}

			// Limit to complete records only.
			IoInformation.InBufferCb = CompleteTlsRecordsCb;
		}

		// See schannel!CSsl3TlsContext::SetApplicationProtocols (10.0.14361)
		if (AlpnSecBuffer && FirstCall) {
			SecStatus = SppSetApplicationProtocols(Context, AlpnSecBuffer);

			if (FAILED(SecStatus)) {
				SppDeleteSecurityContext(NewContextHandle);
				return SecStatus;
			}
		}
	}

	//
	// Initiate the connection or shutdown.
	//

	if (Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		SecStatus = TlsConnect(Context, &IoInformation);
	} else if (Context->State == CONTEXTSTATE_SENDING_CLOSE_NOTIFY) {
		SecStatus = TlsShutdown(Context, &IoInformation);
	}

	//
	// If there's still data in the input buffer left over, we need to
	// inform the caller by using the ExtraSecBuffer
	//

	if (InputSecBuffer && IoInformation.InBufferReadCb < InputSecBuffer->cbBuffer) {
		if (ExtraSecBuffer) {
			ASSERT (ExtraSecBuffer->BufferType == SECBUFFER_EMPTY);

			ExtraSecBuffer->BufferType = SECBUFFER_EXTRA;

			ExtraSecBuffer->cbBuffer = InputSecBuffer->cbBuffer -
									   IoInformation.InBufferReadCb;

			ExtraSecBuffer->pvBuffer = RVA_TO_VA(
				IoInformation.InBuffer,
				IoInformation.InBufferReadCb);
		}
	}

	//
	// Write output buffer information back to the caller.
	// We need to tell the caller how much data we wrote into the output buffer,
	// and if the caller specified ISC_REQ_ALLOCATE_MEMORY, then we need to pass
	// back the new pointer as well.
	//

	if (OutputSecBuffer) {
		OutputSecBuffer->pvBuffer = IoInformation.OutBuffer;
		OutputSecBuffer->cbBuffer = IoInformation.OutBufferWrittenCb;
	}

	//
	// Check the error code from TlsConnect or TlsShutdown.
	//

	if (SecStatus == SEC_E_OK) {
		switch (Context->State) {
		case CONTEXTSTATE_HANDSHAKE_COMPLETE:
			// Gather some information (we need the handshake to be finished for this,
			// because we want to know the ciphers).
			SppGetHeaderBlockAndTrailerSize(Context);
			break;
		case CONTEXTSTATE_CONNECTION_CLOSED:
			break;
		default:
			NOT_REACHED;
		}
	} else if (FirstCall && FAILED(SecStatus)) {
		//
		// TlsConnect returned a fatal error. If this was the first call,
		// we must clean up the context ourselves.
		//

		SppDeleteSecurityContext(NewContextHandle);
	}

	if (SecStatus == SEC_E_OK || SecStatus == SEC_I_CONTINUE_NEEDED) {
		if (Expiry) {
			*Expiry = Context->Credential->Expiry;
		}
	}

	ASSERT (SecStatus != SEC_E_INTERNAL_ERROR);
	return SecStatus;
}

SECURITY_STATUS SppDeleteSecurityContext(
	IN	PCtxtHandle	ContextHandle)
{
	PKXSCHANL_CONTEXT Context;

	ASSERT (SecIsValidHandle(ContextHandle));
	Context = SppReadContextHandle(ContextHandle);

	SafeCertFreeCertificateContext(Context->RemoteCertContext);
	SafeCertCloseStore(Context->RemoteCertStore, 0);

	// This may be created even if we end up selecting TLS 1.2 so it will be
	// destroyed unconditionally.
	SafeBCryptDestroyKey(Context->EphemeralKey13);

	if (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION) {
		SafeSslFreeObject(Context->ReadKey12);
		SafeSslFreeObject(Context->WriteKey12);
		SafeSslFreeObject(Context->HandshakeHash12);
		SafeSslFreeObject(Context->MasterKey12);
		SafeSslFreeObject(Context->EphemeralKey12);
	} else if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
		SafeBCryptDestroyHash(Context->HandshakeHash13);
		SafeBCryptDestroyKey(Context->ServerEphemeralKey13);
	}

	SafeFree(Context->SavedClientHello);
	SafeFree(Context->FragmentedMessage);

	// Zero out to prevent potentially sensitive data from being leaked
	KexRtlZeroMemory(Context, sizeof(*Context));

	SafeFree(Context);
	SecInvalidateHandle(ContextHandle);

	return SEC_E_OK;
}