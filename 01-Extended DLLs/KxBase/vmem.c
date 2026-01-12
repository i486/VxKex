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

KXBASEAPI HANDLE WINAPI CreateFileMappingFromApp(
	IN	HANDLE					FileHandle,
	IN	LPSECURITY_ATTRIBUTES	SecurityAttributes,
	IN	ULONG					PageProtection,
	IN	ULONGLONG				MaximumSize,
	IN	PCWSTR					Name OPTIONAL)
{
	return CreateFileMappingW(
		FileHandle,
		SecurityAttributes,
		PageProtection,
		HIDWORD(MaximumSize),
		LODWORD(MaximumSize),
		Name);
}

KXBASEAPI PVOID WINAPI MapViewOfFileFromApp(
	IN	HANDLE		SectionHandle,
	IN	ULONG		DesiredAccess,
	IN	ULONGLONG	FileOffset,
	IN	SIZE_T		FileMappingSize)
{
	if ((DesiredAccess & FILE_MAP_EXECUTE) &&
		(DesiredAccess & (FILE_MAP_COPY | FILE_MAP_WRITE))) {

		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	return MapViewOfFile(
		SectionHandle,
		DesiredAccess,
		HIDWORD(FileOffset),
		LODWORD(FileOffset),
		FileMappingSize);
}

KXBASEAPI PVOID WINAPI VirtualAllocFromApp(
	IN	PVOID	Address OPTIONAL,
	IN	SIZE_T	Size,
	IN	ULONG	AllocationType,
	IN	ULONG	PageProtection)
{
	if (PageProtection & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	return VirtualAlloc(Address, Size, AllocationType, PageProtection);
}

KXBASEAPI BOOL WINAPI VirtualProtectFromApp(
	IN	PVOID	Address,
	IN	SIZE_T	Size,
	IN	ULONG	NewPageProtection,
	OUT	PULONG	OldProtect)
{
	return VirtualProtect(Address, Size, NewPageProtection, OldProtect);
}

KXBASEAPI HANDLE WINAPI OpenFileMappingFromApp(
	IN	ULONG	DesiredAccess,
	IN	BOOL	InheritableHandle,
	IN	PCWSTR	Name)
{
	// Limit boolean value to TRUE and FALSE only.
	unless (InheritableHandle == TRUE || InheritableHandle == FALSE) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	
	return OpenFileMapping(DesiredAccess, InheritableHandle, Name);
}

KXBASEAPI PVOID WINAPI MapViewOfFileNuma2(
	IN	HANDLE		SectionHandle,
	IN	HANDLE		ProcessHandle,
	IN	ULONGLONG	Offset,
	IN	PVOID		BaseAddress OPTIONAL,
	IN	SIZE_T		ViewSize,
	IN	ULONG		AllocationType,
	IN	ULONG		PageProtection,
	IN	ULONG		PreferredNumaNode)
{
	NTSTATUS Status;

	if (AllocationType & ~(MEM_LARGE_PAGES | MEM_TOP_DOWN | MEM_RESERVE | MEM_ROTATE)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (PreferredNumaNode != NUMA_NO_PREFERRED_NODE && PreferredNumaNode >= 64) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	Status = NtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		&BaseAddress,
		0,
		0,
		(PLONGLONG) &Offset,
		&ViewSize,
		ViewShare,
		AllocationType | (PreferredNumaNode + 1),
		PageProtection);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return NULL;
	}

	return BaseAddress;
}

KXBASEAPI BOOL WINAPI UnmapViewOfFile2(
	IN	HANDLE	ProcessHandle,
	IN	PVOID	BaseAddress,
	IN	ULONG	UnmapFlags)
{
	NTSTATUS Status;

	if (UnmapFlags) {
		KexLogWarningEvent(L"UnmapViewOfFile2 called with flags: 0x%08lx", UnmapFlags);

		if (UnmapFlags & ~MEM_UNMAP_VALID_FLAGS) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return FALSE;
		}
	}

	Status = NtUnmapViewOfSection(ProcessHandle, BaseAddress);
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}

KXBASEAPI BOOL WINAPI UnmapViewOfFileEx(
	IN	PVOID	BaseAddress,
	IN	ULONG	UnmapFlags)
{
	return UnmapViewOfFile2(NtCurrentProcess(), BaseAddress, UnmapFlags);
}

KXBASEAPI PVOID WINAPI MapViewOfFile3(
	IN		HANDLE					SectionHandle,
	IN		HANDLE					ProcessHandle OPTIONAL,
	IN		PVOID					BaseAddress OPTIONAL,
	IN		ULONGLONG				Offset,
	IN		SIZE_T					ViewSize,
	IN		ULONG					AllocationType,
	IN		ULONG					PageProtection,
	IN OUT	MEM_EXTENDED_PARAMETER	*ExtendedParameters OPTIONAL,
	IN		ULONG					ParameterCount)
{
	NTSTATUS Status;
	ULONG PreferredNumaNode;

	PreferredNumaNode = NUMA_NO_PREFERRED_NODE;

	if (AllocationType & ~(MEM_REPLACE_PLACEHOLDER | MEM_LARGE_PAGES | MEM_TOP_DOWN | MEM_RESERVE | MEM_ROTATE)) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	if (ExtendedParameters) {
		ULONG Index;

		for (Index = 0; Index < ParameterCount; ++Index) {
			switch (ExtendedParameters[Index].Type) {
			case MemExtendedParameterAddressRequirements:
				// TODO: Unimplemented. It's possible to do this but I couldn't
				// be bothered at the moment especially since the implementation
				// for this will be moved to kernel mode later on anyway.
				KexLogWarningEvent(L"Address requirements were specified to MapViewOfFile3");
				BaseSetLastNTError(STATUS_INVALID_PARAMETER);
				return NULL;
			case MemExtendedParameterNumaNode:
				PreferredNumaNode = ExtendedParameters[Index].ULong;
				break;
			default:
				BaseSetLastNTError(STATUS_INVALID_PARAMETER);
				return NULL;
			}
		}
	}

	if (ProcessHandle == NULL) {
		ProcessHandle = NtCurrentProcess();
	}

	Status = NtMapViewOfSection(
		SectionHandle,
		ProcessHandle,
		&BaseAddress,
		0,
		0,
		(PLONGLONG) &Offset,
		&ViewSize,
		ViewShare,
		AllocationType | (PreferredNumaNode + 1),
		PageProtection);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return NULL;
	}

	return BaseAddress;
}

KXBASEAPI PVOID WINAPI MapViewOfFile3FromApp(
	IN		HANDLE					SectionHandle,
	IN		HANDLE					ProcessHandle OPTIONAL,
	IN		PVOID					BaseAddress OPTIONAL,
	IN		ULONGLONG				Offset,
	IN		SIZE_T					ViewSize,
	IN		ULONG					AllocationType,
	IN		ULONG					PageProtection,
	IN OUT	MEM_EXTENDED_PARAMETER	*ExtendedParameters OPTIONAL,
	IN		ULONG					ParameterCount)
{
	if (PageProtection & (PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE_READWRITE)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	if (BaseAddress != NULL && (AllocationType & MEM_REPLACE_PLACEHOLDER)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return NULL;
	}

	return MapViewOfFile3(
		SectionHandle,
		ProcessHandle,
		BaseAddress,
		Offset,
		ViewSize,
		AllocationType,
		PageProtection,
		ExtendedParameters,
		ParameterCount);
}