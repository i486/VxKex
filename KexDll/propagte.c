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

STATIC PVOID NativeNtOpenKey;
STATIC PVOID NativeNtOpenKeyEx;

//
// This function unhooks NtOpenKey/NtOpenKeyEx and unmaps the
// temporary KexDll from the current process.
//
STATIC VOID KexpCleanupPropagationRemains(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PBYTE Function;
	PVOID HookDestination;
	MEMORY_BASIC_INFORMATION MemoryInformation;

	unless (KexData->Flags & KEXDATA_FLAG_PROPAGATED) {
		KexLogDebugEvent(L"Propagation flag not set.");
		return;
	}

	//
	// Inspect the entry points of the native NtOpenKey and see if they
	// look like a hook template that has previously been written.
	//

	Function = (PBYTE) NativeNtOpenKey;
	HookDestination = NULL;

	if (KexRtlOperatingSystemBitness() == 64) {
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
	}

	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		HookDestination,
		MemoryBasicInformation,
		&MemoryInformation,
		sizeof(MemoryInformation),
		NULL);

	if (NT_SUCCESS(Status) && (MemoryInformation.Type & MEM_IMAGE)) {
		//
		// Unmap the temporary KexDll.
		//
		Status = NtUnmapViewOfSection(NtCurrentProcess(), MemoryInformation.AllocationBase);

		if (NT_SUCCESS(Status)) {
			ULONG OldProtect;
			ULONG Discard;
			PVOID BaseAddress;
			SIZE_T RegionSize;

			KexLogInformationEvent(
				L"Successfully unmapped temporary KexDll from propagation.\r\n\r\n"
				L"Base address: 0x%p",
				MemoryInformation.AllocationBase);

			//
			// Restore original syscalls.
			//
			
			BaseAddress = NativeNtOpenKey;
			RegionSize = 15;
			NtProtectVirtualMemory(
				NtCurrentProcess(),
				&BaseAddress,
				&RegionSize,
				PAGE_EXECUTE_WRITECOPY,
				&OldProtect);

			BaseAddress = NativeNtOpenKeyEx;
			NtProtectVirtualMemory(
				NtCurrentProcess(),
				&BaseAddress,
				&RegionSize,
				PAGE_EXECUTE_WRITECOPY,
				&OldProtect);

			if (KexRtlOperatingSystemBitness() == 64) {
				PULONG SyscallNumber;
				BYTE SyscallTemplate[] = {
					0x4C, 0x8B, 0xD1,					// mov r10, rcx
					0xB8, 0x00, 0x00, 0x00, 0x00,		// mov eax, <syscallnumber>
					0x0F, 0x05,							// syscall
					0xC3								// ret
				};

				SyscallNumber = (PDWORD) &SyscallTemplate[4];
				
				*SyscallNumber = 0x0F; // NtOpenKey
				RtlCopyMemory(NativeNtOpenKey, SyscallTemplate, sizeof(SyscallTemplate));
				*SyscallNumber = 0xF2; // NtOpenKeyEx
				RtlCopyMemory(NativeNtOpenKeyEx, SyscallTemplate, sizeof(SyscallTemplate));
			} else {
				PULONG SyscallNumber;
				PUSHORT ParamBytes;
				BYTE SyscallTemplate[] = {
					0xB8, 0xB6, 0x00, 0x00, 0x00,		// mov eax, <syscallnumber>
					0xBA, 0x00, 0x03, 0xFE, 0x7F,		// mov edx, 0x7ffe0300
					0xFF, 0x12,							// call [edx]
					0xC2, 0x00, 0x00					// ret <parambytes>
				};

				SyscallNumber = (PULONG) &SyscallTemplate[1];
				ParamBytes = (PUSHORT) &SyscallTemplate[13];

				*SyscallNumber = 0xB6; // NtOpenKey
				*ParamBytes = 0x0C;
				RtlCopyMemory(NativeNtOpenKey, SyscallTemplate, sizeof(SyscallTemplate));
				*SyscallNumber = 0xB7; // NtOpenKeyEx
				*ParamBytes = 0x10;
				RtlCopyMemory(NativeNtOpenKeyEx, SyscallTemplate, sizeof(SyscallTemplate));
			}

			BaseAddress = NativeNtOpenKey;
			NtProtectVirtualMemory(
				NtCurrentProcess(),
				&BaseAddress,
				&RegionSize,
				OldProtect,
				&Discard);

			BaseAddress = NativeNtOpenKeyEx;
			NtProtectVirtualMemory(
				NtCurrentProcess(),
				&BaseAddress,
				&RegionSize,
				OldProtect,
				&Discard);
		} else {
			KexLogWarningEvent(
				L"Failed to unmap temporary KexDll from propagation.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
		}
	}
} PROTECTED_FUNCTION_END_VOID

