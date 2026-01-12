#include "buildcfg.h"
#include <KexComm.h>
#include <bcrypt.h>

#define SECURITY_WIN32
#include <Security.h>
#include <schannel.h>

EXTERN PKEX_PROCESS_DATA KexData;

#define IOCTL_KSEC_RANDOM_FILL_BUFFER CTL_CODE(FILE_DEVICE_KSEC, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef enum _TLS_ALGORITHM_USAGE {
	TlsParametersCngAlgUsageKeyExchange,	// Key exchange algorithm. RSA, ECHDE, DHE, etc.
	TlsParametersCngAlgUsageSignature,		// Signature algorithm. RSA, DSA, ECDSA, etc.
	TlsParametersCngAlgUsageCipher,			// Encryption algorithm. AES, DES, RC4, etc.
	TlsParametersCngAlgUsageDigest,			// Digest of cipher suite. SHA1, SHA256, SHA384, etc.
	TlsParametersCngAlgUsageCertSig			// Signature and/or hash used to sign certificate. RSA, DSA, ECDSA, SHA1, SHA256, etc.
} TYPEDEF_TYPE_NAME(TLS_ALGORITHM_USAGE);

typedef struct _CRYPTO_SETTINGS {
	TLS_ALGORITHM_USAGE	AlgorithmUSage;
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
	ULONG				NumberOfCryptoSettings;
	PCRYPTO_SETTINGS	DisabledCryptoAlgorithms;
	ULONG				Flags;
} TYPEDEF_TYPE_NAME(TLS_PARAMETERS);

#define SCH_CRED_V4 4
#define SCH_CRED_V5 5

// added in win8
#define SCH_SEND_AUX_RECORD 0x00200000

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