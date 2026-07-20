///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     api.c
//
// Abstract:
//
//     Exported functions from KxSchanl.dll
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

#include "buildcfg.h"
#include "kxschanlp.h"

#define KxSChanlPackageInfoFlags (SECPKG_FLAG_MUTUAL_AUTH | \
								  SECPKG_FLAG_STREAM | \
								  SECPKG_FLAG_ACCEPT_WIN32_NAME | \
								  SECPKG_FLAG_EXTENDED_ERROR | \
								  SECPKG_FLAG_MULTI_REQUIRED | \
								  SECPKG_FLAG_CONNECTION | \
								  SECPKG_FLAG_PRIVACY | \
								  SECPKG_FLAG_INTEGRITY)

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEnumerateSecurityPackagesA(
	OUT	PULONG			PackageCount,
	OUT	PSecPkgInfoA	*PackageInfo)
{
	STATIC CONST SecPkgInfoA KxSChanlPackageInfo = {
		KxSChanlPackageInfoFlags,						// fCapabilities
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,	// wVersion
		SECPKG_ID_NONE,									// wRPCID
		KXSCHANL_PACKAGE_INFO_CB_MAX_TOKEN,				// cbMaxToken
		KXSCHANL_NAME_A,								// Name
		KXSCHANL_DESC_A,								// Comment
	};

	*PackageCount = 1;
	*PackageInfo = SafeAlloc(SecPkgInfoA, *PackageCount);
	
	if (*PackageInfo == NULL) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	**PackageInfo = KxSChanlPackageInfo;
	return SEC_E_OK;
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEnumerateSecurityPackagesW(
	OUT	PULONG			PackageCount,
	OUT	PSecPkgInfoW	*PackageInfo)
{
	STATIC CONST SecPkgInfoW KxSChanlPackageInfo = {
		KxSChanlPackageInfoFlags,						// fCapabilities
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,	// wVersion
		SECPKG_ID_NONE,									// wRPCID
		KXSCHANL_PACKAGE_INFO_CB_MAX_TOKEN,				// cbMaxToken
		KXSCHANL_NAME_W,								// Name
		KXSCHANL_DESC_W,								// Comment
	};

	*PackageCount = 1;
	*PackageInfo = SafeAlloc(SecPkgInfoW, *PackageCount);

	if (*PackageInfo == NULL) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	**PackageInfo = KxSChanlPackageInfo;
	return SEC_E_OK;
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryCredentialsAttributesA(
	IN	PCredHandle	CredentialHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer)
{
	ASSERT (FALSE);
	return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryCredentialsAttributesW(
	IN	PCredHandle	CredentialHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer)
{
	ASSERT (FALSE);
	return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpAcquireCredentialsHandleA(
	IN	PSTR			Principal OPTIONAL,
	IN	PSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL)
{
	PVOID RetAddr;

	RetAddr = NULL;

	if (!StringEqualIA(Package, KXSCHANL_NAME_A)) {
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	if (GetKeyFn == NULL && GetKeyArgument != NULL) {
		// The address of the caller of AcquireCredentialsHandle is passed to us
		// via AcquireCredentialsHandle in kxcryp.dll through the unused parameter
		// "GetKeyArgument".
		RetAddr = GetKeyArgument;
	} else {
		// If we were passed NULL, we'll assume an application is calling us directly
		// (after acquiring function table through InitSecurityInterface).
		RetAddr = ReturnAddress();
	}

	return SppAcquireCredentialsHandle(
		CredentialUseFlags,
		AuthData,
		CredentialHandle,
		Expiry,
		RetAddr);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpAcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,
	IN	PVOID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFn OPTIONAL,
	IN	PVOID			GetKeyArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL)
{
	PVOID RetAddr;

	RetAddr = NULL;

	if (!StringEqualIW(Package, KXSCHANL_NAME_W)) {
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	// See comments in SpAcquireCredentialsHandleA
	if (GetKeyFn == NULL && GetKeyArgument != NULL) {
		RetAddr = GetKeyArgument;
	} else {
		RetAddr = ReturnAddress();
	}

	return SppAcquireCredentialsHandle(
		CredentialUseFlags,
		AuthData,
		CredentialHandle,
		Expiry,
		RetAddr);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpFreeCredentialsHandle(
	IN	PCredHandle		CredentialHandle)
{
	return SppFreeCredentialsHandle(CredentialHandle);
}

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
	OUT		PTimeStamp		Expiry OPTIONAL)
{
	PWSTR TargetNameUnicode;
	WCHAR TargetNameBuffer[256];

	TargetNameUnicode = NULL;

	if (TargetName) {
		INT CchConverted;

		CchConverted = MultiByteToWideChar(
			CP_ACP,
			0,
			TargetName,
			-1,
			TargetNameBuffer,
			ARRAYSIZE(TargetNameBuffer));

		if (CchConverted == 0) {
			KexLogErrorEvent(
				L"Conversion of TargetName to Unicode failed: Win32 error %lu",
				GetLastError());

			return SP_LOG_RESULT(SEC_E_INVALID_PARAMETER);
		}

		TargetNameUnicode = TargetNameBuffer;
	}

	return SppInitializeSecurityContext(
		CredentialHandle,
		ReturnAddress(),
		ContextHandle,
		TargetNameUnicode,
		ContextRequirementsFlags,
		Input,
		NewContextHandle,
		Output,
		ContextAttributesFlags,
		Expiry);
}

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
	OUT		PTimeStamp		Expiry OPTIONAL)
{
	return SppInitializeSecurityContext(
		CredentialHandle,
		ReturnAddress(),
		ContextHandle,
		TargetName,
		ContextRequirementsFlags,
		Input,
		NewContextHandle,
		Output,
		ContextAttributesFlags,
		Expiry);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpDeleteSecurityContext(
	IN	PCtxtHandle	ContextHandle)
{
	return SppDeleteSecurityContext(ContextHandle);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpApplyControlToken(
	IN	PCtxtHandle		ContextHandle,
	IN	PSecBufferDesc	Input)
{
	return SppApplyControlToken(ContextHandle, Input);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryContextAttributesA(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer)
{
	return SppQueryContextAttributes(ContextHandle, Attribute, Buffer);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpQueryContextAttributesW(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	OUT	PVOID		Buffer)
{
	return SppQueryContextAttributes(ContextHandle, Attribute, Buffer);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpFreeContextBuffer(
	IN	PVOID	ContextBuffer)
{
	SafeFree(ContextBuffer);
	return SEC_E_OK;
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpEncryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN		ULONG			QualityOfProtectionFlags,
	IN OUT	PSecBufferDesc	Message,
	IN		ULONG			MessageSequenceNumber)
{
	return SppEncryptMessage(ContextHandle, QualityOfProtectionFlags, Message);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpDecryptMessage(
	IN		PCtxtHandle		ContextHandle,
	IN OUT	PSecBufferDesc	Message,
	IN		ULONG			MessageSequenceNumber,
	OUT		PULONG			QualityOfProtectionFlags OPTIONAL)
{
	return SppDecryptMessage(ContextHandle, Message);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpSetContextAttributesA(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	IN	PVOID		Buffer,
	IN	ULONG		BufferCb)
{
	ASSERT (FALSE);
	return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
}

KXSCHANLAPI SECURITY_STATUS SEC_ENTRY SpSetContextAttributesW(
	IN	PCtxtHandle	ContextHandle,
	IN	ULONG		Attribute,
	IN	PVOID		Buffer,
	IN	ULONG		BufferCb)
{
	ASSERT (FALSE);
	return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
}

KXSCHANLAPI PSECURITY_FUNCTION_TABLE_A SEC_ENTRY SpInitSecurityInterfaceA(
	VOID)
{
	STATIC SECURITY_FUNCTION_TABLE_A FunctionTable = {
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
		SpEnumerateSecurityPackagesA,
		SpQueryCredentialsAttributesA,
		SpAcquireCredentialsHandleA,
		SpFreeCredentialsHandle,
		NULL,											// Reserved2
		SpInitializeSecurityContextA,
		NULL,											// AcceptSecurityContext
		NULL,											// CompleteAuthToken
		SpDeleteSecurityContext,
		SpApplyControlToken,							// ApplyControlToken
		SpQueryContextAttributesA,
		NULL,											// ImpersonateSecurityContext
		NULL,											// RevertSecurityContext
		NULL,											// MakeSignature
		NULL,											// VerifySignature
		SpFreeContextBuffer,
		NULL,											// QuerySecurityPackageInfo
		SpEncryptMessage,								// Reserved3
		SpDecryptMessage,								// Reserved4
		NULL,											// ExportSecurityContext
		NULL,											// ImportSecurityContext
		NULL,											// AddCredentials
		NULL,											// Reserved8
		NULL,											// QuerySecurityContextToken
		SpEncryptMessage,
		SpDecryptMessage,
		SpSetContextAttributesA,
		NULL,											// SetCredentialsAttributes
		NULL											// ChangeAccountPassword
	};

	return &FunctionTable;
}

KXSCHANLAPI PSECURITY_FUNCTION_TABLE_W SEC_ENTRY SpInitSecurityInterfaceW(
	VOID)
{
	STATIC SECURITY_FUNCTION_TABLE_W FunctionTable = {
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
		SpEnumerateSecurityPackagesW,
		SpQueryCredentialsAttributesW,
		SpAcquireCredentialsHandleW,
		SpFreeCredentialsHandle,
		NULL,											// Reserved2
		SpInitializeSecurityContextW,
		NULL,											// AcceptSecurityContext
		NULL,											// CompleteAuthToken
		SpDeleteSecurityContext,
		SpApplyControlToken,							// ApplyControlToken
		SpQueryContextAttributesW,
		NULL,											// ImpersonateSecurityContext
		NULL,											// RevertSecurityContext
		NULL,											// MakeSignature
		NULL,											// VerifySignature
		SpFreeContextBuffer,
		NULL,											// QuerySecurityPackageInfo
		SpEncryptMessage,								// Reserved3
		SpDecryptMessage,								// Reserved4
		NULL,											// ExportSecurityContext
		NULL,											// ImportSecurityContext
		NULL,											// AddCredentials
		NULL,											// Reserved8
		NULL,											// QuerySecurityContextToken
		SpEncryptMessage,
		SpDecryptMessage,
		SpSetContextAttributesW,
		NULL,											// SetCredentialsAttributes
		NULL,											// ChangeAccountPassword
	};

	return &FunctionTable;
}