NTSTATUS KexInitializePropagation(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	//
	// Find the entrypoints of NtOpenKey and NtOpenKeyEx because we will
	// need them both for cleaning up propagation remains and also for
	// propagating to child processes.
	//

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		NativeNtOpenKey = NtOpenKey;
		NativeNtOpenKeyEx = NtOpenKeyEx;
	} else {
		Status = KexLdrMiniGetProcedureAddress(
			KexData->SystemDllBase,
			"NtOpenKey",
			&NativeNtOpenKey);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to find the entry point of native NtOpenKey.\r\n\r\n",
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
			return Status;
		}

		Status = KexLdrMiniGetProcedureAddress(
			KexData->SystemDllBase,
			"NtOpenKeyEx",
			&NativeNtOpenKeyEx);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to find the entry point of native NtOpenKeyEx.\r\n\r\n",
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
			return Status;
		}
	}

	KexpCleanupPropagationRemains();

	if (KexData->IfeoParameters.DisableForChild) {
		KexLogInformationEvent(L"Not enabling propagation due to user preferences.");
		return STATUS_USER_DISABLED;
	}

	//
	// Install a permanent hook, because our hook function will directly do
	// a syscall when it wants to call the original function.
	//
	Status = KexHkInstallBasicHook(&NtCreateUserProcess, KexpNtCreateUserProcessHook, NULL);

	if (NT_SUCCESS(Status)) {
		KexLogInformationEvent(L"Successfully initialized propagation system.");
	} else {
		KexLogErrorEvent(
			L"Failed to initialize propagation system.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
	}

	return Status;
} PROTECTED_FUNCTION_END

//
// KexpNtOpenKeyHook and KexpNtOpenKeyExHook are called early in process
// initialization (see LdrpInitializeExecutionOptions), and more importantly,
// *NTDLL imports are not snapped*. This means you cannot call any function
// from NTDLL, directly or indirectly, from the NtOpenKey* hook functions.
//
// Exception handling also doesn't work, since on x64 that depends on the
// NTDLL export __C_specific_handler.
//

NTSTATUS NTAPI KexpNtOpenKeyExHook(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions)
{
	UNICODE_STRING IfeoBaseKeyName;
	UNICODE_STRING DotExe;
	PPEB Peb;

	Peb = NtCurrentPeb();
	Peb->ProcessParameters->Flags &= ~RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING;
	Peb->SubSystemData = (PVOID) 0xB02BA295L;

	if (!ObjectAttributes || !ObjectAttributes->ObjectName || !ObjectAttributes->ObjectName->Buffer) {
		goto BailOut;
	}

	RtlInitConstantUnicodeString(
		&IfeoBaseKeyName, 
		L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT"
		L"\\CurrentVersion\\Image File Execution Options");

	RtlInitConstantUnicodeString(&DotExe, L".exe");

	if (ObjectAttributes->RootDirectory) {
		NTSTATUS Status;
		PUNICODE_STRING RootDirectoryName;
		SIZE_T RootDirectoryNameLength;

		RootDirectoryName = NULL;
		RootDirectoryNameLength = 0x1000;
		Status = KexNtAllocateVirtualMemory(
			NtCurrentProcess(),
			(PPVOID) &RootDirectoryName,
			0,
			&RootDirectoryNameLength,
			MEM_COMMIT | MEM_RESERVE,
			PAGE_READWRITE);

		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		Status = KexNtQueryObject(
			ObjectAttributes->RootDirectory,
			ObjectNameInformation,
			RootDirectoryName,
			(ULONG) RootDirectoryNameLength,
			NULL);

		if (NT_SUCCESS(Status) && 
			KexRtlFindUnicodeSubstring(RootDirectoryName, &IfeoBaseKeyName, TRUE) &&
			KexRtlFindUnicodeSubstring(ObjectAttributes->ObjectName, &DotExe, TRUE)) {

			OBJECT_ATTRIBUTES ModifiedObjectAttributes;
			UNICODE_STRING KexVirtualIfeoEntryName;

			ModifiedObjectAttributes = *ObjectAttributes;
			RtlInitConstantUnicodeString(&KexVirtualIfeoEntryName, L"{VxKexPropagationVirtualKey}");
			ModifiedObjectAttributes.ObjectName = &KexVirtualIfeoEntryName;
			ObjectAttributes = &ModifiedObjectAttributes;
		}

		KexNtFreeVirtualMemory(
			NtCurrentProcess(),
			(PPVOID) &RootDirectoryName,
			&RootDirectoryNameLength,
			MEM_RELEASE);
	}

BailOut:
	return KexNtOpenKeyEx(
		KeyHandle,
		DesiredAccess,
		ObjectAttributes,
		OpenOptions);
}

