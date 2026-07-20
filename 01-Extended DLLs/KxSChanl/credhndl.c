///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     credhndl.c
//
// Abstract:
//
//     Contains the implementation of AcquireCredentialsHandle and
//     FreeCredentialsHandle.
//
//     AcquireCredentialsHandle is what applications call to retrieve a
//     "credentials handle". This handle will point to a structure which stores
//     information received from the application, such as:
//
//       - whether the application will be a server or a client.
//       - private key of the application (if any)
//       - root certificate store (for server applications requiring client
//         authentication)
//       - supported algorithms (e.g. ECDH, ECDSA, RSA, AES)
//       - enabled protocols (e.g. SSL3.0, TLS1.0, TLS1.2)
//       - minimum and maximum cipher strength
//       - session lifespan
//
//     IMPORTANT NOTE ON SSPI HANDLES:
//
//     SSPs (such as ourselves) are not permitted to set or modify the
//     dwLower member of CredHandle or CtxtHandle. This member is used to
//     store a pointer to SSPICLI's internal information. Overwriting it
//     will cause SSPICLI to fail or crash.
//
//     The data type pointed to by dwLower is DLL_SECURITY_PACKAGE.
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
//     vxiiduu               10-May-2026  Initial creation.
//     vxiiduu               23-May-2026  Remove WolfSSL
//     vxiiduu               25-Jun-2026  If all protocols are disabled through
//                                        registry user override, return an error
//                                        code to allow fallback to Schannel
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

INLINE PKXSCHANL_CREDENTIAL SppReadCredentialHandle(
	IN	PCredHandle	CredentialHandle)
{
	ASSERT (SecIsValidHandle(CredentialHandle));
	return (PKXSCHANL_CREDENTIAL) CredentialHandle->dwUpper;
}

STATIC INLINE VOID SppWriteCredentialsHandle(
	OUT	PCredHandle				CredentialHandle,
	IN	PCKXSCHANL_CREDENTIAL	Credential)
{
	ASSERT (Credential != NULL);
	CredentialHandle->dwUpper = (ULONG_PTR) Credential;
}

//
// Read in a Schannel-style mask of SP_PROT_* and apply them to the KxSChanl
// credential.
//
// App-Specific Hacks and IFEO registry configuration are applied here.
//
// PARAMETERS
//
// Credential
//   A pointer to the credential structure.
//
// RetAddr
//   A pointer to somewhere in the module which called into KxSChanl.
//   This parameter is passed by KxCryp in Ext_AcquireCredentialsHandle.
//
// EnabledProtocols
//   The protocols requested by the calling application.
//
STATIC VOID SppApplyEnabledProtocols(
	IN OUT	PKXSCHANL_CREDENTIAL	Credential,
	IN		PCVOID					RetAddr,
	IN		ULONG					EnabledProtocols)
{
	//
	// Forcibly enable and disable protocols according to the return-address of the
	// original caller of AcquireCredentialsHandle.
	//

	if (AshModuleBaseNameIs(RetAddr, L"webio.dll") ||
		AshModuleBaseNameIs(RetAddr, L"wininet.dll")) {

		// WinHTTP (through webio) and WinInet are known-good for TLS 1.3.
		EnabledProtocols |= SP_PROT_TLS1_3_CLIENT;
	}

	//
	// Forcibly enable and disable protocols if we have override values
	// from IFEO. This takes precedence over the automatically applied DLL-specific
	// or app-specific hacks applied in the previous code.
	//

	if ((KexData->IfeoParameters.TlsForceEnabledProtocols &
		 KexData->IfeoParameters.TlsForceDisabledProtocols) != 0) {

		KexLogWarningEvent(
			L"Force-enabled and force-disabled protocols conflict\r\n\r\n"
			L"One or more protocol(s) are both force-enabled and force-disabled.\r\n"
			L"Force-enabling takes precedence and those protocol(s) will be enabled.\r\n"
			L"KEX_TlsForceEnabledProtocols:  0x%08lx\r\n"
			L"KEX_TlsForceDisabledProtocols: 0x%08lx\r\n",
			KexData->IfeoParameters.TlsForceEnabledProtocols,
			KexData->IfeoParameters.TlsForceDisabledProtocols);

		KexDebugCheckpoint();
	}

	EnabledProtocols |= KexData->IfeoParameters.TlsForceEnabledProtocols;
	EnabledProtocols &= ~(KexData->IfeoParameters.TlsForceDisabledProtocols);

	Credential->EnabledProtocols = EnabledProtocols;
	Credential->EnabledProtocols &= KXSCHANL_ALL_SUPPORTED_PROTOCOLS;
}

