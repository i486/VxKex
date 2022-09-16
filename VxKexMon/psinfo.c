#include <KexComm.h>
#include <BaseDll.h>
#include <NtDll.h>
#include "VxKexMon.h"

// Get the virtual address of the PEB of another process.
BOOLEAN GetRemotePebVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	PebVirtualAddress)
{
	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION BasicInformation;

	// param validation
	if (!PebVirtualAddress) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	Status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	*PebVirtualAddress = (ULONG_PTR) BasicInformation.PebBaseAddress;
	return TRUE;
}

// Copy the PEB of a remote process into a buffer
BOOLEAN GetRemotePeb(
	IN	HANDLE	ProcessHandle,
	OUT	PPEB	Buffer,
	IN	ULONG	BufferSize)
{
	ULONG_PTR VaRemotePeb;

	// param validation
	if (!Buffer) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (BufferSize < sizeof(PEB)) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	if (!GetRemotePebVa(ProcessHandle, &VaRemotePeb)) {
		return FALSE;
	}

	return VaRead(ProcessHandle, VaRemotePeb, Buffer, sizeof(PEB));
}

// Get the virtual address of a remote process's RTL_USER_PROCESS_PARAMETERS
// structure within its PEB.
BOOLEAN GetRemoteProcessParametersVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	ProcessParamsVirtualAddress)
{
	ULONG_PTR VaRemotePeb;

	if (!ProcessParamsVirtualAddress) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!GetRemotePebVa(ProcessHandle, &VaRemotePeb)) {
		return FALSE;
	}

	return VaRead(ProcessHandle, VaRemotePeb + FIELD_OFFSET(PEB, ProcessParameters),
				  ProcessParamsVirtualAddress, sizeof(ProcessParamsVirtualAddress));
}

// Copy RTL_USER_PROCESS_PARAMETERS from a remote process into a buffer
BOOLEAN GetRemoteProcessParameters(
	IN	HANDLE							ProcessHandle,
	OUT	PRTL_USER_PROCESS_PARAMETERS	Buffer,
	IN	ULONG							BufferSize)
{
	ULONG_PTR VaRemoteProcessParameters;

	if (!Buffer) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (BufferSize < sizeof(RTL_USER_PROCESS_PARAMETERS)) {
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	if (!GetRemoteProcessParametersVa(ProcessHandle, &VaRemoteProcessParameters)) {
		return FALSE;
	}

	return VaRead(ProcessHandle, VaRemoteProcessParameters, Buffer, sizeof(RTL_USER_PROCESS_PARAMETERS));
}

BOOLEAN GetRemoteEnvironmentBlockVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	EnvironmentBlockVirtualAddress)
{
	ULONG_PTR VaRemoteProcessParameters;

	if (!EnvironmentBlockVirtualAddress) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!GetRemoteProcessParametersVa(ProcessHandle, &VaRemoteProcessParameters)) {
		return FALSE;
	}

	return VaRead(ProcessHandle, VaRemoteProcessParameters + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, Environment),
				  EnvironmentBlockVirtualAddress, sizeof(EnvironmentBlockVirtualAddress));
}

// You must pass the Destination pointer to **RtlDestroyEnvironment** after you're done!
// This function can only clone the process environment, not any arbitrary environment
// block within a target process.
BOOLEAN CloneRemoteEnvironmentBlock(
	IN	HANDLE	ProcessHandle,
	OUT	PVOID	*Destination)
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION MemoryInformation;
	ULONG_PTR VaRemoteEnvironment;

	if (!Destination) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	*Destination = NULL;

	if (!GetRemoteEnvironmentBlockVa(ProcessHandle, &VaRemoteEnvironment)) {
		return FALSE;
	}
	
	//
	// Instead of trying to read through the entire environment block we will use
	// NtQueryVirtualMemory to get the size of the memory block, and bring the whole
	// thing over. Then later you can use the SizeOfEnvironmentBlock function if you
	// really need the exact size (but you probably won't). This is faster and less
	// error-prone than trying to parse the environment of a remote process.
	//
	Status = NtQueryVirtualMemory(
		ProcessHandle,
		(PVOID) VaRemoteEnvironment,
		MemoryBasicInformation,
		&MemoryInformation,
		sizeof(MemoryInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Allocate enough memory in this address space to store the environment block.
	//
	Status = NtAllocateVirtualMemory(
		NtCurrentProcess(),
		Destination,
		0,
		&MemoryInformation.RegionSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Copy environment block into our address space
	//
	if (!VaRead(ProcessHandle, VaRemoteEnvironment, *Destination, MemoryInformation.RegionSize)) {
		MemoryInformation.RegionSize = 0;

		NtFreeVirtualMemory(
			NtCurrentProcess(),
			Destination,
			&MemoryInformation.RegionSize,
			MEM_RELEASE);

		*Destination = NULL;
		return FALSE;
	}

	return TRUE;
}