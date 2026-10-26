///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     excpiwbp.c
//
// Abstract:
//
//     Installer code which allows the running Explorer process(es) to be patched
//     to bypass the "not a valid Win32 application" error without a restart.
//
//     All Explorer processes are patched. This accounts for the configuration
//     where the user enables "Launch folder windows in a separate process".
//
// Author:
//
//     vxiiduu (22-Oct-2026)
//
// Environment:
//
//     Compiled into both KexSetup.exe (32-bit) and zzCpBp64.exe (64-bit).
//
// Revision History:
//
//     vxiiduu               22-Oct-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>

//
// Stolen from kexdll\kexrtl.c, since kexsetup needs to be able to run without
// kexdll functions
//
STATIC BOOLEAN KexSetupRtlUnicodeStringEndsWith(
	IN	PCUNICODE_STRING	String,
	IN	PCUNICODE_STRING	EndsWith,
	IN	BOOLEAN				CaseInsensitive)
{
	UNICODE_STRING EndOfString;

	//
	// Create a subset of the String that just contains the
	// end of it (with the number of characters that EndsWith
	// contains).
	//

	EndOfString.Buffer = String->Buffer + KexRtlUnicodeStringCch(String) - KexRtlUnicodeStringCch(EndsWith);
	EndOfString.Length = EndsWith->Length;
	EndOfString.MaximumLength = EndsWith->Length;

	if (EndOfString.Buffer < String->Buffer) {
		// EndsWith length greater than String length
		return FALSE;
	}

	//
	// Now perform the actual check.
	//

	return RtlEqualUnicodeString(&EndOfString, EndsWith, CaseInsensitive);
}

//
// stolen from kexdll\kexrtl.c
//
STATIC ULONG KexSetupRtlRemoteProcessBitness(
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	ULONG_PTR Peb32;

	if (KexRtlOperatingSystemBitness() == 32) {
		return 32;
	}

	Status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessWow64Information,
		&Peb32,
		sizeof(Peb32),
		NULL);

	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status) && Peb32) {
		return 32;
	} else {
		return 64;
	}
}

