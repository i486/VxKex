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
//     vxiiduu              31-May-2026  Fix bug where WakeByAddressAll may only
//                                       wake one thread if that thread is the
//                                       first thread in the list.
//     vxiiduu              31-May-2026  Replace events with condition variables
//                                       (which internally use keyed events and
//                                       are much more efficient).
//     vxiiduu              01-Jun-2026  Change KexRtlpWakeByAddress to use a
//                                       shared SRW lock instead of exclusive.
//     vxiiduu              03-Jul-2026  Use keyed events instead of condition
//                                       variables and add spin count.
//     vxiiduu              05-Jul-2026  Prevent potential lock convoy issue
//                                       caused by acquiring an exclusive SRW
//                                       lock before bailing out wakers.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

// Spin count to use for multi-processor systems.
// Single-processor systems will not spin (i.e. spin count 0).
// 1024 is the spin count used by Windows 8 and Windows 10.
#define KEX_RTL_WAIT_ON_ADDRESS_MP_SPIN_COUNT 1024

typedef VOLATILE enum {
	SPINNING,
	WAITING,
	WAKING
} TYPEDEF_TYPE_NAME(KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE);

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK *PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK;

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK {
	//
	// Links to the next and previous RTL_WAIT_ON_ADDRESS_WAIT_BLOCK structure
	// in the linked list.
	//
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK Next;
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK Previous;

	//
	// The address that the thread is waiting on.
	//
	PVOID								Address;

	//
	// When this value is SPINNING, the waiter is spinning outside of the kernel,
	// constantly checking this value to see if it becomes equal to WAKING. If so,
	// the waiter can exit with STATUS_SUCCESS, and the waker will avoid calling
	// NtReleaseKeyedEvent (to avoid deadlock).
	//
	// When the spin-count has been exceeded, the waiter will use an interlocked
	// compare exchange to set it to WAITING. If a waker has set it to WAKING in
	// between the last spin iteration and the interlocked compare exchange, then
	// the waiter can exit with STATUS_SUCCESS, and the waker will also avoid
	// calling NtReleaseKeyedEvent.
	//
	// If a waker sets the state to WAKING and the old value was WAITING, then it
	// needs to call NtReleaseKeyedEvent.
	//
	KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE	State;
} TYPEDEF_TYPE_NAME(KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK);

