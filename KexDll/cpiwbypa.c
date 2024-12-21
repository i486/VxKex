///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     cpiwbypa.c
//
// Abstract:
//
//     This file contains functions which patch CreateProcessInternalW inside
//     kernel32.
//
//     CreateProcessInternalW is the function which causes the famous error
//     message "This is not a valid Win32 application" when trying to launch
//     executables which have a major/minor subsystem version which is too
//     high.
//
//     In order to launch these executables we need to patch the code inside
//     CreateProcessInternalW which is responsible for checking the major and
//     minor subsystem version of executables.
//
//     TODO: Exploit the fact that kernel32 is always at the same base address
//     within the same boot of the system. Perhaps cache the locations of the
//     two patch addresses in an ephemeral registry key. This should be
//     benchmarked as well to see if it's faster than simply scanning kernel32
//     every time.
//
// Author:
//
//     vxiiduu (19-Feb-2024)
//
// Environment:
//
//     Native mode. This routine should not be run while there are other threads
//     in the process, but even if you do so, it should be fine.
//
// Revision History:
//
//     vxiiduu              19-Feb-2024   Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// The version check inside kernel32 is located inside the (exported!) function
// CreateProcessInternalW. The code checks the subsystem major and minor versions
// inside the EXE header and compares them with SharedUserData->NtMajorVersion
// and SharedUserData->NtMinorVersion.
//
// The function within the Windows source code that performs this check is called
// BasepCheckImageVersion. (Trivia: on Windows XP, this function used to be named
// BasepIsImageVersionOk).
//
// Regardless of 32-bit or 64-bit, this code accesses the exact same memory
// locations as a absolute addresses. These two addresses are:
//
//   0x7FFE026C			SharedUserData->NtMajorVersion
//   0x7FFE0270			SharedUserData->NtMinorVersion
//
// Now, you might wonder, what is the relevance of this? Well, as a stroke of luck,
// in all builds of Windows 7 kernel32.dll that are known to me (including debug-
// checked versions, 32-bit versions, and WOW64 versions), simply searching for
// either of these addresses as a 32-bit integer yields only one result, and that
// is the one we must patch.
//
// Somewhat less fortunately is the fact that exactly where this value is inside
// kernel32 varies quite a lot over the various builds due to varying compiler
// optimizations. So what we will have to do is scan the entire .text section
// of the DLL for these two byte patterns and patch them.
//
// And what are we going to patch them to? Well, there is a value inside the
// SharedUserData that is guaranteed to be large enough that no version of Windows
// is ever going to have a large enough version number to fail the check. This
// value is SharedUserData->NumberOfPhysicalPages, which is set at boot time to
// the number of bytes of physical memory available divided by 4096 (the page size
// on x86 and x64). As long as we are booting with more than about 50kb of memory,
// this value is sure to be large enough ;)
//
// The address of SharedUserData->NumberOfPhysicalPages is 0x7FFE02E8.
//

