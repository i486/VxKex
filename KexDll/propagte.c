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
//     NtCreateUserProcess is called, 
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS NTAPI KexpProcessCreationHook(
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
	Status = KexHkInstallBasicHook(&NtCreateUserProcess, KexpProcessCreationHook, NULL);

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

//
// Note: Do not change the original parameters inside the hook function.
// Make a copy in a local variable and then change them there.
// This is so that if we need to "bail out" to the original NtCreateUserProcess,
// we can provide the original parameters.
//
NTSTATUS NTAPI KexpProcessCreationHook(
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
	UNICODE_STRING DllPath;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	PVOID RemoteDllBase;
	SIZE_T RemoteDllSize;
	HANDLE FileHandle;
	HANDLE SectionHandle;
	BOOLEAN UsingWow64Dll;
	ULONG RemoteProcessBitness;

	RemoteProcessBitness = KexRtlOperatingSystemBitness();
	UsingWow64Dll = FALSE;

	//
	// Create the process suspended, since we want to inject dlls and mess around
	// with it.
	//

	ModifiedThreadFlags = ThreadFlags | THREAD_CREATE_FLAGS_CREATE_SUSPENDED;
	
	Status = KexNtCreateUserProcess(
		ProcessHandle,
		ThreadHandle,
		ProcessDesiredAccess,
		ThreadDesiredAccess,
		ProcessObjectAttributes,
		ThreadObjectAttributes,
		ProcessFlags,
		ModifiedThreadFlags,
		ProcessParameters,
		CreateInfo,
		AttributeList);
	
	if (!NT_SUCCESS(Status)) {
		KexSrvLogWarningEvent(
			L"KexNtCreateUserProcess failed\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
		return Status;
	}

	//
	// Now we start injecting KexDll into the remote process.
	// We need to figure out whether the process is WOW64 so we
	// can pick the appropriate KexDll to load.
	//
	// Note: WOW64 redirection does not apply to the native API. So
	// don't try to use "sysnative" here, it won't work and is not
	// necessary.
	//

	if (KexRtlOperatingSystemBitness() == 64) {
		ULONG_PTR Peb32;

		Status = NtQueryInformationProcess(
			*ProcessHandle,
			ProcessWow64Information,
			&Peb32,
			sizeof(Peb32),
			NULL);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to query process WOW64 information.\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
			goto BailOut;
		}

		if (Peb32) {
			RemoteProcessBitness = 32;
			UsingWow64Dll = TRUE;
		}
	}
	
	if (UsingWow64Dll) {
		RtlInitConstantUnicodeString(&DllPath, L"\\SystemRoot\\syswow64\\KexDll.dll");
	} else {
		RtlInitConstantUnicodeString(&DllPath, L"\\SystemRoot\\system32\\KexDll.dll");
	}

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
			L"Failed to open the %s KexDll.\r\n\r\n"
			L"Path to the missing or inaccessible DLL: \"%wZ\"\r\n"
			L"NTSTATUS error code: 0x%08lx",
			UsingWow64Dll ? L"WOW64" : L"native",
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

	NtClose(SectionHandle);

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
	// All we need to do now is to queue an APC to its initial thread.
	//

	if (KexRtlCurrentProcessBitness() == RemoteProcessBitness) {
		ULONG KexDllInitializeThunkRva;
		ULONG_PTR RemoteInitializeThunk;

		//
		// Simple and easy: just get the RVA of our KexDllInitializeThunk and
		// add it to the base address of the remote KexDll, and that will give
		// us our APC address.
		//

		KexDllInitializeThunkRva = (ULONG) ((ULONG_PTR) &KexDllInitializeThunk - (ULONG_PTR) KexData->KexDllBase);
		RemoteInitializeThunk = (ULONG_PTR) RemoteDllBase + KexDllInitializeThunkRva;

		Status = NtQueueApcThread(
			*ThreadHandle,
			(PKNORMAL_ROUTINE) RemoteInitializeThunk,
			NULL,
			KexData->SystemDllBase,
			NULL);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogWarningEvent(
				L"Failed to queue an APC to the initial thread of the remote process.\r\n\r\n"
				L"Propagation has failed, KexDll will be unmapped from the remote process.\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);

			NtUnmapViewOfSection(
				*ProcessHandle,
				RemoteDllBase);

			goto BailOut;
		}
	}

BailOut:
	Status = STATUS_SUCCESS;

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