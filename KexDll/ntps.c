#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS NTAPI Ext_NtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL)
{
	// TODO
	if (ProcessInformationClass == 58) {
		KexLogWarningEvent(L"ProcessInformationClass == 58");
		KexDebugCheckpoint();
	}

	if (ProcessInformationClass >= 51) {
		KexLogWarningEvent(L"ProcessInformationClass >= 51");
		KexDebugCheckpoint();
	}

	return KexNtQueryInformationProcess(
		ProcessHandle,
		ProcessInformationClass,
		ProcessInformation,
		ProcessInformationLength,
		ReturnLength);
}