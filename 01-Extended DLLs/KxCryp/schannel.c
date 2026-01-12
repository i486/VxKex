#include "buildcfg.h"
#include "kxcrypp.h"

//
// TODO: Implement the ANSI version of this function.
//

KXCRYPAPI SECURITY_STATUS SEC_ENTRY Ext_AcquireCredentialsHandleW(
	IN	PWSTR			Principal OPTIONAL,
	IN	PWSTR			Package,
	IN	ULONG			CredentialUseFlags,					// SECPKG_CRED_*
	IN	PLUID			LogonId OPTIONAL,
	IN	PVOID			AuthData OPTIONAL,
	IN	SEC_GET_KEY_FN	GetKeyFunction OPTIONAL,
	IN	PVOID			GetKeyFunctionArgument OPTIONAL,
	OUT	PCredHandle		CredentialHandle,
	OUT	PTimeStamp		ExpiryTime OPTIONAL)
{
	SECURITY_STATUS SecurityStatus;
	PSCH_CREDENTIALS Credentials;
	SCHANNEL_CRED ConvertedCredentials;

	if (StringEqual(Package, UNISP_NAME) && AuthData != NULL) {
		Credentials = (PSCH_CREDENTIALS) AuthData;

		//
		// The target application is using the SChannel SSP.
		//

		if (Credentials->Version == SCH_CRED_V5) {
			//
			// Win10+ version of the structure. We need to convert the
			// SCH_CREDENTIALS structure into a SCHANNEL_CRED structure.
			//

			RtlZeroMemory(&ConvertedCredentials, sizeof(ConvertedCredentials));
			ConvertedCredentials.dwVersion			= SCH_CRED_V4;
			ConvertedCredentials.cCreds				= Credentials->NumberOfCertificateContexts;
			ConvertedCredentials.paCred				= Credentials->CertificateContexts;
			ConvertedCredentials.hRootStore			= Credentials->RootStore;
			ConvertedCredentials.cMappers			= Credentials->NumberOfMappers;
			ConvertedCredentials.aphMappers			= Credentials->Mappers;
			ConvertedCredentials.dwSessionLifespan	= Credentials->SessionLifespan;
			ConvertedCredentials.dwFlags			= Credentials->Flags & SCH_WIN7_VALID_FLAGS;

			ASSERT ((Credentials->Flags & ~SCH_WIN7_VALID_FLAGS) == 0);
			ASSERT (Credentials->NumberOfTlsParameters <= 1);

			if (Credentials->NumberOfTlsParameters >= 1 &&
				Credentials->TlsParameters != NULL) {

				PTLS_PARAMETERS TlsParameters;
				
				TlsParameters = Credentials->TlsParameters;

				ASSERT (TlsParameters->NumberOfAlpnIds == 0);
				ASSERT (TlsParameters->AlpnIds == NULL);

				ConvertedCredentials.grbitEnabledProtocols	= ~(TlsParameters->DisabledProtocols);
			}

			//
			// Make AcquireCredentialsHandleW use the structure we made.
			//

			AuthData = &ConvertedCredentials;
			KexLogDebugEvent(L"Converted v5 SChannel credentials structure to v4.");
		} else if (Credentials->Version > SCH_CRED_V5) {
			//
			// This could be a bug in the target application, or it could be
			// some future version of the structure they will make in newer
			// versions of Windows. Who knows what it will be named. Probably
			// something ridiculous like SCHANL_CREDS.
			//

			KexLogWarningEvent(
				L"SChannel credentials structure version is too high.\r\n\r\n"
				L"The application is requesting version %lu.",
				Credentials->Version);

			KexDebugCheckpoint();
		}
	}

	SecurityStatus = AcquireCredentialsHandle(
		Principal,
		Package,
		CredentialUseFlags,
		LogonId,
		AuthData,
		GetKeyFunction,
		GetKeyFunctionArgument,
		CredentialHandle,
		ExpiryTime);

	if (SecurityStatus != SEC_E_OK) {
		KexDebugCheckpoint();
	}

	return SecurityStatus;
}