KEXAPI NTSTATUS NTAPI KexPatchCpiwSubsystemVersionCheck(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING Kernel32Name;
	ANSI_STRING CreateProcessInternalWName;
	PVOID Kernel32;
	PVOID CreateProcessInternalW;
	PBYTE ExecutableSection;
	PBYTE EndOfExecutableSection;
	ULONG SizeOfExecutableSection;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER TextSectionHeader;
	BOOLEAN FoundAddressOfNtMajorVersion;
	BOOLEAN FoundAddressOfNtMinorVersion;
	STATIC BOOLEAN AlreadyPatched = FALSE;

	if (AlreadyPatched) {
		return STATUS_ALREADY_INITIALIZED;
	}

	//
	// Get the address of kernel32.dll.
	//

	RtlInitConstantUnicodeString(&Kernel32Name, L"kernel32.dll");

	Status = LdrGetDllHandleByName(
		&Kernel32Name,
		NULL,
		&Kernel32);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (Kernel32 != NULL);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(L"Could not find the address of kernel32.dll.");
		return Status;
	}

	//
	// Get the address of CreateProcessInternalW within kernel32.dll.
	//

	RtlInitConstantAnsiString(&CreateProcessInternalWName, "CreateProcessInternalW");

	Status = LdrGetProcedureAddress(
		Kernel32,
		&CreateProcessInternalWName,
		0,
		&CreateProcessInternalW);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (CreateProcessInternalW != NULL);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(L"Could not find the address of CreateProcessInternalW.");
		return Status;
	}

	//
	// Find which section contains CreateProcessInternalW. We will assume that
	// this section also contains the code which checks the subsystem version (which
	// is a true assumption, considering there is only ever one executable section
	// inside kernel32.dll).
	//

	Status = RtlImageNtHeaderEx(
		RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
		Kernel32,
		0,
		&NtHeaders);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (NtHeaders != NULL);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(L"Could not find the address of IMAGE_NT_HEADERS inside kernel32.");
		return Status;
	}

	TextSectionHeader = KexRtlSectionTableFromRva(
		NtHeaders,
		(ULONG) VA_TO_RVA(Kernel32, CreateProcessInternalW));

	ASSERT (TextSectionHeader != NULL);

	if (TextSectionHeader == NULL) {
		KexLogErrorEvent(L"Could not find the address of the executable section inside kernel32.");
		return STATUS_IMAGE_SECTION_NOT_FOUND;
	}

	//
	// Find the real address and size of the executable section.
	//

	ExecutableSection = (PBYTE) RVA_TO_VA(Kernel32, TextSectionHeader->VirtualAddress);
	SizeOfExecutableSection = TextSectionHeader->SizeOfRawData;
	EndOfExecutableSection = ExecutableSection + SizeOfExecutableSection - sizeof(ULONG);

	//
	// Scan for the addresses of SharedUserData->NtMajorVersion and
	// SharedUserData->NtMinorVersion.
	//

	FoundAddressOfNtMajorVersion = FALSE;
	FoundAddressOfNtMinorVersion = FALSE;
	
	do {

		if (*(PULONG) ExecutableSection == (ULONG) &SharedUserData->NtMajorVersion ||
			*(PULONG) ExecutableSection == (ULONG) &SharedUserData->NtMinorVersion) {

			PVOID BaseAddress;
			SIZE_T RegionSize;
			ULONG OldProtect;

			//
			// We've found the address of either NtMajorVersion or NtMinorVersion.
			//

			if (*(PULONG) ExecutableSection == (ULONG) &SharedUserData->NtMajorVersion) {
				ASSERT (FoundAddressOfNtMajorVersion == FALSE);
				FoundAddressOfNtMajorVersion = TRUE;
			} else if (*(PULONG) ExecutableSection == (ULONG) &SharedUserData->NtMinorVersion) {
				ASSERT (FoundAddressOfNtMinorVersion == FALSE);
				FoundAddressOfNtMinorVersion = TRUE;
			} else {
				NOT_REACHED;
			}

			BaseAddress = ExecutableSection;
			RegionSize = sizeof(ULONG);

			Status = NtProtectVirtualMemory(
				NtCurrentProcess(),
				&BaseAddress,
				&RegionSize,
				PAGE_EXECUTE_READWRITE,
				&OldProtect);

			ASSERT (NT_SUCCESS(Status));

			if (NT_SUCCESS(Status)) {
				//
				// Overwrite the address to that of SharedUserData->NumberOfPhysicalPages.
				//

				*(PULONG) ExecutableSection = (ULONG) &SharedUserData->NumberOfPhysicalPages;

				Status = NtProtectVirtualMemory(
					NtCurrentProcess(),
					&BaseAddress,
					&RegionSize,
					OldProtect,
					&OldProtect);

				ASSERT (NT_SUCCESS(Status));

				if (!NT_SUCCESS(Status)) {
					KexLogWarningEvent(
						L"Failed to restore original page protections.\r\n\r\n"
						L"Address:     0x%p\r\n"
						L"Region size: %lu",
						BaseAddress,
						RegionSize);
				}
			} else {
				KexLogErrorEvent(
					L"Failed to apply PAGE_EXECUTE_READWRITE protection\r\n\r\n"
					L"Address:     0x%p\r\n"
					L"Region size: %lu",
					BaseAddress,
					RegionSize);
			}

			if (FoundAddressOfNtMajorVersion && FoundAddressOfNtMinorVersion) {
				break;
			}
		}

		++ExecutableSection;

	} until (ExecutableSection >= EndOfExecutableSection);

	if (!FoundAddressOfNtMajorVersion || !FoundAddressOfNtMinorVersion) {
		KexLogErrorEvent(L"Could not find the address of one of the patch locations.");
		return STATUS_NOT_FOUND;
	}

	AlreadyPatched = TRUE;

	KexLogInformationEvent(L"Successfully bypassed subsystem version check in CreateProcessInternalW.");
	return STATUS_SUCCESS;
}