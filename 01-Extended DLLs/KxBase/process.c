#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI GetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	OUT	PVOID						ProcessInformation,
	IN	ULONG						ProcessInformationSize)
{
	// TODO
	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return FALSE;
}

KXBASEAPI BOOL WINAPI SetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	IN	PVOID						ProcessInformation,
	IN	ULONG						ProcessInformationSize)
{
	// TODO
	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return FALSE;
}

KXBASEAPI BOOL WINAPI SetProcessDefaultCpuSets(
	IN	HANDLE	ProcessHandle,
	IN	PULONG	CpuSetIds,
	IN	ULONG	NumberOfCpuSetIds)
{
	if (CpuSetIds == NULL) {
		if (NumberOfCpuSetIds != 0) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI SetProcessDefaultCpuSetMasks(
	IN	HANDLE			ProcessHandle,
	IN	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			NumberOfCpuSetMasks)
{
	if (CpuSetMasks == NULL) {
		if (NumberOfCpuSetMasks != 0) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI GetProcessDefaultCpuSets(
	IN	HANDLE	ProcessHandle,
	OUT	PULONG	CpuSetIds,
	IN	ULONG	CpuSetIdArraySize,
	OUT	PULONG	ReturnCount)
{
	*ReturnCount = 0;

	if (CpuSetIds == NULL) {
		if (CpuSetIdArraySize != 0) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI GetProcessDefaultCpuSetMasks(
	IN	HANDLE			ProcessHandle,
	OUT	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			CpuSetMaskArraySize,
	OUT	PULONG			ReturnCount)
{
	*ReturnCount = 0;

	if (CpuSetMasks == NULL) {
		if (CpuSetMaskArraySize != 0) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI SetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	IN	PVOID						Buffer,
	IN	SIZE_T						BufferCb)
{
	//
	// Note that Windows 7 has SetProcessDEPPolicy but it doesn't do anything
	// for x64. So we will only do anything with it on x86.
	//

	if (KexRtlCurrentProcessBitness() == 32 && MitigationPolicy == ProcessDEPPolicy) {
		PPROCESS_MITIGATION_DEP_POLICY DepPolicy;

		if (BufferCb != sizeof(PROCESS_MITIGATION_DEP_POLICY)) {
			BaseSetLastNTError(STATUS_INVALID_BUFFER_SIZE);
			return FALSE;
		}

		DepPolicy = (PPROCESS_MITIGATION_DEP_POLICY) Buffer;

		if (DepPolicy->Flags.ReservedFlags) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}

		return SetProcessDEPPolicy(DepPolicy->Flags.AsUlong);
	} else {
		KexLogWarningEvent(
			L"SetProcessMitigationPolicy called with unsupported MitigationPolicy value %d",
			MitigationPolicy);
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	PVOID						Buffer,
	IN	SIZE_T						BufferCb)
{
	if (MitigationPolicy == ProcessDEPPolicy) {
		BOOLEAN Success;
		PPROCESS_MITIGATION_DEP_POLICY DepPolicy;
		BOOL Permanent;

		if (BufferCb != sizeof(PROCESS_MITIGATION_DEP_POLICY)) {
			BaseSetLastNTError(STATUS_INVALID_BUFFER_SIZE);
			return FALSE;
		}

		DepPolicy = (PPROCESS_MITIGATION_DEP_POLICY) Buffer;

		Success = GetProcessDEPPolicy(
			NtCurrentProcess(),
			&DepPolicy->Flags.AsUlong,
			&Permanent);

		DepPolicy->Permanent = Permanent;
		return Success;
	} else {
		KexLogWarningEvent(
			L"GetProcessMitigationPolicy called with unsupported MitigationPolicy value %d",
			MitigationPolicy);
	}

	BaseSetLastNTError(STATUS_NOT_SUPPORTED);
	return FALSE;
}