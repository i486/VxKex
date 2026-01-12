///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     propagte.c
//
// Abstract:
//
//     Implements functionality required to propagate VxKex to child processes.
//     The basic gist of it is that we will hook NtCreateUserProcess. When
//     our NtCreateUserProcess hook is called, it will create the child process
//     and then install hooks on NtOpenKey/NtOpenKeyEx.
//
//     When the NtOpenKey/NtOpenKeyEx hooks are called, they will rewrite any
//     attempt to access IFEO keys to a virtualized key. This will cause the
//     loader to load KexDll and then the rest of the initialization proceeds
//     as usual.
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//     vxiiduu              05-Nov-2022  Propagation working for 64 bit.
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS
//     vxiiduu              12-Feb-2024  Fix propagation on WOW64
//     vxiiduu              03-Mar-2024  Fix propagation for 32-bit OSes
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC NTSTATUS NTAPI KexpNtCreateUserProcessHook(
    OUT		CONST PHANDLE						ProcessHandle,
    OUT		CONST PHANDLE						ThreadHandle,
    IN		CONST ACCESS_MASK					ProcessDesiredAccess,
    IN		CONST ACCESS_MASK					ThreadDesiredAccess,
    IN		CONST POBJECT_ATTRIBUTES			ProcessObjectAttributes OPTIONAL,
    IN		CONST POBJECT_ATTRIBUTES			ThreadObjectAttributes OPTIONAL,
    IN		CONST ULONG							ProcessFlags,
    IN		CONST ULONG							ThreadFlags,
    IN		CONST PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
    IN OUT	CONST PPS_CREATE_INFO				CreateInfo,
    IN		CONST PPS_ATTRIBUTE_LIST			AttributeList OPTIONAL);

STATIC ULONG_PTR NativeNtOpenKeyRva;
STATIC ULONG_PTR Wow64NtOpenKeyRva;
STATIC NT_WOW64_QUERY_INFORMATION_PROCESS64 NtWow64QueryInformationProcess64;
STATIC NT_WOW64_WRITE_VIRTUAL_MEMORY64 NtWow64WriteVirtualMemory64;

STATIC CONST BYTE KexpNtOpenKeyHook32[] = {
	0xE8, 0x00, 0x00, 0x00, 0x00, 0x58, 0x83, 0xC0, 0x06, 0xEB, 0x43, 0x00, 0x38, 0x00, 0x3A, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x7B, 0x00, 0x56, 0x00, 0x78, 0x00, 0x4B, 0x00, 0x65, 0x00, 0x78, 0x00,
	0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x61, 0x00, 0x67, 0x00, 0x61, 0x00, 0x74, 0x00,
	0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x56, 0x00, 0x69, 0x00, 0x72, 0x00, 0x74, 0x00, 0x75, 0x00,
	0x61, 0x00, 0x6C, 0x00, 0x4B, 0x00, 0x65, 0x00, 0x79, 0x00, 0x7D, 0x00, 0x00, 0x00, 0x80, 0x38,
	0x00, 0x75, 0x2A, 0x64, 0x8B, 0x15, 0x30, 0x00, 0x00, 0x00, 0x8B, 0x52, 0x10, 0x81, 0x62, 0x08,
	0xFF, 0xBF, 0xFF, 0xFF, 0x8B, 0x54, 0x24, 0x0C, 0x8B, 0x4A, 0x04, 0x85, 0xC9, 0x74, 0x0E, 0xFE,
	0x00, 0x8D, 0x48, 0x01, 0x83, 0xC0, 0x09, 0x89, 0x41, 0x04, 0x89, 0x4A, 0x08, 0xB8, 0xB6, 0x00,
	0x00, 0x00, 0xBA, 0x00, 0x03, 0xFE, 0x7F, 0x83, 0x3A, 0x00, 0x74, 0x05, 0xFF, 0x12, 0xC2, 0x0C,
	0x00, 0xB8, 0x0F, 0x00, 0x00, 0x00, 0x31, 0xC9, 0x8D, 0x54, 0x24, 0x04, 0x64, 0xFF, 0x15, 0xC0,
	0x00, 0x00, 0x00, 0x83, 0xC4, 0x04, 0xC2, 0x0C, 0x00
};

