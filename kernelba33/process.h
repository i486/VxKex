#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

typedef enum _PROCESS_INFORMATION_CLASS {
	ProcessMemoryPriority,
	ProcessMemoryExhaustionInfo,
	ProcessAppMemoryInfo,
	ProcessInPrivateInfo,
	ProcessPowerThrottling,
	ProcessReservedValue1,
	ProcessTelemetryCoverageInfo,
	ProcessProtectionLevelInfo,
	ProcessLeapSecondInfo,
	ProcessMachineTypeInfo,
	ProcessInformationClassMax
} PROCESS_INFORMATION_CLASS;

typedef enum _PROCESS_MITIGATION_POLICY {
	ProcessDEPPolicy,
	ProcessASLRPolicy,
	ProcessDynamicCodePolicy,
	ProcessStrictHandleCheckPolicy,
	ProcessSystemCallDisablePolicy,
	ProcessMitigationOptionsMask,
	ProcessExtensionPointDisablePolicy,
	ProcessControlFlowGuardPolicy,
	ProcessSignaturePolicy,
	ProcessFontDisablePolicy,
	ProcessImageLoadPolicy,
	ProcessSystemCallFilterPolicy,
	ProcessPayloadRestrictionPolicy,
	ProcessChildProcessPolicy,
	ProcessSideChannelIsolationPolicy,
	ProcessUserShadowStackPolicy,
	ProcessRedirectionTrustPolicy,
	MaxProcessMitigationPolicy
} PROCESS_MITIGATION_POLICY, *PPROCESS_MITIGATION_POLICY;

typedef struct _PROCESS_MITIGATION_DEP_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD Enable : 1;
			DWORD DisableAtlThunkEmulation : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;

	BOOLEAN Permanent;
} PROCESS_MITIGATION_DEP_POLICY, *PPROCESS_MITIGATION_DEP_POLICY;

typedef struct _PROCESS_MITIGATION_ASLR_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD EnableBottomUpRandomization : 1;
			DWORD EnableForceRelocateImages : 1;
			DWORD EnableHighEntropy : 1;
			DWORD DisallowStrippedImages : 1;
			DWORD ReservedFlags : 28;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_ASLR_POLICY, *PPROCESS_MITIGATION_ASLR_POLICY;

typedef struct _PROCESS_MITIGATION_DYNAMIC_CODE_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD ProhibitDynamicCode : 1;
			DWORD AllowThreadOptOut : 1;
			DWORD AllowRemoteDowngrade : 1;
			DWORD AuditProhibitDynamicCode : 1;
			DWORD ReservedFlags : 28;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_DYNAMIC_CODE_POLICY, *PPROCESS_MITIGATION_DYNAMIC_CODE_POLICY;

typedef struct _PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD RaiseExceptionOnInvalidHandleReference : 1;
			DWORD HandleExceptionsPermanentlyEnabled : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PPROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

typedef struct _PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD DisallowWin32kSystemCalls : 1;
			DWORD AuditDisallowWin32kSystemCalls : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY, *PPROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD DisableExtensionPoints : 1;
			DWORD ReservedFlags : 31;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY, *PPROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD EnableControlFlowGuard : 1;
			DWORD EnableExportSuppression : 1;
			DWORD StrictMode : 1;
			DWORD EnableXfg : 1;
			DWORD EnableXfgAuditMode : 1;
			DWORD ReservedFlags : 27;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PPROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

typedef struct _PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD MicrosoftSignedOnly : 1;
			DWORD StoreSignedOnly : 1;
			DWORD MitigationOptIn : 1;
			DWORD AuditMicrosoftSignedOnly : 1;
			DWORD AuditStoreSignedOnly : 1;
			DWORD ReservedFlags : 27;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY, *PPROCESS_MITIGATION_BINARY_SIGNATURE_POLICY;

typedef struct _PROCESS_MITIGATION_FONT_DISABLE_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD DisableNonSystemFonts : 1;
			DWORD AuditNonSystemFontLoading : 1;
			DWORD ReservedFlags : 30;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_FONT_DISABLE_POLICY, *PPROCESS_MITIGATION_FONT_DISABLE_POLICY;

typedef struct _PROCESS_MITIGATION_IMAGE_LOAD_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD NoRemoteImages : 1;
			DWORD NoLowMandatoryLabelImages : 1;
			DWORD PreferSystem32Images : 1;
			DWORD AuditNoRemoteImages : 1;
			DWORD AuditNoLowMandatoryLabelImages : 1;
			DWORD ReservedFlags : 27;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_IMAGE_LOAD_POLICY, *PPROCESS_MITIGATION_IMAGE_LOAD_POLICY;

typedef struct _PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD SmtBranchTargetIsolation : 1;
			DWORD IsolateSecurityDomain : 1;
			DWORD DisablePageCombine : 1;
			DWORD SpeculativeStoreBypassDisable : 1;
			DWORD ReservedFlags : 28;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY, *PPROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY;

typedef struct _PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY {
	union {
		DWORD Flags;

		struct {
			DWORD EnableUserShadowStack : 1;
			DWORD AuditUserShadowStack : 1;
			DWORD SetContextIpValidation : 1;
			DWORD AuditSetContextIpValidation : 1;
			DWORD EnableUserShadowStackStrictMode : 1;
			DWORD BlockNonCetBinaries : 1;
			DWORD BlockNonCetBinariesNonEhcont : 1;
			DWORD AuditBlockNonCetBinaries : 1;
			DWORD CetDynamicApisOutOfProcOnly : 1;
			DWORD SetContextIpValidationRelaxedMode : 1;
			DWORD ReservedFlags : 22;
		} DUMMYSTRUCTNAME;
	} DUMMYUNIONNAME;
} PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY, *PPROCESS_MITIGATION_USER_SHADOW_STACK_POLICY;