STATIC INLINE SECURITY_STATUS SppProcessAuthData(
	IN OUT	PKXSCHANL_CREDENTIAL	Credential,
	IN		PCVOID					RetAddr,
	IN		PCVOID					AuthData)
{
	ULONG AuthDataVersion;
	ULONG CertContextFormat;
	ULONG NumberOfCertContexts;
	PPCCERT_CONTEXT CertContexts;
	HCERTSTORE RootStore;
	ULONG NumberOfMappers;
	PPVOID Mappers;
	ULONG NumberOfSupportedAlgorithms;
	PALG_ID SupportedAlgorithms;
	ULONG EnabledProtocols;
	ULONG MinimumCipherStrength;
	ULONG MaximumCipherStrength;
	ULONG SessionLifespan;
	ULONG Flags;

	EnabledProtocols = 0;

	//
	// Parse the two different AuthData structures.
	//

	AuthDataVersion = *(PULONG) AuthData;

	if (AuthDataVersion == SCH_CRED_V4) {
		PCSCHANNEL_CRED CredV4;
		
		CredV4 = (PCSCHANNEL_CRED) AuthData;

		CertContextFormat			= CredV4->dwCredFormat;
		NumberOfCertContexts		= CredV4->cCreds;
		CertContexts				= CredV4->paCred;
		RootStore					= CredV4->hRootStore;
		NumberOfMappers				= CredV4->cMappers;
		Mappers						= (PPVOID) CredV4->aphMappers;
		NumberOfSupportedAlgorithms	= CredV4->cSupportedAlgs;
		SupportedAlgorithms			= CredV4->palgSupportedAlgs;
		EnabledProtocols			= CredV4->grbitEnabledProtocols;
		MinimumCipherStrength		= CredV4->dwMinimumCipherStrength;
		MaximumCipherStrength		= CredV4->dwMaximumCipherStrength;
		SessionLifespan				= CredV4->dwSessionLifespan;
		Flags						= CredV4->dwFlags;
	} else if (AuthDataVersion == SCH_CRED_V5) {
		PCSCH_CREDENTIALS CredV5;
		ULONG Index;
		BOOLEAN AlreadyHaveGlobalTlsParameters;

		CredV5 = (PCSCH_CREDENTIALS) AuthData;

		CertContextFormat			= CredV5->CredentialsFormat;
		NumberOfCertContexts		= CredV5->NumberOfCertificateContexts;
		CertContexts				= CredV5->CertificateContexts;
		RootStore					= CredV5->RootStore;
		NumberOfMappers				= CredV5->NumberOfMappers;
		Mappers						= (PPVOID) CredV5->Mappers;
		SessionLifespan				= CredV5->SessionLifespan;
		Flags						= CredV5->Flags;

		if (CredV5->NumberOfTlsParameters > SCH_CRED_MAX_SUPPORTED_PARAMETERS) {
			return SP_LOG_RESULT(SEC_E_INVALID_PARAMETER);
		}

		AlreadyHaveGlobalTlsParameters = FALSE;

		for (Index = 0; Index < CredV5->NumberOfTlsParameters; ++Index) {
			PCTLS_PARAMETERS TlsParameters;

			TlsParameters = &CredV5->TlsParameters[Index];

			if (TlsParameters->NumberOfAlpnIds > 0 && TlsParameters->AlpnIds != NULL) {
				// ALPN IDs were specified in the TLS_PARAMETERS structure.
				//
				// This is intended to restrict the parameters in the TLS_PARAMETERS
				// structure to only apply when the selected ALPN ID matches one of the
				// IDs in the list. We don't support this since I haven't come across
				// an application which uses this feature.

				KexDebugCheckpoint();
				continue;
			}

			if (AlreadyHaveGlobalTlsParameters) {
				// MS docs say it is invalid to have more than one TLS_PARAMETERS
				// with no ALPN specifier.
				return SP_LOG_RESULT(SEC_E_INVALID_PARAMETER);
			}

			EnabledProtocols = ~(TlsParameters->DisabledProtocols);
			AlreadyHaveGlobalTlsParameters = TRUE;
		}
	} else {
		return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
	}

	if (EnabledProtocols == 0) {
		EnabledProtocols = KXSCHANL_DEFAULT_ENABLED_PROTOCOLS;
	}

	//
	// Validate the gathered information.
	//

	if (CertContextFormat != 0) {
		KexLogDebugEvent(L"dwCredFormat is non-zero, which is not supported");
		return SP_LOG_RESULT(SEC_E_UNKNOWN_CREDENTIALS);
	}

	if ((Flags & SCH_CRED_AUTO_CRED_VALIDATION) &&
		(Flags & SCH_CRED_MANUAL_CRED_VALIDATION)) {

		// Flag conflict - mutually exclusive
		return SP_LOG_RESULT(SEC_E_INVALID_PARAMETER);
	}

	if ((Flags & SCH_CRED_NO_DEFAULT_CREDS) &&
		(Flags & SCH_CRED_USE_DEFAULT_CREDS)) {

		return SP_LOG_RESULT(SEC_E_INVALID_PARAMETER);
	}

	if (Flags & ~SCH_WIN7_VALID_FLAGS) {
		KexLogDebugEvent(
			L"Extended credential flags specified\r\n\r\n"
			L"AuthDataVersion: %lu\r\n"
			L"Flags: 0x%08lx",
			AuthDataVersion,
			Flags);
	}

	Credential->Flags = Flags;

	SppApplyEnabledProtocols(Credential, RetAddr, EnabledProtocols);

	return SEC_E_OK;
}