STATIC NTSTATUS PatchOneExplorer(
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	HRESULT Result;
	ULONG WaitResult;
	SIZE_T RegionSize;
	ULONG ModuleFileNameCch;
	WCHAR InjectedDllPath[MAX_PATH];
	PVOID RemoteInjectedDllPath;
	HANDLE ThreadHandle;

	if (KexSetupRtlRemoteProcessBitness(ProcessHandle) != KexRtlCurrentProcessBitness()) {
		return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
	}

	//
	// Form the path to the DLL that we're going to inject into the remote Explorer.
	// In the VxKex installer archive that DLL is next to our current EXE and
	// named either ExCpBp32.dll or ExCpBp64.dll depending on current process bitness
	//

	ModuleFileNameCch = GetModuleFileName(NULL, InjectedDllPath, ARRAYSIZE(InjectedDllPath));
	ASSERT (ModuleFileNameCch != 0);

	Result = PathCchRemoveFileSpec(InjectedDllPath, ARRAYSIZE(InjectedDllPath));
	ASSERT (SUCCEEDED(Result));

	if (KexRtlCurrentProcessBitness() == 32) {
		Result = PathCchAppend(InjectedDllPath, ARRAYSIZE(InjectedDllPath), L"ExCpBp32.dll");
	} else {
		Result = PathCchAppend(InjectedDllPath, ARRAYSIZE(InjectedDllPath), L"ExCpBp64.dll");
	}

	ASSERT (SUCCEEDED(Result));

	//
	// Allocate space for the DLL path in the address space of the remote process,
	// and then copy the DLL path to the remote process.
	//
	// We are responsible for freeing this allocation later.
	//

	RemoteInjectedDllPath = NULL;
	RegionSize = sizeof(InjectedDllPath);

	Status = NtAllocateVirtualMemory(
		ProcessHandle,
		&RemoteInjectedDllPath,
		0,
		&RegionSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	// STATUS_PROCESS_IS_TERMINATING is just a race condition and is not indicative
	// of a true error. We'll just return from the function and not try to do
	// anything further with this explorer process.
	ASSERT (NT_SUCCESS(Status) || Status == STATUS_PROCESS_IS_TERMINATING);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = NtWriteVirtualMemory(
		ProcessHandle,
		RemoteInjectedDllPath,
		InjectedDllPath,
		sizeof(InjectedDllPath),
		NULL);

	ASSERT (NT_SUCCESS(Status) || Status == STATUS_PROCESS_IS_TERMINATING);

	if (!NT_SUCCESS(Status)) {
		goto FreeMemoryAndReturn;
	}

	//
	// Create the remote thread that will load the DLL and run the DllMain.
	// We are directly using the loader resolved address of LoadLibraryW since it
	// is in kernel32 and the address is the same for all processes (of the same
	// bitness)
	//

	ThreadHandle = CreateRemoteThread(
		ProcessHandle,
		NULL,
		0,
		(PTHREAD_START_ROUTINE) LoadLibraryW,
		RemoteInjectedDllPath,
		0,
		NULL);

	ASSERT (VALID_HANDLE(ThreadHandle));

	if (ThreadHandle == NULL) {
		Status = NtCurrentTeb()->LastStatusValue;
		goto FreeMemoryAndReturn;
	}

	//
	// Wait for the thread to finish its tasks. The 15-second timeout is a failsafe
	// in case the thread gets stuck or something.
	//
	// Then we can free the DLL name allocation.
	//

	WaitResult = WaitForSingleObject(ThreadHandle, 15000);
	ASSERT (WaitResult == WAIT_OBJECT_0);

	SafeClose(ThreadHandle);

	if (WaitResult != WAIT_OBJECT_0) {
		// Don't free the memory since the thread is still executing.
		// Doing so may cause explorer to crash.
		Status = NtCurrentTeb()->LastStatusValue;
		return Status;
	}

FreeMemoryAndReturn:
	{
		NTSTATUS Status2;

		RegionSize = 0;

		Status2 = NtFreeVirtualMemory(
			ProcessHandle,
			&RemoteInjectedDllPath,
			&RegionSize,
			MEM_RELEASE);

		ASSERT (NT_SUCCESS(Status2) || Status2 == STATUS_PROCESS_IS_TERMINATING);
	}

	return Status;
}

NTSTATUS PatchAllExplorers(
	VOID)
{
	NTSTATUS Status;
	HANDLE ProcessHandle;
	BOOLEAN PatchedAtLeastOneExplorer;
	UNICODE_STRING SlashExplorerExe;

	ProcessHandle = NULL;
	PatchedAtLeastOneExplorer = FALSE;
	
	RtlInitConstantUnicodeString(&SlashExplorerExe, L"\\explorer.exe");

	//
	// Enumerate all processes on the system to find Explorer.
	//

	while (TRUE) {
		HANDLE NextProcessHandle;

		typedef struct {
			UNICODE_STRING UnicodeString;
			WCHAR Buffer[MAX_PATH];
		} TYPEDEF_TYPE_NAME(NT_RETURN_PATH_UNICODE_STRING);

		NT_RETURN_PATH_UNICODE_STRING ProcessExeName;

		Status = NtGetNextProcess(
			ProcessHandle,
			PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD |
			PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
			0,
			0,
			&NextProcessHandle);
		
		SafeClose(ProcessHandle);
		ProcessHandle = NextProcessHandle;

		if (!NT_SUCCESS(Status)) {
			break;
		}

		//
		// Query the EXE name of the process.
		//

		Status = NtQueryInformationProcess(
			ProcessHandle,
			ProcessImageFileName,
			&ProcessExeName,
			sizeof(ProcessExeName),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			continue;
		}

		if (KexSetupRtlUnicodeStringEndsWith(&ProcessExeName.UnicodeString, &SlashExplorerExe, TRUE)) {
			// We found an explorer.exe.
			Status = PatchOneExplorer(ProcessHandle);

			if (NT_SUCCESS(Status)) {
				PatchedAtLeastOneExplorer = TRUE;
			}
		}
	}

	if (Status == STATUS_NO_MORE_ENTRIES) {
		Status = STATUS_SUCCESS;
	}

	if (!PatchedAtLeastOneExplorer) {
		Status = STATUS_NOT_FOUND;
	}

	return Status;
}