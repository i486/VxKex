///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     bcrypt.c
//
// Abstract:
//
//     BCrypt extended functions.
//
// Author:
//
//     vxiiduu (??-???-????)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               08-Jun-2026  Add support for predefined keys.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxcrypp.h"

STATIC BCRYPT_ALG_HANDLE CachedHandles[KXCRYP_MAX_PREDEFINED_ALG_HANDLE_INDEX + 1] = {0};

VOID CleanupCachedPredefinedHandles(
	VOID)
{
	ULONG Index;

	for (Index = 0; Index < ARRAYSIZE(CachedHandles); ++Index) {
		SafeBCryptCloseAlgorithmProvider(CachedHandles[Index]);
	}
}

//
// Returns a BCRYPT_ALG_HANDLE for use with the Win7 CNG-API.
// The input handle may be a predefined algorithm handle (feature introduced
// in Windows 10), which is a BCRYPT_*_ALG_HANDLE constant.
//
// When this function fails, it returns NULL if the input handle was a predefined
// handle. This is because Win7 BCrypt functions will cause an access violation
// if you give it a Win10-style predefined handle, but NULL will just cause them
// to return an error code.
//
STATIC BCRYPT_ALG_HANDLE MapPredefinedAlgorithmHandle(
	IN	BCRYPT_ALG_HANDLE	AlgHandle)
{
	NTSTATUS Status;
	ULONG AlgIndex;
	BCRYPT_ALG_HANDLE NewAlgHandle;

	typedef struct {
		PCWSTR	AlgId;
		PCWSTR	ChainingMode;
		ULONG	Flags;
	} TYPEDEF_TYPE_NAME(TRIPLE_ALGID_CHAIN_FLAGS);

	STATIC CONST TRIPLE_ALGID_CHAIN_FLAGS AlgorithmParameters[] = {
		{BCRYPT_MD2_ALGORITHM},
		{BCRYPT_MD4_ALGORITHM},
		{BCRYPT_MD5_ALGORITHM},
		{BCRYPT_SHA1_ALGORITHM},
		{BCRYPT_SHA256_ALGORITHM},
		{BCRYPT_SHA384_ALGORITHM},
		{BCRYPT_SHA512_ALGORITHM},
		{BCRYPT_RC4_ALGORITHM},
		{BCRYPT_RNG_ALGORITHM},
		{BCRYPT_MD5_ALGORITHM,				NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_SHA1_ALGORITHM,				NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_SHA256_ALGORITHM,			NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_SHA384_ALGORITHM,			NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_SHA512_ALGORITHM,			NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_RSA_ALGORITHM},
		{BCRYPT_ECDSA_ALGORITHM},
		{BCRYPT_AES_CMAC_ALGORITHM},
		{BCRYPT_AES_GMAC_ALGORITHM},
		{BCRYPT_MD2_ALGORITHM,				NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_MD4_ALGORITHM,				NULL,					BCRYPT_ALG_HANDLE_HMAC_FLAG},
		{BCRYPT_3DES_ALGORITHM,				BCRYPT_CHAIN_MODE_CBC},
		{BCRYPT_3DES_ALGORITHM,				BCRYPT_CHAIN_MODE_ECB},
		{BCRYPT_3DES_ALGORITHM,				BCRYPT_CHAIN_MODE_CFB},
		{BCRYPT_3DES_112_ALGORITHM,			BCRYPT_CHAIN_MODE_CBC},
		{BCRYPT_3DES_112_ALGORITHM,			BCRYPT_CHAIN_MODE_ECB},
		{BCRYPT_3DES_112_ALGORITHM,			BCRYPT_CHAIN_MODE_CFB},
		{BCRYPT_AES_ALGORITHM,				BCRYPT_CHAIN_MODE_CBC},
		{BCRYPT_AES_ALGORITHM,				BCRYPT_CHAIN_MODE_ECB},
		{BCRYPT_AES_ALGORITHM,				BCRYPT_CHAIN_MODE_CFB},
		{BCRYPT_AES_ALGORITHM,				BCRYPT_CHAIN_MODE_CCM},
		{BCRYPT_AES_ALGORITHM,				BCRYPT_CHAIN_MODE_GCM},
		{BCRYPT_DES_ALGORITHM,				BCRYPT_CHAIN_MODE_CBC},
		{BCRYPT_DES_ALGORITHM				BCRYPT_CHAIN_MODE_ECB},
		{BCRYPT_DES_ALGORITHM,				BCRYPT_CHAIN_MODE_CFB},
		{NULL},
		{NULL},
		{NULL},
		{NULL},
		{NULL},
		{NULL},
		{BCRYPT_DH_ALGORITHM},
		{BCRYPT_ECDH_ALGORITHM},
		{BCRYPT_ECDH_P256_ALGORITHM},
		{BCRYPT_ECDH_P384_ALGORITHM},
		{BCRYPT_ECDH_P521_ALGORITHM},
		{BCRYPT_DSA_ALGORITHM},
		{BCRYPT_ECDSA_P256_ALGORITHM},
		{BCRYPT_ECDSA_P384_ALGORITHM},
		{BCRYPT_ECDSA_P521_ALGORITHM},
		{BCRYPT_RSA_SIGN_ALGORITHM},
	};

	C_ASSERT (ARRAYSIZE(AlgorithmParameters) == ARRAYSIZE(CachedHandles));
	
	if (!KXCRYP_IS_PREDEFINED_ALG_HANDLE(AlgHandle)) {
		return AlgHandle;
	}

	AlgIndex = KXCRYP_PREDEFINED_ALG_HANDLE_TO_INDEX(AlgHandle);

	if (AlgIndex >= ARRAYSIZE(CachedHandles)) {
		// Unsupported or nonexistent predefined handle
		KexDebugCheckpoint();
		return NULL;
	}

	if (AlgorithmParameters[AlgIndex].AlgId == NULL) {
		// Unsupported or nonexistent predefined handle
		KexDebugCheckpoint();
		return NULL;
	}

	if (CachedHandles[AlgIndex] != NULL) {
		return CachedHandles[AlgIndex];
	}

	//
	// We haven't yet opened a cached handle for this algorithm.
	//

	Status = BCryptOpenAlgorithmProvider(
		&NewAlgHandle,
		AlgorithmParameters[AlgIndex].AlgId,
		MS_PRIMITIVE_PROVIDER,
		AlgorithmParameters[AlgIndex].Flags);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexDebugCheckpoint();
		return NULL;
	}

	//
	// Some predefined algorithm handles for block ciphers specify a chaining mode.
	// If so, set it now.
	//

	if (AlgorithmParameters[AlgIndex].ChainingMode) {
		PCWSTR ChainingMode;
		ULONG ChainingModeCb;

		ChainingMode = AlgorithmParameters[AlgIndex].ChainingMode;
		ChainingModeCb = (ULONG) (wcslen(ChainingMode) + 1) * sizeof(WCHAR);

		Status = BCryptSetProperty(
			NewAlgHandle,
			BCRYPT_CHAINING_MODE,
			(PBYTE) ChainingMode,
			ChainingModeCb,
			0);

		if (!NT_SUCCESS(Status)) {
			KexDebugCheckpoint();
			SafeBCryptCloseAlgorithmProvider(NewAlgHandle);
			return AlgHandle;
		}
	}

	//
	// Atomically cache the algorithm handle.
	// If another thread wrote it first, clean up ours.
	//

	if (InterlockedCompareExchangePointer(&CachedHandles[AlgIndex], NewAlgHandle, NULL) != NULL) {
		SafeBCryptCloseAlgorithmProvider(NewAlgHandle);
	}

	ASSERT (CachedHandles[AlgIndex] != NULL);
	return CachedHandles[AlgIndex];
}

