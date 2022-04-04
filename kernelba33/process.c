#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <BaseDll.h>

#include "process.h"

// GetProcessMitigationPolicy and SetProcessMitigationPolicy rely on NtSetInformationProcess
// values that are not present on Windows 7 (namely, ProcessMitigationPolicy (0x34)).
// However, there is an UpdateProcThreadAttribute function, which can set the
// PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY attribute. TODO: Look into this more. It is likely
// to be supported in Windows 7 but just undocumented. In fact I would say that at least
// ProcessASLRPolicy can be supported, in addition to ProcessDEPPolicy.
// Also see https://theryuu.github.io/ifeo-mitigationoptions.txt
// Also see https://support.microsoft.com/en-us/topic/an-update-is-available-for-the-aslr-feature-in-windows-7-or-in-windows-server-2008-r2-aec38646-36f1-08e8-32d2-6374d3c83d9e

WINBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	HANDLE						hProcess,
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY(L"(%p, %d, %p, %Iu)", hProcess, MitigationPolicy, lpBuffer, dwLength);

	switch (MitigationPolicy) {
	case ProcessDEPPolicy: {
		PPROCESS_MITIGATION_DEP_POLICY lpPolicy = (PPROCESS_MITIGATION_DEP_POLICY) lpBuffer;
		if (dwLength < sizeof(PROCESS_MITIGATION_DEP_POLICY)) {
			RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
			return FALSE;
		}

		lpPolicy->Permanent = !!GetSystemDEPPolicy();
						   }
		break;
	case ProcessASLRPolicy:
	case ProcessDynamicCodePolicy:
	case ProcessStrictHandleCheckPolicy:
	case ProcessSystemCallDisablePolicy:
	case ProcessMitigationOptionsMask:
	case ProcessExtensionPointDisablePolicy:
	case ProcessControlFlowGuardPolicy:
	case ProcessSignaturePolicy:
	case ProcessFontDisablePolicy:
	case ProcessImageLoadPolicy:
	case ProcessSystemCallFilterPolicy:
	case ProcessPayloadRestrictionPolicy:
	case ProcessChildProcessPolicy:
	case ProcessSideChannelIsolationPolicy:
	case ProcessUserShadowStackPolicy:
	case ProcessRedirectionTrustPolicy:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	default:
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	RtlSetLastWin32Error(ERROR_SUCCESS);
	return TRUE;
}

WINBASEAPI BOOL WINAPI SetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	IN	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY(L"(%u, %p, %Iu)", MitigationPolicy, lpBuffer, dwLength);
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

WINBASEAPI BOOL WINAPI GetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	OUT	LPVOID						ProcessInformation,
	IN	DWORD						ProcessInformationSize)
{
	NTSTATUS st;
	PROCESSINFOCLASS NtProcessInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationSize);

	if (ProcessInformationClass >= ProcessInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ProcessInformationClass) {
	case ProcessMemoryPriority:
		NtProcessInfoClass = ProcessPagePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtQueryInformationProcess(
		ProcessHandle,
		NtProcessInfoClass,
		ProcessInformation,
		ProcessInformationSize,
		NULL);

	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}

WINBASEAPI BOOL WINAPI SetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	IN	LPVOID						ProcessInformation,
	IN	DWORD						ProcessInformationSize)
{
	NTSTATUS st;
	PROCESSINFOCLASS NtProcessInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationSize);

	if (ProcessInformationClass >= ProcessInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ProcessInformationClass) {
	case ProcessMemoryPriority:
		NtProcessInfoClass = ProcessPagePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtSetInformationProcess(
		ProcessHandle,
		NtProcessInfoClass,
		ProcessInformation,
		ProcessInformationSize);

	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}