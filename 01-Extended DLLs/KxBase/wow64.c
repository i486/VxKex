#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI IsWow64Process2(
	IN	HANDLE	ProcessHandle,
	OUT	PUSHORT	ProcessMachine,
	OUT	PUSHORT	NativeMachine)
{
	NTSTATUS Status;

	Status = KexRtlWow64GetProcessMachines(ProcessHandle, ProcessMachine, NativeMachine);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}