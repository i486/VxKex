///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     credhndl.c
//
// Abstract:
//
//     AcquireCredentialsHandleA and AcquireCredentialsHandleW.
//
//     In order to support SSL/TLS through the KxSchanl library, we will
//     intercept calls to AcquireCredentialsHandle and replace any attempts
//     to get Schannel (UNISP_NAME) with our own DLL.
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
#include "kxcrypp.h"

//
// Translates all forms of Schannel AuthData parameters to the v4 structure
// which is understood by Win7 native Schannel.
//
// PARAMETERS
//
//   CredV4
//     Pointer to scratch space to store a v4 auth data.
//     Callers should not inspect or use the results of this structure directly.
//
//   AuthData
//     Raw AuthData passed by the caller of AcquireCredentialsHandle.
//
// RETURNS
//   AuthData pointer to pass to Schannel.
//
STATIC PVOID TranslateAuthDataForNativeSchannel(
	OUT	PSCHANNEL_CRED		CredV4,
	IN	PVOID				AuthData OPTIONAL)
{
	PCSCH_CREDENTIALS CredV5;
	ULONG Index;

	KexRtlZeroMemory(CredV4, sizeof(*CredV4));

	if (!AuthData) {
		return NULL;
	}

	switch (*(PULONG) AuthData) {
	case SCH_CRED_V4:
		// pass on unchanged - doesn't need translation
		return AuthData;
	case SCH_CRED_V5:
		CredV5 = (PCSCH_CREDENTIALS) AuthData;
		break;
	default:
		// pass on unchanged - we don't recognize this
		return AuthData;
	}

	CredV4->dwVersion					= SCH_CRED_V4;
	CredV4->dwCredFormat				= CredV5->CredentialsFormat;
	CredV4->cCreds						= CredV5->NumberOfCertificateContexts;
	CredV4->paCred						= CredV5->CertificateContexts;
	CredV4->hRootStore					= CredV5->RootStore;
	CredV4->cMappers					= CredV5->NumberOfMappers;
	CredV4->aphMappers					= CredV5->Mappers;
	CredV4->dwSessionLifespan			= CredV5->SessionLifespan;
	CredV4->dwFlags						= CredV5->Flags;

	// Remove unsupported flags
	CredV4->dwFlags &= ~SCH_SEND_AUX_RECORD;

	for (Index = 0; Index < CredV5->NumberOfTlsParameters; ++Index) {
		PTLS_PARAMETERS TlsParameters;

		TlsParameters = &CredV5->TlsParameters[Index];

		if (TlsParameters->NumberOfAlpnIds == 0 || TlsParameters->AlpnIds == NULL) {
			// These TLS parameters apply regardless of ALPN protocol (which native
			// Schannel does not support), so these are what we'll use.

			CredV4->grbitEnabledProtocols = ~(TlsParameters->DisabledProtocols);

			// Remove unsupported protocols
			CredV4->grbitEnabledProtocols &= ~SP_PROT_TLS1_3;
			
			break;
		}
	}

	return CredV4;
}

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleA(
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
	if (StringEqualIA(Package, UNISP_NAME_A) || StringEqualIA(Package, SCHANNEL_NAME_A)) {
		SECURITY_STATUS SecStatus;

		SecStatus = AcquireCredentialsHandleA(
			Principal,
			KXSCHANL_NAME_A,
			CredentialUseFlags,
			LogonId,
			AuthData,
			NULL,
			ReturnAddress(),
			CredentialHandle,
			Expiry);

		if (FAILED(SecStatus)) {
			SCHANNEL_CRED CredV4;

			//
			// KxSChanl failed, so we'll fall back to Schannel.
			// We will translate the SCH_CREDENTIALS (i.e. V5) structure to a
			// SCHANNEL_CRED structure, if necessary.
			//

			return AcquireCredentialsHandleA(
				Principal,
				Package,
				CredentialUseFlags,
				LogonId,
				TranslateAuthDataForNativeSchannel(&CredV4, AuthData),
				GetKeyFn,
				GetKeyArgument,
				CredentialHandle,
				Expiry);
		}

		return SecStatus;
	}

	return AcquireCredentialsHandleA(
		Principal,
		Package,
		CredentialUseFlags,
		LogonId,
		AuthData,
		GetKeyFn,
		GetKeyArgument,
		CredentialHandle,
		Expiry);
}

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleW(
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
	if (StringEqualIW(Package, UNISP_NAME_W) || StringEqualIW(Package, SCHANNEL_NAME_W)) {
		SECURITY_STATUS SecStatus;

		SecStatus = AcquireCredentialsHandleW(
			Principal,
			KXSCHANL_NAME_W,
			CredentialUseFlags,
			LogonId,
			AuthData,
			NULL,
			ReturnAddress(),
			CredentialHandle,
			Expiry);

		if (FAILED(SecStatus)) {
			SCHANNEL_CRED CredV4;

			// Fall back to original
			return AcquireCredentialsHandleW(
				Principal,
				Package,
				CredentialUseFlags,
				LogonId,
				TranslateAuthDataForNativeSchannel(&CredV4, AuthData),
				GetKeyFn,
				GetKeyArgument,
				CredentialHandle,
				Expiry);
		}
		
		return SecStatus;
	}

	return AcquireCredentialsHandleW(
		Principal,
		Package,
		CredentialUseFlags,
		LogonId,
		AuthData,
		GetKeyFn,
		GetKeyArgument,
		CredentialHandle,
		Expiry);
}