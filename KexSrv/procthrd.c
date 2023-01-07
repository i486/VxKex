#include "buildcfg.h"
#include "kexsrvp.h"

PKEXSRV_PER_CLIENT_PROCESS_DATA LookupProcessById(
	IN	ULONG	ProcessId)
{
	PKEXSRV_PER_CLIENT_PROCESS_DATA Process;
	PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry;
	RTL_DYNAMIC_HASH_TABLE_CONTEXT Context;

	Entry = RtlLookupEntryHashTable(
		ProcessThreadTable,
		ProcessId,
		&Context);

	if (!Entry) {
		return NULL;
	}

	Process = CONTAINING_RECORD(
		Entry,
		KEXSRV_PER_CLIENT_PROCESS_DATA,
		HashTableEntry);

	ASSERT (Process->IsProcess);
	ASSERT (Process->ProcessId == ProcessId);

	return Process;
}

NTSTATUS AcceptConnectProcess(
	OUT	PPKEXSRV_PER_CLIENT_PROCESS_DATA	ProcessDataOut OPTIONAL,
	IN	HANDLE								PipeHandle)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	PKEXSRV_PER_CLIENT_PROCESS_DATA Process;

	Status = STATUS_SUCCESS;

	if (ProcessDataOut) {
		*ProcessDataOut = NULL;
	}

	//
	// Allocate memory for the per-process data structure.
	//

	Process = SafeAlloc(KEXSRV_PER_CLIENT_PROCESS_DATA, 1);
	if (!Process) {
		return STATUS_NO_MEMORY;
	}

	//
	// Figure out the client process ID and put that in the structure.
	//

	Status = NtFsControlFile(
		PipeHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
		(PVOID) "ClientProcessId",
		sizeof("ClientProcessId"),
		&Process->ProcessId,
		sizeof(Process->ProcessId));

	if (!NT_SUCCESS(Status)) {
		goto Finished;
	}

	ASSERT (LookupProcessById(Process->ProcessId) == NULL);

	//
	// Fill out miscellaneous data and insert this process into the hash table.
	//

	Process->IsProcess = TRUE;
	Process->PipeHandle = PipeHandle;
	Process->NumberOfThreads = 0;
	Process->Threads = NULL;

	RtlInsertEntryHashTable(
		ProcessThreadTable,
		&Process->HashTableEntry,
		Process->ProcessId,
		NULL);

Finished:
	if (NT_SUCCESS(Status)) {
		if (ProcessDataOut) {
			*ProcessDataOut = Process;
		}
	} else {
		SafeFree(Process);
	}

	return Status;
}

PKEXSRV_PER_CLIENT_THREAD_DATA LookupThreadById(
	IN	ULONG	ThreadId)
{
	PKEXSRV_PER_CLIENT_THREAD_DATA Thread;
	PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry;
	RTL_DYNAMIC_HASH_TABLE_CONTEXT Context;

	Entry = RtlLookupEntryHashTable(
		ProcessThreadTable,
		ThreadId,
		&Context);

	if (!Entry) {
		return NULL;
	}

	Thread = CONTAINING_RECORD(
		Entry,
		KEXSRV_PER_CLIENT_THREAD_DATA,
		HashTableEntry);

	ASSERT (!Thread->IsProcess);
	ASSERT (Thread->ThreadId == ThreadId);

	return Thread;
}