SECURITY_STATUS SppAcquireCredentialsHandle(
	IN	ULONG			CredentialUseFlags,	// SECPKG_CRED_* flags
	IN	PCVOID			AuthData OPTIONAL,	// SCHANNEL_CREDS, SCH_CREDENTIALS, etc.
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		Expiry OPTIONAL,
	IN	PCVOID			RetAddr)
{
	SECURITY_STATUS SecStatus;
	BOOLEAN Success;
	BOOLEAN IsServer;
	PKXSCHANL_CREDENTIAL Credential;

	//
	// Check CredentialUseFlags
	//

	if (CredentialUseFlags & SECPKG_CRED_INBOUND) {
		IsServer = TRUE;
	} else if (CredentialUseFlags & SECPKG_CRED_OUTBOUND) {
		IsServer = FALSE;
	} else {
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	if (IsServer) {
		// KxSchanl does not support the server-side at the moment
		KexLogWarningEvent(L"Server side not supported");
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	//
	// Create and populate a credential structure.
	//

	Credential = SafeAllocEx(
		RtlProcessHeap(),
		HEAP_ZERO_MEMORY,
		KXSCHANL_CREDENTIAL, 1);

	ASSERT (Credential != NULL);

	if (!Credential) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	// Expiry timestamp (FILETIME) of the client certificate, or infinite for
	// no client certificate. Since we don't support client certificates we
	// will always set to infinite.
	Credential->Expiry.QuadPart = MAXLONGLONG;

	//
	// Process AuthData (SCHANNEL_CRED or SCH_CREDENTIALS).
	// This is the stage where the enabled SSL/TLS protocol versions are set.
	// SppProcessAuthData will call SppApplyEnabledProtocols but if we don't
	// have any AuthData then we will just use defaults.
	//
	// SppProcessAuthData will also set Credential->Flags to the flags supplied
	// in the SCHANNEL_CRED/SCH_CREDENTIALS structure; if there are no such flags,
	// they will remain as zero.
	//

	if (AuthData != NULL) {
		SecStatus = SppProcessAuthData(Credential, RetAddr, AuthData);

		if (FAILED(SecStatus)) {
			SppWriteCredentialsHandle(CredentialHandle, Credential);
			SppFreeCredentialsHandle(CredentialHandle);
			return SecStatus;
		}
	} else {
		SppApplyEnabledProtocols(
			Credential,
			RetAddr,
			KXSCHANL_DEFAULT_ENABLED_PROTOCOLS);
	}

	//
	// If no protocols are enabled (which can happen if the user specifies in IFEO
	// parameters TlsForceDisabledProtocols = 0xFFFFFFFF), fail here.
	// If we were invoked through KxCryp, then it will fall back to the system
	// Schannel.
	//

	if (Credential->EnabledProtocols == 0) {
		KexLogDebugEvent(L"KxSChanl disabled because no protocols are enabled");
		return SP_LOG_RESULT(SEC_E_NO_CREDENTIALS);
	}

	//
	// Open the NCrypt SSL provider, if protocol versions 1.2 or older are enabled.
	// The NCrypt SSL provider is not used for TLS 1.3.
	//

	if (Credential->EnabledProtocols & ~(SP_PROT_TLS1_3)) {
		SecStatus = SslOpenProvider(&Credential->SslProvider, MS_SCHANNEL_PROVIDER, 0);
		ASSERT (SUCCEEDED(SecStatus));
		ASSERT (Credential->SslProvider != 0);

		if (FAILED(SecStatus)) {
			SppWriteCredentialsHandle(CredentialHandle, Credential);
			SppFreeCredentialsHandle(CredentialHandle);

			// The status codes returned by Ssl* functions are usually NTSTATUSes or
			// NTE_* error codes, which shouldn't be directly returned to the caller.
			return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
		}
	}

	//
	// Load certificates.
	// We load certificates even if manual cred validation is specified.
	// This is because certificate validation can be enabled on a per-security-
	// context basis as well, and if there aren't certificates loaded in the
	// credential, then all verification will fail.
	//

	Success = SppLoadRootCertificates(Credential);
	ASSERT (Success);

	if (!Success) {
		SppWriteCredentialsHandle(CredentialHandle, Credential);
		SppFreeCredentialsHandle(CredentialHandle);
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Pass the credential pointer back to the caller in the form of a handle.
	//

	SppWriteCredentialsHandle(CredentialHandle, Credential);

	if (Expiry) {
		*Expiry = Credential->Expiry;
	}

	return SEC_E_OK;
}

SECURITY_STATUS SppFreeCredentialsHandle(
	IN	PCredHandle		CredentialHandle)
{
	PKXSCHANL_CREDENTIAL Credential;

	Credential = SppReadCredentialHandle(CredentialHandle);

	SafeCertCloseStore(Credential->RootCertStore, 0);
	SafeSslFreeObject(Credential->SslProvider);
	SafeFree(Credential);
	SecInvalidateHandle(CredentialHandle);
	return SEC_E_OK;
}