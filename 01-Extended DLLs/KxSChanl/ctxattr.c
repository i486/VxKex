///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ctxattr.c
//
// Abstract:
//
//     Contains the implementation of SetContextAttributes and
//     QueryContextAttributes.
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
//     vxiiduu               11-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

// Required by most users of Schannel, including Curl and WinHTTP.
STATIC INLINE SECURITY_STATUS SppQueryStreamSizes(
	IN	PKXSCHANL_CONTEXT			Context,
	OUT	PSecPkgContext_StreamSizes	StreamSizes)
{
	if (Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		// We don't know the ciphers and whatever before the handshake is complete,
		// so we can't do this.
		return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
	}

	StreamSizes->cbMaximumMessage = TLS_MAX_PLAINTEXT_CB;
	StreamSizes->cBuffers = 4;
	StreamSizes->cbHeader = Context->HeaderSize;
	StreamSizes->cbTrailer = Context->TrailerSize;
	StreamSizes->cbBlockSize = Context->BlockSize;

	return SEC_E_OK;
}

//
// This function NEVER returns anything other than SEC_E_OK.
// On any error condition, a buffer of all zeroes will be returned.
//
STATIC INLINE SECURITY_STATUS SppQueryAlpnProtocol(
	IN	PKXSCHANL_CONTEXT					Context,
	OUT	PSecPkgContext_ApplicationProtocol	AlpnProtocol)
{
	KexRtlZeroMemory(AlpnProtocol, sizeof(*AlpnProtocol));

	if (Context->SelectedApplicationProtocolCb == 0) {
		// Leave *AlpnProtocol all zeroes.
		return SEC_E_OK;
	}

	AlpnProtocol->ProtoNegoStatus = SecApplicationProtocolNegotiationStatus_Success;
	AlpnProtocol->ProtoNegoExt = SecApplicationProtocolNegotiationExt_ALPN;
	AlpnProtocol->ProtocolIdSize = Context->SelectedApplicationProtocolCb;

	ASSERT (Context->SelectedApplicationProtocolCb <= sizeof(Context->SelectedApplicationProtocol));

	STATIC_ASSERT (sizeof(Context->SelectedApplicationProtocol) ==
				   sizeof(AlpnProtocol->ProtocolId));
	
	KexRtlCopyMemory(
		AlpnProtocol->ProtocolId,
		Context->SelectedApplicationProtocol,
		Context->SelectedApplicationProtocolCb);

	return SEC_E_OK;
}

// Required for WinHTTP, but it can be a stub.
STATIC INLINE SECURITY_STATUS SppQueryEndpointBindings(
	IN	PKXSCHANL_CONTEXT					Context,
	OUT	PSecPkgContext_Bindings				Bindings)
{
	// webio.dll specifically checks for this return status and is OK with it.
	// Any other error code and we can't connect with winhttp.
	return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
}

// Required for WinHTTP and WinInet
STATIC INLINE SECURITY_STATUS SppQueryRemoteCertContext(
	IN	PKXSCHANL_CONTEXT					Context,
	OUT	PPCCERT_CONTEXT						CertOut)
{
	*CertOut = NULL;

	if (Context->RemoteCertContext == NULL) {
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	// CertDuplicateCertificateContext just increases a reference count, and
	// it cannot fail.
	*CertOut = CertDuplicateCertificateContext(Context->RemoteCertContext);
	return SEC_E_OK;
}

// Called by app to query the current TLS protocol and algorithms.
// Required by WinHTTP
STATIC INLINE SECURITY_STATUS SppQueryConnectionInfo(
	IN	PKXSCHANL_CONTEXT					Context,
	OUT	PSecPkgContext_ConnectionInfo		ConnectionInfo)
{
	SECURITY_STATUS SecStatus;
	NCRYPT_SSL_CIPHER_SUITE CipherSuite;

	ASSERT (ConnectionInfo != NULL);

	if (Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
	}

	SecStatus = TlsLookupCipherSuiteInfo(
		Context->ProtocolVersion,
		Context->CipherSuite,
		Context->EccKeyType,
		&CipherSuite);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Set dwProtocol
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		ConnectionInfo->dwProtocol = SP_PROT_TLS1_2_CLIENT;
		break;
	case TLS1_3_PROTOCOL_VERSION:
		ConnectionInfo->dwProtocol = SP_PROT_TLS1_3_CLIENT;
		break;
	default:
		NOT_REACHED;
	}

	//
	// Set aiCipher and dwCipherStrength
	//

	ConnectionInfo->dwCipherStrength = CipherSuite.dwCipherLen;

	if (StringEqual(CipherSuite.szCipher, BCRYPT_AES_ALGORITHM)) {
		switch (CipherSuite.dwCipherLen) {
		case 128:
			ConnectionInfo->aiCipher = CALG_AES_128;
			break;
		case 256:
			ConnectionInfo->aiCipher = CALG_AES_256;
			break;
		default:
			NOT_REACHED;
		}
	} else if (StringEqual(CipherSuite.szCipher, BCRYPT_CHACHA20_POLY1305_ALGORITHM)) {
		// Not sure what win10/11 returns, since ChaCha20 does not have an ALG_ID.
		ConnectionInfo->aiCipher = CALG_OID_INFO_CNG_ONLY;
	} else {
		NOT_REACHED;
	}

	//
	// Set aiHash and dwHashStrength
	//

	ConnectionInfo->dwHashStrength = CipherSuite.dwHashLen * 8;

	if (StringEqual(CipherSuite.szHash, BCRYPT_SHA1_ALGORITHM)) {
		ConnectionInfo->aiHash = CALG_SHA1;
	} else if (StringEqual(CipherSuite.szHash, BCRYPT_SHA256_ALGORITHM)) {
		ConnectionInfo->aiHash = CALG_SHA_256;
	} else if (StringEqual(CipherSuite.szHash, BCRYPT_SHA384_ALGORITHM)) {
		ConnectionInfo->aiHash = CALG_SHA_384;
	} else if (StringEqual(CipherSuite.szHash, BCRYPT_SHA512_ALGORITHM)) {
		ConnectionInfo->aiHash = CALG_SHA_512;
	} else {
		ConnectionInfo->aiHash = CALG_OID_INFO_CNG_ONLY;
	}

	//
	// Set aiExch and dwExchStrength
	//

	ConnectionInfo->dwExchStrength = CipherSuite.dwMinExchangeLen;

	if (TlspIsStaticRsaKeyExchangeCipherSuite(Context->CipherSuite)) {
		ConnectionInfo->aiExch = CALG_RSA_KEYX;
	} else {
		ConnectionInfo->aiExch = CALG_ECDH;
	}

	return SEC_E_OK;
}

