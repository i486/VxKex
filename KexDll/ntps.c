#include "buildcfg.h"
#include "kexdllp.h"

//
// ProcessHandleInformation. This information class was introduced in
// Windows 8 and returns a PROCESS_HANDLE_SNAPSHOT_INFORMATION structure
// which describes every handle opened by the target process.
//
#define KEX_PROCESS_HANDLE_INFORMATION_CLASS 51

//
// Emulate ProcessHandleInformation on Windows 7.
//
// Windows 7 can't enumerate the handles of one particular process, but it
// can enumerate every handle in the system (SystemExtendedHandleInformation).
// We take a snapshot of all handles and pick out the ones which belong to
// the target process.
//
// The HandleCount and PointerCount members can't be obtained this way and
// are set to zero.
//
// This is needed for the Cygwin/MSYS2 runtime (3.5 and above), which no
// longer contains a code path for Windows 7 and uses this information class
// to locate the other end of a pipe.
//
STATIC NTSTATUS KexpQueryProcessHandleInformation(
	IN	HANDLE	ProcessHandle,
	OUT	PVOID	ProcessInformation,
	IN	ULONG	ProcessInformationLength,
	OUT	PULONG	ReturnLength OPTIONAL)
{
	NTSTATUS Status;
	PROCESS_BASIC_INFORMATION BasicInformation;
	PSYSTEM_HANDLE_INFORMATION_EX SystemHandles;
	PPROCESS_HANDLE_SNAPSHOT_INFORMATION Snapshot;
	ULONG SystemHandlesCb;
	ULONG_PTR Index;
	ULONG_PTR NumberOfHandles;
	SIZE_T RequiredSize;
	ULONG RequiredCb;

	//
	// Find out the process ID of the target process. This also checks that
	// the caller has passed a valid process handle with query access.
	//

	Status = NtQueryInformationProcess(
		ProcessHandle,
		ProcessBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Take a snapshot of all the handles in the system. The required buffer
	// size can change between calls, so keep trying with a larger buffer
	// until it fits.
	//

	SystemHandles = NULL;
	SystemHandlesCb = 0x10000;

	while (TRUE) {
		ULONG ReturnedCb;

		SystemHandles = (PSYSTEM_HANDLE_INFORMATION_EX) SafeAlloc(BYTE, SystemHandlesCb);

		if (!SystemHandles) {
			return STATUS_NO_MEMORY;
		}

		ReturnedCb = 0;

		Status = NtQuerySystemInformation(
			SystemExtendedHandleInformation,
			SystemHandles,
			SystemHandlesCb,
			&ReturnedCb);

		if (NT_SUCCESS(Status)) {
			break;
		}

		SafeFree(SystemHandles);

		if (Status != STATUS_INFO_LENGTH_MISMATCH) {
			return Status;
		}

		if (SystemHandlesCb >= 0x20000000 || ReturnedCb >= 0x20000000) {
			// Something is wrong. Don't try to allocate half a gigabyte.
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		if (ReturnedCb > SystemHandlesCb) {
			// Add some slack in case more handles get opened before the
			// next attempt.
			SystemHandlesCb = ReturnedCb + 0x10000;
		} else {
			SystemHandlesCb *= 2;
		}
	}

	//
	// Count the handles that belong to the target process and calculate
	// how much space the caller needs to provide.
	//

	NumberOfHandles = 0;

	for (Index = 0; Index < SystemHandles->NumberOfHandles; ++Index) {
		if (SystemHandles->Handles[Index].UniqueProcessId == BasicInformation.UniqueProcessId) {
			++NumberOfHandles;
		}
	}

	// The size has to fit in a ULONG. Check before multiplying.
	if (NumberOfHandles > (0x7FFFFFFF / sizeof(PROCESS_HANDLE_TABLE_ENTRY_INFO)) - 1) {
		SafeFree(SystemHandles);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RequiredSize = FIELD_OFFSET(PROCESS_HANDLE_SNAPSHOT_INFORMATION, Handles) +
				   (NumberOfHandles * sizeof(PROCESS_HANDLE_TABLE_ENTRY_INFO));

	RequiredCb = (ULONG) RequiredSize;
	Status = STATUS_SUCCESS;

	//
	// Copy the information to the caller's buffer.
	//

	try {
		if (ReturnLength) {
			*ReturnLength = RequiredCb;
		}

		if (ProcessInformationLength < RequiredCb) {
			Status = STATUS_INFO_LENGTH_MISMATCH;
		} else {
			PPROCESS_HANDLE_TABLE_ENTRY_INFO Entry;

			Snapshot = (PPROCESS_HANDLE_SNAPSHOT_INFORMATION) ProcessInformation;
			Snapshot->NumberOfHandles = NumberOfHandles;
			Snapshot->Reserved = 0;

			Entry = Snapshot->Handles;

			for (Index = 0; Index < SystemHandles->NumberOfHandles; ++Index) {
				PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX SystemEntry;

				SystemEntry = &SystemHandles->Handles[Index];

				if (SystemEntry->UniqueProcessId != BasicInformation.UniqueProcessId) {
					continue;
				}

				Entry->HandleValue		= (HANDLE) SystemEntry->HandleValue;
				Entry->HandleCount		= 0;
				Entry->PointerCount		= 0;
				Entry->GrantedAccess	= SystemEntry->GrantedAccess;
				Entry->ObjectTypeIndex	= SystemEntry->ObjectTypeIndex;
				Entry->HandleAttributes	= SystemEntry->HandleAttributes;
				Entry->Reserved			= 0;

				++Entry;
			}
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		Status = GetExceptionCode();
	}

	SafeFree(SystemHandles);
	return Status;
}

NTSTATUS NTAPI Ext_NtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL)
{
	if (ProcessInformationClass == KEX_PROCESS_HANDLE_INFORMATION_CLASS) {
		return KexpQueryProcessHandleInformation(
			ProcessHandle,
			ProcessInformation,
			ProcessInformationLength,
			ReturnLength);
	}

	// TODO
	if (ProcessInformationClass == 58) {
		KexLogWarningEvent(L"ProcessInformationClass == 58");
		KexDebugCheckpoint();
	}

	if (ProcessInformationClass > KEX_PROCESS_HANDLE_INFORMATION_CLASS) {
		KexLogWarningEvent(L"ProcessInformationClass > 51");
		KexDebugCheckpoint();
	}

	return NtQueryInformationProcess(
		ProcessHandle,
		ProcessInformationClass,
		ProcessInformation,
		ProcessInformationLength,
		ReturnLength);
}
