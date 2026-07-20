///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxschanlp.h
//
// Abstract:
//
//     Private header file for KxSchanl.
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "buildcfg.h"
#include "kxschanl.h"
#include <KxCryp.h>
#include "sslprovider.h"
#include <WinCrypt.h>
#include <ncrypt.h>

GEN_STD_TYPEDEFS(NCRYPT_SSL_CIPHER_SUITE);

// Do not use. Use TLS_GROUP_* macros instead for consistency.
#undef TLS_ECC_P256_CURVE_KEY_TYPE
#undef TLS_ECC_P384_CURVE_KEY_TYPE
#undef TLS_ECC_P521_CURVE_KEY_TYPE

#define SafeSslFreeObject(Object) \
	do { \
		if (Object) { \
			SECURITY_STATUS SafeSslFreeObjectSecStatus; \
			SafeSslFreeObjectSecStatus = SslFreeObject((Object), 0); \
			ASSERT (SUCCEEDED(SafeSslFreeObjectSecStatus)); \
			(Object) = 0; \
		} \
	} while (0)

EXTERN PKEX_PROCESS_DATA KexData;

#define KXSCHANL_PACKAGE_INFO_CB_MAX_TOKEN 0x6000

// 1. Log the error code if it's a failure.
// 2. If debug build and debugger attached and failure code, break.
// 3. Return the input SecStatus.
#define SP_LOG_RESULT(SecStatus) \
	(FAILED(SecStatus) \
		? KexLogWarningEvent(L"Originating error code %s", L#SecStatus) \
		: STATUS_SUCCESS, \
	 FAILED(SecStatus) \
		? (KexIsDebugBuild && NtCurrentPeb()->BeingDebugged ? (__debugbreak(), 0) : 0) \
		: 0, \
	 SecStatus)

// TLS Handshake Message IDs. RFC5246 §7.4, RFC8446 §4
#define TLS_HSMSG_HELLO_REQUEST				((UCHAR) 0)
#define TLS_HSMSG_CLIENT_HELLO				((UCHAR) 1)
#define TLS_HSMSG_SERVER_HELLO				((UCHAR) 2)
#define TLS_HSMSG_NEW_SESSION_TICKET		((UCHAR) 4)
#define TLS_HSMSG_END_OF_EARLY_DATA			((UCHAR) 5)
#define TLS_HSMSG_ENCRYPTED_EXTENSIONS		((UCHAR) 8)
#define TLS_HSMSG_CERTIFICATE				((UCHAR) 11)
#define TLS_HSMSG_SERVER_KEY_EXCHANGE		((UCHAR) 12)
#define TLS_HSMSG_CERTIFICATE_REQUEST		((UCHAR) 13)
#define TLS_HSMSG_SERVER_HELLO_DONE			((UCHAR) 14)
#define TLS_HSMSG_CERTIFICATE_VERIFY		((UCHAR) 15)
#define TLS_HSMSG_CLIENT_KEY_EXCHANGE		((UCHAR) 16)
#define TLS_HSMSG_FINISHED					((UCHAR) 20)
#define TLS_HSMSG_KEY_UPDATE				((UCHAR) 24)
#define TLS_HSMSG_MESSAGE_HASH				((UCHAR) 254)

// TLS Compression Methods. RFC5246 §7.4.1.2
#define TLS_COMPRESSION_METHOD_NULL			((UCHAR) 0)

// TLS Extension IDs. RFC6066 §10.2, RFC7301 §6, RFC8446 §4.2
#define TLS_EXT_SERVER_NAME					((USHORT) 0)
#define TLS_EXT_MAX_FRAGMENT_LENGTH			((USHORT) 1)
#define TLS_EXT_CLIENT_CERT_URL				((USHORT) 2)
#define TLS_EXT_TRUSTED_CA_KEYS				((USHORT) 3)
#define TLS_EXT_TRUNCATED_HMAC				((USHORT) 4)
#define TLS_EXT_STATUS_REQUEST				((USHORT) 5)
#define TLS_EXT_SUPPORTED_GROUPS			((USHORT) 10)
#define TLS_EXT_EC_POINT_FORMATS			((USHORT) 11)
#define TLS_EXT_SIGNATURE_ALGORITHMS		((USHORT) 13)
#define TLS_EXT_USE_SRTP					((USHORT) 14)
#define TLS_EXT_HEARTBEAT					((USHORT) 15)
#define TLS_EXT_ALPN						((USHORT) 16)
#define TLS_EXT_SIGNED_CERT_TIMESTAMP		((USHORT) 18)
#define TLS_EXT_CLIENT_CERT_TYPE			((USHORT) 19)
#define TLS_EXT_SERVER_CERT_TYPE			((USHORT) 20)
#define TLS_EXT_PADDING						((USHORT) 21)
#define TLS_EXT_PSK							((USHORT) 41)
#define TLS_EXT_EARLY_DATA					((USHORT) 42)
#define TLS_EXT_SUPPORTED_VERSIONS			((USHORT) 43)
#define TLS_EXT_COOKIE						((USHORT) 44)
#define TLS_EXT_PSK_KEY_EXCHANGE_MODES		((USHORT) 45)
#define TLS_EXT_CERT_AUTHORITIES			((USHORT) 47)
#define TLS_EXT_OID_FILTERS					((USHORT) 48)
#define TLS_EXT_POST_HANDSHAKE_AUTH			((USHORT) 49)
#define TLS_EXT_SIGNATURE_ALGORITHMS_CERT	((USHORT) 50)
#define TLS_EXT_KEY_SHARE					((USHORT) 51)

// TLS Key Exchange Groups. RFC8446 §4.2.7
#define TLS_GROUP_SECP256R1					((USHORT) 0x0017)
#define TLS_GROUP_SECP384R1					((USHORT) 0x0018)
#define TLS_GROUP_SECP521R1					((USHORT) 0x0019)
#define TLS_GROUP_X25519					((USHORT) 0x001D)
#define TLS_GROUP_X448						((USHORT) 0x001E)
#define TLS_GROUP_FFDHE2048					((USHORT) 0x0100)
#define TLS_GROUP_FFDHE3072					((USHORT) 0x0101)
#define TLS_GROUP_FFDHE4096					((USHORT) 0x0102)
#define TLS_GROUP_FFDHE6144					((USHORT) 0x0103)
#define TLS_GROUP_FFDHE8192					((USHORT) 0x0104)

// TLS Signature Schemes. RFC8446 §4.2.3, RFC9963 §3
#define TLS_SIGSCHEME_RSA_PKCS1_SHA1			((USHORT) 0x0201)
#define TLS_SIGSCHEME_ECDSA_SHA1				((USHORT) 0x0203)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA256			((USHORT) 0x0401)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA256_TLS13	((USHORT) 0x0420)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA384			((USHORT) 0x0501)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA384_TLS13	((USHORT) 0x0520)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA512			((USHORT) 0x0601)
#define TLS_SIGSCHEME_RSA_PKCS1_SHA512_TLS13	((USHORT) 0x0620)
#define TLS_SIGSCHEME_ECDSA_SECP256R1_SHA256	((USHORT) 0x0403)
#define TLS_SIGSCHEME_ECDSA_SECP384R1_SHA384	((USHORT) 0x0503)
#define TLS_SIGSCHEME_ECDSA_SECP521R1_SHA512	((USHORT) 0x0603)
#define TLS_SIGSCHEME_RSA_PSS_RSAE_SHA256		((USHORT) 0x0804)
#define TLS_SIGSCHEME_RSA_PSS_RSAE_SHA384		((USHORT) 0x0805)
#define TLS_SIGSCHEME_RSA_PSS_RSAE_SHA512		((USHORT) 0x0806)
#define TLS_SIGSCHEME_ED25519					((USHORT) 0x0807)
#define TLS_SIGSCHEME_ED448						((USHORT) 0x0808)
#define TLS_SIGSCHEME_RSA_PSS_PSS_SHA256		((USHORT) 0x0809)
#define TLS_SIGSCHEME_RSA_PSS_PSS_SHA384		((USHORT) 0x080A)
#define TLS_SIGSCHEME_RSA_PSS_PSS_SHA512		((USHORT) 0x080B)

// TLS Alert messages. RFC5246 §7.2, RFC8446 §4.2.3
#define TLS_ALERT_LEVEL_WARNING				((UCHAR) 1)
#define TLS_ALERT_LEVEL_FATAL				((UCHAR) 2)

#define TLS_ALERT_CLOSE_NOTIFY				((UCHAR) 0)
#define TLS_ALERT_UNEXPECTED_MESSAGE		((UCHAR) 10)
#define TLS_ALERT_BAD_RECORD_MAC			((UCHAR) 20)
#define TLS_ALERT_RECORD_OVERFLOW			((UCHAR) 22)
#define TLS_ALERT_DECOMPRESSION_FAILURE		((UCHAR) 30)
#define TLS_ALERT_HANDSHAKE_FAILURE			((UCHAR) 40)
#define TLS_ALERT_BAD_CERTIFICATE			((UCHAR) 42)
#define TLS_ALERT_UNSUPPORTED_CERT			((UCHAR) 43)
#define TLS_ALERT_CERT_REVOKED				((UCHAR) 44)
#define TLS_ALERT_CERT_EXPIRED				((UCHAR) 45)
#define TLS_ALERT_CERT_UNKNOWN				((UCHAR) 46)
#define TLS_ALERT_ILLEGAL_PARAMETER			((UCHAR) 47)
#define TLS_ALERT_UNKNOWN_CA				((UCHAR) 48)
#define TLS_ALERT_ACCESS_DENIED				((UCHAR) 49)
#define TLS_ALERT_DECODE_ERROR				((UCHAR) 50)
#define TLS_ALERT_DECRYPT_ERROR				((UCHAR) 51)
#define TLS_ALERT_PROTOCOL_VERSION			((UCHAR) 70)
#define TLS_ALERT_INSUFFICIENT_SECURITY		((UCHAR) 71)
#define TLS_ALERT_INTERNAL_ERROR			((UCHAR) 80)
#define TLS_ALERT_INAPPROPRIATE_FALLBACK	((UCHAR) 86)
#define TLS_ALERT_USER_CANCELED				((UCHAR) 90)
#define TLS_ALERT_NO_RENEGOTIATION			((UCHAR) 100)
#define TLS_ALERT_MISSING_EXTENSION			((UCHAR) 109)
#define TLS_ALERT_UNSUPPORTED_EXTENSION		((UCHAR) 110)
#define TLS_ALERT_UNRECOGNIZED_NAME			((UCHAR) 112)
#define TLS_ALERT_BAD_CERT_STATUS_RESPONSE	((UCHAR) 113)
#define TLS_ALERT_UNKNOWN_PSK_IDENTITY		((UCHAR) 115)
#define TLS_ALERT_CERTIFICATE_REQUIRED		((UCHAR) 116)
#define TLS_ALERT_NO_APPLICATION_PROTOCOL	((UCHAR) 120)

#define TLS_CURVE_TYPE_EXPLICIT_PRIME	((UCHAR) 1)
#define TLS_CURVE_TYPE_EXPLICIT_CHAR2	((UCHAR) 2)
#define TLS_CURVE_TYPE_NAMED_CURVE		((UCHAR) 3)

typedef enum {
	CONTEXTSTATE_SENDING_CLIENT_HELLO,
	CONTEXTSTATE_EXPECTING_SERVER_HELLO,

	CONTEXTSTATE_TLS1_2_EXPECTING_CERTIFICATE,
	CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_KEY_EXCHANGE,
	CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_HELLO_DONE,
	CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE,
	CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC,
	CONTEXTSTATE_TLS1_2_SENDING_FINISHED,
	CONTEXTSTATE_TLS1_2_EXPECTING_CHANGE_CIPHER_SPEC,
	CONTEXTSTATE_TLS1_2_EXPECTING_FINISHED,

	CONTEXTSTATE_TLS1_3_EXPECTING_ENCRYPTED_EXTENSIONS,
	CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE,
	CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE_VERIFY,
	CONTEXTSTATE_TLS1_3_EXPECTING_FINISHED,
	CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC,
	CONTEXTSTATE_TLS1_3_SENDING_FINISHED,

	CONTEXTSTATE_HANDSHAKE_COMPLETE,

	CONTEXTSTATE_SENDING_CLOSE_NOTIFY,
	CONTEXTSTATE_CONNECTION_CLOSED
} TYPEDEF_TYPE_NAME(KXSCHANL_CONTEXT_STATE);

//
// Private KxSChanl structures.
//

#pragma pack(push)
#pragma pack(1)

// All members of this structure are set by SppAcquireCredentialsHandle
// and should not be modified after creation.
typedef struct {
	// Handle to pass to the NCrypt Ssl* functions.
	// This is not valid when TLS 1.2 and older protocols are not enabled.
	NCRYPT_PROV_HANDLE	SslProvider;

	// The root certificate store to use.
	HCERTSTORE			RootCertStore;

	// A timestamp representing the expiry of the client credential.
	// KxSChanl does not currently support client credentials.
	TimeStamp			Expiry;

	// SCH_* bitflags.
	ULONG				Flags;

	// SP_PROT_* bitflags which represent the protocols which can be used
	// for all security contexts descending from this credential.
	ULONG				EnabledProtocols;
} TYPEDEF_TYPE_NAME(KXSCHANL_CREDENTIAL);

//
// Returns TRUE if the cipher suite uses RSA key exchange (no ServerKeyExchange
// message, Pre-Master Secret encrypted with server's RSA key). ECDHE suites
// (signature type is irrelevant) return FALSE.
//
// Note that this macro is not 100% accurate, because it will also return TRUE
// for DHE (not ECDHE) cipher suites. However, since we don't and will likely never
// support DHE, we will use this simplified check rather than checking for each
// invidual static RSA cipher suite.
//
#define TlspIsStaticRsaKeyExchangeCipherSuite(CipherSuite) (((CipherSuite) & 0xFF00) == 0)

//
// KXSCHANL_IO_READ_INFORMATION and KXSCHANL_IO_WRITE_INFORMATION are structures
// stored on the stack. The WolfSSL send and recv contexts are set to point to
// these structures when necessary.
//

typedef struct {
	PCVOID					InBuffer;
	PVOID					OutBuffer;

	ULONG					InBufferCb;
	ULONG					InBufferReadCb;
	ULONG					OutBufferCb;
	ULONG					OutBufferWrittenCb;

	// TRUE when InitializeSecurityContext receives ISC_REQ_ALLOCATE_MEMORY
	// in ContextRequirementsFlags. It means that OutBuffer shouldn't be written
	// to directly and should instead be allocated (with SafeAlloc) and the
	// application is responsible for freeing the memory (with FreeContextBuffer).
	BOOLEAN					OutBufferMustAllocate;
} TYPEDEF_TYPE_NAME(KXSCHANL_IO_INFORMATION);

typedef struct {
	// The credential object used to create this context.
	PCKXSCHANL_CREDENTIAL			Credential;

	PCCERT_CONTEXT					RemoteCertContext;
	HCERTSTORE						RemoteCertStore;

	// Placed outside the union since it has to be freed in both TLS 1.2 and 1.3
	// code paths. (because it gets created for the client hello)
	BCRYPT_KEY_HANDLE				EphemeralKey13;

	// Cryptographic handles.
	union {
		// For TLS 1.2 only.
		struct {
			NCRYPT_HASH_HANDLE		HandshakeHash12;
			NCRYPT_KEY_HANDLE		MasterKey12;
			NCRYPT_KEY_HANDLE		EphemeralKey12;
			NCRYPT_KEY_HANDLE		ReadKey12;
			NCRYPT_KEY_HANDLE		WriteKey12;
		};

		// For TLS 1.3 only.
		struct {
			BCRYPT_HASH_HANDLE		HandshakeHash13;
			BCRYPT_KEY_HANDLE		ServerEphemeralKey13;
			UCHAR					MasterSecret13[BITS_TO_BYTES(384)];

			union {
				struct {
					UCHAR			BaseSecret13[BITS_TO_BYTES(384)];
					UCHAR			ServerBaseSecret13[BITS_TO_BYTES(384)];
				};

				struct {
					UCHAR			PendingReadSecret13[BITS_TO_BYTES(384)];
					UCHAR			PendingWriteSecret13[BITS_TO_BYTES(384)];
				};
			};

			BCRYPT_KEY_HANDLE		ReadKey13;
			BCRYPT_KEY_HANDLE		WriteKey13;
			UCHAR					ReadKeyIv13[12];
			UCHAR					WriteKeyIv13[12];
		};
	};

	// Implicit per-direction TLS sequence numbers.
	// Incremented on each successful encrypt/decrypt call.
	ULONGLONG						ReadSequenceNumber;
	ULONGLONG						WriteSequenceNumber;

	// Which stage of the handshake we're in.
	// Takes a CONTEXTSTATE_* value.
	KXSCHANL_CONTEXT_STATE			State;

	// ISC_REQ_* flags which were passed to InitializeSecurityContext.
	ULONG							Flags;

	// Whether read or write encryption is enabled.
	BOOLEAN							ReadKeyEnabled:1;
	BOOLEAN							WriteKeyEnabled:1;

	// The name of the server to supply in the Server Name Indication extension.
	// Must be encoded in ASCII Punycode format. Is null terminated.
	// Will be empty if no server name was specified.
	// TargetNameCch == strlen(TargetName)
	CHAR							TargetName[256];
	USHORT							TargetNameCch;

	// List of protocols to supply in the ALPN extension.
	// Each protocol is a string, such as "http/1.1" or "h2", which is not null
	// terminated and is prefixed by a single byte indicating the number of
	// characters in the string.
	//
	// Will be empty if no SECBUFFER_APPLICATION_PROTOCOLS was specified during
	// InitializeSecurityContext.
	//
	// ApplicationProtocolsCb represents the entire valid data length of the
	// ApplicationProtocols buffer, including all the length prefix bytes.
	BYTE							ApplicationProtocols[MAX_PROTOCOL_ID_SIZE];
	USHORT							ApplicationProtocolsCb;

	// If the server selects an ALPN protocol, it will be placed here during the
	// handshake.
	BYTE							SelectedApplicationProtocol[MAX_PROTOCOL_ID_SIZE];
	UCHAR							SelectedApplicationProtocolCb;

	// Client and server random values (32 bytes each), populated during the
	// handshake. These are needed for key derivation.
	UCHAR							ClientRandom[32];
	UCHAR							ServerRandom[32];

	USHORT							EnabledCipherSuites[16];
	UCHAR							NumberOfEnabledCipherSuites;

	// The TLS specifications state that handshake messages can be fragmented
	// across record boundaries. This means that we have to buffer incomplete
	// handshake messages.
	//
	// A good website for testing whether this buffering works is
	// 1000-sans.badssl.com or 10000-sans.badssl.com.
	//
	// FragmentedMessageHeader exists to handle the case where the 4-byte message
	// header itself is fragmented across record boundaries (why is that even
	// allowed?). FragmentedMessageHeaderCb is the number of message header
	// bytes which have been stored in FragmentedMessageHeader.
	//
	// FragmentedMessageBufferCb is the size of the entire handshake message
	// (and also the size of the buffer allocation), and FragmentedMessageCb
	// is the number of bytes we actually have of that message.
	UCHAR							FragmentedMessageHeader[4];
	UCHAR							FragmentedMessageHeaderCb;
	PBYTE							FragmentedMessage;
	UCHAR							FragmentedMessageType;
	ULONG							FragmentedMessageBufferCb;
	ULONG							FragmentedMessageCb;

	// Saved ClientHello message bytes, needed to hash into the handshake
	// transcript after the handshake hash is created (which only happens
	// once we receive the ServerHello and know the cipher suite).
	// Allocated on the heap; freed when the handshake completes.
	PBYTE							SavedClientHello;
	ULONG							SavedClientHelloCb;

	// These values are only valid when the handshake has completed.
	USHORT							CipherSuite;		// e.g. TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
	USHORT							EccKeyType;			// e.g. TLS_GROUP_SECP256R1
	USHORT							ProtocolVersion;	// e.g. TLS1_2_PROTOCOL_VERSION
	USHORT							BlockSize;

	// HeaderSize is always valid.
	// TrailerSize becomes valid after the handshake has completed.
	UCHAR							HeaderSize;
	UCHAR							TrailerSize;
} TYPEDEF_TYPE_NAME(KXSCHANL_CONTEXT);

typedef struct {
	USHORT					ExtensionType;				// TLS_EXT_*

	union {
		// TLS_EXT_SERVER_NAME
		// https://datatracker.ietf.org/doc/html/rfc6066#section-3
		struct {
			USHORT			ServerNameCb;				// 1..65532
			PCCHAR			ServerName;					// ascii punycode
		} ServerName;

		// TLS_EXT_SUPPORTED_GROUPS
		// https://datatracker.ietf.org/doc/html/rfc8422#section-5.1.1
		// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.2.7
		struct {
			USHORT			NumberOfSupportedGroups;	// 1..32766
			PCUSHORT		SupportedGroups;			// TLS_GROUP_*
		} SupportedGroups;

		// TLS_EXT_SIGNATURE_ALGORITHMS
		// https://datatracker.ietf.org/doc/html/rfc5246.html#section-7.4.1.4.1
		// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.2.3
		struct {
			USHORT			NumberOfSignatureSchemes;	// 1..32766
			PCUSHORT		SignatureSchemes;			// TLS_SIGSCHEME_*
		} SignatureAlgorithms;

		// TLS_EXT_ALPN
		// https://datatracker.ietf.org/doc/html/rfc7301#section-3.1
		struct {
			USHORT			ProtocolListCb;				// 2..65533
			PCUCHAR			ProtocolList;				// same format as Context->TargetName
		} ApplicationProtocols;

		// TLS_EXT_SUPPORTED_VERSIONS
		// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.2.1
		struct {
			UCHAR			NumberOfProtocolVersions;	// 1..127
			USHORT			SelectedProtocolVersion;	// server hello only
			PCUSHORT		ProtocolVersions;			// TLSx_y_PROTOCOL_VERSION
		} SupportedVersions;

		// TLS_EXT_KEY_SHARE
		// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.2.8
		struct {
			UCHAR					NumberOfPublicKeys;	// 0 for server key share

			union {
				// The caller of serialize/deserialize functions is responsible for
				// freeing all of these.
				PBCRYPT_KEY_HANDLE	PublicKeys;			// client key share only
				BCRYPT_KEY_HANDLE	PublicKey;			// server key share only
			};
		} KeyShare;
	};
} TYPEDEF_TYPE_NAME(TLS_EXTENSION_SPEC);

// RFC5246, ss 7.4.1.2
typedef struct {
	USHORT					ClientVersion;				// TLSx_y_PROTOCOL_VERSION
	UCHAR					ClientRandom[32];			// 32 random bytes

	UCHAR					SessionIdCb;				// 0..32
	PCUCHAR					SessionId;

	USHORT					NumberOfCipherSuites;		// 1..32767
	PCUSHORT				CipherSuites;				// e.g. TLS_CHACHA20_POLY1305_SHA256

	UCHAR					NumberOfCompressionMethods;	// 1..127
	PCUCHAR					CompressionMethods;

	USHORT					NumberOfExtensions;
	PCTLS_EXTENSION_SPEC	Extensions;
} TYPEDEF_TYPE_NAME(TLS_CLIENT_HELLO_SPEC);

typedef struct {
	USHORT					ServerVersion;
	UCHAR					ServerRandom[32];

	UCHAR					SessionIdCb;				// 0..32
	PCUCHAR					SessionId;

	USHORT					CipherSuite;
	UCHAR					CompressionMethod;

	USHORT					NumberOfExtensions;
	PTLS_EXTENSION_SPEC		Extensions;
} TYPEDEF_TYPE_NAME(TLS_SERVER_HELLO_SPEC);

#pragma pack(pop)

//
// DLL_SECURITY_PACKAGE is what is pointed to by the dwLower member of a
// CredHandle structure.
//

typedef struct _DLL_SECURITY_PACKAGE TYPEDEF_TYPE_NAME(DLL_SECURITY_PACKAGE);

// This structure has not changed at all from Windows XP to Windows 10.
// It's fairly safe to assume that all Win7 builds have it the same too.
typedef struct _DLL_SECURITY_PACKAGE {
	LIST_ENTRY					List;
	ULONG						TypeMask;
	ULONG_PTR					PackageId;
	ULONG						PackageIndex;
	ULONG						State;
	ULONG_PTR					OriginalLowerContext;		// dwLower of the handle
	ULONG_PTR					OriginalLowerCredential;	// dwLower of the handle
	PVOID						DllBinding;					// original type PDLL_BINDING
	PDLL_SECURITY_PACKAGE		Root;
	PDLL_SECURITY_PACKAGE		Peer;
	ULONG						Capabilities;
	USHORT						Version;
	USHORT						RpcId;
	ULONG						TokenSize;
	UNICODE_STRING				PackageName;
	UNICODE_STRING				PackageComment;
	PSTR						PackageNameA;
	ULONG						PackageNameACch;			// including null terminator
	PSTR						PackageCommentA;
	ULONG						PackageCommentACch;			// including null terminator
	PSECURITY_FUNCTION_TABLE_A	FunctionTableA;
	PSECURITY_FUNCTION_TABLE_W	FunctionTableW;
	PSECURITY_FUNCTION_TABLE_W	FunctionTable;
	PVOID						UserFunctionTable;			// original type PSECPKG_USER_FUNCTION_TABLE
	PVOID						LoadPackage;				// original type LOAD_SECURITY_INTERFACE
	PVOID						UnloadPackage;				// original type EXIT_SECURITY_INTERFACE
	PVOID						LsaInfo;					// original type PDLL_LSA_PACKAGE_INFO
} TYPEDEF_TYPE_NAME(DLL_SECURITY_PACKAGE);

typedef struct {
	ULONG				cbLength;
	ULONG				dwMagic;		// always 'DDDB'
	ULONG				dwFlags;
	LONG				RefCount;
	NCRYPT_KEY_HANDLE	hSubKey;
	NCRYPT_PROV_HANDLE	pProvider;		// actual type is PNCRYPT_SSL_PROVIDER
} TYPEDEF_TYPE_NAME(NCRYPT_SSL_KEY);

// SSL_EPHEMERAL_KEY is pointed to by a NCRYPT_KEY_HANDLE->hSubKey generated by
// SslImportKey (generally speaking, not always).
typedef struct {
	ULONG				cbLength;
	ULONG				dwMagic;		// always 'ssl6'
	ALG_ID				aiAlgorithm;
	NCRYPT_PROV_HANDLE	hProvider;
	NCRYPT_KEY_HANDLE	hSubKey;
	ULONG				dwKeyType;
} TYPEDEF_TYPE_NAME(SSL_EPHEMERAL_KEY);

//
// Inline Functions
//

FORCEINLINE BOOLEAN TlspIsInputState12(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	switch (Context->State) {
	case CONTEXTSTATE_TLS1_2_EXPECTING_CERTIFICATE:
	case CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_KEY_EXCHANGE:
	case CONTEXTSTATE_TLS1_2_EXPECTING_SERVER_HELLO_DONE:
	case CONTEXTSTATE_TLS1_2_EXPECTING_CHANGE_CIPHER_SPEC:
	case CONTEXTSTATE_TLS1_2_EXPECTING_FINISHED:
		return TRUE;
	default:
		return FALSE;
	}
}

FORCEINLINE BOOLEAN TlspIsResponseState12(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	switch (Context->State) {
	case CONTEXTSTATE_TLS1_2_SENDING_CLIENT_KEY_EXCHANGE:
	case CONTEXTSTATE_TLS1_2_SENDING_CHANGE_CIPHER_SPEC:
	case CONTEXTSTATE_TLS1_2_SENDING_FINISHED:
		return TRUE;
	default:
		return FALSE;
	}
}

FORCEINLINE BOOLEAN TlspIsInputState13(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	switch (Context->State) {
	case CONTEXTSTATE_TLS1_3_EXPECTING_ENCRYPTED_EXTENSIONS:
	case CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE:
	case CONTEXTSTATE_TLS1_3_EXPECTING_CERTIFICATE_VERIFY:
	case CONTEXTSTATE_TLS1_3_EXPECTING_FINISHED:
		return TRUE;
	default:
		return FALSE;
	}
}

FORCEINLINE BOOLEAN TlspIsResponseState13(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	switch (Context->State) {
	case CONTEXTSTATE_TLS1_3_SENDING_CHANGE_CIPHER_SPEC:
	case CONTEXTSTATE_TLS1_3_SENDING_FINISHED:
		return TRUE;
	default:
		return FALSE;
	}
}

FORCEINLINE BOOLEAN TlspIsHandshakeInProgress(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	return Context->State < CONTEXTSTATE_HANDSHAKE_COMPLETE;
}

FORCEINLINE BOOLEAN TlspIsInputState(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	return Context->State == CONTEXTSTATE_EXPECTING_SERVER_HELLO ||
		   TlspIsInputState12(Context) || TlspIsInputState13(Context);
}

FORCEINLINE BOOLEAN TlspIsResponseState(
	IN	PCKXSCHANL_CONTEXT	Context)
{
	// Client hello is not considered a "response" state even though we are
	// sending something, because we're not responding to anything.
	return TlspIsResponseState12(Context) || TlspIsResponseState13(Context);
}

//
// api.c
//

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEnumerateSecurityPackagesA(
	OUT	PULONG			PackageCount,
	OUT	PSecPkgInfoA	*PackageInfo);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEnumerateSecurityPackagesW(
	OUT	PULONG			PackageCount,
	OUT	PSecPkgInfoW	*PackageInfo);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryCredentialsAttributesA(
	IN	PCredHandle	CredentialHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryCredentialsAttributesW(
	IN	PCredHandle	CredentialHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpAcquireCredentialsHandleA(
	IN	PSTR			Principal OPTIONAL,
	IN	PSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpAcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpFreeCredentialsHandle(
	IN	PCredHandle		CredentialHandle);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpInitializeSecurityContextA(
	IN		PCredHandle		CredentialHandle OPTIONAL,
	IN		PCtxtHandle		ContextHandle OPTIONAL,
	IN		PSTR			TargetName OPTIONAL,
	IN		ULONG			ContextRequirementsFlags,
	IN		ULONG			Reserved1,
	IN		ULONG			TargetDataRep,
	IN		PSecBufferDesc	Input OPTIONAL,
	IN		ULONG			Reserved2,
	IN OUT	PCtxtHandle		NewContextHandle OPTIONAL,
	IN OUT	PSecBufferDesc	Output OPTIONAL,
	OUT		PULONG			ContextAttributesFlags,
	OUT		PTimeStamp		Expiry OPTIONAL);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpInitializeSecurityContextW(
	IN		PCredHandle		CredentialHandle OPTIONAL,
	IN		PCtxtHandle		ContextHandle OPTIONAL,
	IN		PWSTR			TargetName OPTIONAL,
	IN		ULONG			ContextRequirementsFlags,
	IN		ULONG			Reserved1,
	IN		ULONG			TargetDataRep,
	IN		PSecBufferDesc	Input OPTIONAL,
	IN		ULONG			Reserved2,
	IN OUT	PCtxtHandle		NewContextHandle OPTIONAL,
	IN OUT	PSecBufferDesc	Output OPTIONAL,
	OUT		PULONG			ContextAttributesFlags,
	OUT		PTimeStamp		Expiry OPTIONAL);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpDeleteSecurityContext(
	IN	PCtxtHandle	ContextHandle);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpApplyControlToken(
	IN	PCtxtHandle		ContextHandle,
	IN	PSecBufferDesc	Input);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryContextAttributesA(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryContextAttributesW(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpFreeContextBuffer(
	IN	PVOID	ContextBuffer);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEncryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN		ULONG			QualityOfProtectionFlags,
	IN OUT	PSecBufferDesc	Message,
	IN		ULONG			MessageSequenceNumber);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpDecryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN OUT	PSecBufferDesc	Message,
	IN		ULONG			MessageSequenceNumber,
	OUT		PULONG			QualityOfProtectionFlags OPTIONAL);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpSetContextAttributesA(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	IN	PVOID		Buffer,
	IN	ULONG		BufferCb);

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpSetContextAttributesW(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	IN	PVOID		Buffer,
	IN	ULONG		BufferCb);

KXSCHANLAPI PSECURITY_FUNCTION_TABLE_A SEC_ENTRY SpInitSecurityInterfaceA(
	VOID);

KXSCHANLAPI PSECURITY_FUNCTION_TABLE_W SEC_ENTRY SpInitSecurityInterfaceW(
	VOID);

//
// certs.c
//

BOOLEAN SppLoadRootCertificates(
	IN OUT	PKXSCHANL_CREDENTIAL	Credential);

//
// credhndl.c
//

INLINE PKXSCHANL_CREDENTIAL SppReadCredentialHandle(
	IN	PCredHandle	CredentialHandle);

SECURITY_STATUS SppAcquireCredentialsHandle(
	IN	ULONG			CredentialUseFlags,
	IN	PCVOID			AuthData OPTIONAL,	// SCHANNEL_CREDS, SCH_CREDENTIALS, etc.
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL,
	IN	PCVOID			RetAddr);

SECURITY_STATUS SppFreeCredentialsHandle(
	IN	PCredHandle		CredentialHandle);

//
// ctltoken.c
//

SECURITY_STATUS SEC_ENTRY SppApplyControlToken(
	IN	PCtxtHandle		ContextHandle,
	IN	PSecBufferDesc	Input);

//
// ctxattr.c
//

SECURITY_STATUS SppQueryContextAttributes(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer);

//
// encdec.c
//

SECURITY_STATUS SppEncryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN		ULONG			QualityOfProtectionFlags,
	IN OUT	PSecBufferDesc	Message);

SECURITY_STATUS SppDecryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN OUT	PSecBufferDesc	Message);

//
// hkdf.c
//

NTSTATUS HkdfExtract(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				InputKeyingMaterial,
	IN	ULONG				InputKeyingMaterialCb,
	IN	PCUCHAR				Salt OPTIONAL,
	IN	ULONG				SaltCb,
	OUT	PUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb);

NTSTATUS HkdfExpand(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				ApplicationData OPTIONAL,
	IN	ULONG				ApplicationDataCb,
	OUT	PUCHAR				OutputKeyingMaterial,
	IN	ULONG				OutputKeyingMaterialCb);

NTSTATUS HkdfExpandLabel(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				Label,
	IN	ULONG				LabelCb,
	IN	PCUCHAR				Context OPTIONAL,
	IN	ULONG				ContextCb,
	OUT	PUCHAR				OutputKeyingMaterial,
	IN	ULONG				OutputKeyingMaterialCb);

NTSTATUS HkdfDeriveSecret(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				Label,
	IN	ULONG				LabelCb,
	IN	BCRYPT_HASH_HANDLE	TranscriptHash,
	OUT	PUCHAR				DerivedSecret,
	IN	ULONG				DerivedSecretCb);

//
// ioinfo.c
//

SECURITY_STATUS IoPeek(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PPCVOID						DataPointerOut,
	IN		ULONG						RequiredCb);

SECURITY_STATUS IoRead(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PPCVOID						DataPointerOut,
	IN		ULONG						RequiredCb);

SECURITY_STATUS IoReadCopy(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PVOID						Buffer,
	IN		ULONG						BufferCb);

SECURITY_STATUS IoWrite(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCVOID						Buffer,
	IN		ULONG						BufferCb);

EXTERN INLINE SECURITY_STATUS IoWrite8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						Value);

EXTERN INLINE SECURITY_STATUS IoWriteSwap16(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		USHORT						Value);

EXTERN INLINE SECURITY_STATUS IoWriteSwap24(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		ULONG						Value);

EXTERN INLINE SECURITY_STATUS IoWriteSwap32(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		ULONG						Value);

EXTERN INLINE SECURITY_STATUS IoRead8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						Value);

