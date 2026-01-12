#include "buildcfg.h"
#include "kxbasep.h"

//
// Private structures.
//

typedef struct _WELL_KNOWN_RID_ARRAY {
	ULONG               Rid;
	WELL_KNOWN_SID_TYPE Type;
} TYPEDEF_TYPE_NAME(WELL_KNOWN_RID_ARRAY);

typedef struct _WELL_KNOWN_AUTHORITIES_TYPE {
	SID_IDENTIFIER_AUTHORITY	Authority;
	PCWELL_KNOWN_RID_ARRAY		WellKnownRids;
	ULONG						Count;
} TYPEDEF_TYPE_NAME(WELL_KNOWN_AUTHORITIES_TYPE);

STATIC CONST WELL_KNOWN_RID_ARRAY NullAuthoritySids[] = {
	{SECURITY_NULL_RID, WinNullSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY WorldAuthoritySids[] = {
	{SECURITY_WORLD_RID, WinWorldSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY LocalAuthoritySids[] = {
	{SECURITY_LOCAL_RID, WinLocalSid},
	{SECURITY_LOCAL_LOGON_RID, WinConsoleLogonSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY CreatorOwnerAuthoritySids[] = {
	{SECURITY_CREATOR_OWNER_RID, WinCreatorOwnerSid},
	{SECURITY_CREATOR_GROUP_RID, WinCreatorGroupSid},
	{SECURITY_CREATOR_OWNER_SERVER_RID, WinCreatorOwnerServerSid},
	{SECURITY_CREATOR_GROUP_SERVER_RID, WinCreatorGroupServerSid},
	{SECURITY_CREATOR_OWNER_RIGHTS_RID, WinCreatorOwnerRightsSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY NtAuthoritySids[] = {
	{SECURITY_DIALUP_RID, WinDialupSid},
	{SECURITY_NETWORK_RID, WinNetworkSid},
	{SECURITY_BATCH_RID, WinBatchSid},
	{SECURITY_INTERACTIVE_RID, WinInteractiveSid},
	{SECURITY_SERVICE_RID, WinServiceSid},
	{SECURITY_ANONYMOUS_LOGON_RID, WinAnonymousSid},
	{SECURITY_PROXY_RID, WinProxySid},
	{SECURITY_ENTERPRISE_CONTROLLERS_RID, WinEnterpriseControllersSid},
	{SECURITY_ENTERPRISE_READONLY_CONTROLLERS_RID, WinEnterpriseReadonlyControllersSid},
	{SECURITY_PRINCIPAL_SELF_RID, WinSelfSid},
	{SECURITY_AUTHENTICATED_USER_RID, WinAuthenticatedUserSid},
	{SECURITY_RESTRICTED_CODE_RID, WinRestrictedCodeSid},
	{SECURITY_TERMINAL_SERVER_RID, WinTerminalServerSid},
	{SECURITY_REMOTE_LOGON_RID, WinRemoteLogonIdSid},
	{SECURITY_THIS_ORGANIZATION_RID, WinThisOrganizationSid},
	{SECURITY_OTHER_ORGANIZATION_RID, WinOtherOrganizationSid},
	{SECURITY_WRITE_RESTRICTED_CODE_RID, WinWriteRestrictedCodeSid},
	{SECURITY_IUSER_RID, WinIUserSid},
	{SECURITY_LOCAL_SYSTEM_RID, WinLocalSystemSid},
	{SECURITY_LOCAL_SERVICE_RID, WinLocalServiceSid},
	{SECURITY_NETWORK_SERVICE_RID, WinNetworkServiceSid},
	{SECURITY_BUILTIN_DOMAIN_RID, WinBuiltinDomainSid},
	{SECURITY_LOCAL_ACCOUNT_RID, WinLocalAccountSid},
	{SECURITY_LOCAL_ACCOUNT_AND_ADMIN_RID, WinLocalAccountAndAdministratorSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY BuiltinDomainSids[] = {
	{DOMAIN_ALIAS_RID_ADMINS, WinBuiltinAdministratorsSid},
	{DOMAIN_ALIAS_RID_USERS, WinBuiltinUsersSid},
	{DOMAIN_ALIAS_RID_GUESTS, WinBuiltinGuestsSid},
	{DOMAIN_ALIAS_RID_POWER_USERS, WinBuiltinPowerUsersSid},
	{DOMAIN_ALIAS_RID_ACCOUNT_OPS, WinBuiltinAccountOperatorsSid},
	{DOMAIN_ALIAS_RID_SYSTEM_OPS, WinBuiltinSystemOperatorsSid},
	{DOMAIN_ALIAS_RID_PRINT_OPS, WinBuiltinPrintOperatorsSid},
	{DOMAIN_ALIAS_RID_BACKUP_OPS, WinBuiltinBackupOperatorsSid},
	{DOMAIN_ALIAS_RID_REPLICATOR, WinBuiltinReplicatorSid},
	{DOMAIN_ALIAS_RID_PREW2KCOMPACCESS, WinBuiltinPreWindows2000CompatibleAccessSid},
	{DOMAIN_ALIAS_RID_REMOTE_DESKTOP_USERS, WinBuiltinRemoteDesktopUsersSid},
	{DOMAIN_ALIAS_RID_NETWORK_CONFIGURATION_OPS, WinBuiltinNetworkConfigurationOperatorsSid},
	{DOMAIN_ALIAS_RID_INCOMING_FOREST_TRUST_BUILDERS, WinBuiltinIncomingForestTrustBuildersSid},
	{DOMAIN_ALIAS_RID_MONITORING_USERS, WinBuiltinPerfMonitoringUsersSid},
	{DOMAIN_ALIAS_RID_LOGGING_USERS, WinBuiltinPerfLoggingUsersSid},
	{DOMAIN_ALIAS_RID_AUTHORIZATIONACCESS, WinBuiltinAuthorizationAccessSid},
	{DOMAIN_ALIAS_RID_TS_LICENSE_SERVERS, WinBuiltinTerminalServerLicenseServersSid},
	{DOMAIN_ALIAS_RID_DCOM_USERS, WinBuiltinDCOMUsersSid},
	{DOMAIN_ALIAS_RID_IUSERS, WinBuiltinIUsersSid},
	{DOMAIN_ALIAS_RID_CRYPTO_OPERATORS, WinBuiltinCryptoOperatorsSid},
	{DOMAIN_ALIAS_RID_EVENT_LOG_READERS_GROUP, WinBuiltinEventLogReadersGroup},
	{DOMAIN_ALIAS_RID_CERTSVC_DCOM_ACCESS_GROUP, WinBuiltinCertSvcDComAccessGroup},
	{DOMAIN_ALIAS_RID_RDS_REMOTE_ACCESS_SERVERS, WinBuiltinRDSRemoteAccessServersSid},
	{DOMAIN_ALIAS_RID_RDS_ENDPOINT_SERVERS, WinBuiltinRDSEndpointServersSid},
	{DOMAIN_ALIAS_RID_RDS_MANAGEMENT_SERVERS, WinBuiltinRDSManagementServersSid},
	{DOMAIN_ALIAS_RID_HYPER_V_ADMINS, WinBuiltinHyperVAdminsSid},
	{DOMAIN_ALIAS_RID_ACCESS_CONTROL_ASSISTANCE_OPS, WinBuiltinAccessControlAssistanceOperatorsSid},
	{DOMAIN_ALIAS_RID_REMOTE_MANAGEMENT_USERS, WinBuiltinRemoteManagementUsersSid},
	{DOMAIN_ALIAS_RID_DEFAULT_ACCOUNT, WinBuiltinDefaultSystemManagedGroupSid},
	{DOMAIN_ALIAS_RID_STORAGE_REPLICA_ADMINS, WinBuiltinStorageReplicaAdminsSid},
	{DOMAIN_ALIAS_RID_DEVICE_OWNERS, WinBuiltinDeviceOwnersSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY AccountDomainSids[] = {
	{DOMAIN_USER_RID_ADMIN, WinAccountAdministratorSid},
	{DOMAIN_USER_RID_GUEST, WinAccountGuestSid},
	{DOMAIN_USER_RID_KRBTGT, WinAccountKrbtgtSid},
	{DOMAIN_GROUP_RID_ADMINS, WinAccountDomainAdminsSid},
	{DOMAIN_GROUP_RID_USERS, WinAccountDomainUsersSid},
	{DOMAIN_GROUP_RID_GUESTS, WinAccountDomainGuestsSid},
	{DOMAIN_GROUP_RID_COMPUTERS, WinAccountComputersSid},
	{DOMAIN_GROUP_RID_CONTROLLERS, WinAccountControllersSid},
	{DOMAIN_GROUP_RID_READONLY_CONTROLLERS, WinAccountReadonlyControllersSid},
	{DOMAIN_GROUP_RID_CERT_ADMINS, WinAccountCertAdminsSid},
	{DOMAIN_GROUP_RID_SCHEMA_ADMINS, WinAccountSchemaAdminsSid},
	{DOMAIN_GROUP_RID_ENTERPRISE_ADMINS, WinAccountEnterpriseAdminsSid},
	{DOMAIN_GROUP_RID_POLICY_ADMINS, WinAccountPolicyAdminsSid},
	{DOMAIN_ALIAS_RID_RAS_SERVERS, WinAccountRasAndIasServersSid},
	{DOMAIN_ALIAS_RID_CACHEABLE_PRINCIPALS_GROUP, WinCacheablePrincipalsGroupSid},
	{DOMAIN_ALIAS_RID_NON_CACHEABLE_PRINCIPALS_GROUP, WinNonCacheablePrincipalsGroupSid},
	{DOMAIN_GROUP_RID_ENTERPRISE_READONLY_DOMAIN_CONTROLLERS, WinNewEnterpriseReadonlyControllersSid},
	{DOMAIN_GROUP_RID_CLONEABLE_CONTROLLERS, WinAccountCloneableControllersSid},
	{DOMAIN_GROUP_RID_PROTECTED_USERS, WinAccountProtectedUsersSid},
	{DOMAIN_USER_RID_DEFAULT_ACCOUNT, WinAccountDefaultSystemManagedSid},
	{DOMAIN_GROUP_RID_KEY_ADMINS, WinAccountKeyAdminsSid},
	{DOMAIN_GROUP_RID_ENTERPRISE_KEY_ADMINS, WinAccountEnterpriseKeyAdminsSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY SecurityPackageSids[] = {
	{SECURITY_PACKAGE_NTLM_RID, WinNTLMAuthenticationSid},
	{SECURITY_PACKAGE_DIGEST_RID, WinDigestAuthenticationSid},
	{SECURITY_PACKAGE_SCHANNEL_RID, WinSChannelAuthenticationSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY SecurityMandatorySids[] = {
	{SECURITY_MANDATORY_UNTRUSTED_RID, WinUntrustedLabelSid},
	{SECURITY_MANDATORY_LOW_RID, WinLowLabelSid},
	{SECURITY_MANDATORY_MEDIUM_RID, WinMediumLabelSid},
	{SECURITY_MANDATORY_MEDIUM_PLUS_RID, WinMediumPlusLabelSid},
	{SECURITY_MANDATORY_HIGH_RID, WinHighLabelSid},
	{SECURITY_MANDATORY_SYSTEM_RID, WinSystemLabelSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY CredTypeSids[] = {
	{SECURITY_CRED_TYPE_THIS_ORG_CERT_RID, WinThisOrganizationCertificateSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY ApplicationPackageTypeSids[] = {
	{SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE, WinBuiltinAnyPackageSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY ApplicationCapabilityTypeSids[] = {
	{SECURITY_CAPABILITY_INTERNET_CLIENT, WinCapabilityInternetClientSid},
	{SECURITY_CAPABILITY_INTERNET_CLIENT_SERVER, WinCapabilityInternetClientServerSid},
	{SECURITY_CAPABILITY_PRIVATE_NETWORK_CLIENT_SERVER, WinCapabilityPrivateNetworkClientServerSid},
	{SECURITY_CAPABILITY_PICTURES_LIBRARY, WinCapabilityPicturesLibrarySid},
	{SECURITY_CAPABILITY_VIDEOS_LIBRARY, WinCapabilityVideosLibrarySid},
	{SECURITY_CAPABILITY_MUSIC_LIBRARY, WinCapabilityMusicLibrarySid},
	{SECURITY_CAPABILITY_DOCUMENTS_LIBRARY, WinCapabilityDocumentsLibrarySid},
	{SECURITY_CAPABILITY_ENTERPRISE_AUTHENTICATION, WinCapabilityEnterpriseAuthenticationSid},
	{SECURITY_CAPABILITY_SHARED_USER_CERTIFICATES, WinCapabilitySharedUserCertificatesSid},
	{SECURITY_CAPABILITY_REMOVABLE_STORAGE, WinCapabilityRemovableStorageSid},
	{SECURITY_CAPABILITY_APPOINTMENTS, WinCapabilityAppointmentsSid},
	{SECURITY_CAPABILITY_CONTACTS, WinCapabilityContactsSid},
};

STATIC CONST WELL_KNOWN_RID_ARRAY AuthenticationAuthoritySids[] = {
	{SECURITY_AUTHENTICATION_AUTHORITY_ASSERTED_RID, WinAuthenticationAuthorityAssertedSid},
	{SECURITY_AUTHENTICATION_SERVICE_ASSERTED_RID, WinAuthenticationServiceAssertedSid},
	{SECURITY_AUTHENTICATION_FRESH_KEY_AUTH_RID, WinAuthenticationFreshKeyAuthSid},
	{SECURITY_AUTHENTICATION_KEY_TRUST_RID, WinAuthenticationKeyTrustSid},
	{SECURITY_AUTHENTICATION_KEY_PROPERTY_MFA_RID, WinAuthenticationKeyPropertyMFASid},
	{SECURITY_AUTHENTICATION_KEY_PROPERTY_ATTESTATION_RID, WinAuthenticationKeyPropertyAttestationSid},
};

#define NULL_AUTHORITY_INDEX			0
#define WORLD_AUTHORITY_INDEX			1
#define LOCAL_AUTHORITY_INDEX			2
#define CREATOR_OWNER_AUTHORITY_INDEX	3
#define NT_AUTHORITY_INDEX				4
#define BUILTIN_DOMAIN_INDEX			5
#define ACCOUNT_DOMAIN_INDEX			6
#define SECURITY_PACKAGE_INDEX			7
#define SECURITY_MANDATORY_INDEX		8
#define CRED_TYPE_INDEX					9
#define APPLICATION_PACKAGE_INDEX		10
#define APPLICATION_CAPABILITY_INDEX	11
#define AUTHENTICATION_AUTHORITY_INDEX	12

STATIC CONST WELL_KNOWN_AUTHORITIES_TYPE KnownAuthoritiesAndDomains[] = {
	{SECURITY_NULL_SID_AUTHORITY,			NullAuthoritySids,				ARRAYSIZE(NullAuthoritySids)},
	{SECURITY_WORLD_SID_AUTHORITY,			WorldAuthoritySids,				ARRAYSIZE(WorldAuthoritySids)},
	{SECURITY_LOCAL_SID_AUTHORITY,			LocalAuthoritySids,				ARRAYSIZE(LocalAuthoritySids)},
	{SECURITY_CREATOR_SID_AUTHORITY,		CreatorOwnerAuthoritySids,		ARRAYSIZE(CreatorOwnerAuthoritySids)},
	{SECURITY_NT_AUTHORITY,					NtAuthoritySids,				ARRAYSIZE(NtAuthoritySids)},
	{SECURITY_NT_AUTHORITY,					BuiltinDomainSids,				ARRAYSIZE(BuiltinDomainSids)},
	{SECURITY_NT_AUTHORITY,					AccountDomainSids,				ARRAYSIZE(AccountDomainSids)},
	{SECURITY_NT_AUTHORITY,					SecurityPackageSids,			ARRAYSIZE(SecurityPackageSids)},
	{SECURITY_MANDATORY_LABEL_AUTHORITY,	SecurityMandatorySids,			ARRAYSIZE(SecurityMandatorySids)},
	{SECURITY_NT_AUTHORITY,					CredTypeSids,					ARRAYSIZE(CredTypeSids)},
	{SECURITY_APP_PACKAGE_AUTHORITY,		ApplicationPackageTypeSids,		ARRAYSIZE(ApplicationPackageTypeSids)},
	{SECURITY_APP_PACKAGE_AUTHORITY,		ApplicationCapabilityTypeSids,	ARRAYSIZE(ApplicationCapabilityTypeSids)},
	{SECURITY_AUTHENTICATION_AUTHORITY,		AuthenticationAuthoritySids,	ARRAYSIZE(AuthenticationAuthoritySids)},
};

//
// See the document "[MS-SECO] Windows Security Overview.pdf" to find out
// what the fuck RIDs and SIDs even are.
//

KXBASEAPI BOOL WINAPI Ext_CreateWellKnownSid(
	IN		WELL_KNOWN_SID_TYPE	WellKnownSidType,
	IN		PSID				DomainSid OPTIONAL,
	OUT		PSID				Sid,
	IN OUT	PULONG				SidSize)
{
	NTSTATUS Status;
	ULONG Index;
	ULONG Rid;
	UCHAR NumberOfSubAuthorities;
	ULONG RequiredSidSize;

	//
	// Ensure that the DomainSid is valid, if it is present.
	//

	if (DomainSid && !RtlValidSid(DomainSid)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (SidSize == NULL) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//
	// WinLogonIdsSid is special cased and cannot be created using this
	// function.
	//

	if (WellKnownSidType == WinLogonIdsSid) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
	}

	//
	// Loop through the table and find the index of the table entry which
	// corresponds with the type of well known SID which the caller has
	// specified.
	//

	for (Index = 0; Index < ARRAYSIZE(KnownAuthoritiesAndDomains); ++Index) {
		ULONG Index2;

		for (Index2 = 0; Index2 < KnownAuthoritiesAndDomains[Index].Count; ++Index2) {
			if (WellKnownSidType == KnownAuthoritiesAndDomains[Index].WellKnownRids[Index2].Type) {
				Rid = KnownAuthoritiesAndDomains[Index].WellKnownRids[Index2].Rid;
				goto FoundMatch;
			}
		}
	}

	//
	// If we got here, that means we didn't find a match in the loop above.
	//

	if (WellKnownSidType == WinNtAuthoritySid || WellKnownSidType == WinUserModeDriversSid) {
		// Special cases.
		Index = NT_AUTHORITY_INDEX;
	} else {
		// An invalid well-known SID type was specified.
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

FoundMatch:
	//
	// A valid well-known SID type was specified.
	// The Index variable is now initialized.
	// We will now determine whether the user-supplied buffer is large enough
	// to hold the requested SID.
	//

	switch (Index) {
	case NULL_AUTHORITY_INDEX:
	case WORLD_AUTHORITY_INDEX:
	case LOCAL_AUTHORITY_INDEX:
	case CREATOR_OWNER_AUTHORITY_INDEX:
	case NT_AUTHORITY_INDEX:
	case SECURITY_MANDATORY_INDEX:
	case AUTHENTICATION_AUTHORITY_INDEX:
		if (WellKnownSidType == WinNtAuthoritySid) {
			NumberOfSubAuthorities = 0;
		} else if (WellKnownSidType == WinUserModeDriversSid) {
			NumberOfSubAuthorities = SECURITY_USERMODEDRIVERHOST_ID_RID_COUNT;
		} else {
			NumberOfSubAuthorities = 1;
		}

		break;

	case SECURITY_PACKAGE_INDEX:
	case BUILTIN_DOMAIN_INDEX:
	case APPLICATION_PACKAGE_INDEX:
	case APPLICATION_CAPABILITY_INDEX:
	case CRED_TYPE_INDEX:
		NumberOfSubAuthorities = 2;
		break;

	case ACCOUNT_DOMAIN_INDEX:
		if (DomainSid == NULL) {
			RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		NumberOfSubAuthorities = *RtlSubAuthorityCountSid(DomainSid);

		//
		// We will add one to the number of sub-authorities so there is space
		// for the RID. If adding one would cause the number of sub-authorities
		// to exceed the limit, return a failure status to the caller.
		//
		
		if (NumberOfSubAuthorities >= SID_MAX_SUB_AUTHORITIES) {
			RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		++NumberOfSubAuthorities;
		break;

	default:
		// Invalid index.
		NOT_REACHED;
	}

	//
	// Now calculate the actual length, in bytes, that this SID is going to be.
	//

	RequiredSidSize = GetSidLengthRequired(NumberOfSubAuthorities);

	if (RequiredSidSize > *SidSize) {
		//
		// There is not enough space in the caller-supplied buffer.
		// Tell the caller how much space is needed, and return failure status.
		//

		*SidSize = RequiredSidSize;

		RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	//
	// The caller-supplied buffer is large enough to hold the SID.
	// Tell the caller how long the SID is.
	//

	*SidSize = RequiredSidSize;

	//
	// Write out the majority of the SID.
	//

	switch (Index) {
	case ACCOUNT_DOMAIN_INDEX:
		
		Status = RtlCopySid(*SidSize, Sid, DomainSid);

		if (NT_SUCCESS(Status)) {
			++*RtlSubAuthorityCountSid(Sid);
		}

		break;

	case NULL_AUTHORITY_INDEX:
	case WORLD_AUTHORITY_INDEX:
	case LOCAL_AUTHORITY_INDEX:
	case CREATOR_OWNER_AUTHORITY_INDEX:
	case NT_AUTHORITY_INDEX:
	case BUILTIN_DOMAIN_INDEX:
	case SECURITY_PACKAGE_INDEX:
	case SECURITY_MANDATORY_INDEX:
	case CRED_TYPE_INDEX:
	case APPLICATION_PACKAGE_INDEX:
	case APPLICATION_CAPABILITY_INDEX:
	case AUTHENTICATION_AUTHORITY_INDEX:
		
		Status = RtlInitializeSid(
			Sid,
			&KnownAuthoritiesAndDomains[Index].Authority,
			NumberOfSubAuthorities);

		if (NT_SUCCESS(Status)) {
			switch (Index) {
			case BUILTIN_DOMAIN_INDEX:
				ASSERT (NumberOfSubAuthorities > 1);
				*RtlSubAuthoritySid(Sid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
				break;
			case SECURITY_PACKAGE_INDEX:
				ASSERT (NumberOfSubAuthorities == SECURITY_PACKAGE_RID_COUNT);
				*RtlSubAuthoritySid(Sid, 0) = SECURITY_PACKAGE_BASE_RID;
				break;
			case CRED_TYPE_INDEX:
				ASSERT (NumberOfSubAuthorities == SECURITY_CRED_TYPE_RID_COUNT);
				*RtlSubAuthoritySid(Sid, 0) = SECURITY_CRED_TYPE_BASE_RID;
				break;
			case APPLICATION_PACKAGE_INDEX:
				ASSERT (NumberOfSubAuthorities == SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT);
				*RtlSubAuthoritySid(Sid, 0) = SECURITY_APP_PACKAGE_BASE_RID;
				break;
			case APPLICATION_CAPABILITY_INDEX:
				ASSERT (NumberOfSubAuthorities == SECURITY_BUILTIN_CAPABILITY_RID_COUNT);
				*RtlSubAuthoritySid(Sid, 0) = SECURITY_CAPABILITY_BASE_RID;
				break;
			case NT_AUTHORITY_INDEX:
				if (WellKnownSidType == WinUserModeDriversSid) {
					ASSERT (NumberOfSubAuthorities == SECURITY_USERMODEDRIVERHOST_ID_RID_COUNT);
					*RtlSubAuthoritySid(Sid, 0) = SECURITY_USERMODEDRIVERHOST_ID_BASE_RID;
					*RtlSubAuthoritySid(Sid, 1) = 0;
					*RtlSubAuthoritySid(Sid, 2) = 0;
					*RtlSubAuthoritySid(Sid, 3) = 0;
					*RtlSubAuthoritySid(Sid, 4) = 0;
					*RtlSubAuthoritySid(Sid, 5) = 0;
				}

				break;
			default:
				break;
			}
		}

		break;

	default:
		// Invalid index.
		NOT_REACHED;
	}

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Finally, add the RID.
	//

	if (NumberOfSubAuthorities > 0 && WellKnownSidType != WinUserModeDriversSid) {
		*RtlSubAuthoritySid(Sid, NumberOfSubAuthorities - 1) = Rid;
	}

	return TRUE;
}

//
// TODO: implement IsWellKnownSid
//