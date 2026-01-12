///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     synch.c
//
// Abstract:
//
//     Contains functions related to thread synchronization.
//
// Author:
//
//     vxiiduu (11-Feb-2022)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              11-Feb-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

//
// This function is a wrapper around (Kex)RtlWaitOnAddress.
//
KXBASEAPI BOOL WINAPI WaitOnAddress(
	IN	VOLATILE VOID	*Address,
	IN	PVOID			CompareAddress,
	IN	SIZE_T			AddressSize,
	IN	DWORD			Milliseconds OPTIONAL)
{
	NTSTATUS Status;
	PLARGE_INTEGER TimeOutPointer;
	LARGE_INTEGER TimeOut;

	TimeOutPointer = BaseFormatTimeOut(&TimeOut, Milliseconds);

	Status = KexRtlWaitOnAddress(
		Address,
		CompareAddress,
		AddressSize,
		TimeOutPointer);

	BaseSetLastNTError(Status);
	
	if (NT_SUCCESS(Status) && Status != STATUS_TIMEOUT) {
		return TRUE;
	} else {
		return FALSE;
	}
}