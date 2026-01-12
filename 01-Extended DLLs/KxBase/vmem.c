#include "buildcfg.h"
#include "kxbasep.h"

STATIC ULONG OfferVirtualMemoryInternal(
	IN	PVOID			VirtualAddress,
	IN	SIZE_T			Size,
	IN	OFFER_PRIORITY	Priority,
	IN	BOOL			DiscardMemory)
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION BasicInformation;
	PVOID VirtualAllocResult;
	ULONG OldProtect;

	//
	// Parameter validation.
	//

	if (!VirtualAddress || !Size) {
		return ERROR_INVALID_PARAMETER;
	}

	if ((ULONG_PTR) VirtualAddress & 0xFFF) {
		// The virtual address must be page-aligned.
		return ERROR_INVALID_PARAMETER;
	}

	if (Size & 0xFFF) {
		// The size must be a multiple of the page size.
		return ERROR_INVALID_PARAMETER;
	}

	ASSERT (Priority && Priority < VMOfferPriorityMaximum);

	//
	// Check to see if the memory region provided is valid.
	// The entire region must be readable, writable, and committed.
	//

	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		VirtualAddress,
		MemoryBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		return RtlNtStatusToDosError(Status);
	}

	if (BasicInformation.RegionSize < Size ||
		BasicInformation.Protect != PAGE_READWRITE ||
		BasicInformation.State != MEM_COMMIT) {

		Status = STATUS_INVALID_PAGE_PROTECTION;
		return RtlNtStatusToDosError(Status);
	}

	//
	// Tell the kernel that we won't be needing the contents of this memory
	// anymore.
	//

	VirtualAllocResult = VirtualAlloc(
		VirtualAddress,
		Size,
		MEM_RESET,
		PAGE_READWRITE);

	if (VirtualAllocResult != VirtualAddress) {
		return GetLastError();
	}

	if (DiscardMemory) {
		VirtualUnlock(VirtualAddress, Size);
	} else {
		// If OfferVirtualMemory was called, then make those pages
		// inaccessible.
		VirtualProtect(VirtualAddress, Size, PAGE_NOACCESS, &OldProtect);
	}

	return ERROR_SUCCESS;
}

KXBASEAPI ULONG WINAPI OfferVirtualMemory(
	IN	PVOID			VirtualAddress,
	IN	SIZE_T			Size,
	IN	OFFER_PRIORITY	Priority)
{
	KexLogDebugEvent(
		L"OfferVirtualMemory called\r\n\r\n"
		L"VirtualAddress: 0x%p\r\n"
		L"Size:           %lu",
		VirtualAddress,
		Size);

	if (!Priority || Priority >= VMOfferPriorityMaximum) {
		return ERROR_INVALID_PARAMETER;
	}

	return OfferVirtualMemoryInternal(VirtualAddress, Size, Priority, FALSE);
}

KXBASEAPI ULONG WINAPI DiscardVirtualMemory(
	IN	PVOID	VirtualAddress,
	IN	SIZE_T	Size)
{
	KexLogDebugEvent(
		L"DiscardVirtualMemory called\r\n\r\n"
		L"VirtualAddress: 0x%p\r\n"
		L"Size:           %lu",
		VirtualAddress,
		Size);

	return OfferVirtualMemoryInternal(VirtualAddress, Size, VMOfferPriorityVeryLow, TRUE);
}

KXBASEAPI ULONG WINAPI ReclaimVirtualMemory(
	IN	PVOID	VirtualAddress,
	IN	SIZE_T	Size)
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION BasicInformation;
	ULONG OldProtect;

	KexLogDebugEvent(
		L"ReclaimVirtualMemory called\r\n\r\n"
		L"VirtualAddress: 0x%p\r\n"
		L"Size:           %lu",
		VirtualAddress,
		Size);

	if (!VirtualAddress || !Size) {
		return ERROR_INVALID_PARAMETER;
	}

	if ((ULONG_PTR) VirtualAddress & 0xFFF) {
		// The virtual address must be page-aligned.
		return ERROR_INVALID_PARAMETER;
	}

	if (Size & 0xFFF) {
		// The size must be a multiple of the page size.
		return ERROR_INVALID_PARAMETER;
	}

	//
	// Check to see whether the memory region provided is valid.
	// ReclaimVirtualMemory is only intended to be called as a counterpart
	// to OfferVirtualMemory, so ensure the page protections and memory
	// status are consistent with the intended usage.
	//

	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		VirtualAddress,
		MemoryBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		return RtlNtStatusToDosError(Status);
	}

	if (BasicInformation.RegionSize < Size ||
		BasicInformation.Protect != PAGE_NOACCESS ||
		BasicInformation.State != MEM_COMMIT) {

		Status = STATUS_INVALID_PAGE_PROTECTION;
		return RtlNtStatusToDosError(Status);
	}

	//
	// Make the memory region accessible again.
	//

	VirtualProtect(VirtualAddress, Size, PAGE_READWRITE, &OldProtect);

	//
	// Since Windows 7 does not have the MEM_RESET_UNDO functionality, we
	// will always return ERROR_BUSY here.
	//
	// In the future, if we find an application that responds poorly to this,
	// we can add an app-specific hack that makes OfferVirtualMemory and
	// ReclaimVirtualMemory a no-op for that application.
	//

	return ERROR_BUSY;
}

KXBASEAPI BOOL WINAPI PrefetchVirtualMemory(
	IN	HANDLE						ProcessHandle,
	IN	ULONG_PTR					NumberOfEntries,
	IN	PWIN32_MEMORY_RANGE_ENTRY	VirtualAddresses,
	IN	ULONG						Flags)
{
	//
	// This function is a no-op.
	//

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return FALSE;
}