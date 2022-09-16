#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

#include "gmem.h"

WINBASEAPI BOOL WINAPI PrefetchVirtualMemory(
	IN	HANDLE						hProcess,
	IN	ULONG_PTR					NumberOfEntries,
	IN	PWIN32_MEMORY_RANGE_ENTRY	VirtualAddresses,
	IN	ULONG						Flags)
{
	ODS_ENTRY(L"(%p, %p, %p, %I32u)", hProcess, NumberOfEntries, VirtualAddresses, Flags);

	// The underlying implementation of this function in Windows 8+ uses the NTAPI
	// function "NtSetInformationVirtualMemory", which is available starting from
	// Windows 8 and documented (in its Zw- form) starting in Windows 10.
	//
	// From the MSDN docs:
	// Since the PrefetchVirtualMemory function can never be necessary for correct operation
	// of applications, it is treated as a strong hint by the system and is subject to usual
	// physical memory constraints where it can completely or partially fail under
	// low-memory conditions.
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}