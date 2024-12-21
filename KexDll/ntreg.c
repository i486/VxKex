#include "buildcfg.h"
#include "kexdllp.h"

#define REG_NOTIFY_THREAD_AGNOSTIC 0x10000000

NTSTATUS NTAPI Ext_NtNotifyChangeMultipleKeys(
	IN	HANDLE				MasterKeyHandle,
	IN	ULONG				Count OPTIONAL,
	IN	OBJECT_ATTRIBUTES	SlaveObjects[] OPTIONAL,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous)
{
	//
	// If CompletionFilter contains REG_NOTIFY_THREAD_AGNOSTIC, simply
	// strip it out.
	// TODO: Implement this properly.
	//

	if (CompletionFilter & REG_NOTIFY_THREAD_AGNOSTIC) {
		KexLogDebugEvent(L"Stripping REG_NOTIFY_THREAD_AGNOSTIC flag from CompletionFilter");
	}

	CompletionFilter &= ~REG_NOTIFY_THREAD_AGNOSTIC;

	return NtNotifyChangeMultipleKeys(
		MasterKeyHandle,
		Count,
		SlaveObjects,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		CompletionFilter,
		WatchTree,
		Buffer,
		BufferSize,
		Asynchronous);
}

NTSTATUS NTAPI Ext_NtNotifyChangeKey(
	IN	HANDLE				KeyHandle,
	IN	HANDLE				Event OPTIONAL,
	IN	PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN	PVOID				ApcContext OPTIONAL,
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG				CompletionFilter,
	IN	BOOLEAN				WatchTree,
	OUT	PVOID				Buffer OPTIONAL,
	IN	ULONG				BufferSize,
	IN	BOOLEAN				Asynchronous)
{
	//
	// Pass through to NtNotifyChangeMultipleKeys to avoid code duplication.
	// This is how the NtNotifyChangeKey function is implemented in the kernel anyway,
	// so there should be no issues in doing this.
	//

	return Ext_NtNotifyChangeMultipleKeys(
		KeyHandle,
		0,
		NULL,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		CompletionFilter,
		WatchTree,
		Buffer,
		BufferSize,
		Asynchronous);
}