///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     thread.c
//
// Abstract:
//
//     Extended functions for dealing with threads.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu               07-Nov-2022  Initial creation.
//     vxiiduu               29-Apr-2025  Add SetThreadpoolTimerEx and
//                                        GetSystemCpuSetInformation from dotexe
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

//
// Retrieves the description that was assigned to a thread by calling
// SetThreadDescription.
//
// Parameters:
//
//   ThreadHandle
//     A handle to the thread for which to retrieve the description. The handle
//     must have THREAD_QUERY_LIMITED_INFORMATION access.
//
//   ThreadDescription
//     A Unicode string that contains the description of the thread.
//
// Return value:
//
//   If the function succeeds, the return value is the HRESULT that denotes a
//   successful operation. If the function fails, the return value is an HRESULT
//   that denotes the error.
//
// Remarks:
//
//   The description for a thread can change at any time. For example, a different
//   thread can change the description of a thread of interest while you try to
//   retrieve that description.
//
//   Thread descriptions do not need to be unique.
//
//   To free the memory for the thread description, call the LocalFree method.
//
KXBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	ThreadHandle,
	OUT	PPWSTR	ThreadDescription)
{
	NTSTATUS Status;
	PUNICODE_STRING Description;
	ULONG DescriptionCb;

	*ThreadDescription = NULL;
	Description = NULL;
	DescriptionCb = 64 * sizeof(WCHAR);

	//
	// The thread description can be changed at any time by another
	// thread. Therefore, we need to put the code in the loop in case
	// the length changes.
	//

	while (TRUE) {
		Description = (PUNICODE_STRING) SafeAlloc(BYTE, DescriptionCb);

		if (!Description) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = NtQueryInformationThread(
			ThreadHandle,
			ThreadNameInformation,
			&Description,
			DescriptionCb,
			&DescriptionCb);

		//
		// If the call succeeded, or if there was a failure caused by
		// anything other than our buffer being too small, break out of
		// the loop.
		//

		if (Status != STATUS_INFO_LENGTH_MISMATCH &&
			Status != STATUS_BUFFER_TOO_SMALL &&
			Status != STATUS_BUFFER_OVERFLOW) {
			break;
		}

		SafeFree(Description);
	}

	if (NT_SUCCESS(Status)) {
		PWSTR ReturnDescription;
		ULONG DescriptionCch;

		//
		// Shift the buffer of the UNICODE_STRING backwards onto its
		// base address, and make sure it's null terminated.
		//

		ReturnDescription = (PWSTR) Description;
		DescriptionCch = Description->Length / sizeof(WCHAR);
		RtlMoveMemory(ReturnDescription, Description->Buffer, Description->Length);
		ReturnDescription[DescriptionCch] = L'\0';

		*ThreadDescription = ReturnDescription;
		Description = NULL;
	}

	SafeFree(Description);
	return HRESULT_FROM_NT(Status);
}

//
// Assigns a description to a thread.
//
// Parameters:
//
//   ThreadHandle
//     A handle for the thread for which you want to set the description.
//     The handle must have THREAD_SET_LIMITED_INFORMATION access.
//
//   ThreadDescription
//     A Unicode string that specifies the description of the thread.
//
// Return value:
//
//   If the function succeeds, the return value is the HRESULT that denotes
//   a successful operation. If the function fails, the return value is an
//   HRESULT that denotes the error.
//
// Remarks:
//
//   The description of a thread can be set more than once; the most recently
//   set value is used. You can retrieve the description of a thread by
//   calling GetThreadDescription.
//
KXBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	ThreadHandle,
	IN	PCWSTR	ThreadDescription)
{
	NTSTATUS Status;
	UNICODE_STRING Description;

	Status = RtlInitUnicodeStringEx(&Description, ThreadDescription);
	
	if (NT_SUCCESS(Status)) {
		Status = NtSetInformationThread(
			ThreadHandle,
			ThreadNameInformation,
			&Description,
			sizeof(Description));
	}

	return HRESULT_FROM_NT(Status);
}

//
// This very simple function was introduced in Windows 8 as a documented way
// to query a few values out of the TEB.
//
KXBASEAPI VOID WINAPI GetCurrentThreadStackLimits(
	OUT	PULONG_PTR	LowLimit,
	OUT	PULONG_PTR	HighLimit)
{
	PTEB Teb;

	Teb = NtCurrentTeb();

	*LowLimit = (ULONG_PTR) Teb->DeallocationStack;
	*HighLimit = (ULONG_PTR) Teb->NtTib.StackBase;
}

