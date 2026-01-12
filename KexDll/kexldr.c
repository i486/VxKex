///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexldr.c
//
// Abstract:
//
//     Functions for dealing with PE images and the loader subsystem.
//
// Author:
//
//     vxiiduu (06-Nov-2022)
//
// Revision History:
//
//     vxiiduu              06-Nov-2022  Initial creation.
//     vxiiduu              06-Nov-2022  Rework KexLdrGetDllFullNameFromAddress
//     vxiiduu              22-Feb-2024  Add some asserts.
//     vxiiduu              27-Feb-2024  Improve efficiency of routines which
//                                       find NTDLL base addresses.
//     vxiiduu              29-Feb-2024  Revert previous change (wrong assumption).
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Compatible with Win8.
// In Windows 8, this function forms the basis of GetModuleFileName,
// whereas in Win7 and before, the scanning of the loader data table
// is done directly in kernelbase.dll (or kernel32.dll).
//
NTSTATUS NTAPI KexLdrGetDllFullName(
	IN	PVOID			DllBase OPTIONAL,
	OUT	PUNICODE_STRING	DllFullPath)
{
	PUNICODE_STRING FullPath;

	if (DllBase) {
		PLDR_DATA_TABLE_ENTRY Entry;

		if (!LdrpFindLoadedDllByHandle(DllBase, &Entry)) {
			return STATUS_DLL_NOT_FOUND;
		}

		FullPath = &Entry->FullDllName;
	} else {
		FullPath = &NtCurrentPeb()->ProcessParameters->ImagePathName;
	}

	if (FullPath) {
		RtlCopyUnicodeString(DllFullPath, FullPath);
	} else {
		DllFullPath->Length = 0;
	}

	if (FullPath->Length > DllFullPath->MaximumLength) {
		// should technically be STATUS_BUFFER_OVERFLOW because at this
		// point we've already written the data, but go complain to the
		// win8 devs.
		return STATUS_BUFFER_TOO_SMALL;
	}

	return STATUS_SUCCESS;
}

//
// This function matches for any address inside the DLL instead of
// only its base address. It is therefore less efficient than using
// KexLdrGetDllFullName, but required for e.g. figuring out which
// DLL a function call comes from.
//
NTSTATUS NTAPI KexLdrGetDllFullNameFromAddress(
	IN	PVOID			Address,
	OUT	PUNICODE_STRING	DllFullPath)
{
	NTSTATUS Status;
	PLDR_DATA_TABLE_ENTRY Entry;

	if (!Address || !DllFullPath) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = LdrFindEntryForAddress(Address, &Entry);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (Entry->FullDllName.Length > DllFullPath->MaximumLength) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlCopyUnicodeString(DllFullPath, &Entry->FullDllName);
	return STATUS_SUCCESS;
}

//
// Find location of a DLL's init routine (i.e. DllMain).
//
NTSTATUS NTAPI KexLdrFindDllInitRoutine(
	IN	PVOID	DllBase,
	OUT	PPVOID	InitRoutine)
{
	PIMAGE_NT_HEADERS NtHeaders;

	ASSERT (DllBase != NULL);
	ASSERT (InitRoutine != NULL);

	if (!DllBase || !InitRoutine) {
		return STATUS_INVALID_PARAMETER;
	}

	*InitRoutine = NULL;

	NtHeaders = RtlImageNtHeader(DllBase);
	ASSERT (NtHeaders != NULL);

	if (!NtHeaders) {
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (NtHeaders->OptionalHeader.AddressOfEntryPoint == 0) {
		return STATUS_ENTRYPOINT_NOT_FOUND;
	}

	*InitRoutine = RVA_TO_VA(DllBase, NtHeaders->OptionalHeader.AddressOfEntryPoint);
	return STATUS_SUCCESS;
}

//
// Main reason for using this is to:
//   - get proc address in DLLs mapped but not registered with loader
//   - get proc address in "wrong" bitness dlls (e.g. native ntdll.dll from
//     wow64 process or vice versa)
//
// However, for DLLs registered with loader, this function is not useful
// as it is far slower than LdrGetProcedureAddress.
//
NTSTATUS NTAPI KexLdrMiniGetProcedureAddress(
	IN	PVOID	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PPVOID	ProcedureAddress)
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	ULONG ExportDirectorySize;
	PULONG NameRvas;
	PULONG FunctionRvas;
	PUSHORT NameOrdinals;
	ULONG Index;

	ASSERT (DllBase != NULL);
	ASSERT (ProcedureName != NULL);
	ASSERT (ProcedureAddress != NULL);

	if (!DllBase || !ProcedureName || !ProcedureAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	*ProcedureAddress = NULL;

	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY) RtlImageDirectoryEntryToData(
		DllBase, 
		TRUE, 
		IMAGE_DIRECTORY_ENTRY_EXPORT, 
		&ExportDirectorySize);

	ASSERT (ExportDirectory != NULL);

	if (!ExportDirectory) {
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	NameRvas = (PULONG) RVA_TO_VA(DllBase, ExportDirectory->AddressOfNames);
	FunctionRvas = (PULONG) RVA_TO_VA(DllBase, ExportDirectory->AddressOfFunctions);
	NameOrdinals = (PUSHORT) RVA_TO_VA(DllBase, ExportDirectory->AddressOfNameOrdinals);

	for (Index = 0; Index < ExportDirectory->NumberOfNames; ++Index) {
		PCSTR CurrentProcedureName;

		CurrentProcedureName = (PCSTR) RVA_TO_VA(DllBase, NameRvas[Index]);

		if (StringEqualA(ProcedureName, CurrentProcedureName)) {
			*ProcedureAddress = RVA_TO_VA(DllBase, FunctionRvas[NameOrdinals[Index]]);
			return STATUS_SUCCESS;
		}
	}

	return STATUS_ENTRYPOINT_NOT_FOUND;
}

//
// Get the base address of NTDLL.
//
KEXAPI PVOID NTAPI KexLdrGetSystemDllBase(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllBaseName;
	PVOID NtdllBaseAddress;

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");

	Status = LdrGetDllHandleByName(&NtdllBaseName, NULL, &NtdllBaseAddress);
	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status)) {
		return NtdllBaseAddress;
	} else {
		return NULL;
	}
}

