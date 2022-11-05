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

NTSTATUS KexInitializePropagation(
	VOID) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (KexData->IfeoParameters.DisableForChild) {
		KexSrvLogDetailEvent(L"Not enabling propagation due to user preferences.");
		return STATUS_USER_DISABLED;
	}

	//
	// Install a permanent hook, because our hook function will directly do
	// a syscall when it wants to call the original function.
	//
	Status = KexHkInstallBasicHook(&NtCreateUserProcess, KexpNtCreateUserProcessHook, NULL);

	if (NT_SUCCESS(Status)) {
		KexSrvLogInformationEvent(L"Successfully initialized propagation system.");
	} else {
		KexSrvLogErrorEvent(
			L"Failed to initialize propagation system.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
	}

	return Status;
} PROTECTED_FUNCTION_END

NTSTATUS NTAPI KexpNtOpenKeyExHook(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions)
{
	UNICODE_STRING IfeoBaseKeyName;

	if (!ObjectAttributes || !ObjectAttributes->ObjectName || !ObjectAttributes->ObjectName->Buffer) {
		goto BailOut;
	}

	RtlInitConstantUnicodeString(&IfeoBaseKeyName, L"Image File Execution Options");

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

		if (NT_SUCCESS(Status) && KexRtlFindUnicodeSubstring(RootDirectoryName, &IfeoBaseKeyName, TRUE)) {
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
	RTL_USER_PROCESS_PARAMETERS ModifiedProcessParameters;

	NTSTATUS Status;
	UNICODE_STRING DllPath;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PVOID RemoteDllBase;
	SIZE_T RemoteDllSize;
	HANDLE FileHandle;
	HANDLE SectionHandle;
	PVOID NativeNtOpenKey;
	PVOID NativeNtOpenKeyEx;
	ULONG_PTR RemoteNtOpenKeyHook;
	ULONG_PTR RemoteNtOpenKeyExHook;
	PBYTE HookTemplate;

	SectionHandle = NULL;

	ModifiedProcessDesiredAccess = ProcessDesiredAccess;
	ModifiedThreadDesiredAccess = ThreadDesiredAccess;
	ModifiedThreadFlags = ThreadFlags;
	ModifiedProcessParameters = *ProcessParameters;

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
	// 4. We need to clear a flag in the process parameters structure,
	//    because if that flag is set, the loader in the new process will
	//    not even attempt to read our virtualized IFEO key.
	//

	ModifiedProcessDesiredAccess |= PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE;
	ModifiedThreadDesiredAccess |= THREAD_SUSPEND_RESUME;
	ModifiedThreadFlags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;
	ModifiedProcessParameters.Flags &= ~RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING;
	
	Status = KexNtCreateUserProcess(
		ProcessHandle,
		ThreadHandle,
		ModifiedProcessDesiredAccess,
		ModifiedThreadDesiredAccess,
		ProcessObjectAttributes,
		ThreadObjectAttributes,
		ProcessFlags,
		ModifiedThreadFlags,
		&ModifiedProcessParameters,
		CreateInfo,
		AttributeList);
	
	if (!NT_SUCCESS(Status)) {
		KexSrvLogWarningEvent(
			L"Failed to create user process with modified parameters. "
			L"Reverting to system standard behavior.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);

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
		KexSrvLogWarningEvent(
			L"Failed to open KexDll.\r\n\r\n"
			L"Path to the missing or inaccessible DLL: \"%wZ\"\r\n"
			L"NTSTATUS error code: 0x%08lx",
			&DllPath,
			Status);
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
		KexSrvLogWarningEvent(
			L"Failed to create an image section backed by KexDll.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
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
		KexSrvLogWarningEvent(
			L"Failed to map the KexDll section into the remote process.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
		goto BailOut;
	}

	KexSrvLogDebugEvent(
		L"Successfully mapped KexDll into remote process.\r\n\r\n"
		L"Remote base address: 0x%p\r\n"
		L"Remote view size:    %Iu bytes",
		RemoteDllBase,
		RemoteDllSize);

	//
	// KexDll has now been mapped into the remote process.
	// Now, we have the following tasks:
	//
	//   1. Find the entry points of NtOpenKey and NtOpenKeyEx in the
	//      *native* NTDLL in the current process, and use those entry
	//      point addresses to patch the remote native NTDLL.
	//
	//   2. Update the remote PEB to remove the "image key missing" flag
	//      from its ProcessParameters.
	//

	if (KexRtlCurrentProcessBitness() == KexRtlOperatingSystemBitness()) {
		NativeNtOpenKey = NtOpenKey;
		NativeNtOpenKeyEx = NtOpenKeyEx;
		RemoteNtOpenKeyHook = (ULONG_PTR) VA_TO_RVA(KexData->KexDllBase, KexpNtOpenKeyHook);
		RemoteNtOpenKeyExHook = (ULONG_PTR) VA_TO_RVA(KexData->KexDllBase, KexpNtOpenKeyExHook);
	} else {
		PVOID TemporaryMapping;
		SIZE_T TemporaryMappingSize;

		Status = KexRtlMiniGetProcedureAddress(
			KexData->SystemDllBase,
			"NtOpenKey",
			&NativeNtOpenKey);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to find the entry point of native NtOpenKey.\r\n\r\n",
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

		Status = KexRtlMiniGetProcedureAddress(
			KexData->SystemDllBase,
			"NtOpenKeyEx",
			&NativeNtOpenKeyEx);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to find the entry point of native NtOpenKeyEx.\r\n\r\n",
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

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
			KexSrvLogWarningEvent(
				L"Failed to map temporary KexDll section into current process.\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

		Status = KexRtlMiniGetProcedureAddress(
			TemporaryMapping,
			"KexpNtOpenKeyHook",
			(PPVOID) &RemoteNtOpenKeyHook);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to find the procedure address of KexpNtOpenKeyHook.\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

		Status = KexRtlMiniGetProcedureAddress(
			TemporaryMapping,
			"KexpNtOpenKeyExHook",
			(PPVOID) &RemoteNtOpenKeyExHook);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to find the procedure address of KexpNtOpenKeyExHook.\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

		NtUnmapViewOfSection(NtCurrentProcess(), TemporaryMapping);

		RemoteNtOpenKeyHook = (ULONG_PTR) VA_TO_RVA(TemporaryMapping, RemoteNtOpenKeyHook);
		RemoteNtOpenKeyExHook = (ULONG_PTR) VA_TO_RVA(TemporaryMapping, RemoteNtOpenKeyExHook);
	}

	RemoteNtOpenKeyHook = (ULONG_PTR) RVA_TO_VA(RemoteDllBase, RemoteNtOpenKeyHook);
	RemoteNtOpenKeyExHook = (ULONG_PTR) RVA_TO_VA(RemoteDllBase, RemoteNtOpenKeyExHook);

	if (KexRtlRemoteProcessBitness(*ProcessHandle) == 64) {
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
	} else {
		HookTemplate = StackAlloc(BYTE, 6);
		RtlZeroMemory(HookTemplate, 6);
		HookTemplate[0] = 0x68;
		HookTemplate[5] = 0xC3;
		*((PULONG) &HookTemplate[1]) = (ULONG) RemoteNtOpenKeyExHook;

		Status = KexRtlWriteProcessMemory(
			*ProcessHandle,
			(ULONG_PTR) NativeNtOpenKeyEx,
			HookTemplate,
			6);
	}

	if (!NT_SUCCESS(Status)) {
		KexSrvLogWarningEvent(
			L"Failed to write hook template to remote process.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
		goto BailOut;
	}

BailOut:
	Status = STATUS_SUCCESS;

	if (SectionHandle) {
		NtClose(SectionHandle);
	}

	//
	// Finally, resume the initial thread (unless the original caller
	// wanted the thread to remain suspended).
	//

	unless (ThreadFlags & THREAD_CREATE_FLAGS_CREATE_SUSPENDED) {
		Status = NtResumeThread(*ThreadHandle, NULL);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
			L"Failed to resume the initial thread of the remote process.\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
		}
	}

	return Status;
} PROTECTED_FUNCTION_END