//
// Required by WinInet.
//
// WinInet does not use this information itself; it just provides the cipher
// info to its callers upon request (through InternetQueryOption with dwOption
// value INTERNET_OPTION_SECURITY_CONNECTION_INFO, returning the struct called
// INTERNET_SECURITY_CONNECTION_INFO).
//
// I'm not aware of any WinInet callers which make use of that information.
//
// See Also: SslLookupCipherSuiteInfo, NCRYPT_SSL_CIPHER_SUITE (sslprovider.h)
//
STATIC INLINE SECURITY_STATUS SppQueryCipherInfo(
	IN	PKXSCHANL_CONTEXT					Context,
	OUT	PSecPkgContext_CipherInfo			CipherInfo)
{
	SECURITY_STATUS SecStatus;
	HRESULT Result;
	NCRYPT_SSL_CIPHER_SUITE CipherSuite;

	if (Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE) {
		return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
	}

	KexRtlZeroMemory(CipherInfo, sizeof(*CipherInfo));

	SecStatus = TlsLookupCipherSuiteInfo(
		Context->ProtocolVersion,
		Context->CipherSuite,
		Context->EccKeyType,
		&CipherSuite);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	CipherInfo->dwVersion				= SECPKGCONTEXT_CIPHERINFO_V1;
	CipherInfo->dwProtocol				= CipherSuite.dwProtocol;
	CipherInfo->dwCipherSuite			= CipherSuite.dwCipherSuite;
	CipherInfo->dwBaseCipherSuite		= CipherSuite.dwBaseCipherSuite;
	CipherInfo->dwCipherLen				= CipherSuite.dwCipherLen;
	CipherInfo->dwCipherBlockLen		= CipherSuite.dwCipherBlockLen;
	CipherInfo->dwHashLen				= CipherSuite.dwHashLen;
	CipherInfo->dwKeyType				= CipherSuite.dwKeyType;
	CipherInfo->dwMinExchangeLen		= CipherSuite.dwMinExchangeLen;
	CipherInfo->dwMaxExchangeLen		= CipherSuite.dwMaxExchangeLen;

	Result = StringCchCopy(
		CipherInfo->szCipherSuite,
		ARRAYSIZE(CipherInfo->szCipherSuite),
		CipherSuite.szCipherSuite);

	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopy(
		CipherInfo->szCipher,
		ARRAYSIZE(CipherInfo->szCipher),
		CipherSuite.szCipher);

	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopy(
		CipherInfo->szHash,
		ARRAYSIZE(CipherInfo->szHash),
		CipherSuite.szHash);

	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopy(
		CipherInfo->szExchange,
		ARRAYSIZE(CipherInfo->szExchange),
		CipherSuite.szExchange);

	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopy(
		CipherInfo->szCertificate,
		ARRAYSIZE(CipherInfo->szCertificate),
		CipherSuite.szCertificate);

	ASSERT (SUCCEEDED(Result));

	return SEC_E_OK;
}

SECURITY_STATUS SppQueryContextAttributes(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer)
{
	PKXSCHANL_CONTEXT Context;

	ASSERT (Buffer != NULL);

	Context = SppReadContextHandle(ContextHandle);

	switch (Attribute) {
	case SECPKG_ATTR_STREAM_SIZES:
		return SppQueryStreamSizes(Context, (PSecPkgContext_StreamSizes) Buffer);
	case SECPKG_ATTR_ENDPOINT_BINDINGS:
		return SppQueryEndpointBindings(Context, (PSecPkgContext_Bindings) Buffer);
	case SECPKG_ATTR_APPLICATION_PROTOCOL:
		return SppQueryAlpnProtocol(Context, (PSecPkgContext_ApplicationProtocol) Buffer);
	case SECPKG_ATTR_REMOTE_CERT_CONTEXT:
		return SppQueryRemoteCertContext(Context, (PPCCERT_CONTEXT) Buffer);
	case SECPKG_ATTR_CONNECTION_INFO:
		return SppQueryConnectionInfo(Context, (PSecPkgContext_ConnectionInfo) Buffer);
	case SECPKG_ATTR_CIPHER_INFO:
		return SppQueryCipherInfo(Context, (PSecPkgContext_CipherInfo) Buffer);
	default:
		KexLogWarningEvent(
			L"Attempt to query an unsupported context attribute\r\n\r\n"
			L"Attribute: %lu",
			Attribute);
		
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
	}

	NOT_REACHED;
}