NTSTATUS RegisterThread(
	OUT	PPKEXSRV_PER_CLIENT_THREAD_DATA	ThreadDataOut OPTIONAL,
	IN	PKEXSRV_PER_CLIENT_PROCESS_DATA	ParentProcess,
	IN	ULONG							ThreadId)
{
	ULONG ThreadTableIndex;
	PKEXSRV_PER_CLIENT_THREAD_DATA Thread;
	PPKEXSRV_PER_CLIENT_THREAD_DATA NewProcessThreadsList;

	ASSERT (ParentProcess != NULL);
	ASSERT (LookupThreadById(ThreadId) == NULL);

	if (ThreadDataOut) {
		*ThreadDataOut = NULL;
	}

	//
	// Allocate per-thread data
	//

	Thread = SafeAlloc(KEXSRV_PER_CLIENT_THREAD_DATA, 1);
	if (!Thread) {
		return STATUS_NO_MEMORY;
	}

	//
	// Initialize the structure
	//

	Thread->ParentProcess = ParentProcess;
	Thread->ThreadId = ThreadId;
	RtlInitEmptyUnicodeString(&Thread->ThreadDescription, NULL, 0);

	//
	// Link with parent process's thread table
	//

	++ParentProcess->NumberOfThreads;

	NewProcessThreadsList = SafeReAlloc(
		ParentProcess->Threads,
		PKEXSRV_PER_CLIENT_THREAD_DATA,
		ParentProcess->NumberOfThreads);

	if (ParentProcess->Threads == NULL) {
		SafeFree(Thread);
		return STATUS_NO_MEMORY;
	}

	ParentProcess->Threads = NewProcessThreadsList;
	ThreadTableIndex = ParentProcess->NumberOfThreads - 1;
	ParentProcess->Threads[ThreadTableIndex] = Thread;
	Thread->ThreadTableIndex = ThreadTableIndex;

	//
	// Insert into the hash table
	//

	RtlInsertEntryHashTable(
		ProcessThreadTable,
		&Thread->HashTableEntry,
		ThreadId,
		NULL);

	if (ThreadDataOut) {
		*ThreadDataOut = Thread;
	}

	return STATUS_SUCCESS;
}

NTSTATUS UnregisterThread(
	IN	PKEXSRV_PER_CLIENT_THREAD_DATA	Thread,
	IN	BOOLEAN							ProcessTearDown)
{
	ASSERT (Thread != NULL);

	//
	// Delete from hash table
	//

	RtlRemoveEntryHashTable(ProcessThreadTable, &Thread->HashTableEntry, NULL);

	//
	// Delete from parent process's thread table, but only if the process
	// is not being destroyed. If the process is being destroyed as well then
	// there's no point in doing all this extra work.
	//

	unless (ProcessTearDown) {
		ULONG ThreadTableIndex;
		ULONG NumberOfThreads;
		PKEXSRV_PER_CLIENT_PROCESS_DATA ParentProcess;
		PKEXSRV_PER_CLIENT_THREAD_DATA LastProcessThread;
		PVOID ReallocReturnValue;

		ParentProcess = Thread->ParentProcess;
		ThreadTableIndex = Thread->ThreadTableIndex;
		NumberOfThreads = ParentProcess->NumberOfThreads;
		LastProcessThread = Thread->ParentProcess->Threads[NumberOfThreads - 1];

		ASSERT (ParentProcess->Threads[ThreadTableIndex] == Thread);

		if (Thread != LastProcessThread) {
			// Overwrite our to-be-destroyed thread table entry with that of
			// the last entry in the table. We also need to update the last
			// entry's cached thread table index to match.
			ParentProcess->Threads[ThreadTableIndex] = LastProcessThread;
			LastProcessThread->ThreadTableIndex = ThreadTableIndex;
		}

		--ParentProcess->NumberOfThreads;

		ReallocReturnValue = SafeReAllocEx(
			RtlProcessHeap(),
			HEAP_REALLOC_IN_PLACE_ONLY,
			ParentProcess->Threads,
			PKEXSRV_PER_CLIENT_THREAD_DATA,
			ParentProcess->NumberOfThreads);

		ASSERT (ReallocReturnValue != NULL);
	}
	
	//
	// Free memory for the thread structure
	//

	RtlFreeUnicodeString(&Thread->ThreadDescription);
	SafeFree(Thread);

	return STATUS_SUCCESS;
}

NTSTATUS DisconnectProcess(
	IN	PKEXSRV_PER_CLIENT_PROCESS_DATA	Process)
{
	NTSTATUS Status;
	ULONG Index;

	ASSERT (Process != NULL);
	
	NtClose(Process->PipeHandle);

	for (Index = 0; Index < Process->NumberOfThreads; ++Index) {
		Status = UnregisterThread(Process->Threads[Index], TRUE);
		ASSERT (NT_SUCCESS(Status));
	}

	RtlRemoveEntryHashTable(ProcessThreadTable, &Process->HashTableEntry, NULL);

	SafeFree(Process->Threads);
	SafeFree(Process);

	return STATUS_SUCCESS;
}