EXTERN INLINE SECURITY_STATUS IoReadSwap16(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUSHORT						Value);

EXTERN INLINE SECURITY_STATUS IoReadSwap24(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PULONG						Value);

EXTERN INLINE SECURITY_STATUS IoReadSwap32(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PULONG						Value);

SECURITY_STATUS IoPeek8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						Value);

//
// secctx.c
//

INLINE PKXSCHANL_CONTEXT SppReadContextHandle(
	IN	PCtxtHandle	ContextHandle);

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
	OUT		PTimeStamp		Expiry OPTIONAL);

SECURITY_STATUS SppDeleteSecurityContext(
	IN	PCtxtHandle	ContextHandle);

//
// sspihack.c
//

VOID SppFixupFunctionPointers(
	IN	PCredHandle	CredentialHandle,
	IN	PVOID		RetAddr);

//
// tlsalert.c
//

SECURITY_STATUS TlspProcessIncomingAlert(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

//
// tlshs.c
//

SECURITY_STATUS TlsConnect(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

SECURITY_STATUS TlsProcessPostHandshakeHandshakeMessageFragment(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

//
// tlshs12.c
//

SECURITY_STATUS TlspHashHandshakeData12(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCVOID				MessageWithHeader,
	IN		ULONG				MessageWithHeaderCb);

SECURITY_STATUS TlspProcessHandshakeMessage12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						MessageType);

SECURITY_STATUS TlspGenerateHandshakeMessage12(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType);

//
// tlshs13.c
//

SECURITY_STATUS TlspDeriveHandshakeKeys13(
	IN OUT	PKXSCHANL_CONTEXT	Context);

SECURITY_STATUS TlspDeriveApplicationKeys13(
	IN OUT	PKXSCHANL_CONTEXT	Context);

SECURITY_STATUS TlspHashHandshakeData13(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCVOID				Data,
	IN		ULONG				DataCb);

SECURITY_STATUS TlspProcessHandshakeMessage13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						MessageType);

SECURITY_STATUS TlspGenerateHandshakeMessage13(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType);

//
// tlsdec.c
//

SECURITY_STATUS TlsDecrypt(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						ContentType,
	OUT		PVOID						OutBuffer OPTIONAL,
	IN		ULONG						OutBufferCb,
	OUT		PULONG						OutBufferWrittenCb OPTIONAL);

//
// tlsenc.c
//

SECURITY_STATUS TlsEncrypt(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						ContentType,
	IN		PCVOID						InBuffer OPTIONAL,
	IN		ULONG						InBufferCb,
	OUT		PULONG						InBufferReadCb OPTIONAL);

//
// tlsext.c
//

SECURITY_STATUS TlspSerializeExtension(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCTLS_EXTENSION_SPEC		Spec);

SECURITY_STATUS TlspDeserializeExtension(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PTLS_EXTENSION_SPEC			Spec);

SECURITY_STATUS TlspHandleExtensions(
	IN OUT	PKXSCHANL_CONTEXT		Context,
	IN		PCTLS_EXTENSION_SPEC	Extensions,
	IN		ULONG					NumberOfExtensions);

//
// tlshello.c
//

SECURITY_STATUS TlspWriteClientHello(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

SECURITY_STATUS TlspHandleServerHello(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

//
// tlsrdwr.c
//

SECURITY_STATUS TlspPeekRecordHeader(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PTLS_RECORD_HEADER			RecordHeader);

SECURITY_STATUS TlspReadMessageHeader(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						MessageType,
	OUT		PULONG						MessageCb);

//
// tlsshut.c
//

SECURITY_STATUS TlsShutdown(
	IN		PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation);

//
// util.c
//

EXTERN INLINE VOID TlsDeserializeRecordHeader(
	IN	PCTLS_RECORD_HEADER	HeaderIn,
	OUT	PTLS_RECORD_HEADER	HeaderOut);

EXTERN INLINE VOID TlspSerializeRecordHeader(
	OUT	PTLS_RECORD_HEADER	HeaderOut,
	IN	UCHAR				ContentType,
	IN	USHORT				ProtocolVersion,
	IN	USHORT				DataCb);

SECURITY_STATUS TlspValidateRecordHeader(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	IN		PCTLS_RECORD_HEADER	Header);

SECURITY_STATUS TlsValidateApplicationProtocolList(
	IN	PCKXSCHANL_CONTEXT	Context,
	IN	PCUCHAR				ProtocolList,
	IN	USHORT				ProtocolListCb,
	IN	BOOLEAN				AllowMultipleProtocolIds);

BOOLEAN TlsIsProtocolInProtocolList(
	IN	PCUCHAR				Protocol,
	IN	UCHAR				ProtocolCb,
	IN	PCUCHAR				ProtocolList,
	IN	USHORT				ProtocolListCb);

PCWSTR TlsMapCipherSuiteToIanaName(
	IN	USHORT				CipherSuite);

SECURITY_STATUS TlspEccKeyToKeyBlob(
	IN	ULONG	EccKeyType,
	IN	PCBYTE	EccKey,
	IN	ULONG	EccKeyCb,
	OUT	PBYTE	KeyBlob,
	IN	ULONG	KeyBlobCb);

SECURITY_STATUS TlspDerSignatureToRaw(
	IN	ULONG	CertificateKeyBits,
	IN	PCBYTE	DerSignature,
	IN	ULONG	DerSignatureCb,
	OUT	PBYTE	RawSignature,
	IN	ULONG	RawSignatureCb);

SECURITY_STATUS TlspGetServerPublicKeyFromCertificate(
	IN OUT	PKXSCHANL_CONTEXT	Context,
	OUT		PNCRYPT_KEY_HANDLE	PublicKeyNCrypt OPTIONAL,
	OUT		PBCRYPT_KEY_HANDLE	PublicKeyBCrypt OPTIONAL);

SECURITY_STATUS TlspCheckServerCertificateOk(
	IN OUT	PKXSCHANL_CONTEXT	Context);

USHORT TlspMapCipherSuiteForNcrypt(
	IN	USHORT	RealCipherSuite);

SECURITY_STATUS TlspGetEccKeyBits(
	IN	USHORT	EccKeyType,
	OUT	PULONG	EphemeralKeyBits);

SECURITY_STATUS TlspGetKeyExchangeParameters(
	IN	USHORT				SignatureScheme,
	IN	USHORT				EccKeyType,
	OUT	PPCWSTR				HashAlgorithm,
	OUT	PBCRYPT_ALG_HANDLE	HashAlgHandle,
	OUT	PUCHAR				HashCb,
	OUT	PULONG				EphemeralKeyBits,
	OUT	PULONG				CertificateKeyBits OPTIONAL);

SECURITY_STATUS TlsLookupCipherSuiteInfo(
	IN	USHORT						ProtocolVersion,
	IN	USHORT						CipherSuite,
	IN	USHORT						EccKeyType,
	OUT	PNCRYPT_SSL_CIPHER_SUITE	SuiteInfo);

SECURITY_STATUS TlspLookupCipherSuiteHashInfo13(
	IN	USHORT						CipherSuite,
	OUT	PBCRYPT_ALG_HANDLE			HashAlgHandle OPTIONAL,
	OUT	PBCRYPT_ALG_HANDLE			HmacAlgHandle OPTIONAL,
	OUT	PULONG						HashOutputCb OPTIONAL);

SECURITY_STATUS TlspLookupCipherSuiteCipherInfo13(
	IN	USHORT						CipherSuite,
	OUT	PBCRYPT_ALG_HANDLE			CipherAlgHandle OPTIONAL,
	OUT	PULONG						SymmetricKeyCb OPTIONAL,
	OUT	PULONG						EncDecIvCb OPTIONAL);

SECURITY_STATUS TlspVerifyCertificateSignature(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN		USHORT						SignatureScheme,
	IN		PCUCHAR						SignedData,
	IN		ULONG						SignedDataCb,
	IN		PCUCHAR						Signature,
	IN		ULONG						SignatureCb);