///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsutil.c
//
// Abstract:
//
//     Miscellaneous small functions
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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// This function fixes the endianness of the TLS record header.
//
INLINE VOID TlsDeserializeRecordHeader(
	IN	PCTLS_RECORD_HEADER	HeaderIn,
	OUT	PTLS_RECORD_HEADER	HeaderOut)
{
	HeaderOut->Type				= HeaderIn->Type;
	HeaderOut->ProtocolVersion	= ByteSwap16(HeaderIn->ProtocolVersion);
	HeaderOut->DataCb			= ByteSwap16(HeaderIn->DataCb);
}

INLINE VOID TlspSerializeRecordHeader(
	OUT	PTLS_RECORD_HEADER	HeaderOut,
	IN	UCHAR				ContentType,
	IN	USHORT				ProtocolVersion,
	IN	USHORT				DataCb)
{
	// make sure we have the right endianness
	ASSERT (ProtocolVersion == TLS1_2_PROTOCOL_VERSION);

	HeaderOut->Type				= ContentType;
	HeaderOut->ProtocolVersion	= ByteSwap16(ProtocolVersion);
	HeaderOut->DataCb			= ByteSwap16(DataCb);
}

// Validate an incoming record header considering the current protocol version,
// context state, encryption state, etc.
// If not valid, returns an appropriate SECURITY_STATUS to pass to the application.
SECURITY_STATUS TlspValidateRecordHeader(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCTLS_RECORD_HEADER	Header)
{
	ULONG MaxPermissibleDataCb;

	// Note: Protocol version is not validated.
	// The specification does not require checking it.

	//
	// Validate content type.
	//

	switch (Header->Type) {
	case CT_CHANGE_CIPHER_SPEC:
	case CT_ALERT:
	case CT_HANDSHAKE:
		if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
			if (Context->ReadKeyEnabled && Header->Type != CT_CHANGE_CIPHER_SPEC) {
				// All records must be encrypted CT_APPLICATIONDATA after encryption
				// is turned on, except for change cipher spec.
				ASSERT (FALSE);
				goto BadContentType;
			}

			if (!TlspIsHandshakeInProgress(Context)) {
				// These messages no longer permitted outside of the handshake.
				ASSERT (FALSE);
				goto BadContentType;
			}
		} else {
			if (Header->Type == CT_ALERT) {
				// Alerts are allowed at any time in TLS 1.2.
				break;
			}

			if (!TlspIsHandshakeInProgress(Context) &&
				Context->State != CONTEXTSTATE_TLS1_2_EXPECTING_CHANGE_CIPHER_SPEC) {

				// Change Cipher Spec not permitted here.
				ASSERT (FALSE);
				goto BadContentType;
			}
		}

		break;
	case CT_APPLICATIONDATA:
		if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
			if (Context->ReadKeyEnabled == FALSE) {
				// Application Data not permitted if encryption not enabled.
				ASSERT (FALSE);
				goto BadContentType;
			}
		} else if (TlspIsHandshakeInProgress(Context)) {
			// Application Data not permitted before handshake completion for
			// TLS 1.2 (or indeterminate).
			ASSERT (FALSE);
			goto BadContentType;
		}

		break;
	default:
	BadContentType:
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Validate record data length.
	//

	MaxPermissibleDataCb = TLS_MAX_PLAINTEXT_CB;

	if (Context->ReadKeyEnabled) {
		if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
			// When encryption is enabled we get an extra 255 bytes for encryption
			// overhead + 1 byte for real content-type.
			MaxPermissibleDataCb = TLS_MAX_PLAINTEXT_CB + 255 + 1;
		} else {
			// When encryption is enabled we get an extra 2kb of permissible payload
			// length.
			MaxPermissibleDataCb = TLS_MAX_PLAINTEXT_CB + 2048;
		}
	}

	if (Header->DataCb > MaxPermissibleDataCb) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	return SEC_E_OK;
}

