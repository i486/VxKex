#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>
#include <KxSChanl.h>
#define SECURITY_WIN32
#include <Security.h>
#include <bcrypt.h>
#include <KxCryp.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef struct DECLSPEC_ALIGN(4) {
	ULONG				cbLength;		// 0x14
	ULONG				dwMagic;		// always 'MSEC'
	ULONG				cbBuffer;
	ULONG				cbSecretAgreement;
	BYTE				rgbBuffer[ANYSIZE_ARRAY];
} TYPEDEF_TYPE_NAME(MSCRYPT_SECRET);

// Cast from BCRYPT_SECRET_HANDLE to obtain pointer to this.
typedef struct {
	ULONG				cbLength;
	ULONG				dwMagic;		// always 'UUUT'
	BCRYPT_ALG_HANDLE	hAlgorithm;
	PMSCRYPT_SECRET		pSecret;
} TYPEDEF_TYPE_NAME(BCRYPT_SECRET_HEADER);

//
// bcrypt.c
//

VOID CleanupCachedPredefinedHandles(
	VOID);

//
// credhndl.c
//

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleA(
	IN	PSTR			Principal OPTIONAL,
	IN	PSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL);