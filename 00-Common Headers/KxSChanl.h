///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxschanl.h
//
// Abstract:
//
//     Header file which defines the functions and the structures of the
//     public API of KxSchanl.
//
//     KxSchanl is a Schannel-compatible SSP which is intended as a drop-in
//     replacement for Schannel on Windows 7, in order to add support for
//     TLSv1.3. It uses WolfSSL as a backend.
//
// Author:
//
//     vxiiduu (09-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               09-May-2026  Initial creation.
//     vxiiduu               17-May-2026  Change KXSCHANL_NAME to "KxSChanl"
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>

#define SECURITY_WIN32
#include <Security.h>
#include <schannel.h>

typedef SecurityFunctionTableA TYPEDEF_TYPE_NAME(SECURITY_FUNCTION_TABLE_A);
typedef SecurityFunctionTableW TYPEDEF_TYPE_NAME(SECURITY_FUNCTION_TABLE_W);

#define KXSCHANL_NAME_A "KxSChanl"
#define KXSCHANL_DESC_A "VxKex SSL/TLS Security Protocol Provider"
#define KXSCHANL_NAME_W _L(KXSCHANL_NAME_A)
#define KXSCHANL_DESC_W _L(KXSCHANL_DESC_A)

#if _UNICODE
typedef SECURITY_FUNCTION_TABLE_W TYPEDEF_TYPE_NAME(SECURITY_FUNCTION_TABLE);
#  define KXSCHANL_NAME KXSCHANL_NAME_W
#  define KXSCHANL_DESC KXSCHANL_DESC_W
#else
typedef SECURITY_FUNCTION_TABLE_A TYPEDEF_TYPE_NAME(SECURITY_FUNCTION_TABLE);
#  define KXSCHANL_NAME KXSCHANL_NAME_A
#  define KXSCHANL_DESC KXSCHANL_DESC_A
#endif

#define TLS_MAX_PLAINTEXT_CB ((ULONG) 16384)

#define TLS1_3_PROTOCOL_VERSION 0x0304

// TLS 1.3 ciphersuites
#define TLS_AES_128_GCM_SHA256			0x1301
#define TLS_AES_256_GCM_SHA384			0x1302
#define TLS_CHACHA20_POLY1305_SHA256	0x1303

// TLS 1.2 extended ciphersuites
#define TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256	0xC02F
#define TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384	0xC030
#define TLS_RSA_WITH_AES_128_GCM_SHA256			0x009C
#define TLS_RSA_WITH_AES_256_GCM_SHA384			0x009D

#define CT_CHANGE_CIPHER_SPEC	0x14
#define CT_ALERT				0x15
#define CT_HANDSHAKE			0x16
#define CT_APPLICATIONDATA		0x17

// extended CNG algorithm IDs
#define BCRYPT_ECDSA_ALGORITHM					L"ECDSA"
#define BCRYPT_CHACHA20_POLY1305_ALGORITHM		L"CHACHA20_POLY1305"

#define CALG_THIRDPARTY_CIPHER					0x7000

#pragma pack(push)
#pragma pack(1)
typedef struct {
	UCHAR				Type;
	USHORT				ProtocolVersion;	// BIG ENDIAN!
	USHORT				DataCb;				// BIG ENDIAN!
} TYPEDEF_TYPE_NAME(TLS_RECORD_HEADER);

C_ASSERT(sizeof(TLS_RECORD_HEADER) == 5);

typedef struct {
	UCHAR				Level;				// TLS_ALERT_LEVEL_*
	UCHAR				Code;				// TLS_ALERT_*
} TYPEDEF_TYPE_NAME(TLS_ALERT_PAYLOAD);
#pragma pack(pop)

