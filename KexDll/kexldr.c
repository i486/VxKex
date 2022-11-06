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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Compatible with Win8.
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

NTSTATUS NTAPI KexLdrGetDllFullPathFromAddress(
	IN	PVOID			Address,
	OUT	PUNICODE_STRING	DllFullPath) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION BasicInformation;

	if (!Address || !DllFullPath) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		Address,
		MemoryBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	return KexLdrGetDllFullName(
		BasicInformation.AllocationBase,
		DllFullPath);
} PROTECTED_FUNCTION_END

//
// Find location of a DLL's init routine (i.e. DllMain).
//
NTSTATUS NTAPI KexLdrFindDllInitRoutine(
	IN	PVOID	DllBase,
	OUT	PPVOID	InitRoutine) PROTECTED_FUNCTION
{
	PIMAGE_NT_HEADERS NtHeaders;

	if (!DllBase || !InitRoutine) {
		return STATUS_INVALID_PARAMETER;
	}

	*InitRoutine = NULL;

	NtHeaders = RtlImageNtHeader(DllBase);

	if (!NtHeaders) {
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (NtHeaders->OptionalHeader.AddressOfEntryPoint == 0) {
		return STATUS_ENTRYPOINT_NOT_FOUND;
	}

	*InitRoutine = RVA_TO_VA(DllBase, NtHeaders->OptionalHeader.AddressOfEntryPoint);
	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

//
// Main reason for using this is to:
//   - get proc address in DLLs mapped but not registered with loader
//   - get proc address in "wrong" bitness dlls (e.g. native ntdll.dll from wow64 process)
//
NTSTATUS NTAPI KexLdrMiniGetProcedureAddress(
	IN	PVOID	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PPVOID	ProcedureAddress) PROTECTED_FUNCTION
{
	PIMAGE_EXPORT_DIRECTORY ExportDirectory;
	ULONG ExportDirectorySize;
	PULONG NameRvas;
	PULONG FunctionRvas;
	PUSHORT NameOrdinals;
	ULONG Index;

	if (!DllBase || !ProcedureName || !ProcedureAddress) {
		return STATUS_INVALID_PARAMETER;
	}

	*ProcedureAddress = NULL;

	ExportDirectory = (PIMAGE_EXPORT_DIRECTORY) RtlImageDirectoryEntryToData(
		DllBase, 
		TRUE, 
		IMAGE_DIRECTORY_ENTRY_EXPORT, 
		&ExportDirectorySize);

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
} PROTECTED_FUNCTION_END

//
// Get the base address of the native NTDLL. In other words:
// if this is a 32-bit process running on a 64-bit operating
// system, get the 64-bit NTDLL, and so on.
//
PVOID NTAPI KexLdrGetNativeSystemDllBase(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	UNICODE_STRING NtdllPathFragment;
	UNICODE_STRING NtdllBaseName;
	ULONG_PTR NtdllBaseAddress;
	PUNICODE_STRING MappedFileNameInformation;
	ULONG MappedFileNameLength;

	RtlInitConstantUnicodeString(&NtdllBaseName, L"ntdll.dll");

	//
	// NTDLL is mapped between 0x7F000000 and 0x7FFF0000 on
	// boundaries of 0x10000 (due to ASLR). This gives us 256
	// possibilities we need to search for. To avoid this penalty,
	// we will avoid performing the search if we could just get
	// NTDLL's base address from the loader subsystem.
	//

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		Status = LdrGetDllHandleByName(&NtdllBaseName, NULL, (PPVOID) &NtdllBaseAddress);

		if (NT_SUCCESS(Status)) {
			return (PVOID) NtdllBaseAddress;
		}
	}

	//
	// Either the loader call failed or this is a 32-bit process running
	// on a 64-bit operating system. We must search for NTDLL as described.
	//

	MappedFileNameLength = 512;
	MappedFileNameInformation = (PUNICODE_STRING) StackAlloc(BYTE, 512);
	RtlInitConstantUnicodeString(&NtdllPathFragment, L"Windows\\system32\\ntdll.dll");

	for (NtdllBaseAddress = 0x7FFD0000; NtdllBaseAddress > 0x70000000; NtdllBaseAddress -= 0x10000) {
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

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		NtdllBaseAddress = (ULONG_PTR) BasicInformation.AllocationBase;

		if (BasicInformation.Type != MEM_IMAGE) {
			continue;
		}

		//
		// Confirm that this memory-mapped image is in fact the native
		// NTDLL inside the system32 directory.
		//

		if (KexRtlUnicodeStringEndsWith(MappedFileNameInformation, &NtdllPathFragment, TRUE)) {
			return (PVOID) NtdllBaseAddress;
		}
	}

	//
	// Could not find.
	//

	return NULL;
} PROTECTED_FUNCTION_END_BOOLEAN