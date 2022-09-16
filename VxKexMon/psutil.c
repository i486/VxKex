#include <KexComm.h>
#include <BaseDll.h>
#include <NtDll.h>
#include "VxKexMon.h"

BOOLEAN UpdateRemoteEnvironmentBlock(
	IN	HANDLE	ProcessHandle,
	IN	PVOID	Environment)
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION MemoryInformation;
	ULONG_PTR VaNewEnvironmentBlock;
	ULONG_PTR VaOldEnvironmentBlock;
	ULONG_PTR VaProcessParametersEnvironment;

	if (!Environment) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//
	// Figure out the size of the memory block we need to copy over.
	//
	Status = NtQueryVirtualMemory(
		NtCurrentProcess(),
		Environment,
		MemoryBasicInformation,
		&MemoryInformation,
		sizeof(MemoryInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Allocate same sized memory block in the remote process.
	//
	Status = NtAllocateVirtualMemory(
		ProcessHandle,
		(PPVOID) &VaNewEnvironmentBlock,
		0,
		&MemoryInformation.RegionSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Copy the environment block into the remote process.
	//
	if (!VaWrite(ProcessHandle, VaNewEnvironmentBlock, Environment,
				 MemoryInformation.RegionSize)) {
		goto FreeAndExit;
	}
	
	//
	// Free the old environment block
	//
	if (!GetRemoteProcessParametersVa(ProcessHandle, &VaProcessParametersEnvironment)) {
		goto FreeAndExit;
	}

	VaProcessParametersEnvironment += FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment);

	if (VaRead(ProcessHandle, VaProcessParametersEnvironment,
			   &VaOldEnvironmentBlock, sizeof(VaOldEnvironmentBlock))) {
		MemoryInformation.RegionSize = 0;

		NtFreeVirtualMemory(
			ProcessHandle,
			(PPVOID) &VaOldEnvironmentBlock,
			&MemoryInformation.RegionSize,
			MEM_RELEASE);
	} else {
		// log the error
	}

	//
	// Update pointer in the ProcessParameters struct to point to our newly allocated
	// environment block.
	//
	if (!VaWriteP(ProcessHandle, VaProcessParametersEnvironment,
				  VaNewEnvironmentBlock)) {
		goto FreeAndExit;
	}

	return TRUE;

FreeAndExit:
	//
	// failed - free the new environment block and return failure status
	//
	MemoryInformation.RegionSize = 0;
		
	NtFreeVirtualMemory(
		ProcessHandle,
		(PVOID *) VaNewEnvironmentBlock,
		&MemoryInformation.RegionSize,
		MEM_RELEASE);

	return FALSE;
}