KXCRYPAPI NTSTATUS NTAPI Ext_BCryptCreateHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	OUT	PBCRYPT_HASH_HANDLE	HashHandle,
	OUT	PUCHAR				HashObject OPTIONAL,
	IN	ULONG				HashObjectCb,
	IN	PUCHAR				Secret OPTIONAL,
	IN	ULONG				SecretCb OPTIONAL,
	IN	ULONG				Flags)
{
	return BCryptCreateHash(
		MapPredefinedAlgorithmHandle(Algorithm),
		HashHandle,
		HashObject,
		HashObjectCb,
		Secret,
		SecretCb,
		Flags);
}

//
// KeyDerivationFunction == BCRYPT_KDF_RAW_SECRET was added in Windows 10.
// This functionality is not known to be used by  applications, but KxSChanl
// uses it, and it was decided to expose this extension to applications.
//

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptDeriveKey(
	IN	BCRYPT_SECRET_HANDLE	SecretHandle,
	IN	PCWSTR					KeyDerivationFunction,
	IN	PBCryptBufferDesc		ParameterList OPTIONAL,
	OUT	PUCHAR					DerivedKey OPTIONAL,
	IN	ULONG					DerivedKeyCb,
	OUT	PULONG					DerivedKeyOutCb,
	IN	ULONG					Flags)
{
	NTSTATUS Status;

	if (SecretHandle != NULL && KeyDerivationFunction != NULL) {
		// See bcryptprimitives.dll!MSCryptDeriveKey (10.0.14361)
		// See bcryptprimitives.dll!MSCryptKDF_TRUNCATE (10.0.14361)
		if (StringEqual(KeyDerivationFunction, BCRYPT_KDF_RAW_SECRET)) {
			PCBCRYPT_SECRET_HEADER SecretHeader;
			PMSCRYPT_SECRET Secret;

			if ((Flags & ~KDF_USE_SECRET_AS_HMAC_KEY_FLAG) ||
				DerivedKeyOutCb == NULL) {

				return STATUS_INVALID_PARAMETER;
			}

			SecretHeader = (PCBCRYPT_SECRET_HEADER) SecretHandle;

			if (SecretHeader->cbLength != sizeof(*SecretHeader) ||
				SecretHeader->dwMagic != 'UUUT' ||
				SecretHeader->hAlgorithm == NULL ||
				SecretHeader->pSecret == NULL) {

				KexDebugCheckpoint();
				goto BailOut;
			}

			Secret = SecretHeader->pSecret;

			if (Secret->cbLength != sizeof(*Secret) ||
				Secret->dwMagic != 'MSEC') {

				KexDebugCheckpoint();
				goto BailOut;
			}

			*DerivedKeyOutCb = min(DerivedKeyCb, Secret->cbSecretAgreement);

			if (DerivedKey != NULL) {
				//
				// The secret is "encrypted" with a process-specific key and we have to
				// call the kernel in order to decrypt it.
				// The secret as stored in the BCrypt structure is big-endian, but the
				// Microsoft API documentation says we need to convert to little-endian.
				//

				Status = KexRtlDecryptMemory(Secret->rgbBuffer, Secret->cbBuffer, 0);
				ASSERT (NT_SUCCESS(Status));

				if (!NT_SUCCESS(Status)) {
					return Status;
				}

				KexRtlReverseCopyMemory(DerivedKey, Secret->rgbBuffer, Secret->cbSecretAgreement);

				Status = KexRtlEncryptMemory(Secret->rgbBuffer, Secret->cbBuffer, 0);
				ASSERT (NT_SUCCESS(Status));

				if (!NT_SUCCESS(Status)) {
					return Status;
				}
			}

			return STATUS_SUCCESS;
		}
	}

BailOut:
	return BCryptDeriveKey(
		SecretHandle,
		KeyDerivationFunction,
		ParameterList,
		DerivedKey,
		DerivedKeyCb,
		DerivedKeyOutCb,
		Flags);
}