typedef struct _KEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET {
	//
	// Locks the wait block list. Can be acquired in shared mode if no wait blocks
	// are going to be added or removed from the list.
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
// The size of this array can be freely adjusted here. The code that uses it
// will automatically adapt to the changed size.
// If you change it, it must remain a power of two. Otherwise, the code that
// hashes addresses will become larger and slower by more than a factor of 2.
// demo: https://godbolt.org/z/K9q9KheYj
//
STATIC KEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET KexRtlWaitOnAddressHashTable[128] = {0};

STATIC FORCEINLINE BOOLEAN KexRtlpEqualVolatileMemory(
	IN	VOLATILE VOID	*Address1,
	IN	PCVOID			Address2,
	IN	SIZE_T			Size)
{
	switch (Size) {
	case 1:		return (*(VOLATILE UCHAR *) Address1 == *(PCUCHAR) Address2);
	case 2:		return (*(VOLATILE USHORT *) Address1 == *(PCUSHORT) Address2);
	case 4:		return (*(VOLATILE ULONG *) Address1 == *(PCULONG) Address2);
	case 8:
#ifdef _M_X64
		return (*(VOLATILE ULONGLONG *) Address1 == *(PCULONGLONG) Address2);
#else
		{
			ULONGLONG CapturedValue;
			CapturedValue = InterlockedCompareExchange64((VOLATILE LONGLONG *) Address1, 0, 0);
			return (CapturedValue == *(PCULONGLONG) Address2);
		}
#endif
	default:	NOT_REACHED;
	}
}

STATIC FORCEINLINE PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET KexRtlpGetWoaHashBucket(
	IN	VOLATILE VOID	*Address)
{
	STATIC_ASSERT(IS_POWER_OF_TWO(ARRAYSIZE(KexRtlWaitOnAddressHashTable)));

	// This is the same as Windows 8 and Windows 10.
	return &KexRtlWaitOnAddressHashTable[
		(((ULONG) Address) >> 5) & (ARRAYSIZE(KexRtlWaitOnAddressHashTable) - 1)];
}

//
// This function must be called while the hash bucket is locked.
//
STATIC FORCEINLINE VOID KexRtlpAddWoaWaitBlock(
	IN OUT	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET	HashBucket,
	IN OUT	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK		WaitBlock)
{
	if (HashBucket->WaitBlocks == NULL) {
		// No existing wait blocks.
		WaitBlock->Previous = WaitBlock;
		WaitBlock->Next = WaitBlock;
		HashBucket->WaitBlocks = WaitBlock;
	} else {
		// One or more wait blocks already exist.
		// Add ourselves to the end of the list.
		WaitBlock->Previous = HashBucket->WaitBlocks->Previous;
		WaitBlock->Next = HashBucket->WaitBlocks;
		HashBucket->WaitBlocks->Previous->Next = WaitBlock;
		HashBucket->WaitBlocks->Previous = WaitBlock;
	}
}

//
// This function must also be called while the hash bucket is locked.
//
STATIC FORCEINLINE VOID KexRtlpRemoveWoaWaitBlock(
	IN	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET	HashBucket,
	IN	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK		WaitBlock)
{
	if (WaitBlock->Next == WaitBlock) {
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
KEXAPI NTSTATUS NTAPI RtlWaitOnAddress(
	IN	VOLATILE VOID	*Address,
	IN	PVOID			CompareAddress,
	IN	SIZE_T			AddressSize,
	IN	PLARGE_INTEGER	Timeout OPTIONAL)
{
	NTSTATUS Status;
	HANDLE GlobalKeyedEvent;
	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET HashBucket;
	KEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK WaitBlock;
	KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE OldState;
	ULONG SpinCount;

	ASSERT (Address != NULL);
	ASSERT (CompareAddress != NULL);

	ASSERT (AddressSize == 1 || AddressSize == 2 ||
			AddressSize == 4 || AddressSize == 8);

	if (AddressSize != 1 && AddressSize != 2 &&
		AddressSize != 4 && AddressSize != 8) {

		return STATUS_INVALID_PARAMETER;
	}

	//
	// Check that the values at *Address and *CompareAddress are the same
	// before continuing.
	//

	if (!KexRtlpEqualVolatileMemory(Address, CompareAddress, AddressSize)) {
		// Values are different, so we can return straight away.
		return STATUS_SUCCESS;
	}

	//
	// Set up our wait block.
	//

	WaitBlock.Address = (PVOID) Address;

	// Single-processor systems don't need to spin because that just wastes
	// CPU for no benefit.
	if (NtCurrentPeb()->NumberOfProcessors > 1) {
		WaitBlock.State = SPINNING;
		SpinCount = KEX_RTL_WAIT_ON_ADDRESS_MP_SPIN_COUNT;
	} else {
		WaitBlock.State = WAITING;
		SpinCount = 0;
	}

	//
	// Figure out which hash bucket we belong in.
	//

	HashBucket = KexRtlpGetWoaHashBucket(Address);

	//
	// Acquire bucket lock.
	//

	RtlAcquireSRWLockExclusive(&HashBucket->Lock);

	//
	// Make sure that *Address and *CompareAddress are still the same - if not,
	// release lock and exit. This check is re-done to avoid a lost wake (waker
	// trying to wake us up after changing *Address before we have added ourselves
	// to the wait block list, finding no threads, and then we go ahead and start
	// waiting - without anyone to wake us up).
	//

	if (!KexRtlpEqualVolatileMemory(Address, CompareAddress, AddressSize)) {
		RtlReleaseSRWLockExclusive(&HashBucket->Lock);
		return STATUS_SUCCESS;
	}

	//
	// The values are the same.
	// Add ourselves into the wait list and then release the bucket lock.
	//

	KexRtlpAddWoaWaitBlock(HashBucket, &WaitBlock);
	RtlReleaseSRWLockExclusive(&HashBucket->Lock);

	//
	// Spin if desired. This can help us to avoid a kernel transition in some
	// cases. The spin count is adjustable via the macro near the top of the file.
	//

	if (SpinCount == 0) {
		goto SkipSpin;
	}

	do {
		if (WaitBlock.State == WAKING) {
			Status = STATUS_SUCCESS;
			goto SkipKeyedEventWait;
		}

		--SpinCount;
		YieldProcessor();
	} until (SpinCount == 0);

	// SPINNING -> WAITING
	OldState = (KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE) InterlockedCompareExchange(
		(VOLATILE LONG *) &WaitBlock.State,
		WAITING,
		SPINNING);

	if (OldState == WAKING) {
		// Another thread woke us up in between the spin loop and the interlocked
		// compare exchange.
		Status = STATUS_SUCCESS;
		goto SkipKeyedEventWait;
	}

	//
	// Wait.
	//

SkipSpin:
	GlobalKeyedEvent = KexRtlpGetGlobalKeyedEvent();

	Status = NtWaitForKeyedEvent(
		GlobalKeyedEvent,
		&WaitBlock,
		FALSE,
		Timeout);

	ASSERT (Status == STATUS_SUCCESS || Status == STATUS_TIMEOUT);

	if (Status == STATUS_SUCCESS) {
		// Someone else woke us up.
		ASSERT (WaitBlock.State == WAKING);
	} else {
		//
		// We got STATUS_TIMEOUT or some error status.
		// Prevent wakers from trying to wake us up now that we've already woken
		// ourselves up.
		// This is done before removing our wait block from the list to avoid the
		// edge case where we block wakers in NtReleaseKeyedEvent for the entire
		// duration of acquiring the SRW lock to remove our wait block.
		//

		OldState = (KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE) InterlockedExchange(
			(VOLATILE LONG *) &WaitBlock.State,
			WAKING);

		ASSERT (OldState == WAITING || OldState == WAKING);

		if (OldState == WAKING) {
			NTSTATUS BailoutStatus;

			//
			// Another thread has committed to calling NtReleaseKeyedEvent to wake us up.
			// It is using a blocking call to NtReleaseKeyedEvent with no timeout, which
			// means we need to bail them out by calling NtWaitForKeyedEvent again.
			//
			// As a side effect, we'll set Status == STATUS_SUCCESS, to tell the caller
			// that we were in fact woken up by a waker.
			//

			BailoutStatus = NtWaitForKeyedEvent(
				GlobalKeyedEvent,
				&WaitBlock,
				FALSE,
				NULL);

			ASSERT (BailoutStatus == STATUS_SUCCESS);

			if (Status == STATUS_TIMEOUT) {
				// We timed out and got woken at the "same" time - we'll call that a
				// success.
				Status = STATUS_SUCCESS;
			}
		}
	}

	//
	// Remove ourselves from the wait list.
	//

SkipKeyedEventWait:
	RtlAcquireSRWLockExclusive(&HashBucket->Lock);
	KexRtlpRemoveWoaWaitBlock(HashBucket, &WaitBlock);
	RtlReleaseSRWLockExclusive(&HashBucket->Lock);

	// The state machine should always end at WAKING.
	ASSERT (WaitBlock.State == WAKING);

	return Status;
}

//
// This function is the implementation of the WakeByAddressSingle and
// WakeByAddressAll extended APIs.
// See RtlWakeAddressSingle, RtlWakeAddressAll, and the MSDN
// docs.
//
STATIC VOID KexRtlpWakeByAddress(
	IN	PVOID			Address,
	IN	BOOLEAN			WakeAll)
{
	HANDLE GlobalKeyedEvent;
	PKEX_RTL_WAIT_ON_ADDRESS_HASH_BUCKET HashBucket;
	PKEX_RTL_WAIT_ON_ADDRESS_WAIT_BLOCK WaitBlock;
	PVOID KeysToRelease[256];
	ULONG NumberOfKeysToRelease;
	ULONG Index;
	BOOLEAN NeedToWakeMoreThreads;

	HashBucket = KexRtlpGetWoaHashBucket(Address);

WakeMoreThreads:
	NumberOfKeysToRelease = 0;
	NeedToWakeMoreThreads = FALSE;

	RtlAcquireSRWLockShared(&HashBucket->Lock);

	if (HashBucket->WaitBlocks == NULL) {
		// No threads to wake.
		RtlReleaseSRWLockShared(&HashBucket->Lock);
		return;
	}

	//
	// Traverse the list starting from the beginning.
	// The API documentation from MS states that threads are woken starting
	// from the one that first started waiting.
	//

	WaitBlock = HashBucket->WaitBlocks;

	do {
		if (WaitBlock->Address == Address && WaitBlock->State != WAKING) {
			KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE OldState;

			if (NumberOfKeysToRelease >= ARRAYSIZE(KeysToRelease)) {
				//
				// This is a pathological case where a ton of threads are waiting
				// on the same address. We don't have enough buffer space to record
				// all of their keyed event keys, so we'll have to set a boolean
				// which indicates that we have to go around and wake all the
				// remaining threads (if any).
				//

				ASSERT (WakeAll);
				STATIC_ASSERT (ARRAYSIZE(KeysToRelease) > 1);

				NeedToWakeMoreThreads = TRUE;
				break;
			}

			//
			// Attempt to commit to waking up this thread.
			// We'll set WaitBlock->State to WAKING using an interlocked operation to
			// signal to both the waiting thread and other waker threads that we're
			// the one who is going to wake this waiter up.
			//
			// The interlocked operation is necessary even while we're holding the
			// bucket lock, because we're holding it in shared mode - other wakers
			// can execute concurrently.
			//

			OldState = (KEX_RTL_WAIT_ON_ADDRESS_WAIT_STATE) InterlockedExchange(
				(VOLATILE LONG *) &WaitBlock->State,
				WAKING);

			if (OldState == SPINNING) {
				//
				// We've woken up a spinning waiter. That thread has now already
				// woken and we should not call NtReleaseKeyedEvent (we simply
				// won't record the keyed-event key in our list).
				//

				if (!WakeAll) {
					break;
				}
			} else if (OldState == WAITING) {
				//
				// We're now responsible for waking up this waiter by signaling
				// the keyed event.
				// Record the address of the wait block as the key for the keyed event.
				//

				KeysToRelease[NumberOfKeysToRelease] = WaitBlock;
				++NumberOfKeysToRelease;

				if (!WakeAll) {
					// we only want to wake this one
					break;
				}
			} else {
				// If OldState is neither SPINNING nor WAITING, that means another
				// waker has set it to WAKING first and that thread is responsible
				// for waking the waiter.
				ASSERT (OldState == WAKING);
			}
		}

		WaitBlock = WaitBlock->Next;
	} until (WaitBlock == HashBucket->WaitBlocks);

	RtlReleaseSRWLockShared(&HashBucket->Lock);

	ASSERT (WakeAll || (NumberOfKeysToRelease <= 1));

	//
	// Wake up all the threads that we've committed to wake.
	//

	GlobalKeyedEvent = KexRtlpGetGlobalKeyedEvent();

	for (Index = 0; Index < NumberOfKeysToRelease; ++Index) {
		NTSTATUS Status;

		Status = NtReleaseKeyedEvent(
			GlobalKeyedEvent,
			KeysToRelease[Index],
			FALSE,
			NULL);

		ASSERT (Status == STATUS_SUCCESS);
	}

	//
	// If there could be more threads waiting on this address that we need to wake,
	// go around and do that.
	//

	if (NeedToWakeMoreThreads) {
		goto WakeMoreThreads;
	}
}

KEXAPI VOID NTAPI RtlWakeAddressSingle(
	IN	PVOID			Address)
{
	KexRtlpWakeByAddress(Address, FALSE);
}

KEXAPI VOID NTAPI RtlWakeAddressAll(
	IN	PVOID			Address)
{
	KexRtlpWakeByAddress(Address, TRUE);
}