//
// Get the base address of NTDLL for another process.
// This function returns the address of 32-bit NTDLL for a
// 32-bit process, and 64-bit NTDLL for a 64-bit process.
//
KEXAPI PVOID NTAPI KexLdrGetRemoteSystemDllBase(
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllPathFragment;
	UNICODE_STRING NtdllBaseName;
	ULONG_PTR NtdllBaseAddress;
	PUNICODE_STRING MappedFileNameInformation;
	ULONG MappedFileNameLength;
	ULONG RemoteProcessBitness;

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");
	
	RemoteProcessBitness = KexRtlRemoteProcessBitness(ProcessHandle);
	
	//
	// We can avoid scanning for NTDLL if we are the same bitness as the
	// remote process, or if we are a WOW64 process
	//

	if (KexRtlCurrentProcessBitness() == RemoteProcessBitness) {
		ASSERT (KexData->SystemDllBase != NULL);
		
		return KexData->SystemDllBase;
	} else if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) {
		ASSERT (KexRtlCurrentProcessBitness() == 32);
		ASSERT (RemoteProcessBitness == 64);
		ASSERT (KexData->NativeSystemDllBase != NULL);

		return KexData->NativeSystemDllBase;
	}

	//
	// We must search for NTDLL if we are a 64 bit process looking for the
	// WOW64 NTDLL.
	//

	ASSUME (KexRtlCurrentProcessBitness() == 64);
	ASSUME (KexRtlOperatingSystemBitness() == 64);
	ASSUME (RemoteProcessBitness == 32);

	MappedFileNameLength = 256;
	MappedFileNameInformation = (PUNICODE_STRING) StackAlloc(BYTE, MappedFileNameLength);

	RtlInitConstantUnicodeString(&NtdllPathFragment, L"syswow64\\ntdll.dll");

	for (NtdllBaseAddress = 0x7FFD0000; NtdllBaseAddress >= 0x70000000; NtdllBaseAddress -= 0x10000) {
		MEMORY_BASIC_INFORMATION BasicInformation;

		Status = NtQueryVirtualMemory(
			ProcessHandle,
			(PVOID) NtdllBaseAddress,
			MemoryMappedFilenameInformation,
			MappedFileNameInformation,
			MappedFileNameLength,
			NULL);

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		KexLogDebugEvent(
			L"Found mapped file in remote process: %wZ",
			MappedFileNameInformation);

		//
		// Check if this memory-mapped image is NTDLL.
		//

		if (!KexRtlUnicodeStringEndsWith(MappedFileNameInformation, &NtdllPathFragment, TRUE)) {
			continue;
		}

		//
		// We will now confirm that this file is an image, and find
		// the base address of this image file.
		//

		Status = NtQueryVirtualMemory(
			ProcessHandle,
			(PVOID) NtdllBaseAddress,
			MemoryBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		if (BasicInformation.Type != MEM_IMAGE) {
			continue;
		}

		NtdllBaseAddress = (ULONG_PTR) BasicInformation.AllocationBase;

		return (PVOID) NtdllBaseAddress;
	}

	//
	// Could not find.
	//

	ASSERT (FALSE);
	return NULL;
}