typedef enum _TLS_ALGORITHM_USAGE {
	TlsParametersCngAlgUsageKeyExchange,	// Key exchange algorithm. RSA, ECHDE, DHE, etc.
	TlsParametersCngAlgUsageSignature,		// Signature algorithm. RSA, DSA, ECDSA, etc.
	TlsParametersCngAlgUsageCipher,			// Encryption algorithm. AES, DES, RC4, etc.
	TlsParametersCngAlgUsageDigest,			// Digest of cipher suite. SHA1, SHA256, SHA384, etc.
	TlsParametersCngAlgUsageCertSig			// Signature and/or hash used to sign certificate. RSA, DSA, ECDSA, SHA1, SHA256, etc.
} TYPEDEF_TYPE_NAME(TLS_ALGORITHM_USAGE);

typedef struct _CRYPTO_SETTINGS {
	TLS_ALGORITHM_USAGE	AlgorithmUsage;
	UNICODE_STRING		AlgorithmId;
	ULONG				NumberOfChainingModes;
	PUNICODE_STRING		ChainingModes;
	ULONG				MinimumBitLength;
	ULONG				MaximumBitLength;
} TYPEDEF_TYPE_NAME(CRYPTO_SETTINGS);

typedef struct _TLS_PARAMETERS {
	ULONG				NumberOfAlpnIds;
	PUNICODE_STRING		AlpnIds;
	ULONG				DisabledProtocols; // bit field
	ULONG				NumberOfDisabledAlgorithms;
	PCRYPTO_SETTINGS	DisabledAlgorithms;
	ULONG				Flags;
} TYPEDEF_TYPE_NAME(TLS_PARAMETERS);

#define SCH_CRED_V4 4
#define SCH_CRED_V5 5
#define SCH_CREDENTIALS_VERSION 5

// added in win8+
#define SCH_SEND_AUX_RECORD		0x00200000
#define SCH_USE_STRONG_CRYPTO	0x00400000

#define SCH_WIN7_VALID_FLAGS (SCH_CRED_NO_SYSTEM_MAPPER | \
							  SCH_CRED_NO_SERVERNAME_CHECK | \
							  SCH_CRED_MANUAL_CRED_VALIDATION | \
							  SCH_CRED_NO_DEFAULT_CREDS | \
						 	  SCH_CRED_AUTO_CRED_VALIDATION | \
						 	  SCH_CRED_USE_DEFAULT_CREDS | \
						 	  SCH_CRED_DISABLE_RECONNECTS | \
						 	  SCH_CRED_REVOCATION_CHECK_END_CERT | \
						 	  SCH_CRED_REVOCATION_CHECK_CHAIN | \
						 	  SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | \
						 	  SCH_CRED_IGNORE_NO_REVOCATION_CHECK | \
						 	  SCH_CRED_IGNORE_REVOCATION_OFFLINE | \
						 	  SCH_CRED_RESTRICTED_ROOTS | \
						 	  SCH_CRED_REVOCATION_CHECK_CACHE_ONLY | \
						 	  SCH_CRED_CACHE_ONLY_URL_RETRIEVAL | \
						 	  SCH_CRED_MEMORY_STORE_CERT | \
						 	  SCH_CRED_CACHE_ONLY_URL_RETRIEVAL_ON_CREATE | \
						 	  SCH_SEND_ROOT_CERT)

GEN_STD_TYPEDEFS(SCHANNEL_CRED);
GEN_STD_TYPEDEFS(CERT_CONTEXT);
GEN_STD_TYPEDEFS(ALG_ID);

#define SCH_CRED_MAX_SUPPORTED_PARAMETERS 16
#define SCH_CRED_MAX_SUPPORTED_ALPN_IDS 16
#define SCH_CRED_MAX_SUPPORTED_CRYPTO_SETTINGS 16
#define SCH_CRED_MAX_SUPPORTED_CHAINING_MODES 16

#define SECPKG_ATTR_APPLICATION_PROTOCOL 35

#define SECBUFFER_APPLICATION_PROTOCOLS 18

#define SP_PROT_TLS1_3_CLIENT 0x00002000
#define SP_PROT_TLS1_3_SERVER 0x00001000
#define SP_PROT_TLS1_3 (SP_PROT_TLS1_3_CLIENT | SP_PROT_TLS1_3_SERVER)

