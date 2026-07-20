///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlshs12.c
//
// Abstract:
//
//     TLS 1.2 client handshake state machine. Processes server handshake
//     messages and generates client responses. Uses the NCrypt SSL Provider
//     API for all cryptographic operations.
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
// Hashes one complete handshake message (including the 4-byte header)
// into the running handshake hash.
//
SECURITY_STATUS TlspHashHandshakeData12(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCVOID				Data,
	IN		ULONG				DataCb)
{
	SECURITY_STATUS SecStatus;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (Data != NULL && DataCb >= 0);

	if (Context->HandshakeHash12 == 0) {
		ASSERT (Context->CipherSuite != 0);

		SecStatus = SslCreateHandshakeHash(
			Context->Credential->SslProvider,
			&Context->HandshakeHash12,
			Context->ProtocolVersion,
			TlspMapCipherSuiteForNcrypt(Context->CipherSuite),
			0);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}
	}

	SecStatus = SslHashHandshake(
		Context->Credential->SslProvider,
		Context->HandshakeHash12,
		(PBYTE) Data,
		DataCb,
		0);

	ASSERT (SUCCEEDED(SecStatus));
	return SecStatus;
}

STATIC SECURITY_STATUS TlspHandleCertificate12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	ULONG CertListCb;
	KXSCHANL_IO_INFORMATION CertListIoInformation;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	//
	// Get the length of the entire certificate list.
	//

	SecStatus = IoReadSwap24(IoInformation, &CertListCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (CertListCb == 0) {
		// Empty certificate list.
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Make a new IO information for the cert list.
	//

	KexRtlZeroMemory(&CertListIoInformation, sizeof(CertListIoInformation));
	
	SecStatus = IoRead(
		IoInformation,
		&CertListIoInformation.InBuffer,
		CertListCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	CertListIoInformation.InBufferCb = CertListCb;
	
	// don't need that anymore
	IoInformation = NULL;

	//
	// Create a memory-based certificate store to put the certificates into.
	//

	Context->RemoteCertStore = CertOpenStore(
		CERT_STORE_PROV_MEMORY,
		0,
		0,
		CERT_STORE_CREATE_NEW_FLAG,
		NULL);

	ASSERT (Context->RemoteCertStore != NULL);

	if (Context->RemoteCertStore == NULL) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	//
	// Loop through the certificate list and store each certificate.
	//

	do {
		BOOL Success;
		PCVOID CertData;
		ULONG CertDataCb;

		//
		// Get length of a certificate.
		//

		SecStatus = IoReadSwap24(&CertListIoInformation, &CertDataCb);

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (CertDataCb < 1) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		//
		// Get a pointer to the certificate.
		//

		SecStatus = IoRead(&CertListIoInformation, &CertData, CertDataCb);

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		//
		// Add the certificate to the certificate store.
		// If this is the first certificate, it means it's the server's certificate
		// and we'll store its PCCERT_CONTEXT.
		//

		Success = CertAddEncodedCertificateToStore(
			Context->RemoteCertStore,
			X509_ASN_ENCODING,
			(PCBYTE) CertData,
			CertDataCb,
			CERT_STORE_ADD_USE_EXISTING,
			Context->RemoteCertContext ? NULL : &Context->RemoteCertContext);

		ASSERT (Success);

		if (!Success) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}
	} until (CertListIoInformation.InBufferReadCb == CertListIoInformation.InBufferCb);

	ASSERT (Context->RemoteCertContext != NULL);

	//
	// Verify the server's certificate.
	// TlspCheckServerCertificateOk handles all application-specified options,
	// including not checking the certificate at all if not needed.
	//

	SecStatus = TlspCheckServerCertificateOk(Context);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Determine whether ServerKeyExchange follows.
	// ECDHE cipher suites (0xC0 prefix) always include it.
	// RSA key exchange suites (0x00 prefix) skip it.
	//

	if (TlspIsStaticRsaKeyExchangeCipherSuite(Context->CipherSuite)) {
		// RSA key exchange: go straight to ServerHelloDone.
		Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_HELLO_DONE;
	} else {
		// ECDHE (ECDSA or RSA signing): expect ServerKeyExchange.
		Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_KEY_EXCHANGE;
	}

	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspHandleServerKeyExchangeEcdhe12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	UCHAR CurveType;
	USHORT EccKeyType;
	UCHAR PublicKeyCb;
	PCUCHAR PublicKey;
	USHORT SignatureScheme;
	USHORT SignatureCb;
	PCUCHAR Signature;
	KXSCHANL_IO_INFORMATION SignedDataIoInformation;

	ULONG EphemeralKeyBits;
	PBYTE KeyBlob;
	ULONG KeyBlobCb;
	NCRYPT_KEY_HANDLE ServerEphemeralPublicKey;
	NCryptBuffer ParamBuffers[2];
	NCryptBufferDesc ParamList;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	//
	// The RFC specs are not very clear on exactly what the format of this
	// handshake message actually is because there's a bunch of deprecated
	// old crap that no one uses anymore, plus a bunch of stupid nested
	// structures which require a lot of scrolling back and forth. I really
	// do not like the structure representation they use in the RFCs.
	//
	// Here's the ACTUAL format which we will be parsing in this function:
	//
	//   UINT8			CurveType;				// TLS_CURVE_TYPE_*
	//   UINT16BE		EccKeyType;				// TLS_GROUP_*
	//   UINT8			PublicKeyCb;			// 1..255
	//   UINT8			PublicKey[PublicKeyCb];
	//   UINT16BE		SignatureScheme;		// TLS_SIGSCHEME_*
	//   UINT16BE		SignatureCb;			// 1..65535
	//   UINT8			Signature[SignatureCb];
	//
	// The signature covers the following concatenated items in order:
	//
	//   Context->ClientRandom
	//   Context->ServerRandom
	//   CurveType
	//   EccKeyType
	//   PublicKeyCb
	//   PublicKey
	//
	// In order to verify the signature, we hash the concatenation of the above
	// data using the hash algorithm specified by EccKeyType (e.g. SHA-256,
	// SHA-384), and then verify the signature is valid for the server's public
	// key from the certificate. That can be done using the BCrypt API.
	//

	//
	// Deserialize the incoming message.
	//

	PublicKeyCb = 0;
	SignatureCb = 0;

	SecStatus = IoRead8(IoInformation, &CurveType);
	SecStatus = IoReadSwap16(IoInformation, &EccKeyType);
	SecStatus = IoRead8(IoInformation, &PublicKeyCb);
	SecStatus = IoRead(IoInformation, (PPCVOID) &PublicKey, PublicKeyCb);
	SecStatus = IoReadSwap16(IoInformation, &SignatureScheme);
	SecStatus = IoReadSwap16(IoInformation, &SignatureCb);
	SecStatus = IoRead(IoInformation, (PPCVOID) &Signature, SignatureCb);

	if (FAILED(SecStatus) || !PublicKey || !Signature) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Validate message contents.
	//

	if (CurveType != TLS_CURVE_TYPE_NAMED_CURVE) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = TlspGetEccKeyBits(EccKeyType, &EphemeralKeyBits);
	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	Context->EccKeyType = EccKeyType;

	if (SignatureCb == 0) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	// 1-byte length + uncompressed point (0x04 || X || Y).
	if (PublicKeyCb != 1 + (BITS_TO_BYTES(EphemeralKeyBits) * 2)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Prepare for signature verification. Concatenate all the signed data:
	//
	//   Context->ClientRandom
	//   Context->ServerRandom
	//   CurveType
	//   EccKeyType
	//   PublicKeyCb
	//   PublicKey
	//
	// into SignedDataIoInformation.
	//

	KexRtlZeroMemory(&SignedDataIoInformation, sizeof(SignedDataIoInformation));

	SignedDataIoInformation.OutBufferCb =
		sizeof(Context->ClientRandom) +
		sizeof(Context->ServerRandom) +
		sizeof(CurveType) +
		sizeof(EccKeyType) +
		sizeof(PublicKeyCb) +
		PublicKeyCb;

	ASSERT (SignedDataIoInformation.OutBufferCb == PublicKeyCb + 68);
	ASSERT (SignedDataIoInformation.OutBufferCb <= 323);

	SignedDataIoInformation.OutBuffer = StackAlloc(UCHAR, SignedDataIoInformation.OutBufferCb);

	SecStatus = IoWrite(&SignedDataIoInformation, Context->ClientRandom, sizeof(Context->ClientRandom));
	SecStatus = IoWrite(&SignedDataIoInformation, Context->ServerRandom, sizeof(Context->ServerRandom));
	SecStatus = IoWrite8(&SignedDataIoInformation, CurveType);
	SecStatus = IoWriteSwap16(&SignedDataIoInformation, EccKeyType);
	SecStatus = IoWrite8(&SignedDataIoInformation, PublicKeyCb);
	SecStatus = IoWrite(&SignedDataIoInformation, PublicKey, PublicKeyCb);

	ASSERT (SUCCEEDED(SecStatus));
	ASSERT (SignedDataIoInformation.OutBufferWrittenCb == SignedDataIoInformation.OutBufferCb);

	//
	// Import the server's public key using SslImportKey.
	//

	KeyBlobCb = sizeof(BCRYPT_ECCKEY_BLOB) + (BITS_TO_BYTES(EphemeralKeyBits) * 2);
	KeyBlob = StackAlloc(BYTE, KeyBlobCb);

	SecStatus = TlspEccKeyToKeyBlob(
		EccKeyType,
		PublicKey,
		PublicKeyCb,
		KeyBlob,
		KeyBlobCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	SecStatus = SslImportKey(
		Context->Credential->SslProvider,
		&ServerEphemeralPublicKey,
		BCRYPT_ECCPUBLIC_BLOB,
		KeyBlob,
		KeyBlobCb,
		0);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	ASSERT (ServerEphemeralPublicKey != 0);

	try {
		//
		// Verify the signature.
		//

		SecStatus = TlspVerifyCertificateSignature(
			Context,
			SignatureScheme,
			(PUCHAR) SignedDataIoInformation.OutBuffer,
			SignedDataIoInformation.OutBufferWrittenCb,
			Signature,
			SignatureCb);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		//
		// Create ephemeral ECDHE key pair.
		//

		SecStatus = SslCreateEphemeralKey(
			Context->Credential->SslProvider,
			&Context->EphemeralKey12,
			Context->ProtocolVersion,
			TlspMapCipherSuiteForNcrypt(Context->CipherSuite),
			EccKeyType,
			EphemeralKeyBits,
			NULL,
			0,
			0);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
		}

		//
		// Create the master secret.
		//

		ParamBuffers[0].BufferType	= NCRYPTBUFFER_SSL_CLIENT_RANDOM;
		ParamBuffers[0].cbBuffer	= sizeof(Context->ClientRandom);
		ParamBuffers[0].pvBuffer	= Context->ClientRandom;

		ParamBuffers[1].BufferType	= NCRYPTBUFFER_SSL_SERVER_RANDOM;
		ParamBuffers[1].cbBuffer	= sizeof(Context->ServerRandom);
		ParamBuffers[1].pvBuffer	= Context->ServerRandom;

		ParamList.ulVersion	= NCRYPTBUFFER_VERSION;
		ParamList.cBuffers	= ARRAYSIZE(ParamBuffers);
		ParamList.pBuffers	= ParamBuffers;

		SecStatus = SslGenerateMasterKey(
			Context->Credential->SslProvider,
			Context->EphemeralKey12,		// our private key
			ServerEphemeralPublicKey,		// server's public key
			&Context->MasterKey12,
			Context->ProtocolVersion,
			TlspMapCipherSuiteForNcrypt(Context->CipherSuite),
			&ParamList,
			NULL,
			0,
			NULL,
			NCRYPT_SSL_CLIENT_FLAG);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}
	} finally {
		SafeSslFreeObject(ServerEphemeralPublicKey);
	}

	//
	// Create the read/write session keys.
	//

	SecStatus = SslGenerateSessionKeys(
		Context->Credential->SslProvider,
		Context->MasterKey12,
		&Context->ReadKey12,
		&Context->WriteKey12,
		&ParamList,
		0);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	// These should still be 0 at this point, since we had no encryption keys
	// prior to the call to this function.
	ASSERT (Context->ReadSequenceNumber == 0);
	ASSERT (Context->WriteSequenceNumber == 0);

	//
	// Advance to the next expected state.
	//

	Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_HELLO_DONE;
	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspHandleServerHelloDone12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN		PCKXSCHANL_IO_INFORMATION	IoInformation)
{
	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	//
	// ServerHelloDone is an empty message (just the 4-byte header).
	// After receiving it, the client sends its key exchange flight.
	//

	if (IoInformation->InBufferReadCb != IoInformation->InBufferCb) {
		// This means there was data in the message, which there isn't
		// supposed to be.
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	Context->State = CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE;
	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspHandleFinished12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	UCHAR OurVerifyData[12];
	PCUCHAR TheirVerifyData;

	ASSERT (Context != NULL);
	ASSERT (IoInformation != NULL);

	//
	// Verify data is always 12 bytes in TLS 1.2.
	//

	SecStatus = IoRead(IoInformation, &TheirVerifyData, 12);
	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Compute expected verify_data for server Finished.
	// Uses NCRYPT_SSL_SERVER_FLAG to select the "server finished" label.
	//

	SecStatus = SslComputeFinishedHash(
		Context->Credential->SslProvider,
		Context->MasterKey12,
		Context->HandshakeHash12,
		OurVerifyData,
		sizeof(OurVerifyData),
		NCRYPT_SSL_SERVER_FLAG);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
	}

	//
	// Compare verify_data.
	//

	if (!RtlEqualMemory(OurVerifyData, TheirVerifyData, 12)) {
		return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
	}

	//
	// Handshake complete.
	//

	Context->State = CONTEXTSTATE_HANDSHAKE_COMPLETE;

	//
	// Free anything that we don't need anymore.
	//

	SafeSslFreeObject(Context->MasterKey12);
	SafeSslFreeObject(Context->HandshakeHash12);
	SafeSslFreeObject(Context->EphemeralKey12);

	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspHandleHelloRequest12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	//
	// A Hello Request can technically be sent at any time. We are supposed
	// to ignore it if we don't support renegotiation, so we'll ignore it.
	// Note that this message is supposed to have zero length.
	//

	return SEC_E_OK;
}

//
// Processes a single, complete handshake message in the IO information buffer.
// The handshake message in the buffer does not include the 4-byte message
// header.
//
// Does not make any use of the output buffer.
//
// MessageType is a TLS_HSMSG_* value.
//
SECURITY_STATUS TlspProcessHandshakeMessage12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						MessageType)
{
	SECURITY_STATUS SecStatus;

	//
	// Dispatch by handshake type.
	// We only accept messages that are valid for the current state.
	// MessageIoInformation describes just the contents of a single message, not
	// including the 4-byte message header.
	//

	switch (MessageType) {
	case TLS_HSMSG_CERTIFICATE:
		if (Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_CERTIFICATE) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleCertificate12(Context, IoInformation);
		break;

	case TLS_HSMSG_SERVER_KEY_EXCHANGE:
		if (Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_KEY_EXCHANGE) {
			goto UnexpectedMessage;
		}
			
		SecStatus = TlspHandleServerKeyExchangeEcdhe12(Context, IoInformation);
		break;

	case TLS_HSMSG_SERVER_HELLO_DONE:
		if (Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_HELLO_DONE) {
			goto UnexpectedMessage;
		}
			
		SecStatus = TlspHandleServerHelloDone12(Context, IoInformation);
		break;

	case TLS_HSMSG_FINISHED:
		if (Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_FINISHED) {
			goto UnexpectedMessage;
		}
			
		SecStatus = TlspHandleFinished12(Context, IoInformation);
		break;

	case TLS_HSMSG_HELLO_REQUEST:
		if (Context->State != CONTEXTSTATE_HANDSHAKE_COMPLETE) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleHelloRequest12(Context, IoInformation);
		break;

	default:
	UnexpectedMessage:
		KexLogErrorEvent(
			L"Unexpected handshake message type %hu in state %d",
			MessageType,
			Context->State);

		SecStatus = SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);

		break;
	}

	return SecStatus;
}