STATIC CONST BYTE KexpNtOpenKeyHook64[] = {
	0x80, 0x3D, 0x43, 0x00, 0x00, 0x00, 0x00, 0x75, 0x36, 0x65, 0x48, 0x8B, 0x04, 0x25, 0x60, 0x00,
	0x00, 0x00, 0x48, 0x8B, 0x40, 0x20, 0x81, 0x60, 0x08, 0xFF, 0xBF, 0xFF, 0xFF, 0x49, 0x8B, 0x40,
	0x08, 0x85, 0xC0, 0x74, 0x1A, 0xFE, 0x05, 0x1F, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x05, 0x1E, 0x00,
	0x00, 0x00, 0x48, 0x89, 0x40, 0x08, 0x48, 0x83, 0x40, 0x08, 0x10, 0x49, 0x89, 0x40, 0x10, 0x49,
	0x89, 0xCA, 0xB8, 0x0F, 0x00, 0x00, 0x00, 0x0F, 0x05, 0xC3, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90,
	0x38, 0x00, 0x3A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x7B, 0x00, 0x56, 0x00, 0x78, 0x00, 0x4B, 0x00, 0x65, 0x00, 0x78, 0x00, 0x50, 0x00, 0x72, 0x00,
	0x6F, 0x00, 0x70, 0x00, 0x61, 0x00, 0x67, 0x00, 0x61, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F, 0x00,
	0x6E, 0x00, 0x56, 0x00, 0x69, 0x00, 0x72, 0x00, 0x74, 0x00, 0x75, 0x00, 0x61, 0x00, 0x6C, 0x00,
	0x4B, 0x00, 0x65, 0x00, 0x79, 0x00, 0x7D, 0x00, 0x00, 0x00
};

//
// This function unhooks NtOpenKey/NtOpenKeyEx and unmaps the
// temporary KexDll from the current process.
//
STATIC VOID KexpCleanupPropagationRemains(
	VOID)
{
	NTSTATUS Status;
	PBYTE Function;
	PVOID HookDestination;

	unless (KexData->Flags & KEXDATA_FLAG_PROPAGATED) {
		KexLogDebugEvent(L"Propagation flag not set.");
		return;
	}

	//
	// Inspect the entry points of the native NtOpenKey and see if they
	// look like a hook template that has previously been written.
	//

	Function = (PBYTE) NtOpenKey;
	HookDestination = NULL;

	if (KexRtlCurrentProcessBitness() == 64) {
		//
		// Look for 0xFF 0x25 0x00 0x00 0x00 0x00, followed by 8 byte address
		//

		if (Function[0] == 0xFF && Function[1] == 0x25 && *((PULONG) &Function[2]) == 0) {
			HookDestination = (PVOID) *((PULONGLONG) &Function[6]);
		}
	} else {
		//
		// Look for 0x68, followed by 4 byte address and then 0xC3
		//

		if (Function[0] == 0x68 && Function[5] == 0xC3) {
			HookDestination = (PVOID) *((PULONG) &Function[1]);
		}
	}

	if (!HookDestination) {
		KexLogWarningEvent(L"Propagation flag set, but no propagation remains found.");
		return;
	} else {
		ULONG OldProtect;
		PVOID BaseAddress;
		SIZE_T RegionSize;

		BaseAddress = HookDestination;
		RegionSize = 4096; // page size
		ASSERT (max(sizeof(KexpNtOpenKeyHook32), sizeof(KexpNtOpenKeyHook64)) <= RegionSize);

		Status = NtFreeVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			MEM_RELEASE);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to free memory from NtOpenKey hook procedure.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
		} else {
			KexLogDebugEvent(
				L"Successfully freed memory from NtOpenKey hook procedure.\r\n\r\n"
				L"Base address: 0x%p",
				BaseAddress);
		}

		//
		// Restore original syscalls.
		// TODO: Rearrange this code so that a failure to set memory protection does not
		// crash the process by making it impossible to call NtOpenKey.
		// TODO: Why is region size just set to 15???
		//
			
		BaseAddress = NtOpenKey;
		RegionSize = 15;

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			PAGE_EXECUTE_WRITECOPY,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (KexRtlCurrentProcessBitness() == 64) {
			//
			// We are 64 bit, native
			//

			CONST BYTE SyscallTemplate[] = {
				0x4C, 0x8B, 0xD1,					// mov r10, rcx
				0xB8, 0x0F, 0x00, 0x00, 0x00,		// mov eax, 0x0F
				0x0F, 0x05,							// syscall
				0xC3								// ret
			};

			RtlCopyMemory(NtOpenKey, SyscallTemplate, sizeof(SyscallTemplate));
		} else if (KexRtlOperatingSystemBitness() == 64) {
			//
			// We are 32 bit, WOW64
			//

			CONST BYTE SyscallTemplate[] = {
				0xB8, 0x0F, 0x00, 0x00, 0x00,				// mov eax, 0x0F
				0x33, 0xC9,									// xor ecx, ecx
				0x8D, 0x54, 0x24, 0x04,						// lea edx, [esp+4]
				0x64, 0xFF, 0x15, 0xC0, 0x00, 0x00, 0x00,	// call [fs:0xC0]
				0x83, 0xC4, 0x04,							// add esp, 4
				0xC2, 0x0C, 0x00							// ret 12
			};

			RtlCopyMemory(NtOpenKey, SyscallTemplate, sizeof(SyscallTemplate));
		} else {
			//
			// We are 32 bit, native
			//

			CONST BYTE SyscallTemplate[] = {
				0xB8, 0xB6, 0x00, 0x00, 0x00,		// mov eax, 0xB6
				0xBA, 0x00, 0x03, 0xFE, 0x7F,		// mov edx, 0x7ffe0300
				0xFF, 0x12,							// call [edx]
				0xC2, 0x0C, 0x00					// ret 12
			};
			
			RtlCopyMemory(NtOpenKey, SyscallTemplate, sizeof(SyscallTemplate));
		}

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));
	}
}

