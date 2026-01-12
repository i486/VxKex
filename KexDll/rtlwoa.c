///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     rtlwoa.c
//
// Abstract:
//
//     Implementation of WaitOnAddress and friends.
//
// Author:
//
//     vxiiduu (11-Feb-2024)
//
// Revision History:
//
//     vxiiduu              11-Feb-2024  Initial creation.
//     vxiiduu              15-Feb-2024  Fix a typing error.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK *PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK;

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK {
	//
	// The address that the thread is waiting on.
	//
	PVOID								Address;

	//
	// The event handle upon which this thread is waiting.
	//
	HANDLE								EventHandle;

	//
	// Links to the next and previous RTL_WAIT_ON_ADDRESS_WAIT_BLOCK structure
	// in the linked list.
	//
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK Next;
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK Previous;
} TYPEDEF_TYPE_NAME(KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK);

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET {
	//
	// Locks the hash bucket. Threads calling WoA or one of the wake functions
	// for a particular address (range) will be blocked until all pending linked
	// list operations are complete.
	//
	RTL_SRWLOCK							Lock;

	//
	// This item will be NULL if no threads are waiting on the addresses that
	// fall under this hash bucket.
	//
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK	WaitBlocks;
} TYPEDEF_TYPE_NAME(KEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET);

//
// 128 entries is what's used in Windows 8.
// Perhaps it's a little excessive.
// The size of this array can be freely adjusted here. The code that uses it
// will automatically adapt to the changed size.
// If you change it, it must remain a power of two. Otherwise, the code that
// hashes addresses will become larger and slower by more than a factor of 2.
// demo: https://godbolt.org/z/K9q9KheYj
//
STATIC KEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET KexRtlWaitOnAddressHashTable[32] = {0};

#pragma warning(disable:4715) // not all control paths return a value
STATIC INLINE BOOLEAN KexRtlpEqualVolatileMemory(
	IN	VOLATILE VOID	*Address1,
	IN	PCVOID			Address2,
	IN	SIZE_T			Size)
{
	switch (Size) {
	case 1:		return (*(PUCHAR) Address1 == *(PUCHAR) Address2);
	case 2:		return (*(PUSHORT) Address1 == *(PUSHORT) Address2);
	case 4:		return (*(PULONG) Address1 == *(PULONG) Address2);
	case 8:		return (*(PULONGLONG) Address1 == *(PULONGLONG) Address2);
	default:	ASSUME (FALSE);
	}
}
#pragma warning(default:4715)

STATIC FORCEINLINE PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET KexRtlpGetWoaHashBucket(
	IN	VOLATILE VOID	*Address)
{
	return &KexRtlWaitOnAddressHashTable[
		(((ULONG_PTR) Address) >> 4) % ARRAYSIZE(KexRtlWaitOnAddressHashTable)];
}

//
// This function must be called while the hash bucket is locked.
//
STATIC INLINE VOID KexRtlpRemoveWoaWaitBlock(
	IN	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET	HashBucket,
	IN	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK		WaitBlock)
{
	if (WaitBlock->Previous == NULL) {
		// this is a signal that we do NOT touch anything
		return;
	} else if (WaitBlock->Next == WaitBlock) {
		// this wait block is the only entry in the list
		HashBucket->WaitBlocks = NULL;
	} else {
		// there's more than one wait block in the list so we have to
		// remove it "properly"

		if (WaitBlock == HashBucket->WaitBlocks) {
			// this wait block is at the beginning so we have to update
			// the pointer in the hash bucket
			HashBucket->WaitBlocks = WaitBlock->Next;
		}

		WaitBlock->Previous->Next = WaitBlock->Next;
		WaitBlock->Next->Previous = WaitBlock->Previous;
	}
}