KXCRYPAPI NTSTATUS NTAPI Ext_BCryptGenRandom(
	IN		BCRYPT_ALG_HANDLE	Algorithm,
	IN OUT	PUCHAR				Buffer,
	IN		ULONG				BufferCb,
	IN		ULONG				Flags)
{
	return BCryptGenRandom(
		MapPredefinedAlgorithmHandle(Algorithm),
		Buffer,
		BufferCb,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptGenerateKeyPair(
	IN	BCRYPT_ALG_HANDLE		Algorithm,
	OUT	PBCRYPT_KEY_HANDLE		KeyHandle,
	IN	ULONG					KeyBits,
	IN	ULONG					Flags)
{
	return BCryptGenerateKeyPair(
		MapPredefinedAlgorithmHandle(Algorithm),
		KeyHandle,
		KeyBits,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptGenerateSymmetricKey(
	IN	BCRYPT_ALG_HANDLE		Algorithm,
	OUT	PBCRYPT_KEY_HANDLE		KeyHandle,
	OUT	PUCHAR					KeyObject OPTIONAL,
	IN	ULONG					KeyObjectCb,
	IN	PUCHAR					Secret,
	IN	ULONG					SecretCb,
	IN	ULONG					Flags)
{
	return BCryptGenerateSymmetricKey(
		MapPredefinedAlgorithmHandle(Algorithm),
		KeyHandle,
		KeyObject,
		KeyObjectCb,
		Secret,
		SecretCb,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptGetProperty(
	IN	BCRYPT_HANDLE			Object,
	IN	PCWSTR					Property,
	IN	PUCHAR					Buffer,
	IN	ULONG					BufferCb,
	OUT	PULONG					BufferOutCb,
	IN	ULONG					Flags)
{
	return BCryptGetProperty(
		MapPredefinedAlgorithmHandle(Object),
		Property,
		Buffer,
		BufferCb,
		BufferOutCb,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptImportKey(
	IN	BCRYPT_ALG_HANDLE		Algorithm,
	IN	BCRYPT_KEY_HANDLE		ImportKey OPTIONAL,
	IN	PCWSTR					BlobType,
	OUT	PBCRYPT_KEY_HANDLE		KeyHandle,
	OUT	PUCHAR					KeyObject OPTIONAL,
	IN	ULONG					KeyObjectCb,
	IN	PUCHAR					Input,
	IN	ULONG					InputCb,
	IN	ULONG					Flags)
{
	return BCryptImportKey(
		MapPredefinedAlgorithmHandle(Algorithm),
		ImportKey,
		BlobType,
		KeyHandle,
		KeyObject,
		KeyObjectCb,
		Input,
		InputCb,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptImportKeyPair(
	IN	BCRYPT_ALG_HANDLE		Algorithm,
	IN	BCRYPT_KEY_HANDLE		ImportKey OPTIONAL,
	IN	PCWSTR					BlobType,
	OUT	PBCRYPT_KEY_HANDLE		KeyHandle,
	IN	PUCHAR					Input,
	IN	ULONG					InputCb,
	IN	ULONG					Flags)
{
	return BCryptImportKeyPair(
		MapPredefinedAlgorithmHandle(Algorithm),
		ImportKey,
		BlobType,
		KeyHandle,
		Input,
		InputCb,
		Flags);
}

KXCRYPAPI NTSTATUS WINAPI Ext_BCryptSetProperty(
	IN	BCRYPT_HANDLE			Object,
	IN	PCWSTR					Property,
	IN	PUCHAR					Input,
	IN	ULONG					InputCb,
	IN	ULONG					Flags)
{
	if (KXCRYP_IS_PREDEFINED_ALG_HANDLE(Object)) {
		// Although BCryptSetProperty is supposed to fail when supplied a
		// predefined handle, the Win7 function will cause an access violation.
		// Catch predefined handles and return an appropriate status code.
		return STATUS_INVALID_HANDLE;
	}

	return BCryptSetProperty(
		Object,
		Property,
		Input,
		InputCb,
		Flags);
}

//
// This is just a convenience function which wraps other functions.
// Introduced in Windows 10.
//

KXCRYPAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PCUCHAR				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PCUCHAR				Input,
	IN	ULONG				InputCb,
	OUT	PUCHAR				Output,
	IN	ULONG				OutputCb)
{
	NTSTATUS Status;
	BCRYPT_HASH_HANDLE HashHandle;

	HashHandle = NULL;

	Status = Ext_BCryptCreateHash(Algorithm, &HashHandle, 0, 0, (PBYTE) Secret, SecretCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptHashData(HashHandle, (PBYTE) Input, InputCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptFinishHash(HashHandle, Output, OutputCb, 0);

CleanupExit:
	SafeBCryptDestroyHash(HashHandle);

	return Status;
}