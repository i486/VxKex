#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI Ext_NtAssignProcessToJobObject(
	IN	HANDLE	JobHandle,
	IN	HANDLE	ProcessHandle)
{
	NTSTATUS Status;
	
	Status = NtAssignProcessToJobObject(
		JobHandle,
		ProcessHandle);

	//
	// In some situations, Chromium can shit itself by trying to launch child
	// processes over and over if it sees this fail.
	//

	if (Status == STATUS_ACCESS_DENIED && (KexData->Flags & KEXDATA_FLAG_CHROMIUM)) {
		KexLogDebugEvent(L"Faking NtAssignProcessToJobObject success for Chromium compatibility");
		Status = STATUS_SUCCESS;
	}

	return Status;
}