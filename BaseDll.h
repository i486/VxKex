#pragma once
#include <NtDll.h>

// This routine converts a NTSTATUS to a Win32 error code and (effectively) SetLastError()'s it.
INLINE DWORD BaseSetLastNTError(
	IN	NTSTATUS	st)
{
	return (NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError(st));
}