//
// RSA client key exchange is easy because SslGenerateMasterKey literally
// creates the whole thing for us (all we have to do is prepend the length).
// We'll also have to create the read and write keys afterwards.
//
STATIC SECURITY_STATUS TlspBuildClientKeyExchangeRsa12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	NCRYPT_KEY_HANDLE RsaKey;
	NCryptBuffer ParamBuffers[3];
	NCryptBufferDesc ParamList;
	ULONG HighestProtocolVersion;
	UCHAR EncryptedPreMasterSecret[1024];
	ULONG EncryptedPreMasterSecretCb;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE);
	ASSERT (IoInformation != NULL);

	//
	// Get the server's RSA public key from its certificate.
	//

	SecStatus = TlspGetServerPublicKeyFromCertificate(
		Context,
		&RsaKey,
		NULL);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Generate the master key and encrypted pre-master secret (which is the
	// main part of the static RSA ClientKeyExchange).
	//

	ParamBuffers[0].BufferType	= NCRYPTBUFFER_SSL_CLIENT_RANDOM;
	ParamBuffers[0].cbBuffer	= sizeof(Context->ClientRandom);
	ParamBuffers[0].pvBuffer	= Context->ClientRandom;

	ParamBuffers[1].BufferType	= NCRYPTBUFFER_SSL_SERVER_RANDOM;
	ParamBuffers[1].cbBuffer	= sizeof(Context->ServerRandom);
	ParamBuffers[1].pvBuffer	= Context->ServerRandom;

	HighestProtocolVersion		= TLS1_2_PROTOCOL_VERSION;
	ParamBuffers[2].BufferType	= NCRYPTBUFFER_SSL_HIGHEST_VERSION;
	ParamBuffers[2].cbBuffer	= sizeof(HighestProtocolVersion);
	ParamBuffers[2].pvBuffer	= &HighestProtocolVersion;

	ParamList.ulVersion			= NCRYPTBUFFER_VERSION;
	ParamList.cBuffers			= ARRAYSIZE(ParamBuffers);
	ParamList.pBuffers			= ParamBuffers;

	EncryptedPreMasterSecretCb	= sizeof(EncryptedPreMasterSecret);

	ASSERT (Context->MasterKey12 == 0);

	SecStatus = SslGenerateMasterKey(
		Context->Credential->SslProvider,
		0,
		RsaKey,
		&Context->MasterKey12,
		Context->ProtocolVersion,
		TlspMapCipherSuiteForNcrypt(Context->CipherSuite),
		&ParamList,
		EncryptedPreMasterSecret,
		EncryptedPreMasterSecretCb,
		&EncryptedPreMasterSecretCb,
		NCRYPT_SSL_CLIENT_FLAG);

	ASSERT (SUCCEEDED(SecStatus));
	SafeNCryptFreeObject(RsaKey);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Generate read and write keys.
	//

	ASSERT (Context->ReadKey12 == 0);
	ASSERT (Context->WriteKey12 == 0);

	SecStatus = SslGenerateSessionKeys(
		Context->Credential->SslProvider,
		Context->MasterKey12,
		&Context->ReadKey12,
		&Context->WriteKey12,
		&ParamList,
		0);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	ASSERT (Context->ReadSequenceNumber == 0);
	ASSERT (Context->WriteSequenceNumber == 0);

	//
	// Serialize the Client Key Exchange message to the IO information
	// buffer.
	//

	ASSERT (EncryptedPreMasterSecretCb < USHRT_MAX);
	SecStatus = IoWriteSwap16(IoInformation, (USHORT) EncryptedPreMasterSecretCb);
	SecStatus = IoWrite(IoInformation, EncryptedPreMasterSecret, EncryptedPreMasterSecretCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}

//
// Export our ephemeral public key and send it to the server.
//
STATIC SECURITY_STATUS TlspBuildClientKeyExchangeEcdhe12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	ULONG EphemeralKeyBits;
	ULONG CoordinateCb;
	PBYTE KeyBlob;
	ULONG KeyBlobCb;
	ULONG KeyBlobWrittenCb;
	PBYTE X;
	PBYTE Y;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (Context->State == CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE);
	ASSERT (Context->EphemeralKey12 != 0);
	ASSERT (Context->ReadKey12 != 0);
	ASSERT (Context->WriteKey12 != 0);
	ASSERT (IoInformation != NULL);

	//
	// Export the ephemeral public key as a Bcrypt key blob.
	//

	SecStatus = TlspGetEccKeyBits(Context->EccKeyType, &EphemeralKeyBits);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	CoordinateCb = BITS_TO_BYTES(EphemeralKeyBits);
	KeyBlobCb = sizeof(BCRYPT_ECCKEY_BLOB) + (2 * CoordinateCb);
	KeyBlob = StackAlloc(BYTE, KeyBlobCb);

	SecStatus = SslExportKey(
		Context->Credential->SslProvider,
		Context->EphemeralKey12,
		BCRYPT_ECCPUBLIC_BLOB,
		KeyBlob,
		KeyBlobCb,
		&KeyBlobWrittenCb,
		0);

	ASSERT (SUCCEEDED(SecStatus));
	ASSERT (KeyBlobWrittenCb == KeyBlobCb);
	ASSERT (((PBCRYPT_ECCKEY_BLOB) KeyBlob)->cbKey == CoordinateCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Write the client key exchange message body.
	// The format is 1-byte-length-prefixed (0x04 + X + Y).
	//

	X = &KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB)];
	Y = &KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB) + CoordinateCb];

	ASSERT ((CoordinateCb * 2) + 1 <= UCHAR_MAX);

	SecStatus = IoWrite8(IoInformation, (UCHAR) (1 + (2 * CoordinateCb)));
	SecStatus = IoWrite8(IoInformation, 0x04);
	SecStatus = IoWrite(IoInformation, X, CoordinateCb);
	SecStatus = IoWrite(IoInformation, Y, CoordinateCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	return SEC_E_OK;
}