NTSTATUS NTAPI KexpNtOpenKeyHook(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes)
{
	return KexpNtOpenKeyExHook(
		KeyHandle,
		DesiredAccess,
		ObjectAttributes,
		0);
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
	ULONG ModifiedThreadFlags;
	ULONG ModifiedProcessDesiredAccess;
	ULONG ModifiedThreadDesiredAccess;

	NTSTATUS Status;
	UNICODE_STRING DllPath;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PVOID RemoteDllBase;
	SIZE_T RemoteDllSize;
	HANDLE FileHandle;
	HANDLE SectionHandle;
	ULONG_PTR RemoteNtOpenKeyHook;
	ULONG_PTR RemoteNtOpenKeyExHook;
	PBYTE HookTemplate;

	SectionHandle = NULL;

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
		KexLogWarningEvent(
			L"Failed to create user process with modified parameters. "
			L"Reverting to system standard behavior.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));

		//
		// Perhaps it failed due to the desired access/flags changes.
		// Retry again but with the exact original parameters passed by
		// the caller. Of course, this means that VxKex will not be
		// enabled for the child process.
		//
		return KexNtCreateUserProcess(
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
	}
	
	RtlInitConstantUnicodeString(&DllPath, L"\\SystemRoot\\system32\\KexDll.dll");
	InitializeObjectAttributes(&ObjectAttributes, &DllPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenFile(
		&FileHandle,
		GENERIC_READ | GENERIC_EXECUTE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ,
		0);

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to open KexDll.\r\n\r\n"
			L"Path to the missing or inaccessible DLL: \"%wZ\"\r\n"
			L"NTSTATUS error code: %s",
			&DllPath,
			KexRtlNtStatusToString(Status));
		goto BailOut;
	}

	Status = NtCreateSection(
		&SectionHandle,
		SECTION_ALL_ACCESS,
		NULL,
		NULL,
		PAGE_EXECUTE_WRITECOPY,
		SEC_IMAGE,
		FileHandle);

	NtClose(FileHandle);

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to create an image section backed by KexDll.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		goto BailOut;
	}

	RemoteDllBase = NULL;
	RemoteDllSize = 0;
	Status = NtMapViewOfSection(
		SectionHandle,
		*ProcessHandle,
		&RemoteDllBase,
		4,
		0,
		NULL,
		&RemoteDllSize,
		ViewUnmap,
		0,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to map the KexDll section into the remote process.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		goto BailOut;
	}

	KexLogDebugEvent(
		L"Successfully mapped KexDll into remote process.\r\n\r\n"
		L"Remote base address: 0x%p\r\n"
		L"Remote view size:    %Iu bytes",
		RemoteDllBase,
		RemoteDllSize);

	//
	// Find the addresses of the native-bitness hook procedures in the temporary
	// KexDll mapping into the child process.
	//

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		RemoteNtOpenKeyHook = (ULONG_PTR) VA_TO_RVA(KexData->KexDllBase, KexpNtOpenKeyHook);
		RemoteNtOpenKeyExHook = (ULONG_PTR) VA_TO_RVA(KexData->KexDllBase, KexpNtOpenKeyExHook);
	} else {
		PVOID TemporaryMapping;
		SIZE_T TemporaryMappingSize;

		Status = NtMapViewOfSection(
			SectionHandle,
			NtCurrentProcess(),
			&TemporaryMapping,
			0,
			0,
			NULL,
			&TemporaryMappingSize,
			ViewUnmap,
			0,
			PAGE_READWRITE);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to map temporary KexDll section into current process.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
			goto BailOut;
		}

		Status = KexLdrMiniGetProcedureAddress(
			TemporaryMapping,
			"KexpNtOpenKeyHook",
			(PPVOID) &RemoteNtOpenKeyHook);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to find the procedure address of KexpNtOpenKeyHook.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
			goto BailOut;
		}

		Status = KexLdrMiniGetProcedureAddress(
			TemporaryMapping,
			"KexpNtOpenKeyExHook",
			(PPVOID) &RemoteNtOpenKeyExHook);

		if (!NT_SUCCESS(Status)) {
			KexLogWarningEvent(
				L"Failed to find the procedure address of KexpNtOpenKeyExHook.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
			goto BailOut;
		}

		NtUnmapViewOfSection(NtCurrentProcess(), TemporaryMapping);

		RemoteNtOpenKeyHook = (ULONG_PTR) VA_TO_RVA(TemporaryMapping, RemoteNtOpenKeyHook);
		RemoteNtOpenKeyExHook = (ULONG_PTR) VA_TO_RVA(TemporaryMapping, RemoteNtOpenKeyExHook);
	}

	RemoteNtOpenKeyHook = (ULONG_PTR) RVA_TO_VA(RemoteDllBase, RemoteNtOpenKeyHook);
	RemoteNtOpenKeyExHook = (ULONG_PTR) RVA_TO_VA(RemoteDllBase, RemoteNtOpenKeyExHook);

	//
	// Create hook templates and write them into the target process.
	//

	if (KexRtlOperatingSystemBitness() == 64) {
		HookTemplate = StackAlloc(BYTE, 14);
		RtlZeroMemory(HookTemplate, 14);
		HookTemplate[0] = 0xFF;
		HookTemplate[1] = 0x25;
		*((PULONGLONG) &HookTemplate[6]) = RemoteNtOpenKeyHook;

		Status = KexRtlWriteProcessMemory(
			*ProcessHandle,
			(ULONG_PTR) NativeNtOpenKey,
			HookTemplate,
			14);

		if (!NT_SUCCESS(Status)) {
			goto WriteProcessMemoryFailure;
		}

		*((PULONGLONG) &HookTemplate[6]) = RemoteNtOpenKeyExHook;

		Status = KexRtlWriteProcessMemory(
			*ProcessHandle,
			(ULONG_PTR) NativeNtOpenKeyEx,
			HookTemplate,
			14);
	} else {
		HookTemplate = StackAlloc(BYTE, 6);
		RtlZeroMemory(HookTemplate, 6);
		HookTemplate[0] = 0x68;
		HookTemplate[5] = 0xC3;

		*((PULONG) &HookTemplate[1]) = (ULONG) RemoteNtOpenKeyHook;

		Status = KexRtlWriteProcessMemory(
			*ProcessHandle,
			(ULONG_PTR) NativeNtOpenKey,
			HookTemplate,
			14);

		if (!NT_SUCCESS(Status)) {
			goto WriteProcessMemoryFailure;
		}

		*((PULONG) &HookTemplate[1]) = (ULONG) RemoteNtOpenKeyExHook;

		Status = KexRtlWriteProcessMemory(
			*ProcessHandle,
			(ULONG_PTR) NativeNtOpenKeyEx,
			HookTemplate,
			6);
	}

WriteProcessMemoryFailure:
	if (!NT_SUCCESS(Status)) {
		KexLogWarningEvent(
			L"Failed to write hook template to remote process.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
		goto BailOut;
	}

BailOut:
	Status = STATUS_SUCCESS;

	SafeClose(SectionHandle);

	//
	// Finally, resume the initial thread (unless the original caller
	// wanted the thread to remain suspended).
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