KXBASEAPI BOOL WINAPI SetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	IN	PVOID						ThreadInformation,
	IN	ULONG						ThreadInformationSize)
{
	NTSTATUS Status;
	THREADINFOCLASS ThreadInformationClassNt;
	THREAD_POWER_THROTTLING_STATE ThrottlingState;

	switch (ThreadInformationClass) {
	case ThreadMemoryPriority:
		ThreadInformationClassNt = ThreadPagePriority;
		break;
	case ThreadAbsoluteCpuPriority:
		ThreadInformationClassNt = ThreadActualBasePriority;
		break;
	case ThreadDynamicCodePolicy:
		ThreadInformationClassNt = ThreadDynamicCodePolicyInfo;
		break;
	case ThreadPowerThrottling:
		ThreadInformationClassNt = ThreadPowerThrottlingState;
		break;
	default:
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	if (ThreadInformationClass == ThreadPowerThrottling) {
		if (ThreadInformationSize < sizeof(THREAD_POWER_THROTTLING_STATE)) {
			Status = STATUS_INVALID_PARAMETER;
			goto Exit;
		}

		ThreadInformationSize = sizeof(THREAD_POWER_THROTTLING_STATE);
		ThrottlingState = *(PTHREAD_POWER_THROTTLING_STATE) ThreadInformation;

		if (ThrottlingState.Version > 1 ||
			(ThrottlingState.ControlMask & 0xFFFFFFFE) != 0 ||
			(~ThrottlingState.ControlMask & ThrottlingState.StateMask) != 0) {

			Status = STATUS_INVALID_PARAMETER;
			goto Exit;
		}

		ThrottlingState.Version = 1;

		ThreadInformation = &ThrottlingState;
	}

	Status = NtSetInformationThread(
		ThreadHandle,
		ThreadInformationClassNt,
		ThreadInformation,
		ThreadInformationSize);

Exit:
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
	}

	return NT_SUCCESS(Status);
}

KXBASEAPI BOOL WINAPI GetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	OUT	PVOID						ThreadInformation,
	IN	ULONG						ThreadInformationSize)
{
	NTSTATUS Status;
	THREADINFOCLASS ThreadInformationClassNt;

	switch (ThreadInformationClass) {
	case ThreadMemoryPriority:
		ThreadInformationClassNt = ThreadPagePriority;
		break;
	case ThreadAbsoluteCpuPriority:
		ThreadInformationClassNt = ThreadActualBasePriority;
		break;
	case ThreadDynamicCodePolicy:
		ThreadInformationClassNt = ThreadDynamicCodePolicyInfo;
		break;
	default:
		Status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	Status = NtQueryInformationThread(
		ThreadHandle,
		ThreadInformationClassNt,
		ThreadInformation,
		ThreadInformationSize,
		NULL);

Exit:
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
	}

	return NT_SUCCESS(Status);
}

KXBASEAPI BOOL WINAPI SetThreadSelectedCpuSets(
	IN	HANDLE	ThreadHandle,
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

KXBASEAPI BOOL WINAPI SetThreadSelectedCpuSetMasks(
	IN	HANDLE			ThreadHandle,
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

KXBASEAPI BOOL WINAPI GetThreadSelectedCpuSets(
	IN	HANDLE	ThreadHandle,
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

KXBASEAPI BOOL WINAPI GetThreadSelectedCpuSetMasks(
	IN	HANDLE			ThreadHandle,
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

KXBASEAPI BOOL WINAPI SetThreadpoolTimerEx(
	IN OUT	PTP_TIMER	pti,
	IN		PFILETIME	pftDueTime OPTIONAL,
	IN		DWORD		msPeriod,
	IN		DWORD		msWindowLength OPTIONAL)
{
	BOOLEAN ThreadpoolTimerSet;

	ThreadpoolTimerSet = IsThreadpoolTimerSet(pti);
	SetThreadpoolTimer(pti, pftDueTime, msPeriod, msWindowLength);
	return ThreadpoolTimerSet;
}

KXBASEAPI BOOL WINAPI GetSystemCpuSetInformation(
	PVOID	Information,
	ULONG	BufferLength,
	PULONG	ReturnedLength,
	HANDLE	Process,
	ULONG	Flags)
{
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}