STATIC SECURITY_STATUS TlspBuildFinished12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	UCHAR VerifyData[12];

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (Context->State == CONTEXTSTATE_TLS1_2_SENDING_FINISHED);
	ASSERT (Context->MasterKey12 != 0);
	ASSERT (Context->HandshakeHash12 != 0);
	ASSERT (IoInformation != NULL);

	//
	// Call the NCrypt provider to get the verify data.
	//

	SecStatus = SslComputeFinishedHash(
		Context->Credential->SslProvider,
		Context->MasterKey12,
		Context->HandshakeHash12,
		VerifyData,
		sizeof(VerifyData),
		NCRYPT_SSL_CLIENT_FLAG);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	SecStatus = IoWrite(IoInformation, VerifyData, sizeof(VerifyData));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}

//
// Generates a single handshake message, not including the message header.
// The exact type of handshake message generated is dependent on the context
// state.
// This function updates the context state on each call.
// This function does not make use of the input buffer.
// MessageType is set to a TLS_HSMSG_* value on success.
//
SECURITY_STATUS TlspGenerateHandshakeMessage12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType)
{
	SECURITY_STATUS SecStatus;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (IoInformation != NULL);
	ASSERT (TlspIsResponseState12(Context));
	ASSERT (Context->State != CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC);

	switch (Context->State) {
	case CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE:
		if (TlspIsStaticRsaKeyExchangeCipherSuite(Context->CipherSuite)) {
			SecStatus = TlspBuildClientKeyExchangeRsa12(Context, IoInformation);
		} else {
			SecStatus = TlspBuildClientKeyExchangeEcdhe12(Context, IoInformation);
		}

		*MessageType = TLS_HSMSG_CLIENT_KEY_EXCHANGE;
		Context->State = CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC;
		break;

	case CONTEXTSTATE_TLS1_2_SENDING_FINISHED:
		SecStatus = TlspBuildFinished12(Context, IoInformation);
		*MessageType = TLS_HSMSG_FINISHED;
		Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_CHANGE_CIPHER_SPEC;
		break;

	default:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	return SecStatus;
}