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
	// for x64.
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

		// Fall through and pretend we succeeded.
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	PVOID						Buffer,
	IN	SIZE_T						BufferCb)
{
	if (MitigationPolicy == ProcessMitigationOptionsMask) {
		PULONGLONG Mask;

		//
		// Buffer is a pointer to either one or two ULONGLONGs.
		// The first one contains PROCESS_CREATION_MITIGATION_POLICY_*.
		// The second one if present contains PROCESS_CREATION_MITIGATION_POLICY2_*
		//

		if (BufferCb != 8 && BufferCb != 16) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}

		Mask = (PULONGLONG) Buffer;

		Mask[0] = PROCESS_CREATION_MITIGATION_POLICY_VALID_MASK;

		if (BufferCb > 8) {
			Mask[1] = 0;
		}

		return TRUE;
	} else if (MitigationPolicy == ProcessDEPPolicy) {
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

KXBASEAPI BOOL WINAPI Ext_IsProcessInJob(
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	JobHandle,
	OUT	PBOOL	IsInJob)
{
	//
	// APPSPECIFICHACK: Make Chromium non-official builds not meddle with the IAT.
	// See sandbox\policy\win\sandbox_win.cc SandboxWin::InitBrokerServices.
	// This function should only be called in one place.
	//

	if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM) &&
		AshModuleBaseNameIs(ReturnAddress(), L"chrome.dll")) {

		ASSERT (ProcessHandle == NtCurrentProcess());
		ASSERT (JobHandle == NULL);
		ASSERT (IsInJob != NULL);

		KexLogDebugEvent(L"Returning fake IsProcessInJob return value for Chrome compatibility");

		if (ProcessHandle == NtCurrentProcess() &&
			JobHandle == NULL &&
			IsInJob != NULL) {

			*IsInJob = TRUE;
			return TRUE;
		}
	}

	return IsProcessInJob(ProcessHandle, JobHandle, IsInJob);
}

KXBASEAPI BOOL WINAPI Ext_UpdateProcThreadAttribute(
	IN OUT	PPROC_THREAD_ATTRIBUTE_LIST	AttributeList,
	IN		ULONG						Flags,
	IN		ULONG_PTR					Attribute,
	IN		PVOID						Value,
	IN		SIZE_T						Size,
	OUT		PVOID						PreviousValue OPTIONAL,
	OUT		PSIZE_T						ReturnSize OPTIONAL)
{
	BOOLEAN Success;
	BOOLEAN AlreadyTriedAgain;
	ULONG MitigationPolicy;

	AlreadyTriedAgain = FALSE;

TryAgain:
	Success = UpdateProcThreadAttribute(
		AttributeList,
		Flags,
		Attribute,
		Value,
		Size,
		PreviousValue,
		ReturnSize);

	if (!Success) {
		ULONG LastError;

		// Save last-error code so we can restore it. The code below might modify it
		// by accident.
		LastError = GetLastError();

		if (AlreadyTriedAgain) {
			KexLogWarningEvent(
				L"UpdateProcThreadAttribute failed despite modifying parameters.\r\n\r\n"
				L"Win32 error: (%d) %s",
				LastError, Win32ErrorAsString(LastError));

			ASSERT (FALSE);
		} else {
			if (LastError == ERROR_NOT_SUPPORTED) {
				switch (Attribute) {
				case PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY:
					//
					// Edit the mitigation policy and go try again.
					//

					if (!AlreadyTriedAgain) {
						KexLogDebugEvent(L"Unsupported mitigation policies specified. Stripping.");

						if (Size > sizeof(ULONG)) {
							ASSERT (Size == sizeof(ULONGLONG) || Size == 2 * sizeof(ULONGLONG));
							Size = sizeof(ULONG);
						}

						MitigationPolicy = (*(PULONG) Value) & PROCESS_CREATION_MITIGATION_POLICY_VALID_MASK;
						Value = &MitigationPolicy;
				
						AlreadyTriedAgain = TRUE;
						goto TryAgain;
					}
				
					break;
				default:
					// Just pretend we succeeded and hope nothing bad happens.
					// Most of the extra mitigation policies don't really do anything anyway,
					// so it should be fine to just pretend it succeeded.
					Success = TRUE;
					LastError = ERROR_SUCCESS;
					break;
				}
			}
		}

		SetLastError(LastError);
	}

	return Success;
}