//
// This function is the implementation of the WaitOnAddress extended API.
// See WaitOnAddress in KxBase\synch.c and the MSDN docs.
//
KEXAPI NTSTATUS NTAPI KexRtlWaitOnAddress(
	IN	VOLATILE VOID	*Address,
	IN	PVOID			CompareAddress,
	IN	SIZE_T			AddressSize,
	IN	PLARGE_INTEGER	Timeout OPTIONAL)
{
	NTSTATUS Status;
	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET HashBucket;
	KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK WaitBlock;

	ASSERT (PopulationCount(ARRAYSIZE(KexRtlWaitOnAddressHashTable)) == 1);
	ASSERT (Address != NULL);
	ASSERT (CompareAddress != NULL);

	ASSERT (AddressSize == 1 || AddressSize == 2 ||
			AddressSize == 4 || AddressSize == 8);

	if (AddressSize != 1 && AddressSize != 2 &&
		AddressSize != 4 && AddressSize != 8) {

		return STATUS_INVALID_PARAMETER;
	}

	//
	// Figure out which hash bucket we belong in.
	//

	HashBucket = KexRtlpGetWoaHashBucket(Address);

	RtlAcquireSRWLockExclusive(&HashBucket->Lock);

	//
	// Check that the values at *Address and *CompareAddress are the same
	// before continuing.
	//

	if (!KexRtlpEqualVolatileMemory(Address, CompareAddress, AddressSize)) {
		// Values are different, so we can return straight away.
		RtlReleaseSRWLockExclusive(&HashBucket->Lock);
		return STATUS_SUCCESS;
	}

	//
	// The values are different.
	// Create the event upon which we will wait.
	//

	Status = NtCreateEvent(
		&WaitBlock.EventHandle,
		SYNCHRONIZE | EVENT_MODIFY_STATE,
		NULL,
		NotificationEvent,
		FALSE);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		RtlReleaseSRWLockExclusive(&HashBucket->Lock);
		return Status;
	}

	//
	// Add ourselves into the linked list.
	//

	WaitBlock.Address = (PVOID) Address;

	if (HashBucket->WaitBlocks == NULL) {
		// No existing wait blocks.
		WaitBlock.Previous = &WaitBlock;
		WaitBlock.Next = &WaitBlock;
		HashBucket->WaitBlocks = &WaitBlock;
	} else {
		// One or more wait blocks already exist.
		// Add ourselves to the end of the list.
		WaitBlock.Previous = HashBucket->WaitBlocks->Previous;
		WaitBlock.Next = HashBucket->WaitBlocks;
		HashBucket->WaitBlocks->Previous->Next = &WaitBlock;
		HashBucket->WaitBlocks->Previous = &WaitBlock;
	}

	//
	// Wait.
	//

	RtlReleaseSRWLockExclusive(&HashBucket->Lock);

	Status = NtWaitForSingleObject(
		WaitBlock.EventHandle,
		FALSE,
		Timeout);

	ASSERT (NT_SUCCESS(Status));

	//
	// The thread that woke us up is in charge of removing us from the
	// list. However, if we timed out, there is no such thread, so if the
	// status from NtWaitForSingleObject is STATUS_TIMEOUT or some other
	// error code, we have to do that ourselves.
	//

	if (Status == STATUS_TIMEOUT || !NT_SUCCESS(Status)) {
		RtlAcquireSRWLockExclusive(&HashBucket->Lock);
		KexRtlpRemoveWoaWaitBlock(HashBucket, &WaitBlock);
		RtlReleaseSRWLockExclusive(&HashBucket->Lock);
	}

	NtClose(WaitBlock.EventHandle);
	return Status;
}

//
// This function is the implementation of the WakeByAddressSingle and
// WakeByAddressAll extended APIs.
// See KexRtlWakeByAddressSingle, KexRtlWakeByAddressAll, and the MSDN
// docs.
//
STATIC VOID KexRtlpWakeByAddress(
	IN	PVOID			Address,
	IN	BOOLEAN			WakeAll)
{
	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET HashBucket;
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK WaitBlock;

	HashBucket = KexRtlpGetWoaHashBucket(Address);

	RtlAcquireSRWLockExclusive(&HashBucket->Lock);

	if (HashBucket->WaitBlocks == NULL) {
		RtlReleaseSRWLockExclusive(&HashBucket->Lock);
		return;
	}

	//
	// Traverse the list starting from the beginning.
	// The API documentation from MS states that threads are woken starting
	// from the one that first started waiting.
	//

	WaitBlock = HashBucket->WaitBlocks;

	while (TRUE) {
		PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK NextWaitBlock;

		NextWaitBlock = WaitBlock->Next;

		if (WaitBlock->Address == Address) {
			NTSTATUS Status;

			KexRtlpRemoveWoaWaitBlock(HashBucket, WaitBlock);

			//
			// Signal to future invocations of KexRtlpRemoveWoaWaitBlock to tell it
			// that we've already removed this block from the list.
			// We need to do this due to the possibility of the following situation:
			//   1. A thread calls WaitOnAddress with a timeout.
			//   2. NtWaitForSingleObject returns with STATUS_TIMEOUT.
			//   3. In between the STATUS_TIMEOUT return and when that thread removes
			//      itself from the list, someone calls WakeByAddress and we get to
			//      this point in the code.
			//   4. We remove the list entry from the list.
			//   5. The other thread that timed out removes the list entry from the
			//      list a 2nd time and potentially causes corruption.
			//
			// It's a rare edge case but the cost to eliminate it is luckily very small.
			//

			WaitBlock->Previous = NULL;
			
			//
			// Wake up the thread.
			//

			Status = NtSetEvent(WaitBlock->EventHandle, NULL);
			ASSERT (NT_SUCCESS(Status));

			//
			// After the call to NtSetEvent, the contents of WaitBlock should be
			// considered undefined, since when the KexRtlWaitOnAddress call returns
			// the contents of the stack are no longer defined.
			//

			if (!WakeAll) {
				// we only want to wake this one
				break;
			}
		}

		if (HashBucket->WaitBlocks == NULL || NextWaitBlock == HashBucket->WaitBlocks) {
			break;
		}

		WaitBlock = NextWaitBlock;
	}

	RtlReleaseSRWLockExclusive(&HashBucket->Lock);
}

KEXAPI VOID NTAPI KexRtlWakeAddressSingle(
	IN	PVOID			Address)
{
	KexRtlpWakeByAddress(Address, FALSE);
}

KEXAPI VOID NTAPI KexRtlWakeAddressAll(
	IN	PVOID			Address)
{
	KexRtlpWakeByAddress(Address, TRUE);
}