//
// Microsoft has provided a list of applications which break when TLS 1.0/1.1
// are disabled.
//
// https://techcommunity.microsoft.com/blog/windows-itpro-blog/tls-1-0-and-tls-1-1-soon-to-be-disabled-in-windows/3887947
//

#define KXSCHANL_DEFAULT_ENABLED_PROTOCOLS (SP_PROT_TLS1_2_CLIENT | \
											SP_PROT_TLS1_3_CLIENT)

#define KXSCHANL_ALL_SUPPORTED_PROTOCOLS (SP_PROT_TLS1_2_CLIENT | \
										  SP_PROT_TLS1_3_CLIENT)

// Make sure there are no default protocols that aren't actually supported
C_ASSERT((KXSCHANL_ALL_SUPPORTED_PROTOCOLS & KXSCHANL_DEFAULT_ENABLED_PROTOCOLS) ==
		 KXSCHANL_DEFAULT_ENABLED_PROTOCOLS);

// This is the Win10+ version of SCHANNEL_CRED.
// The naming scheme is retarded.
typedef struct _SCH_CREDENTIALS {
	ULONG			Version;					// always 5 (SCH_CRED_V5)
	ULONG			CredentialsFormat;
	ULONG			NumberOfCertificateContexts;
	PCCERT_CONTEXT	*CertificateContexts;
	HCERTSTORE		RootStore;
	ULONG			NumberOfMappers;
	struct _HMAPPER	**Mappers;
	ULONG			SessionLifespan;
	ULONG			Flags;
	ULONG			NumberOfTlsParameters;
	PTLS_PARAMETERS	TlsParameters;
} TYPEDEF_TYPE_NAME(SCH_CREDENTIALS);

typedef enum {
	SecApplicationProtocolNegotiationStatus_None,
	SecApplicationProtocolNegotiationStatus_Success,
	SecApplicationProtocolNegotiationStatus_SelectedClientOnly
} TYPEDEF_TYPE_NAME(SEC_APPLICATION_PROTOCOL_NEGOTIATION_STATUS);

typedef enum {
	SecApplicationProtocolNegotiationExt_None,
	SecApplicationProtocolNegotiationExt_NPN,
	SecApplicationProtocolNegotiationExt_ALPN
} TYPEDEF_TYPE_NAME(SEC_APPLICATION_PROTOCOL_NEGOTIATION_EXT);

#define MAX_PROTOCOL_ID_SIZE 0xff

typedef struct {
	// Application protocol negotiation status
	SEC_APPLICATION_PROTOCOL_NEGOTIATION_STATUS	ProtoNegoStatus;

	// Protocol negotiation extension type corresponding to this protocol ID
	SEC_APPLICATION_PROTOCOL_NEGOTIATION_EXT	ProtoNegoExt;

	// Size in bytes of the application protocol ID
	// Uncertain if this includes any null terminator. For libcurl
	// it doesn't matter.
	UCHAR										ProtocolIdSize;

	// Byte string representing the negotiated application protocol ID
	UCHAR										ProtocolId[MAX_PROTOCOL_ID_SIZE];
} SecPkgContext_ApplicationProtocol, *PSecPkgContext_ApplicationProtocol;

typedef struct {
  SEC_APPLICATION_PROTOCOL_NEGOTIATION_EXT	ProtoNegoExt;
  USHORT									ProtocolListSize;
  UCHAR										ProtocolList[ANYSIZE_ARRAY];
} TYPEDEF_TYPE_NAME(SEC_APPLICATION_PROTOCOL_LIST);

C_ASSERT(sizeof(SEC_APPLICATION_PROTOCOL_LIST) == 8);

typedef struct {
	// Size in bytes of the ProtocolLists any-size array.
	ULONG							ProtocolListsSize;
	SEC_APPLICATION_PROTOCOL_LIST	ProtocolLists[ANYSIZE_ARRAY];
} TYPEDEF_TYPE_NAME(SEC_APPLICATION_PROTOCOLS);