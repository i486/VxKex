///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlshs13.c
//
// Abstract:
//
//     TLS 1.3 version-specific handshake functions.
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
//     vxiiduu               26-May-2026  Initial creation (stubs).
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

STATIC SECURITY_STATUS TlspKeyFromSecret13(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	BCRYPT_ALG_HANDLE	CipherAlgHandle,
	IN	ULONG				SymmetricKeyCb,
	IN	PCUCHAR				Secret,
	IN	ULONG				SecretCb,
	OUT	PUCHAR				EncDecIv,
	IN	ULONG				EncDecIvCb,
	OUT	PBCRYPT_KEY_HANDLE	KeyHandle)
{
	NTSTATUS Status;
	PBYTE RawKey;

	*KeyHandle = NULL;
	RawKey = StackAlloc(BYTE, SymmetricKeyCb);
	
	Status = HkdfExpandLabel(
		HmacAlgHandle,
		Secret,
		SecretCb,
		"key",
		StringLiteralLength("key"),
		NULL,
		0,
		RawKey,
		SymmetricKeyCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Status = HkdfExpandLabel(
		HmacAlgHandle,
		Secret,
		SecretCb,
		"iv",
		StringLiteralLength("iv"),
		NULL,
		0,
		EncDecIv,
		EncDecIvCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Status = BCryptGenerateSymmetricKey(
		CipherAlgHandle,
		KeyHandle,
		NULL,
		0,
		RawKey,
		SymmetricKeyCb,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	return SEC_E_OK;
}

//
// Sets Context->ReadKey13 and Context->WriteKey13 as well as their associated
// initialization vectors (ReadKeyIv13 and WriteKeyIv13).
//
STATIC SECURITY_STATUS TlspKeysFromSecrets13(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCUCHAR				ReadSecret,
	IN		ULONG				ReadSecretCb,
	IN		PCUCHAR				WriteSecret,
	IN		ULONG				WriteSecretCb)
{
	SECURITY_STATUS SecStatus;
	BCRYPT_ALG_HANDLE HmacAlgHandle;
	BCRYPT_ALG_HANDLE CipherAlgHandle;
	ULONG SymmetricKeyCb;
	ULONG EncDecIvCb;
	PBYTE RawKey;

	ASSERT (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION);

	//
	// Remove old keys (e.g. if we are replacing handshake keys with application
	// data keys).
	//

	SafeBCryptDestroyKey(Context->ReadKey13);
	SafeBCryptDestroyKey(Context->WriteKey13);

	//
	// Query some information about the cipher suite.
	//

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		NULL,
		&HmacAlgHandle,
		NULL);

	ASSERT (SUCCEEDED(SecStatus));

	SecStatus = TlspLookupCipherSuiteCipherInfo13(
		Context->CipherSuite,
		&CipherAlgHandle,
		&SymmetricKeyCb,
		&EncDecIvCb);

	ASSERT (SUCCEEDED(SecStatus));

	RawKey = StackAlloc(BYTE, SymmetricKeyCb);

	//
	// Populate the context key handles and IVs.
	//

	ASSERT (EncDecIvCb <= sizeof(Context->ReadKeyIv13));
	ASSERT (EncDecIvCb <= sizeof(Context->WriteKeyIv13));

	SecStatus = TlspKeyFromSecret13(
		HmacAlgHandle,
		CipherAlgHandle,
		SymmetricKeyCb,
		ReadSecret,
		ReadSecretCb,
		Context->ReadKeyIv13,
		EncDecIvCb,
		&Context->ReadKey13);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	SecStatus = TlspKeyFromSecret13(
		HmacAlgHandle,
		CipherAlgHandle,
		SymmetricKeyCb,
		WriteSecret,
		WriteSecretCb,
		Context->WriteKeyIv13,
		EncDecIvCb,
		&Context->WriteKey13);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}
	
	//
	// Reset sequence numbers back to 0.
	//
	//   Each sequence number is
	//   set to zero at the beginning of a connection and whenever the key is
	//   changed; the first record transmitted under a particular traffic key
	//   MUST use sequence number 0.
	//
	//   https://datatracker.ietf.org/doc/html/rfc8446.html#section-5.3
	//

	Context->ReadSequenceNumber = 0;
	Context->WriteSequenceNumber = 0;

	return SEC_E_OK;
}

SECURITY_STATUS TlspDeriveHandshakeKeys13(
	IN OUT	PKXSCHANL_CONTEXT	Context)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_SECRET_HANDLE SecretHandle;
	PBYTE EcdhSecret;
	ULONG EcdhSecretCb;
	ULONG EcdhSecretWrittenCb;
	ULONG ClientKeyBits;
	BCRYPT_ALG_HANDLE HashAlgHandle;
	BCRYPT_ALG_HANDLE HmacAlgHandle;
	PBYTE Zero;
	PBYTE EarlySecret;
	BCRYPT_HASH_HANDLE EmptyHashHandle;
	PBYTE DerivedSecret;
	PBYTE HandshakeSecret;
	ULONG HmacOutputCb;

	ASSERT (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION);
	ASSERT (Context->HandshakeHash13 != NULL);
	ASSERT (Context->EphemeralKey13 != NULL);
	ASSERT (Context->ServerEphemeralKey13 != NULL);
	ASSERT (Context->ReadKey13 == NULL);
	ASSERT (Context->WriteKey13 == NULL);

	//
	// Get the ECDH secret using our client ephemeral key and the server's
	// ephemeral key.
	//

	Status = BCryptSecretAgreement(
		Context->EphemeralKey13,
		Context->ServerEphemeralKey13,
		&SecretHandle,
		0);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyKey(Context->EphemeralKey13);
	SafeBCryptDestroyKey(Context->ServerEphemeralKey13);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	SecStatus = TlspGetEccKeyBits(Context->EccKeyType, &ClientKeyBits);
	ASSERT (SUCCEEDED(SecStatus));

	EcdhSecretCb = BITS_TO_BYTES(ClientKeyBits);
	EcdhSecret = StackAlloc(BYTE, EcdhSecretCb);

	Status = BCryptDeriveKey(
		SecretHandle,
		BCRYPT_KDF_RAW_SECRET,
		NULL,
		EcdhSecret,
		EcdhSecretCb,
		&EcdhSecretWrittenCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (EcdhSecretWrittenCb == EcdhSecretCb);
	SafeBCryptDestroySecret(SecretHandle);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	EcdhSecretCb = EcdhSecretWrittenCb;

	// BCryptDeriveKey gives us little endian but we need big endian
	KexRtlReverseCopyMemoryInPlace(EcdhSecret, EcdhSecretCb);

	//
	// Calculate the Early Secret.
	// Since we do not support PSK, the early secret is always a predictable value.
	//

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		&HashAlgHandle,
		&HmacAlgHandle,
		&HmacOutputCb);

	ASSERT (SUCCEEDED(SecStatus));

	EarlySecret = StackAlloc(BYTE, HmacOutputCb);
	Zero = StackAlloc(BYTE, HmacOutputCb);
	KexRtlZeroMemory(Zero, HmacOutputCb);

	Status = HkdfExtract(
		HmacAlgHandle,
		Zero,
		HmacOutputCb,
		Zero,
		HmacOutputCb,
		EarlySecret,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Derive the handshake secret from the early secret and the results of the
	// ECDH key exchange.
	//

	// Need an empty hash to derive the handshake secret.
	Status = BCryptCreateHash(
		HashAlgHandle,
		&EmptyHashHandle,
		NULL,
		0,
		NULL,
		0,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	DerivedSecret = StackAlloc(BYTE, HmacOutputCb);

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		EarlySecret,
		HmacOutputCb,
		"derived",
		StringLiteralLength("derived"),
		EmptyHashHandle,
		DerivedSecret,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		SafeBCryptDestroyHash(EmptyHashHandle);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	HandshakeSecret = StackAlloc(BYTE, HmacOutputCb);

	Status = HkdfExtract(
		HmacAlgHandle,
		EcdhSecret,
		EcdhSecretCb,
		DerivedSecret,
		HmacOutputCb,
		HandshakeSecret,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));
	
	if (!NT_SUCCESS(Status)) {
		SafeBCryptDestroyHash(EmptyHashHandle);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Derive the client handshake secret.
	//

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		HandshakeSecret,
		HmacOutputCb,
		"c hs traffic",
		StringLiteralLength("c hs traffic"),
		Context->HandshakeHash13,
		Context->BaseSecret13,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		SafeBCryptDestroyHash(EmptyHashHandle);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Derive the server handshake secret.
	//

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		HandshakeSecret,
		HmacOutputCb,
		"s hs traffic",
		StringLiteralLength("s hs traffic"),
		Context->HandshakeHash13,
		Context->ServerBaseSecret13,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		SafeBCryptDestroyHash(EmptyHashHandle);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Derive the master secret which we will store in the context.
	// Later on the master secret will be used for generating application traffic
	// keys.
	//

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		HandshakeSecret,
		HmacOutputCb,
		"derived",
		StringLiteralLength("derived"),
		EmptyHashHandle,
		DerivedSecret,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyHash(EmptyHashHandle);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	ASSERT (HmacOutputCb <= sizeof(Context->MasterSecret13));

	Status = HkdfExtract(
		HmacAlgHandle,
		Zero,
		HmacOutputCb,
		DerivedSecret,
		HmacOutputCb,
		Context->MasterSecret13,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Generate the handshake read and write keys from the server and
	// client handshake secrets.
	//

	SecStatus = TlspKeysFromSecrets13(
		Context,
		Context->ServerBaseSecret13,
		HmacOutputCb,
		Context->BaseSecret13,
		HmacOutputCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	Context->ReadKeyEnabled = TRUE;
	Context->WriteKeyEnabled = TRUE;
	return SEC_E_OK;
}

SECURITY_STATUS TlspDeriveApplicationKeys13(
	IN OUT	PKXSCHANL_CONTEXT	Context)
{
	SECURITY_STATUS SecStatus;
	ULONG HashOutputCb;

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		NULL,
		NULL,
		&HashOutputCb);

	ASSERT (SUCCEEDED(SecStatus));

	SecStatus = TlspKeysFromSecrets13(
		Context,
		Context->PendingReadSecret13,
		HashOutputCb,
		Context->PendingWriteSecret13,
		HashOutputCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	return SEC_E_OK;
}

SECURITY_STATUS TlspHashHandshakeData13(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCVOID				Data,
	IN		ULONG				DataCb)
{
	NTSTATUS Status;

	ASSERT (Context != NULL);
	ASSERT (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION);
	ASSERT (Data != NULL && DataCb >= 0);

	if (Context->HandshakeHash13 == NULL) {
		SECURITY_STATUS SecStatus;
		BCRYPT_ALG_HANDLE HashAlgHandle;

		SecStatus = TlspLookupCipherSuiteHashInfo13(
			Context->CipherSuite,
			&HashAlgHandle,
			NULL,
			NULL);

		ASSERT (SUCCEEDED(SecStatus));

		Status = BCryptCreateHash(
			HashAlgHandle,
			&Context->HandshakeHash13,
			NULL,
			0,
			NULL,
			0,
			0);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}
	}

	ASSERT (Context->HandshakeHash13 != NULL);

	Status = BCryptHashData(
		Context->HandshakeHash13,
		(PUCHAR) Data,
		DataCb,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	return SEC_E_OK;
}

// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.3.1
STATIC SECURITY_STATUS TlspHandleEncryptedExtensions13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION ExtensionsIoInformation;
	USHORT ExtensionsCb;

	//
	// Set up an IO information just for the extensions data.
	//

	SecStatus = IoReadSwap16(IoInformation, &ExtensionsCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	KexRtlZeroMemory(&ExtensionsIoInformation, sizeof(ExtensionsIoInformation));
	ExtensionsIoInformation.InBufferCb = ExtensionsCb;

	SecStatus = IoRead(
		IoInformation,
		&ExtensionsIoInformation.InBuffer,
		ExtensionsIoInformation.InBufferCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	IoInformation = NULL;

	//
	// Process extensions until nothing's left.
	//

	until (ExtensionsIoInformation.InBufferReadCb == ExtensionsIoInformation.InBufferCb) {
		TLS_EXTENSION_SPEC Extension;

		SecStatus = TlspDeserializeExtension(
			Context,
			&ExtensionsIoInformation,
			&Extension);

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		SecStatus = TlspHandleExtensions(
			Context,
			&Extension,
			1);

		if (FAILED(SecStatus)) {
			return SecStatus;
		}
	}

	Context->State = CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE;
	return SEC_E_OK;
}

// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.4.2
STATIC SECURITY_STATUS TlspHandleCertificate13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION CertListIoInformation;
	UCHAR CertificateRequestContextCb;
	ULONG CertificateListCb;

	SecStatus = IoRead8(IoInformation, &CertificateRequestContextCb);
	SecStatus = IoReadSwap24(IoInformation, &CertificateListCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (CertificateRequestContextCb > 0 || CertificateListCb == 0) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Set up IO information for certificate list.
	//

	KexRtlZeroMemory(&CertListIoInformation, sizeof(CertListIoInformation));
	CertListIoInformation.InBufferCb = CertificateListCb;

	SecStatus = IoRead(
		IoInformation,
		&CertListIoInformation.InBuffer,
		CertListIoInformation.InBufferCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	IoInformation = NULL;

	//
	// Create certificate store to hold incoming certificates.
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
	// Add each certificate to the certificate store.
	// The first certificate (the server's certificate) is referenced in
	// Context->RemoteCertContext.
	//

	do {
		BOOL Success;
		PCVOID CertData;
		ULONG CertDataCb;
		USHORT ExtensionsCb;

		SecStatus = IoReadSwap24(&CertListIoInformation, &CertDataCb);
		SecStatus = IoRead(&CertListIoInformation, &CertData, CertDataCb);
		SecStatus = IoReadSwap16(&CertListIoInformation, &ExtensionsCb);

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (CertDataCb < 1 || ExtensionsCb != 0) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

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
	// Check the server's certificate.
	//

	SecStatus = TlspCheckServerCertificateOk(Context);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	Context->State = CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE_VERIFY;
	return SEC_E_OK;
}

// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.4.3
STATIC SECURITY_STATUS TlspHandleCertificateVerify13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_HASH_HANDLE DuplicatedHash;
	USHORT SignatureScheme;
	USHORT SignatureCb;
	PCVOID Signature;
	PBYTE SignedData;
	ULONG SignedDataCb;
	PBYTE HandshakeHash;
	ULONG HandshakeHashCb;

	//
	// Deserialize the message contents.
	//

	SecStatus = IoReadSwap16(IoInformation, &SignatureScheme);
	SecStatus = IoReadSwap16(IoInformation, &SignatureCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = IoRead(IoInformation, &Signature, SignatureCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Get the current handshake hash.
	//

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		NULL,
		NULL,
		&HandshakeHashCb);

	ASSERT (SUCCEEDED(SecStatus));

	HandshakeHash = StackAlloc(BYTE, HandshakeHashCb);

	Status = BCryptDuplicateHash(
		Context->HandshakeHash13,
		&DuplicatedHash,
		NULL,
		0,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Status = BCryptFinishHash(
		DuplicatedHash,
		HandshakeHash,
		HandshakeHashCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyHash(DuplicatedHash);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// The server signs, using the private key in its certificate, the
	// concatenation of the following data:
	//   1. 64 ASCII space characters.
	//   2. The ASCII string "TLS 1.3, server CertificateVerify".
	//   3. A single zero byte.
	//   4. The current handshake hash.
	//
	// We will concatenate all that data into a buffer.
	//

	SignedDataCb =
		64 +
		StringLiteralLength("TLS 1.3, server CertificateVerify") +
		1 +
		HandshakeHashCb;

	SignedData = StackAlloc(BYTE, SignedDataCb);

	KexRtlFillMemory(SignedData, 64, ' ');
	
	KexRtlCopyMemory(
		SignedData + 64,
		"TLS 1.3, server CertificateVerify",
		sizeof("TLS 1.3, server CertificateVerify"));

	KexRtlCopyMemory(
		SignedData + 64 + sizeof("TLS 1.3, server CertificateVerify"),
		HandshakeHash,
		HandshakeHashCb);

	//
	// Verify the signature.
	//

	SecStatus = TlspVerifyCertificateSignature(
		Context,
		SignatureScheme,
		SignedData,
		SignedDataCb,
		Signature,
		SignatureCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	Context->State = CONTEXTSTATE_TLS1_3_EXPECTING_FINISHED;
	return SEC_E_OK;
}

// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.4.4
STATIC SECURITY_STATUS TlspHandleFinished13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_ALG_HANDLE HashAlgHandle;
	BCRYPT_ALG_HANDLE HmacAlgHandle;
	BCRYPT_HASH_HANDLE DuplicatedHash;
	ULONG HashOutputCb;
	PBYTE FinishedKey;
	PBYTE HandshakeHash;
	PBYTE OurVerifyData;
	PCBYTE TheirVerifyData;

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		&HashAlgHandle,
		&HmacAlgHandle,
		&HashOutputCb);

	ASSERT (SUCCEEDED(SecStatus));

	//
	// The Finished message just consists of the plain verify data.
	//

	SecStatus = IoRead(IoInformation, &TheirVerifyData, HashOutputCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Calculate the finished key.
	//

	FinishedKey = StackAlloc(BYTE, HashOutputCb);

	Status = HkdfExpandLabel(
		HmacAlgHandle,
		Context->ServerBaseSecret13,
		HashOutputCb,
		"finished",
		StringLiteralLength("finished"),
		NULL,
		0,
		FinishedKey,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// The verify data is a HMAC of the transcript hash. Duplicate the transcript
	// hash and get its value.
	//

	Status = BCryptDuplicateHash(
		Context->HandshakeHash13,
		&DuplicatedHash,
		NULL,
		0,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	HandshakeHash = StackAlloc(BYTE, HashOutputCb);

	Status = BCryptFinishHash(
		DuplicatedHash,
		HandshakeHash,
		HashOutputCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyHash(DuplicatedHash);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Calculate the verify data.
	//

	OurVerifyData = StackAlloc(BYTE, HashOutputCb);

	Status = BCryptHash(
		HmacAlgHandle,
		FinishedKey,
		HashOutputCb,
		HandshakeHash,
		HashOutputCb,
		OurVerifyData,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}
	
	//
	// Check if our verify data matches the server's verify data. If not, it's
	// an error.
	//

	if (!RtlEqualMemory(OurVerifyData, TheirVerifyData, HashOutputCb)) {
		return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
	}

	Context->State = CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC;
	return SEC_E_OK;
}

// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.6.1
STATIC SECURITY_STATUS TlspHandleNewSessionTicket13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	ULONG TicketLifetime;
	ULONG TicketAgeAdd;
	UCHAR TicketNonceCb;
	PCVOID TicketNonce;
	USHORT TicketCb;
	PCVOID Ticket;
	USHORT ExtensionsCb;
	PCVOID Extensions;

	//
	// Deserialize the message.
	//

	SecStatus = IoReadSwap32(IoInformation, &TicketLifetime);
	SecStatus = IoReadSwap32(IoInformation, &TicketAgeAdd);
	SecStatus = IoRead8(IoInformation, &TicketNonceCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = IoRead(IoInformation, &TicketNonce, TicketNonceCb);
	SecStatus = IoReadSwap16(IoInformation, &TicketCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = IoRead(IoInformation, &Ticket, TicketCb);
	SecStatus = IoReadSwap16(IoInformation, &ExtensionsCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}
	
	SecStatus = IoRead(IoInformation, &Extensions, ExtensionsCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Ignore the message contents since we don't currently support session tickets.
	//

	return SEC_E_OK;
}

SECURITY_STATUS TlspProcessHandshakeMessage13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						MessageType)
{
	SECURITY_STATUS SecStatus;

	switch (MessageType) {
	case TLS_HSMSG_ENCRYPTED_EXTENSIONS:
		if (Context->State != CONTEXTSTATE_TLS1_3_EXPECTING_ENCRYPTED_EXTENSIONS) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleEncryptedExtensions13(Context, IoInformation);
		break;

	case TLS_HSMSG_CERTIFICATE:
		if (Context->State != CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleCertificate13(Context, IoInformation);
		break;

	case TLS_HSMSG_CERTIFICATE_VERIFY:
		if (Context->State != CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE_VERIFY) {
			goto UnexpectedMessage;
		}
		
		SecStatus = TlspHandleCertificateVerify13(Context, IoInformation);
		break;

	case TLS_HSMSG_FINISHED:
		if (Context->State != CONTEXTSTATE_TLS1_3_EXPECTING_FINISHED) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleFinished13(Context, IoInformation);
		break;

	case TLS_HSMSG_NEW_SESSION_TICKET:
		if (Context->State != CONTEXTSTATE_HANDSHAKE_COMPLETE) {
			goto UnexpectedMessage;
		}

		SecStatus = TlspHandleNewSessionTicket13(Context, IoInformation);
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

STATIC SECURITY_STATUS TlspBuildFinished13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_HASH_HANDLE DuplicatedHash;
	BCRYPT_ALG_HANDLE HmacAlgHandle;
	ULONG HashOutputCb;
	PBYTE FinishedKey;
	PBYTE VerifyData;
	PBYTE HandshakeHash;

	SecStatus = TlspLookupCipherSuiteHashInfo13(
		Context->CipherSuite,
		NULL,
		&HmacAlgHandle,
		&HashOutputCb);

	ASSERT (SUCCEEDED(SecStatus));

	//
	// Compute the finished key.
	//

	FinishedKey = StackAlloc(BYTE, HashOutputCb);

	Status = HkdfExpandLabel(
		HmacAlgHandle,
		Context->BaseSecret13,
		HashOutputCb,
		"finished",
		StringLiteralLength("finished"),
		NULL,
		0,
		FinishedKey,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Get the current transcript hash.
	//

	Status = BCryptDuplicateHash(
		Context->HandshakeHash13,
		&DuplicatedHash,
		NULL,
		0,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	HandshakeHash = StackAlloc(BYTE, HashOutputCb);

	Status = BCryptFinishHash(
		DuplicatedHash,
		HandshakeHash,
		HashOutputCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyHash(DuplicatedHash);

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Compute the verify data.
	//

	VerifyData = StackAlloc(BYTE, HashOutputCb);

	Status = BCryptHash(
		HmacAlgHandle,
		FinishedKey,
		HashOutputCb,
		HandshakeHash,
		HashOutputCb,
		VerifyData,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// The Finished message is just the verify data.
	//

	SecStatus = IoWrite(IoInformation, VerifyData, HashOutputCb);
	
	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	*MessageType = TLS_HSMSG_FINISHED;

	//
	// Derive new read and write secrets which will be used for encrypting and decrypting
	// application data later.
	//

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		Context->MasterSecret13,
		HashOutputCb,
		"s ap traffic",
		StringLiteralLength("s ap traffic"),
		Context->HandshakeHash13,
		Context->PendingReadSecret13,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Status = HkdfDeriveSecret(
		HmacAlgHandle,
		Context->MasterSecret13,
		HashOutputCb,
		"c ap traffic",
		StringLiteralLength("c ap traffic"),
		Context->HandshakeHash13,
		Context->PendingWriteSecret13,
		HashOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Context->State = CONTEXTSTATE_HANDSHAKE_COMPLETE;

	// Clean up stuff we don't need after the handshake.
	SafeBCryptDestroyHash(Context->HandshakeHash13);
	SafeBCryptDestroyKey(Context->EphemeralKey13);
	SafeBCryptDestroyKey(Context->ServerEphemeralKey13);

	KexRtlZeroMemory(Context->MasterSecret13, sizeof(Context->MasterSecret13));

	return SEC_E_OK;
}

SECURITY_STATUS TlspGenerateHandshakeMessage13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType)
{
	SECURITY_STATUS SecStatus;

	switch (Context->State) {
	case CONTEXTSTATE_TLS1_3_SENDING_FINISHED:
		SecStatus = TlspBuildFinished13(Context, IoInformation, MessageType);
		break;

	default:
		SecStatus = SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		break;
	}

	return SecStatus;
}