KEXAPI NTSTATUS NTAPI KexInitializePropagation(
	VOID)
{
	NTSTATUS Status;
	PPEB Peb;

	NtWow64QueryInformationProcess64 = NULL;
	NtWow64WriteVirtualMemory64 = NULL;
	NativeNtOpenKeyRva = 0;
	Wow64NtOpenKeyRva = 0;

	//
	// Check the SubSystemData pointer. If it's non-null, it points to a
	// KEX_IFEO_PARAMETERS structure inherited from the parent process, and
	// it means we are propagated.
	//

	Peb = NtCurrentPeb();

	if (Peb->SubSystemData) {
		NTSTATUS Status;
		PKEX_IFEO_PARAMETERS InheritedIfeoParameters;
		SIZE_T RegionSize;

		KexData->Flags |= KEXDATA_FLAG_PROPAGATED;
		InheritedIfeoParameters = (PKEX_IFEO_PARAMETERS) Peb->SubSystemData;

		KexLogDebugEvent(
			L"Found Peb->SubSystemData pointer 0x%p",
			Peb->SubSystemData);

		//
		// Do a quick validity check to make sure this isn't garbage data.
		//

		ASSERT ((InheritedIfeoParameters->DisableForChild & ~1) == 0);
		ASSERT ((InheritedIfeoParameters->DisableAppSpecific & ~1) == 0);
		ASSERT ((InheritedIfeoParameters->StrongVersionSpoof & ~KEX_STRONGSPOOF_VALID_MASK) == 0);
		ASSERT (InheritedIfeoParameters->WinVerSpoof < WinVerSpoofMax);

		//
		// Copy the inherited process parameters into KexData if we didn't already
		// have KEX_ options read from IFEO. In other words, if the user specifically
		// configured VxKex options in the shell extension, we won't override them
		// with the propagated values.
		//

		if (!(KexData->Flags & KEXDATA_FLAG_IFEO_OPTIONS_PRESENT)) {
			KexData->IfeoParameters = *InheritedIfeoParameters;
		}

		//
		// De-allocate the inherited structure.
		//

		RegionSize = 0;

		Status = NtFreeVirtualMemory(
			NtCurrentProcess(),
			(PPVOID) &InheritedIfeoParameters,
			&RegionSize,
			MEM_RELEASE);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to free the temporary propagation KEX_IFEO_PARAMETERS.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
		}

		Peb->SubSystemData = NULL;
	} else {
		KexLogDebugEvent(L"Could not find any inherited KEX_IFEO_PARAMETERS.");
	}

	//
	// Remove any NtOpenKey hook that may be there.
	//

	KexpCleanupPropagationRemains();

	//
	// At this point we are done cleaning up.
	// If the user doesn't want us to propagate to child processes we can stop
	// right here.
	//

	if (KexData->IfeoParameters.DisableForChild) {
		KexLogInformationEvent(L"Not enabling propagation due to user preferences.");
		return STATUS_USER_DISABLED;
	}

	//
	// If we are a WOW64 process, then we need to calculate the RVAs of the
	// NtOpenKey export in the native (i.e. 64-bit) NTDLL.
	//
	// If we are a 64-bit process, then we need to calculate the RVA of
	// NtOpenKey in the WOW64 NTDLL.
	//

	if (KexRtlOperatingSystemBitness() != KexRtlCurrentProcessBitness()) {
		ANSI_STRING NtWow64QueryInformationProcess64Name;
		ANSI_STRING NtWow64WriteVirtualMemory64Name;

		//
		// We are a WOW64 process running in a 64-bit OS.
		// Get the RVA of the NtOpenKey in the native NTDLL.
		//

		Status = KexLdrMiniGetProcedureAddress(
			KexData->NativeSystemDllBase,
			"NtOpenKey",
			(PPVOID) &NativeNtOpenKeyRva);

		ASSERT (NT_SUCCESS(Status));
		ASSERT (NativeNtOpenKeyRva != 0);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		NativeNtOpenKeyRva = VA_TO_RVA(KexData->NativeSystemDllBase, NativeNtOpenKeyRva);
		ASSERT (NativeNtOpenKeyRva != 0);

		//
		// We're a 32 bit process, but we might need to write data to 64-bit processes.
		// Luckily the WOW64 NTDLL contains some useful functions for that.
		//

		RtlInitConstantAnsiString(&NtWow64QueryInformationProcess64Name, "NtWow64QueryInformationProcess64");
		RtlInitConstantAnsiString(&NtWow64WriteVirtualMemory64Name, "NtWow64WriteVirtualMemory64");
		ASSERT (KexData->SystemDllBase != NULL);

		Status = LdrGetProcedureAddress(
			KexData->SystemDllBase,
			&NtWow64QueryInformationProcess64Name,
			0,
			(PPVOID) &NtWow64QueryInformationProcess64);

		ASSERT (NT_SUCCESS(Status));
		ASSERT (NtWow64QueryInformationProcess64 != NULL);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to get procedure address of NtWow64QueryInformationProcess64\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			return Status;
		}

		Status = LdrGetProcedureAddress(
			KexData->SystemDllBase,
			&NtWow64WriteVirtualMemory64Name,
			0,
			(PPVOID) &NtWow64WriteVirtualMemory64);

		ASSERT (NT_SUCCESS(Status));
		ASSERT (NtWow64WriteVirtualMemory64 != NULL);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to get procedure address of NtWow64WriteVirtualMemory64\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			return Status;
		}
	} else if (KexRtlCurrentProcessBitness() == 64) {
		UNICODE_STRING Wow64NtdllPath;
		OBJECT_ATTRIBUTES ObjectAttributes;
		IO_STATUS_BLOCK IoStatusBlock;
		HANDLE Wow64NtdllFileHandle;
		HANDLE Wow64NtdllSectionHandle;
		PVOID Wow64NtdllMappedBase;
		SIZE_T Wow64NtdllMappedSize;

		//
		// We are a 64-bit process.
		// This is actually more of a pain than the other case, because now we need to
		// manually map the WOW64 NTDLL first.
		//

		RtlInitConstantUnicodeString(&Wow64NtdllPath, L"\\SystemRoot\\syswow64\\ntdll.dll");
		InitializeObjectAttributes(&ObjectAttributes, &Wow64NtdllPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

		Status = NtOpenFile(
			&Wow64NtdllFileHandle,
			GENERIC_READ | GENERIC_EXECUTE,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_READ,
			0);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to open WOW64 NTDLL.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		Status = NtCreateSection(
			&Wow64NtdllSectionHandle,
			SECTION_ALL_ACCESS,
			NULL,
			NULL,
			PAGE_EXECUTE,
			SEC_IMAGE,
			Wow64NtdllFileHandle);

		SafeClose(Wow64NtdllFileHandle); // don't need this anymore

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to create section for WOW64 NTDLL.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		Wow64NtdllMappedBase = NULL;
		Wow64NtdllMappedSize = 0;

		Status = NtMapViewOfSection(
			Wow64NtdllSectionHandle,
			NtCurrentProcess(),
			&Wow64NtdllMappedBase,
			0,
			0,
			NULL,
			&Wow64NtdllMappedSize,
			ViewUnmap,
			0,
			PAGE_READWRITE);

		SafeClose(Wow64NtdllSectionHandle);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to map WOW64 NTDLL into current process.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		Status = KexLdrMiniGetProcedureAddress(
			Wow64NtdllMappedBase,
			"NtOpenKey",
			(PPVOID) &Wow64NtOpenKeyRva);

		NtUnmapViewOfSection(NtCurrentProcess(), Wow64NtdllMappedBase);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to find procedure address of NtOpenKey in WOW64 NTDLL.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		Wow64NtOpenKeyRva = VA_TO_RVA(Wow64NtdllMappedBase, Wow64NtOpenKeyRva);
	} else {
		NOT_REACHED;
	}

	ASSERT (NativeNtOpenKeyRva != 0 || Wow64NtOpenKeyRva != 0);

	//
	// Install a permanent hook, because our hook function will directly do
	// a syscall when it wants to call the original function.
	//

	Status = KexHkInstallBasicHook(&NtCreateUserProcess, KexpNtCreateUserProcessHook, NULL);

	if (NT_SUCCESS(Status)) {
		KexLogInformationEvent(L"Successfully initialized propagation system.");
	} else {
		KexLogErrorEvent(
			L"Failed to install hook on NtCreateUserProcess.\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status), Status);
	}

	return Status;
}

STATIC NTSTATUS NTAPI KexpNtCreateUserProcessHook(
    OUT		CONST PHANDLE						ProcessHandle,
    OUT		CONST PHANDLE						ThreadHandle,
    IN		CONST ACCESS_MASK					ProcessDesiredAccess,
    IN		CONST ACCESS_MASK					ThreadDesiredAccess,
    IN		CONST POBJECT_ATTRIBUTES			ProcessObjectAttributes OPTIONAL,
    IN		CONST POBJECT_ATTRIBUTES			ThreadObjectAttributes OPTIONAL,
    IN		CONST ULONG							ProcessFlags,
    IN		CONST ULONG							ThreadFlags,
    IN		CONST PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
    IN OUT	CONST PPS_CREATE_INFO				CreateInfo,
    IN		CONST PPS_ATTRIBUTE_LIST			AttributeList OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	ULONG ModifiedThreadFlags;
	ULONG ModifiedProcessDesiredAccess;
	ULONG ModifiedThreadDesiredAccess;
	
	ULONG ChildProcessBitness;

	PBYTE HookTemplate;
	ULONG HookTemplateCb;
	PVOID RemoteHookBaseAddress;
	SIZE_T RemoteHookSize;
	ULONG_PTR RemoteNtOpenKey;

	PVOID IfeoParametersBaseAddress;
	SIZE_T IfeoParametersSize;

	RemoteNtOpenKey = 0;

	ModifiedProcessDesiredAccess = ProcessDesiredAccess;
	ModifiedThreadDesiredAccess = ThreadDesiredAccess;
	ModifiedThreadFlags = ThreadFlags;

	//
	// 1. We need to be able to write to the new process's memory space
	//    in order to install hooks on NtOpenKey/NtOpenKeyEx.
	//
	// 2. We may need to be able to resume the new thread after creating
	//    it suspended.
	//
	// 3. We need to create the initial thread of the process suspended
	//    so that we can install the hooks and have them called at the
	//    appropriate time.
	//

	ModifiedProcessDesiredAccess |= PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;
	ModifiedThreadDesiredAccess |= THREAD_SUSPEND_RESUME;
	ModifiedThreadFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;
	
	Status = KexNtCreateUserProcess(
		ProcessHandle,
		ThreadHandle,
		ModifiedProcessDesiredAccess,
		ModifiedThreadDesiredAccess,
		ProcessObjectAttributes,
		ThreadObjectAttributes,
		ProcessFlags,
		ModifiedThreadFlags,
		ProcessParameters,
		CreateInfo,
		AttributeList);

	if (!NT_SUCCESS(Status)) {
		NTSTATUS Status2;

		//
		// Perhaps it failed due to the desired access/flags changes.
		// Retry again but with the exact original parameters passed by
		// the caller. Of course, this means that VxKex will not be
		// enabled for the child process.
		//
		Status2 = KexNtCreateUserProcess(
			ProcessHandle,
			ThreadHandle,
			ProcessDesiredAccess,
			ThreadDesiredAccess,
			ProcessObjectAttributes,
			ThreadObjectAttributes,
			ProcessFlags,
			ThreadFlags,
			ProcessParameters,
			CreateInfo,
			AttributeList);

		if (NT_SUCCESS(Status2)) {
			// 2nd call succeeded where the 1st one failed... Investigate this.
			ASSERT (NT_SUCCESS(Status));

			KexLogWarningEvent(
				L"Failed to create user process with modified parameters.\r\n\r\n"
				L"Reversion to system standard behavior caused the call to succeed.\r\n"
				L"NTSTATUS error code for modified call: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
		}

		return Status2;
	}

	//
	// Now we've created the child process suspended, find out whether it
	// is 64 bit.
	//

	ChildProcessBitness = KexRtlRemoteProcessBitness(*ProcessHandle);

	KexLogDebugEvent(
		L"The current process is %d-bit and the remote process is %d-bit.",
		KexRtlCurrentProcessBitness(),
		ChildProcessBitness);

	//
	// Allocate space for the hook procedure inside the child process.
	// The page protections must be set to RWX because the hook procedure
	// blobs contain modifiable data.
	//

	RemoteHookBaseAddress = NULL;

	if (ChildProcessBitness == 64) {
		RemoteHookSize = sizeof(KexpNtOpenKeyHook64);
	} else {
		RemoteHookSize = sizeof(KexpNtOpenKeyHook32);
	}

	Status = NtAllocateVirtualMemory(
		*ProcessHandle,
		&RemoteHookBaseAddress,
		2,
		&RemoteHookSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_EXECUTE_READWRITE);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to allocate virtual memory for NtOpenKey hook procedure in the parent process.\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status), Status);

		goto BailOut;
	}

	//
	// Write the hook procedures into the allocated memory.
	//

	if (ChildProcessBitness == 64) {
		Status = NtWriteVirtualMemory(
			*ProcessHandle,
			RemoteHookBaseAddress,
			KexpNtOpenKeyHook64,
			sizeof(KexpNtOpenKeyHook64),
			NULL);
	} else {
		Status = NtWriteVirtualMemory(
			*ProcessHandle,
			RemoteHookBaseAddress,
			KexpNtOpenKeyHook32,
			sizeof(KexpNtOpenKeyHook32),
			NULL);
	}

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to copy NtOpenKey hook procedure to the parent process.\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status), Status);

		goto BailOut;
	}

	//
	// Find out where the address of NtOpenKey is in the context of the
	// child process.
	//

	if (ChildProcessBitness == KexRtlCurrentProcessBitness()) {
		RemoteNtOpenKey = (ULONG_PTR) NtOpenKey;
	} else {
		PVOID RemoteNtdllBase;

		RemoteNtdllBase = KexLdrGetRemoteSystemDllBase(*ProcessHandle);

		//
		// Make sure the remote NTDLL base was found and looks valid.
		//

		ASSERT (RemoteNtdllBase != NULL);
		ASSERT ((ULONG_PTR) RemoteNtdllBase >= 0x70000000);
		ASSERT ((ULONG_PTR) RemoteNtdllBase <= 0x7FFD0000);

		if (RemoteNtdllBase == NULL ||
			(ULONG_PTR) RemoteNtdllBase < 0x70000000 ||
			(ULONG_PTR) RemoteNtdllBase > 0x7FFD0000) {

			KexLogWarningEvent(L"Failed to get NTDLL address in the child process.");
			goto BailOut;
		}

		KexLogDebugEvent(L"Child process NTDLL address: 0x%p", RemoteNtdllBase);

		if (ChildProcessBitness == 64) {
			// Child is 64 bit, we are 32 bit.
			ASSERT (KexRtlCurrentProcessBitness() == 32);
			ASSERT (NativeNtOpenKeyRva != 0);
			RemoteNtOpenKey = (ULONG_PTR) RVA_TO_VA(RemoteNtdllBase, NativeNtOpenKeyRva);
		} else {
			// Child is 32 bit, we are 64 bit.
			ASSERT (KexRtlCurrentProcessBitness() == 64);
			ASSERT (Wow64NtOpenKeyRva != 0);
			RemoteNtOpenKey = (ULONG_PTR) RVA_TO_VA(RemoteNtdllBase, Wow64NtOpenKeyRva);
		}
	}

	ASSERT (RemoteNtOpenKey != 0);

	//
	// Create hook template.
	//

	if (ChildProcessBitness == 64) {
		HookTemplateCb = 14;
		HookTemplate = StackAlloc(BYTE, HookTemplateCb);
		RtlZeroMemory(HookTemplate, HookTemplateCb);

		// JMP QWORD PTR [rip+0], followed by 64-byte address
		HookTemplate[0] = 0xFF;
		HookTemplate[1] = 0x25;
		*((PULONGLONG) &HookTemplate[6]) = (ULONGLONG) RemoteHookBaseAddress;
	} else {
		HookTemplateCb = 6;
		HookTemplate = StackAlloc(BYTE, HookTemplateCb);
		RtlZeroMemory(HookTemplate, HookTemplateCb);

		// PUSH imm32, followed by RET
		HookTemplate[0] = 0x68;
		HookTemplate[5] = 0xC3;
		*((PULONG) &HookTemplate[1]) = (ULONG) RemoteHookBaseAddress;
	}

	//
	// Write hook into remote process.
	//

	Status = KexRtlWriteProcessMemory(
		*ProcessHandle,
		RemoteNtOpenKey,
		HookTemplate,
		HookTemplateCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to write hook template to remote process.\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status), Status);
		goto BailOut;
	}

	//
	// Pass down the VxKex IFEO parameters into the child process.
	// We will allocate memory for the parameters, copy the data, and then
	// place a pointer into the PEB of the child process.
	//
	// If the child process is a WOW64 process, then we will use the 32-bit
	// PEB. If the child process is a 64-bit process, we will use the 64-bit
	// PEB.
	//

	IfeoParametersBaseAddress = NULL;
	IfeoParametersSize = sizeof(KEX_IFEO_PARAMETERS);

	Status = NtAllocateVirtualMemory(
		*ProcessHandle,
		&IfeoParametersBaseAddress,
		0,
		&IfeoParametersSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to allocate for KEX_IFEO_PARAMETERS in child process.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		goto BailOut;
	}

	Status = NtWriteVirtualMemory(
		*ProcessHandle,
		IfeoParametersBaseAddress,
		&KexData->IfeoParameters,
		sizeof(KexData->IfeoParameters),
		NULL);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to write KEX_IFEO_PARAMETERS into child process.\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			KexRtlNtStatusToString(Status), Status);
		goto BailOut;
	}

	if (KexRtlCurrentProcessBitness() == 32 && ChildProcessBitness == 64) {
		PROCESS_BASIC_INFORMATION64 BasicInformation64;
		PVOID64 IfeoParametersBaseAddress64;
		ULONGLONG RemoteSubSystemData;

		//
		// Get the address of the 64-bit PEB in the child process.
		//

		Status = NtWow64QueryInformationProcess64(
			*ProcessHandle,
			ProcessBasicInformation,
			&BasicInformation64,
			sizeof(BasicInformation64),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to query PEB address of 64-bit process\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			goto BailOut;
		}

		KexLogDebugEvent(
			L"PEB address of 64-bit child process: 0x%I64x",
			BasicInformation64.PebBaseAddress);

		//
		// Write the pointer to the IFEO parameters data into the SubSystemData of the
		// child process's PEB.
		//

		IfeoParametersBaseAddress64 = IfeoParametersBaseAddress;

		//
		// careful with the math here - otherwise things will accidentally get truncated
		// to 32 bit and we will get STATUS_ACCESS_VIOLATION or crash the child process
		// if unlucky.
		//
		// The 0x28 is a hard-coded offset for the 64-bit FIELD_OFFSET(PEB, SubSystemData).
		//

		RemoteSubSystemData = (ULONGLONG) (BasicInformation64.PebBaseAddress);
		RemoteSubSystemData += 0x28;

		Status = NtWow64WriteVirtualMemory64(
			*ProcessHandle,
			(PVOID64) RemoteSubSystemData,
			&IfeoParametersBaseAddress64,
			sizeof(IfeoParametersBaseAddress64),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to write Peb->SubSystemData of 64-bit process\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			goto BailOut;
		}
	} else {
		ULONG_PTR RemotePeb;
		ULONG_PTR RemoteSubSystemData;

		if (ChildProcessBitness == KexRtlOperatingSystemBitness()) {
			PROCESS_BASIC_INFORMATION BasicInformation;

			Status = NtQueryInformationProcess(
				*ProcessHandle,
				ProcessBasicInformation,
				&BasicInformation,
				sizeof(BasicInformation),
				NULL);

			RemotePeb = (ULONG_PTR) BasicInformation.PebBaseAddress;
		} else {
			//
			// Query the location of the WOW64 PEB.
			// NtQueryInformationProcess+ProcessBasicInformation will tell us the location of
			// the 64-bit PEB, which is not what we want.
			//

			Status = NtQueryInformationProcess(
				*ProcessHandle,
				ProcessWow64Information,
				&RemotePeb,
				sizeof(RemotePeb),
				NULL);
		}

		ASSERT (NT_SUCCESS(Status));
		ASSERT (RemotePeb != 0);

		if (ChildProcessBitness == 32) {
			ASSERT (RemotePeb < ULONG_MAX);
		}

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to query PEB address of child process\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			goto BailOut;
		}

		KexLogDebugEvent(L"PEB address of child process: 0x%p", RemotePeb);

		if (ChildProcessBitness == 64) {
			RemoteSubSystemData = RemotePeb + 0x28;
		} else {
			RemoteSubSystemData = RemotePeb + 0x14;
		}

		Status = NtWriteVirtualMemory(
			*ProcessHandle,
			(PVOID) RemoteSubSystemData,
			&IfeoParametersBaseAddress,
			ChildProcessBitness / 8,
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to write Peb->SubSystemData of child process\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);
			goto BailOut;
		}

		if (KexRtlOperatingSystemBitness() == 32) {
			ULONG_PTR RemoteProcessParametersFlags;
			ULONG ProcessParametersFlags;

			ASSUME (KexRtlCurrentProcessBitness() == 32);
			ASSUME (ChildProcessBitness == 32);

			//
			// Clear RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING in the remote
			// Peb->ProcessParameters->Flags.
			//
			// This is necessary because the kernel sets that flag if it doesn't see any IFEO
			// options for the process, and if NTDLL sees the flag, it will not bother checking
			// IFEO, which is bad because we won't successfully propagate.
			//
			// You might wonder, "why can't you just clear that flag from within the NtOpenKey
			// hook?" This method actually works on 64-bit operating systems, and it's a lot
			// easier and faster, but unfortunately on 32-bit operating systems the flag is
			// checked before NtOpenKey is ever called. On 64-bit operating systems (including
			// on WOW64), the flag is checked after the first time NtOpenKey is called, so it's
			// OK.
			//
			// The difference is caused by the fact that 64-bit operating systems check for
			// the DisableUserModeCallbackFilter IFEO value prior to doing the usual GlobalFlag,
			// VerifierDlls etc. checks.
			//
			// For reference, the function responsible for performing this check is called
			// LdrpIsSystemwideUserCallbackExceptionFilterDisabled (it got inlined on some x64
			// builds).
			//

			RemoteProcessParametersFlags = RemotePeb + FIELD_OFFSET(PEB, ProcessParameters);

			Status = NtReadVirtualMemory(
				*ProcessHandle,
				(PVOID) RemoteProcessParametersFlags,
				&RemoteProcessParametersFlags,
				ChildProcessBitness / 8,
				NULL);

			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				KexLogWarningEvent(
					L"Failed to read Peb->ProcessParameters pointer of child process\r\n\r\n"
					L"NTSTATUS error code: %s (0x%08lx)",
					KexRtlNtStatusToString(Status), Status);
				goto BailOut;
			}

			//
			// RemoteProcessParametersFlags now contains a pointer to a
			// RTL_USER_PROCESS_PARAMETERS structure within the child process.
			//

			RemoteProcessParametersFlags += FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Flags);

			Status = NtReadVirtualMemory(
				*ProcessHandle,
				(PVOID) RemoteProcessParametersFlags,
				&ProcessParametersFlags,
				sizeof(ProcessParametersFlags),
				NULL);

			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				KexLogWarningEvent(
					L"Failed to read ProcessParameters->Flags of child process\r\n\r\n"
					L"NTSTATUS error code: %s (0x%08lx)",
					KexRtlNtStatusToString(Status), Status);
				goto BailOut;
			}

			//
			// Clear the flag and write back the new value to the child process.
			//

			ProcessParametersFlags &= ~RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING;

			Status = NtWriteVirtualMemory(
				*ProcessHandle,
				(PVOID) RemoteProcessParametersFlags,
				&ProcessParametersFlags,
				sizeof(ProcessParametersFlags),
				NULL);

			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				KexLogWarningEvent(
					L"Failed to write ProcessParameters->Flags of child process\r\n\r\n"
					L"NTSTATUS error code: %s (0x%08lx)",
					KexRtlNtStatusToString(Status), Status);
				goto BailOut;
			}
		}
	}

BailOut:
	Status = STATUS_SUCCESS;

	//
	// Finally, resume the initial thread (unless the original caller
	// wanted the thread to remain suspended).
	//
	// Note that CreateProcess always requests the thread to be created
	// suspended anyway.
	//

	unless (ThreadFlags & THREAD_CREATE_FLAGS_CREATE_SUSPENDED) {
		Status = NtResumeThread(*ThreadHandle, NULL);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to resume the initial thread of the remote process.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
		}
	}

	return Status;
} PROTECTED_FUNCTION_END