//
// Get the base address of the native NTDLL. In other words:
// if this is a 32-bit process running on a 64-bit operating
// system, get the 64-bit NTDLL, and so on.
//
KEXAPI PVOID NTAPI KexLdrGetNativeSystemDllBase(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING NtdllPathFragment;
	UNICODE_STRING NtdllBaseName;
	ULONG_PTR NtdllBaseAddress;
	PUNICODE_STRING MappedFileNameInformation;
	ULONG MappedFileNameLength;

	//
	// 64-bit NTDLL is mapped between 0x7FFD0000 and 0x70000000 on
	// boundaries of 0x10000 (due to ASLR). This gives us 256
	// possibilities we need to search for. To avoid this penalty,
	// we will avoid performing the search if we could just get
	// NTDLL's base address from the loader subsystem.
	//

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		Status = LdrGetDllHandleByName(&NtdllBaseName, NULL, (PPVOID) &NtdllBaseAddress);
		ASSERT (NT_SUCCESS(Status));
		return (PVOID) NtdllBaseAddress;
	}

	ASSUME (KexRtlCurrentProcessBitness() == 32);
	ASSUME (KexRtlOperatingSystemBitness() == 64);

	//
	// This is a 32-bit process running on a 64-bit operating system. We must
	// search for the 64-bit NTDLL as described.
	//

	MappedFileNameLength = 256;
	MappedFileNameInformation = (PUNICODE_STRING) StackAlloc(BYTE, MappedFileNameLength);
	RtlInitConstantUnicodeString(&NtdllPathFragment, L"system32\\ntdll.dll");

	for (NtdllBaseAddress = 0x7FFD0000; NtdllBaseAddress >= 0x70000000; NtdllBaseAddress -= 0x10000) {
		MEMORY_BASIC_INFORMATION BasicInformation;

		Status = NtQueryVirtualMemory(
			NtCurrentProcess(),
			(PVOID) NtdllBaseAddress,
			MemoryMappedFilenameInformation,
			MappedFileNameInformation,
			MappedFileNameLength,
			NULL);

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		//
		// Confirm that this memory-mapped image is in fact the native
		// NTDLL inside the system32 directory.
		//

		if (!KexRtlUnicodeStringEndsWith(MappedFileNameInformation, &NtdllPathFragment, TRUE)) {
			continue;
		}

		//
		// We now have a pointer to a memory mapped file.
		// We will now determine whether this file is an image, and also
		// the base address of this image file.
		//

		Status = NtQueryVirtualMemory(
			NtCurrentProcess(),
			(PVOID) NtdllBaseAddress,
			MemoryBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		if (BasicInformation.Type != MEM_IMAGE) {
			continue;
		}

		NtdllBaseAddress = (ULONG_PTR) BasicInformation.AllocationBase;
		return (PVOID) NtdllBaseAddress;
	}

	//
	// Could not find.
	//

	ASSERT (FALSE);
	return NULL;
}

//
// This function changes the page protections on the entire section which
// contains the import directory.
//

KEXAPI NTSTATUS NTAPI KexLdrProtectImageImportSection(
	IN	PVOID	ImageBase,
	IN	ULONG	PageProtection,
	OUT	PULONG	OldProtection)
{
	NTSTATUS Status;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_FILE_HEADER CoffHeader;
	PIMAGE_OPTIONAL_HEADER OptionalHeader;
	PIMAGE_DATA_DIRECTORY ImportDirectory;
	PIMAGE_SECTION_HEADER ImportSectionHeader;
	PVOID ImportSectionAddress;
	SIZE_T ImportSectionSize;

	ASSERT (ImageBase != NULL);
	ASSERT (OldProtection != NULL);

	Status = RtlImageNtHeaderEx(
		RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
		ImageBase,
		0,
		&NtHeaders);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to retrieve the address of the image NT headers for the "
			L"image at base: 0x%p\r\n\r\n"
			L"NTSTATUS error code: %s",
			ImageBase,
			KexRtlNtStatusToString(Status));

		return Status;
	}

	CoffHeader = &NtHeaders->FileHeader;
	OptionalHeader = &NtHeaders->OptionalHeader;
	ImportDirectory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	//
	// Find the section that contains the import directory.
	//
	
	ImportSectionHeader = KexRtlSectionTableFromRva(
		NtHeaders,
		ImportDirectory->VirtualAddress);

	if (!ImportSectionHeader) {
		return STATUS_IMAGE_SECTION_NOT_FOUND;
	}

	ImportSectionAddress = RVA_TO_VA(ImageBase, ImportSectionHeader->VirtualAddress);
	ImportSectionSize = ImportSectionHeader->Misc.VirtualSize;

	ASSERT (ImportSectionAddress != ImageBase);
	ASSERT (ImportSectionSize != 0);

	//
	// Set our page protections.
	//

	Status = NtProtectVirtualMemory(
		NtCurrentProcess(),
		&ImportSectionAddress,
		&ImportSectionSize,
		PageProtection,
		OldProtection);

	ASSERT (NT_SUCCESS(Status));

	return Status;
}