//
// Validates the body of a TLS protocol_name_list.
// The format is one byte containing the length of a protocol name,
// followed by the protocol name (of the length indicated by the preceding
// byte), repeated until the end of the list.
// Refer to schannel!CSsl3TlsContext::ValidateApplicationProtocolList.
//
SECURITY_STATUS TlsValidateApplicationProtocolList(
	IN	PCKXSCHANL_CONTEXT	Context,
	IN	PCUCHAR				ProtocolList,
	IN	USHORT				ProtocolListCb,
	IN	BOOLEAN				AllowMultipleProtocolIds)
{
	ULONG Index;

	ASSERT (Context != NULL);
	ASSERT (ProtocolList != NULL);

	// must be at least 2 bytes for a single nonzero length byte + at least
	// one byte representing a protocol
	if (ProtocolListCb < 2 ||
		ProtocolListCb > sizeof(Context->ApplicationProtocols)) {

		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// If we aren't allowing more than one protocol, then we just need to make
	// sure that the length of the first protocol in the list plus the length
	// byte itself is equal to the length of the entire list.
	//

	if (!AllowMultipleProtocolIds) {
		UCHAR FirstProtocolCb;

		FirstProtocolCb = *ProtocolList;

		if (FirstProtocolCb < sizeof(UCHAR) ||
			FirstProtocolCb + sizeof(UCHAR) != ProtocolListCb) {

			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		return SEC_E_OK;
	}

	//
	// Otherwise loop through all the protocols, making sure that each protocol
	// length is greater than or equal to 1 as per the TLS spec, and also that
	// we don't overflow or underflow the list length.
	//

	Index = 0;

	do {
		UCHAR ProtocolCb;

		ProtocolCb = ProtocolList[Index];

		if (ProtocolCb < sizeof(UCHAR)) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		++Index;

		if (Index >= ProtocolListCb) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		Index += ProtocolCb;

		if (Index > ProtocolListCb) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	} until (Index == ProtocolListCb);

	return SEC_E_OK;
}

//
// The Protocol parameter is NOT a length prefixed list of any kind.
// It is just a raw, non-null-terminated string such as "h2".
// Call with trusted/validated data only.
//
BOOLEAN TlsIsProtocolInProtocolList(
	IN	PCUCHAR				Protocol,
	IN	UCHAR				ProtocolCb,
	IN	PCUCHAR				ProtocolList,
	IN	USHORT				ProtocolListCb)
{
	ULONG Index;

	Index = 0;

	do {
		UCHAR ListProtocolCb;

		ListProtocolCb = ProtocolList[Index];
		++Index;

		if (ListProtocolCb == ProtocolCb) {
			if (RtlEqualMemory(Protocol, &ProtocolList[Index], ProtocolCb)) {
				return TRUE;
			}
		}

		Index += ListProtocolCb + 1;
	} until (Index >= ProtocolListCb);

	return FALSE;
}

PCWSTR TlsMapCipherSuiteToIanaName(
	IN	USHORT				CipherSuite)
{
	switch (CipherSuite) {
	// TLS 1.3
	case 0x1301: return L"TLS_AES_128_GCM_SHA256";
	case 0x1302: return L"TLS_AES_256_GCM_SHA384";

	// TLS 1.2
	case 0x002F: return L"TLS_RSA_WITH_AES_128_CBC_SHA";
	case 0x0035: return L"TLS_RSA_WITH_AES_256_CBC_SHA";
	case 0xC013: return L"TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA";
	case 0xC014: return L"TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA";
	case 0xC02B: return L"TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256";
	case 0xC02C: return L"TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384";
	case 0xC02F: return L"TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256";
	case 0xC030: return L"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384";

	default:
		NOT_REACHED;
	}
}

//
// Converts an ECC key from wire format to BCrypt/NCrypt format.
// KeyType is BCRYPT_ECDH_PUBLIC_P*_MAGIC (e.g. BCRYPT_ECDH_PUBLIC_P256_MAGIC)
//
SECURITY_STATUS TlspEccKeyToKeyBlob(
	IN	ULONG	EccKeyType,
	IN	PCBYTE	EccKey,
	IN	ULONG	EccKeyCb,
	OUT	PBYTE	KeyBlob,
	IN	ULONG	KeyBlobCb)
{
	ULONG KeyBits;
	ULONG CoordinateCb;
	PCBYTE PointIn;
	PBYTE PointOut;
	ULONG PointCb;
	ULONG ExpectedEccKeyCb;
	ULONG ExpectedKeyBlobCb;
	PBCRYPT_ECCKEY_BLOB BcryptKeyBlob;
	ULONG BcryptKeyMagic;

	switch (EccKeyType) {
	case TLS_GROUP_SECP256R1:
		BcryptKeyMagic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
		KeyBits = 256;
		break;
	case TLS_GROUP_SECP384R1:
		BcryptKeyMagic = BCRYPT_ECDH_PUBLIC_P384_MAGIC;
		KeyBits = 384;
		break;
	case TLS_GROUP_SECP521R1:
		BcryptKeyMagic = BCRYPT_ECDH_PUBLIC_P521_MAGIC;
		KeyBits = 521;
		break;
	default:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Verify buffer lengths.
	//

	// 0x04 X Y, where X and Y are data of length CoordinateCb each
	CoordinateCb = BITS_TO_BYTES(KeyBits);
	PointCb = 2 * CoordinateCb;
	ExpectedEccKeyCb = PointCb + 1;
	ExpectedKeyBlobCb = sizeof(BCRYPT_ECCKEY_BLOB) + PointCb;

	if (EccKeyCb != ExpectedEccKeyCb) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	if (KeyBlobCb < ExpectedKeyBlobCb) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Make sure the first byte is 0x04 (uncompressed point).
	// If that's not true it means the server sent bad data, or we parsed
	// the message wrong.
	//

	if (EccKey[0] != 0x04) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Copy ECC point data into the bcrypt blob.
	//

	BcryptKeyBlob = (PBCRYPT_ECCKEY_BLOB) KeyBlob;
	PointIn = &EccKey[1];
	PointOut = (PBYTE) &BcryptKeyBlob[1];

	// Note: Even though we copy PointCb bytes after the structure, for some
	// reason cbKey is not actually the number of bytes after the structure
	// but rather the number of BITS in the key divided by 8 and rounded up.
	// Very confusing and not documented.
	BcryptKeyBlob->dwMagic = BcryptKeyMagic;
	BcryptKeyBlob->cbKey = BITS_TO_BYTES(KeyBits);

	KexRtlCopyMemory(PointOut, PointIn, PointCb);

	return SEC_E_OK;
}

//
// DER signature format: 0x30 <Cb1..3> 0x02 <RCb> <R[RCb]> 0x02 <SCb> <S[SCb]>
// where Cb == DerSignatureCb - 1 (minus one for the 0x30)
//
SECURITY_STATUS TlspDerSignatureToRaw(
	IN	ULONG	CertificateKeyBits,
	IN	PCBYTE	DerSignature,
	IN	ULONG	DerSignatureCb,
	OUT	PBYTE	RawSignature,
	IN	ULONG	RawSignatureCb)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION SignatureIoInformation;
	ULONG CoordinateCb;
	ULONG PointCb;
	UCHAR SequenceId;
	UCHAR SequenceCb;
	UCHAR IntegerId1;
	UCHAR IntegerId2;
	UCHAR RCb;
	UCHAR SCb;
	PCBYTE R;
	PCBYTE S;

	CoordinateCb = BITS_TO_BYTES(CertificateKeyBits);
	PointCb = CoordinateCb * 2;

	ASSERT (RawSignatureCb == PointCb);

	if (DerSignatureCb < 8 || DerSignatureCb > 256) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (RawSignatureCb < PointCb) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Create an IO information structure to cover the signature.
	//

	KexRtlZeroMemory(&SignatureIoInformation, sizeof(SignatureIoInformation));
	SignatureIoInformation.InBuffer = DerSignature;
	SignatureIoInformation.InBufferCb = DerSignatureCb;

	//
	// Deserialize and verify all fields in the DER signature.
	//

	SecStatus = IoRead8(&SignatureIoInformation, &SequenceId);
	SecStatus = IoRead8(&SignatureIoInformation, &SequenceCb);

	if (FAILED(SecStatus)) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	// Handle multi-byte length field which is needed only for secp521r1.
	if (SequenceCb == 0x81) {
		if (CertificateKeyBits != 521) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		SecStatus = IoRead8(&SignatureIoInformation, &SequenceCb);
	} else if (SequenceCb < 0x80) {
		// It's ok, just use SequenceCb as is.
		NOTHING;
	} else {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = IoRead8(&SignatureIoInformation, &IntegerId1);
	SecStatus = IoRead8(&SignatureIoInformation, &RCb);
	SecStatus = IoRead(&SignatureIoInformation, (PPCVOID) &R, RCb);
	SecStatus = IoRead8(&SignatureIoInformation, &IntegerId2);
	SecStatus = IoRead8(&SignatureIoInformation, &SCb);
	SecStatus = IoRead(&SignatureIoInformation, (PPCVOID) &S, SCb);

	if (FAILED(SecStatus)) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (SignatureIoInformation.InBufferReadCb != SignatureIoInformation.InBufferCb) {
		// Junk data was found at the end of the signature
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (SequenceId != 0x30) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (RCb == 0 || SCb == 0) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (IntegerId1 != 0x02 || IntegerId2 != 0x02) {
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Inspect R and S.
	//

	if (RCb > CoordinateCb) {
		if (RCb != CoordinateCb + 1) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (R[0] != 0 || R[1] < 0x80) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		++R;
		--RCb;
	}

	ASSERT (RCb <= CoordinateCb);

	if (SCb > CoordinateCb) {
		if (SCb != CoordinateCb + 1) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (S[0] != 0 || S[1] < 0x80) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		++S;
		--SCb;
	}

	ASSERT (SCb <= CoordinateCb);

	//
	// Place R and S into the raw signature buffer, left-padded with zeroes.
	//

	KexRtlZeroMemory(RawSignature, CoordinateCb - RCb);
	KexRtlCopyMemory(RawSignature + (CoordinateCb - RCb), R, RCb);
	KexRtlZeroMemory(RawSignature + CoordinateCb, CoordinateCb - SCb);
	KexRtlCopyMemory(RawSignature + CoordinateCb + (CoordinateCb - SCb), S, SCb);

	return SEC_E_OK;
}

//
// Get a BCrypt or NCrypt key handle of the server's public key from the
// server's certificate. Free the key handle with SafeNCryptFreeObject or
// SafeBCryptDestroyKey after using it.
//
SECURITY_STATUS TlspGetServerPublicKeyFromCertificate(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	OUT		PNCRYPT_KEY_HANDLE	PublicKeyNCrypt OPTIONAL,
	OUT		PBCRYPT_KEY_HANDLE	PublicKeyBCrypt OPTIONAL)
{
	BOOL Success;
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_KEY_HANDLE BCryptKeyHandle;
	NCRYPT_PROV_HANDLE KeyStorageProvider;
	PBYTE KeyBlob;
	ULONG KeyBlobCb;

	ASSERT (Context->CipherSuite != 0);
	ASSERT (Context->RemoteCertContext != NULL);
	ASSERT (PublicKeyNCrypt != NULL || PublicKeyBCrypt != NULL);

	//
	// Get a BCrypt key handle which represents the public key from the server's
	// certificate.
	//

	Success = CryptImportPublicKeyInfoEx2(
		X509_ASN_ENCODING,
		&Context->RemoteCertContext->pCertInfo->SubjectPublicKeyInfo,
		CRYPT_OID_INFO_PUBKEY_SIGN_KEY_FLAG,
		NULL,
		&BCryptKeyHandle);

	ASSERT (Success);

	if (!Success) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	if (PublicKeyBCrypt) {
		*PublicKeyBCrypt = BCryptKeyHandle;
	}

	// If we don't need to give the caller a ncrypt handle, return here
	if (!PublicKeyNCrypt) {
		return SEC_E_OK;
	}

	//
	// Export the BCrypt key to a blob.
	//

	KeyBlobCb = 0;

	// Find out how much to allocate
	Status = BCryptExportKey(
		BCryptKeyHandle,
		NULL,
		BCRYPT_PUBLIC_KEY_BLOB,
		NULL,
		0,
		&KeyBlobCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (KeyBlobCb != 0);

	if (!NT_SUCCESS(Status) || KeyBlobCb == 0) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	KeyBlob = StackAlloc(BYTE, KeyBlobCb);

	Status = BCryptExportKey(
		BCryptKeyHandle,
		NULL,
		BCRYPT_PUBLIC_KEY_BLOB,
		KeyBlob,
		KeyBlobCb,
		&KeyBlobCb,
		0);

	ASSERT (NT_SUCCESS(Status));

	// If we didn't pass the BCrypt handle to the caller, get rid of it now
	if (!PublicKeyBCrypt) {
		SafeBCryptDestroyKey(BCryptKeyHandle);
	}

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Open the NCrypt storage provider and import the key blob, passing the
	// key handle to the caller.
	//

	SecStatus = NCryptOpenStorageProvider(
		&KeyStorageProvider,
		MS_KEY_STORAGE_PROVIDER,
		0);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	SecStatus = NCryptImportKey(
		KeyStorageProvider,
		0,
		BCRYPT_PUBLIC_KEY_BLOB,
		NULL,
		PublicKeyNCrypt,
		KeyBlob,
		KeyBlobCb,
		0);

	ASSERT (SUCCEEDED(SecStatus));
	SafeNCryptFreeObject(KeyStorageProvider);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	return SEC_E_OK;
}

//
// Verify the server's certificate according to the parameters set by the
// application.
// Refer to schannel.dll!VerifyServerCertificate.
//
SECURITY_STATUS TlspCheckServerCertificateOk(
	IN OUT	PKXSCHANL_CONTEXT	Context)
{
	BOOL Success;
	ULONG ChainFlags;
	HCERTCHAINENGINE ChainEngine;
	CERT_CHAIN_PARA ChainParameters;
	CERT_CHAIN_POLICY_PARA PolicyParameters;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	SSL_EXTRA_CERT_CHAIN_POLICY_PARA HttpsPolicy;
	PCCERT_CHAIN_CONTEXT ChainContext;

	PSTR UsageOids[] = {
		szOID_PKIX_KP_SERVER_AUTH,
		szOID_SERVER_GATED_CRYPTO,
		szOID_SGC_NETSCAPE
	};

	ASSERT (Context->RemoteCertContext != NULL);
	ASSERT (Context->RemoteCertStore != NULL);

	//
	// If the application has specified not to validate certificates, then we will
	// just return OK straight away.
	//

	// Assert for mutually exclusive flags.
	// Mutually exclusive flags should have been filtered out by AcquireCredentialsHandle
	// and InitializeSecurityContext.
	ASSERT (PopulationCount(Context->Flags & (ISC_REQ_MUTUAL_AUTH |
											  ISC_REQ_MANUAL_CRED_VALIDATION)) <= 1);

	ASSERT (PopulationCount(Context->Credential->Flags & (SCH_CRED_AUTO_CRED_VALIDATION |
														  SCH_CRED_MANUAL_CRED_VALIDATION)) <= 1);

	if (Context->Flags & ISC_REQ_MUTUAL_AUTH) {
		// fall through and DO validate the certificate
		NOTHING;
	} else if (Context->Flags & ISC_REQ_MANUAL_CRED_VALIDATION) {
		return SEC_E_OK;
	} else {
		if (Context->Credential->Flags & SCH_CRED_MANUAL_CRED_VALIDATION) {
			return SEC_E_OK;
		}
	}

	//
	// Figure out what certificate chain engine we'll use.
	// If we have a ROOT.sst file loaded, then we'll set up an engine using that
	// root file; if not, we will use the default engine.
	//

	ChainEngine = NULL;

	if (Context->Credential->RootCertStore != NULL) {
		CERT_CHAIN_ENGINE_CONFIG EngineConfig;

		KexRtlZeroMemory(&EngineConfig, sizeof(EngineConfig));
		EngineConfig.cbSize = sizeof(EngineConfig);
		EngineConfig.hExclusiveRoot = Context->Credential->RootCertStore;

		// If CertCreateCertificateChainEngine fails, it sets ChainEngine to NULL.
		// In that case we will of course just use the default chain engine, which
		// is OK.
		Success = CertCreateCertificateChainEngine(
			&EngineConfig,
			&ChainEngine);

		ASSERT (Success);
	}

	//
	// Set flags based on what the application specified.
	//

	ChainFlags = 0;

	if (Context->Credential->Flags & SCH_CRED_REVOCATION_CHECK_END_CERT) {
		ChainFlags |= CERT_CHAIN_REVOCATION_CHECK_END_CERT;
	}

	if (Context->Credential->Flags & SCH_CRED_REVOCATION_CHECK_CHAIN) {
		ChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
	}

	if (Context->Credential->Flags & SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT) {
		ChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;
	}

	//
	// Get a certificate chain context.
	//

	KexRtlZeroMemory(&ChainParameters, sizeof(ChainParameters));
	ChainParameters.cbSize										= sizeof(ChainParameters);
	ChainParameters.RequestedUsage.dwType						= USAGE_MATCH_TYPE_OR;
	ChainParameters.RequestedUsage.Usage.cUsageIdentifier		= ARRAYSIZE(UsageOids);
	ChainParameters.RequestedUsage.Usage.rgpszUsageIdentifier	= UsageOids;

	Success = CertGetCertificateChain(
		ChainEngine,
		Context->RemoteCertContext,
		NULL,
		Context->RemoteCertStore,
		&ChainParameters,
		ChainFlags,
		NULL,
		&ChainContext);

	SafeCertFreeCertificateChainEngine(ChainEngine);
	ASSERT (Success);

	if (!Success) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Validate the certificate chain.
	//

	KexRtlZeroMemory(&HttpsPolicy, sizeof(HttpsPolicy));
	HttpsPolicy.cbSize = sizeof(HttpsPolicy);
	HttpsPolicy.dwAuthType = AUTHTYPE_SERVER;

	unless (Context->Credential->Flags & SCH_CRED_NO_SERVERNAME_CHECK) {
		ULONG Cch;

		if (Context->TargetNameCch == 0) {
			SafeCertFreeCertificateChain(ChainContext);
			return SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);
		}

		Cch = MultiByteToWideChar(
			CP_ACP,
			MB_ERR_INVALID_CHARS,
			Context->TargetName,
			Context->TargetNameCch + 1,
			NULL,
			0);

		ASSERT (Cch > 0);

		if (Cch == 0) {
			SafeCertFreeCertificateChain(ChainContext);
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		HttpsPolicy.pwszServerName = StackAlloc(WCHAR, Cch);

		Cch = MultiByteToWideChar(
			CP_ACP,
			MB_ERR_INVALID_CHARS,
			Context->TargetName,
			Context->TargetNameCch + 1,
			HttpsPolicy.pwszServerName,
			Cch);

		ASSERT (Cch > 0);

		if (Cch == 0) {
			SafeCertFreeCertificateChain(ChainContext);
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}
	}

	KexRtlZeroMemory(&PolicyParameters, sizeof(PolicyParameters));
	PolicyParameters.cbSize = sizeof(PolicyParameters);
	PolicyParameters.pvExtraPolicyPara = &HttpsPolicy;

	Success = CertVerifyCertificateChainPolicy(
		CERT_CHAIN_POLICY_SSL,
		ChainContext,
		&PolicyParameters,
		&PolicyStatus);

	ASSERT (Success);
	SafeCertFreeCertificateChain(ChainContext);

	if (!Success) {
		return GetLastError();
	}

	//
	// Ignore certain errors if requested.
	//

	if (Context->Credential->Flags & SCH_CRED_IGNORE_NO_REVOCATION_CHECK) {
		if (PolicyStatus.dwError == CRYPT_E_NO_REVOCATION_CHECK) {
			PolicyStatus.dwError = S_OK;
		}
	}

	if (Context->Credential->Flags & SCH_CRED_IGNORE_REVOCATION_OFFLINE) {
		if (PolicyStatus.dwError == CRYPT_E_REVOCATION_OFFLINE) {
			PolicyStatus.dwError = S_OK;
		}
	}

	//
	// Map CERT_E_* or CRYPT_E_* to the appropriate return code.
	// This is done the same way as Schannel does it - if any error code other
	// than the ones listed is returned, then we just ignore it and return OK.
	//

	switch (PolicyStatus.dwError) {
	case CERT_E_EXPIRED:
	case CERT_E_VALIDITYPERIODNESTING:
		return SP_LOG_RESULT(SEC_E_CERT_EXPIRED);

	case CERT_E_UNTRUSTEDROOT:
	case CERT_E_UNTRUSTEDCA:
		return SP_LOG_RESULT(SEC_E_UNTRUSTED_ROOT);

	case CERT_E_REVOKED:
		return CRYPT_E_REVOKED;

	case CERT_E_CN_NO_MATCH:
		return SP_LOG_RESULT(SEC_E_WRONG_PRINCIPAL);

	case CERT_E_WRONG_USAGE:
		return SP_LOG_RESULT(SEC_E_CERT_WRONG_USAGE);
	}

	return SEC_E_OK;
}

//
// The ECDHE_RSA cipher suites in GCM mode are not supported by Windows 7,
// and the SSL provider functions will fail if they encounter this suite. As a
// hack to make these work, we can just pretend that we're using ECDHE_ECDSA
// variants of the cipher suites instead, and everything will work.
//
// Note that you should not use this function when querying the cipher suite
// for informational purposes, i.e. do not pass to SslLookupCipherSuiteInfo and
// similar functions. Otherwise you'll get fake data returned.
//
USHORT TlspMapCipherSuiteForNcrypt(
	IN	USHORT	RealCipherSuite)
{
	switch (RealCipherSuite) {
	case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
		return TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
	case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
		return TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
	default:
		return RealCipherSuite;
	}
}

SECURITY_STATUS TlspGetEccKeyBits(
	IN	USHORT	EccKeyType,
	OUT	PULONG	EphemeralKeyBits)
{
	*EphemeralKeyBits = 0;

	switch (EccKeyType) {
	case TLS_GROUP_SECP256R1:
		*EphemeralKeyBits	= 256;
		break;
	case TLS_GROUP_SECP384R1:
		*EphemeralKeyBits	= 384;
		break;
	case TLS_GROUP_SECP521R1:
		*EphemeralKeyBits	= 521;
		break;
	default:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
	}

	return SEC_E_OK;
}

//
// CertificateKeyBits should only be used for TLS 1.3.
// For TLS 1.2, the signature scheme does not indicate the number of bits in the
// certificate's public key.
//
SECURITY_STATUS TlspGetKeyExchangeParameters(
	IN	USHORT				SignatureScheme,
	IN	USHORT				EccKeyType,
	OUT	PPCWSTR				HashAlgorithm,
	OUT	PBCRYPT_ALG_HANDLE	HashAlgHandle,
	OUT	PUCHAR				HashCb,
	OUT	PULONG				EphemeralKeyBits,
	OUT	PULONG				CertificateKeyBits OPTIONAL)
{
	SECURITY_STATUS SecStatus;
	ULONG CertificateKeyBitsTemp;

	*HashAlgorithm = NULL;
	*HashCb = 0;
	*EphemeralKeyBits = 0;
	CertificateKeyBitsTemp = 0;

	switch (SignatureScheme) {
	case TLS_SIGSCHEME_ECDSA_SECP256R1_SHA256:
		CertificateKeyBitsTemp	= 256;
		*HashAlgorithm			= BCRYPT_SHA256_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA256_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(256);
		break;
	case TLS_SIGSCHEME_ECDSA_SECP384R1_SHA384:
		CertificateKeyBitsTemp	= 384;
		*HashAlgorithm			= BCRYPT_SHA384_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA384_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(384);
		break;
	case TLS_SIGSCHEME_ECDSA_SECP521R1_SHA512:
		CertificateKeyBitsTemp	= 521;
		*HashAlgorithm			= BCRYPT_SHA512_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA512_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(512);
		break;
	case TLS_SIGSCHEME_RSA_PKCS1_SHA256:
	case TLS_SIGSCHEME_RSA_PSS_RSAE_SHA256:
		*HashAlgorithm			= BCRYPT_SHA256_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA256_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(256);
		break;
	case TLS_SIGSCHEME_RSA_PKCS1_SHA384:
	case TLS_SIGSCHEME_RSA_PSS_RSAE_SHA384:
		*HashAlgorithm			= BCRYPT_SHA384_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA384_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(384);
		break;
	case TLS_SIGSCHEME_RSA_PKCS1_SHA512:
	case TLS_SIGSCHEME_RSA_PSS_RSAE_SHA512:
		*HashAlgorithm			= BCRYPT_SHA512_ALGORITHM;
		*HashAlgHandle			= BCRYPT_SHA512_ALG_HANDLE;
		*HashCb					= BITS_TO_BYTES(512);
		break;
	default:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
	}

	if (CertificateKeyBits) {
		*CertificateKeyBits = CertificateKeyBitsTemp;
	}

	SecStatus = TlspGetEccKeyBits(EccKeyType, EphemeralKeyBits);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	return SEC_E_OK;
}

//
// Does essentially the same thing as SslLookupCipherSuiteInfo.
// However, that function is quite broken because certain valid combinations of cipher
// suite and key type are rejected for no apparent reason. Also, it doesn't support
// TLS 1.3 protocol version or cipher suite, so we'll just reimplement it.
//
SECURITY_STATUS TlsLookupCipherSuiteInfo(
	IN	USHORT						ProtocolVersion,
	IN	USHORT						CipherSuite,
	IN	USHORT						EccKeyType,
	OUT	PNCRYPT_SSL_CIPHER_SUITE	SuiteInfo)
{
	HRESULT Result;
	PCWSTR szCipher;
	PCWSTR szHash;
	PCWSTR szCertificate;
	PCWSTR szExchange;
	BOOLEAN Ecdhe;

	szCipher = NULL;
	szHash = NULL;
	szCertificate = NULL;
	szExchange = NULL;
	Ecdhe = FALSE;

	KexRtlZeroMemory(SuiteInfo, sizeof(*SuiteInfo));

	SuiteInfo->dwProtocol			= ProtocolVersion;
	SuiteInfo->dwCipherSuite		= CipherSuite;
	SuiteInfo->dwBaseCipherSuite	= CipherSuite;
	SuiteInfo->dwKeyType			= EccKeyType;

	Result = StringCchCopy(
		SuiteInfo->szCipherSuite,
		ARRAYSIZE(SuiteInfo->szCipherSuite),
		TlsMapCipherSuiteToIanaName(CipherSuite));

	ASSERT (SUCCEEDED(Result));

	// Set a default because most of the cipher suites are AES.
	szCipher = BCRYPT_AES_ALGORITHM;
	SuiteInfo->dwCipherBlockLen = 16;

	switch (CipherSuite) {
	case TLS_AES_128_GCM_SHA256:
		SuiteInfo->dwCipherLen = 128;
		szCertificate = L"";
		Ecdhe = TRUE;
		break;
	case TLS_AES_256_GCM_SHA384:
		SuiteInfo->dwCipherLen = 256;
		szCertificate = L"";
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384:
		SuiteInfo->dwCipherLen = 256;
		szCertificate = BCRYPT_ECDSA_ALGORITHM;
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256:
		SuiteInfo->dwCipherLen = 128;
		szCertificate = BCRYPT_ECDSA_ALGORITHM;
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384:
		SuiteInfo->dwCipherLen = 256;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
		SuiteInfo->dwCipherLen = 128;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
		SuiteInfo->dwCipherLen = 128;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		szHash = BCRYPT_SHA1_ALGORITHM;
		SuiteInfo->dwHashLen = 20;
		Ecdhe = TRUE;
		break;
	case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
		SuiteInfo->dwCipherLen = 256;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		szHash = BCRYPT_SHA1_ALGORITHM;
		SuiteInfo->dwHashLen = 20;
		Ecdhe = TRUE;
		break;
	case TLS_RSA_WITH_AES_128_CBC_SHA:
		SuiteInfo->dwCipherLen = 128;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		szHash = BCRYPT_SHA1_ALGORITHM;
		SuiteInfo->dwHashLen = 20;
		break;
	case TLS_RSA_WITH_AES_256_CBC_SHA:
		SuiteInfo->dwCipherLen = 256;
		szCertificate = BCRYPT_RSA_ALGORITHM;
		szHash = BCRYPT_SHA1_ALGORITHM;
		SuiteInfo->dwHashLen = 20;
		break;
	default:
		ASSERT (FALSE);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	if (Ecdhe) {
		ASSERT (EccKeyType != 0);

		switch (EccKeyType) {
		case TLS_GROUP_SECP256R1:
			szExchange = BCRYPT_ECDH_P256_ALGORITHM;
			SuiteInfo->dwMinExchangeLen = 256;
			SuiteInfo->dwMaxExchangeLen = 256;

			Result = StringCchCat(
				SuiteInfo->szCipherSuite,
				ARRAYSIZE(SuiteInfo->szCipherSuite),
				L"_P256");

			break;
		case TLS_GROUP_SECP384R1:
			szExchange = BCRYPT_ECDH_P384_ALGORITHM;
			SuiteInfo->dwMinExchangeLen = 384;
			SuiteInfo->dwMaxExchangeLen = 384;

			Result = StringCchCat(
				SuiteInfo->szCipherSuite,
				ARRAYSIZE(SuiteInfo->szCipherSuite),
				L"_P384");

			break;
		case TLS_GROUP_SECP521R1:
			szExchange = BCRYPT_ECDH_P521_ALGORITHM;
			SuiteInfo->dwMinExchangeLen = 521;
			SuiteInfo->dwMaxExchangeLen = 521;
			
			Result = StringCchCat(
				SuiteInfo->szCipherSuite,
				ARRAYSIZE(SuiteInfo->szCipherSuite),
				L"_P521");

			break;
		default:
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		ASSERT (SUCCEEDED(Result));
	}

	Result = StringCchCopy(
		SuiteInfo->szCipher,
		ARRAYSIZE(SuiteInfo->szCipher),
		szCipher);

	ASSERT (SUCCEEDED(Result));

	if (szHash) {
		Result = StringCchCopy(
			SuiteInfo->szHash,
			ARRAYSIZE(SuiteInfo->szHash),
			szHash);

		ASSERT (SUCCEEDED(Result));
	}

	if (szExchange) {
		Result = StringCchCopy(
			SuiteInfo->szExchange,
			ARRAYSIZE(SuiteInfo->szExchange),
			szExchange);

		ASSERT (SUCCEEDED(Result));
	}

	if (szCertificate) {
		Result = StringCchCopy(
			SuiteInfo->szCertificate,
			ARRAYSIZE(SuiteInfo->szCertificate),
			szCertificate);

		ASSERT (SUCCEEDED(Result));
	}

	return SEC_E_OK;
}

//
// Get information for a TLS 1.3 cipher suite's hash algorithm. This hash algorithm is
// used for calculating the transcript hash and (in its HMAC form) is used for deriving
// keys and traffic secrets.
//
// The returned BCrypt handles are predefined handles and do not need to be freed.
//
SECURITY_STATUS TlspLookupCipherSuiteHashInfo13(
	IN	USHORT						CipherSuite,
	OUT	PBCRYPT_ALG_HANDLE			HashAlgHandle OPTIONAL,
	OUT	PBCRYPT_ALG_HANDLE			HmacAlgHandle OPTIONAL,
	OUT	PULONG						HashOutputCb OPTIONAL)
{
	ULONG HashOutputCbTemp;

	switch (CipherSuite) {
	case TLS_AES_128_GCM_SHA256:
	case TLS_CHACHA20_POLY1305_SHA256:
		if (HashAlgHandle) {
			*HashAlgHandle = BCRYPT_SHA256_ALG_HANDLE;
		}

		if (HmacAlgHandle) {
			*HmacAlgHandle = BCRYPT_HMAC_SHA256_ALG_HANDLE;
		}

		HashOutputCbTemp = BITS_TO_BYTES(256);
		break;
	case TLS_AES_256_GCM_SHA384:
		if (HashAlgHandle) {
			*HashAlgHandle = BCRYPT_SHA384_ALG_HANDLE;
		}

		if (HmacAlgHandle) {
			*HmacAlgHandle = BCRYPT_HMAC_SHA384_ALG_HANDLE;
		}

		HashOutputCbTemp = BITS_TO_BYTES(384);
		break;
	default:
		NOT_REACHED;
	}

	if (HashOutputCb) {
		*HashOutputCb = HashOutputCbTemp;
	}

	// The following fields of the context structure are used to store HMAC or HKDF
	// outputs and must be large enough.
	ASSERT (RTL_FIELD_SIZE(KXSCHANL_CONTEXT, MasterSecret13) >= HashOutputCbTemp);
	ASSERT (RTL_FIELD_SIZE(KXSCHANL_CONTEXT, BaseSecret13) >= HashOutputCbTemp);
	ASSERT (RTL_FIELD_SIZE(KXSCHANL_CONTEXT, ServerBaseSecret13) >= HashOutputCbTemp);
	ASSERT (RTL_FIELD_SIZE(KXSCHANL_CONTEXT, PendingReadSecret13) >= HashOutputCbTemp);
	ASSERT (RTL_FIELD_SIZE(KXSCHANL_CONTEXT, PendingWriteSecret13) >= HashOutputCbTemp);

	return SEC_E_OK;
}

SECURITY_STATUS TlspLookupCipherSuiteCipherInfo13(
	IN	USHORT						CipherSuite,
	OUT	PBCRYPT_ALG_HANDLE			CipherAlgHandle OPTIONAL,
	OUT	PULONG						SymmetricKeyCb OPTIONAL,
	OUT	PULONG						EncDecIvCb OPTIONAL)
{
	switch (CipherSuite) {
	case TLS_AES_128_GCM_SHA256:
		if (CipherAlgHandle) {
			*CipherAlgHandle = BCRYPT_AES_GCM_ALG_HANDLE;
		}

		if (SymmetricKeyCb) {
			*SymmetricKeyCb = BITS_TO_BYTES(128);
		}

		if (EncDecIvCb) {
			*EncDecIvCb = 12;
		}

		break;
	case TLS_AES_256_GCM_SHA384:
		if (CipherAlgHandle) {
			*CipherAlgHandle = BCRYPT_AES_GCM_ALG_HANDLE;
		}

		if (SymmetricKeyCb) {
			*SymmetricKeyCb = BITS_TO_BYTES(256);
		}

		if (EncDecIvCb) {
			*EncDecIvCb = 12;
		}

		break;
	default:
		NOT_REACHED;
	}

	return SEC_E_OK;
}

//
// Verify a signature over a hash of some data.
// This function will hash the data using the hashing algorithm specified in the
// SignatureScheme. There is no need to pre-hash the data.
//
SECURITY_STATUS TlspVerifyCertificateSignature(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN		USHORT						SignatureScheme,
	IN		PCUCHAR						SignedData,
	IN		ULONG						SignedDataCb,
	IN		PCUCHAR						Signature,
	IN		ULONG						SignatureCb)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	BCRYPT_KEY_HANDLE ServerCertPublicKey;
	PBYTE Hash;
	PCWSTR HashAlgorithm;
	BCRYPT_ALG_HANDLE HashAlgHandle;
	UCHAR HashCb;
	ULONG EphemeralKeyBits;

	//
	// Get key exchange parameters from the signature scheme.
	//

	SecStatus = TlspGetKeyExchangeParameters(
		SignatureScheme,
		Context->EccKeyType,
		&HashAlgorithm,
		&HashAlgHandle,
		&HashCb,
		&EphemeralKeyBits,
		NULL);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Hash the signed data using the hashing algorithm specified in the
	// signature scheme.
	//

	Hash = StackAlloc(BYTE, HashCb);

	Status = BCryptHash(
		HashAlgHandle,
		NULL,
		0,
		SignedData,
		SignedDataCb,
		Hash,
		HashCb);

	ASSERT (NT_SUCCESS(Status));
	SignedData = NULL;
	SignedDataCb = 0;

	if (!NT_SUCCESS(Status)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Get the server's public key.
	// ServerCertPublicKey needs to be destroyed after use.
	//

	SecStatus = TlspGetServerPublicKeyFromCertificate(
		Context,
		NULL,
		&ServerCertPublicKey);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Process the signature which was given by the server. Depending on which
	// signature scheme, different processing is needed.
	//

	try {
		PUCHAR RawSignature;
		ULONG RawSignatureCb;
		ULONG BCryptVerifySignatureFlags;
		PVOID BCryptVerifySignaturePaddingInfo;

		BCryptVerifySignaturePaddingInfo = NULL;
		BCryptVerifySignatureFlags = 0;
		
		if (SignatureScheme == TLS_SIGSCHEME_RSA_PKCS1_SHA1 ||
			SignatureScheme == TLS_SIGSCHEME_RSA_PKCS1_SHA256 ||
			SignatureScheme == TLS_SIGSCHEME_RSA_PKCS1_SHA384 ||
			SignatureScheme == TLS_SIGSCHEME_RSA_PKCS1_SHA512) {

			BCRYPT_PKCS1_PADDING_INFO *PaddingInfo;

			//
			// RSA: signature sent by the server is already in the format that BCrypt
			// can use (PKCS#1). We also need to tell BCrypt the hash algorithm
			// which is being used in the pszAlgId member.
			//

			PaddingInfo = StackAlloc(BCRYPT_PKCS1_PADDING_INFO, 1);
			PaddingInfo->pszAlgId = HashAlgorithm;

			BCryptVerifySignatureFlags |= BCRYPT_PAD_PKCS1;
			BCryptVerifySignaturePaddingInfo = PaddingInfo;

			RawSignatureCb = SignatureCb;
			RawSignature = (PUCHAR) Signature;
		} else if (SignatureScheme == TLS_SIGSCHEME_RSA_PSS_RSAE_SHA256 ||
				   SignatureScheme == TLS_SIGSCHEME_RSA_PSS_RSAE_SHA384 ||
				   SignatureScheme == TLS_SIGSCHEME_RSA_PSS_RSAE_SHA512) {

			BCRYPT_PSS_PADDING_INFO *PaddingInfo;

			//
			// RSA-PSS: signature sent by the server is directly usable by BCrypt.
			// Tell BCrypt the hash algorithm and salt length (which is the same as
			// the output hash length).
			//

			PaddingInfo = StackAlloc(BCRYPT_PSS_PADDING_INFO, 1);
			PaddingInfo->pszAlgId = HashAlgorithm;
			PaddingInfo->cbSalt = HashCb;

			BCryptVerifySignatureFlags |= BCRYPT_PAD_PSS;
			BCryptVerifySignaturePaddingInfo = PaddingInfo;

			RawSignatureCb = SignatureCb;
			RawSignature = (PUCHAR) Signature;
		} else {
			ULONG CertificateKeyBits;
			ULONG CertificateKeyBitsCb;

			//
			// ECDSA: signature sent by the server is DER-encoded, and we have to call a
			// helper function to convert it to the raw format that CNG uses.
			//

			Status = BCryptGetProperty(
				ServerCertPublicKey,
				BCRYPT_KEY_LENGTH,
				(PUCHAR) &CertificateKeyBits,
				sizeof(CertificateKeyBits),
				&CertificateKeyBitsCb,
				0);

			ASSERT (NT_SUCCESS(Status));
			ASSERT (CertificateKeyBitsCb == sizeof(CertificateKeyBits));

			if (!NT_SUCCESS(Status)) {
				return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
			}

			RawSignatureCb = BITS_TO_BYTES(CertificateKeyBits) * 2;
			RawSignature = StackAlloc(BYTE, RawSignatureCb);

			SecStatus = TlspDerSignatureToRaw(
				CertificateKeyBits,
				Signature,
				SignatureCb,
				RawSignature,
				RawSignatureCb);

			ASSERT (SUCCEEDED(SecStatus));

			if (FAILED(SecStatus)) {
				return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
			}
		}

		//
		// Verify the signature over the hashed data.
		//

		Status = BCryptVerifySignature(
			ServerCertPublicKey,
			BCryptVerifySignaturePaddingInfo,
			Hash,
			HashCb,
			RawSignature,
			RawSignatureCb,
			BCryptVerifySignatureFlags);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_MESSAGE_ALTERED);
		}
	} finally {
		SafeBCryptDestroyKey(ServerCertPublicKey);
	}

	return SEC_E_OK;
}