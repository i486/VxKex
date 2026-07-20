///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntalrtid.c
//
// Abstract:
//
//     Implementations of NtAlertThreadByThreadId and NtWaitForAlertByThreadId.
//
//     These APIs are designed for minimum per-thread initialization cost, since
//     they are not called by many applications. They are not designed for high
//     performance of the APIs themselves.
//
//     Since these are emulations of system calls, they must be tolerant of any
//     kind of exception (e.g. access violation) and return the appropriate status
//     code rather than crashing. They must be tolerant of threads being terminated
//     or exiting.
//
//     Known apps that use these: Zig. Zig tends to call each of these functions
//     a bit under 4,000 times while compiling a hello world app.
//
// Author:
//
//     vxiiduu (05-Jul-2026)
//
// Revision History:
//
//     vxiiduu               05-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Convenience macros related to TebExtension->AlertByThreadIdState.
//
// AlertByThreadIdState is a packed value consisting of the thread ID (which is
// guaranteed to have the lower 2 bits zero) and a state (a value between 0 and
// 3). We pack the thread ID together with the state, instead of just using the
// state by itself, to guard against this (very rare) scenario:
//
//   - a waker attempt to wake a thread.
//   - the waker gets preempted. the original target thread exits and a new one
//     is created with the same TEB address.
//   - wrong thread is woken.
//
// It also makes it less likely that any random value will be misinterpreted as
// a valid ABTI state in the case that some other memory happens to get mapped
// into the zone usually reserved for TEBs, since 0 is never a valid combination
// of thread ID and ABTI state.
//

#define MAKE_ABTI(ThreadId, State) (((LONG) (ThreadId)) | ((LONG) (State)))
#define ABTI_STATE(Abti) ((Abti) & OBJ_HANDLE_TAGBITS)
#define ABTI_THREAD_ID(Abti) ((HANDLE) (((ULONG) (Abti)) & (~OBJ_HANDLE_TAGBITS)))

C_ASSERT (OBJ_HANDLE_TAGBITS == 3);

//
// WAITERS drive these transitions:
//   Pending	-> Normal		When trying to wait but finding a pending alert.
//   Normal		-> Waiting		When waiting and no alert is pending.
//   Waiting	-> Waking		When timing out.
//   Waking		-> Normal		Upon exit of NtWaitForAlertByThreadId.
//
// WAKERS drive these transitions:
//   Normal		-> Pending		When trying to wake but thread is not waiting.
//   Waiting    -> Waking		When trying to wake a thread that is waiting.
//
enum {
	// Normal state. No alert is pending and the thread is not waiting for alert by
	// thread ID.
	NORMAL		= 0,

	// Alert is pending. The next time the waiter tries to wait for alert by thread
	// ID, it will reset state to normal and continue execution without blocking.
	PENDING		= 1,

	// Waiter is waiting or about to wait (using NtWaitForKeyedEvent). Waker must
	// call NtReleaseKeyedEvent.
	WAITING		= 2,

	// Waker is waking or about to wake (using NtReleaseKeyedEvent). If timed out,
	// the waiter must bail out the waker using NtWaitForKeyedEvent again.
	WAKING		= 3
};

//
// Thread attach callback.
// This routine should be as short and fast as possible since it's called for every
// thread creation. Don't want to pay any real initialization cost for this API
// since only Zig uses it.
//
// Call this for DLL_PROCESS_ATTACH as well as DLL_THREAD_ATTACH, since
// DLL_PROCESS_ATTACH is the one we get for the loader initialization thread.
//

FORCEINLINE VOID KexAlertByThreadIdThreadAttach(
	VOID)
{
	ASSERT (KexCurrentTebExtension()->AlertByThreadIdState == 0);

	KexCurrentTebExtension()->AlertByThreadIdState = MAKE_ABTI(
		NtCurrentTeb()->ClientId.UniqueThread,
		NORMAL);
}

//
// NtWaitForAlertByThreadId does not actually use the Hint parameter in any
// meaningful way. It is essentially just a fancy keyed event which does not
// require a handle *and*, I believe, buffers up to one wake.
//
// The Hint parameter is merely stored inside the kernel for debugging
// purposes.
//
// https://dennisbabkin.com/blog/?t=how-to-put-thread-into-kernel-wait-and-to-wake-it-by-thread-id
//
// This function may return STATUS_ALERTED or STATUS_TIMEOUT.
// It should not return any other status code during normal operation.
// Zig assumes that those two status codes are the only ones returned, and
// marks any other status codes as "unreachable".
//
KEXAPI NTSTATUS NTAPI NtWaitForAlertByThreadId(
	IN	PVOID			Hint OPTIONAL,
	IN	PLARGE_INTEGER	Timeout OPTIONAL)
{
	NTSTATUS Status;
	HANDLE GlobalKeyedEvent;
	PTEB Teb;
	PKEX_TEB_EXTENSION TebExtension;
	LARGE_INTEGER CapturedTimeout;
	LONG OldState;

	Teb = NtCurrentTeb();
	TebExtension = KexCurrentTebExtension();

	// this should've been set on thread attach
	// if it's different it means thread attach wasn't called due to a bug,
	// or someone corrupted the memory
	ASSERT (ABTI_THREAD_ID(TebExtension->AlertByThreadIdState) == Teb->ClientId.UniqueThread);

	//
	// Capture parameters and if they are inaccessible, return an appropriate status
	// code to the user.
	//

	if (Timeout) {
		try {
			CapturedTimeout = *Timeout;
			Timeout = &CapturedTimeout;
		} except (EXCEPTION_EXECUTE_HANDLER) {
			return GetExceptionCode();
		}
	}

	//
	// Perform the Pending -> Normal transition if someone else already alerted us
	// while we weren't waiting.
	//

	if (ABTI_STATE(TebExtension->AlertByThreadIdState) == PENDING) {
		// Use a normal, non-interlocked write to the thread ID state.
		// Wakers are forbidden from setting state to anything else if an alert
		// is already PENDING.
HandlePending:
		TebExtension->AlertByThreadIdState = MAKE_ABTI(
			Teb->ClientId.UniqueThread,
			NORMAL);

		return STATUS_ALERTED;
	}

	//
	// The Windows kernel returns STATUS_TIMEOUT instantly if the timeout parameter
	// is present, has a zero timeout, and we weren't already in the PENDING state.
	//

	if (Timeout && Timeout->QuadPart == 0) {
		return STATUS_TIMEOUT;
	}

	//
	// Perform the Normal -> Waiting transition.
	//

	OldState = InterlockedCompareExchange(
		&TebExtension->AlertByThreadIdState,
		MAKE_ABTI(Teb->ClientId.UniqueThread, WAITING),
		MAKE_ABTI(Teb->ClientId.UniqueThread, NORMAL));

	ASSERT (ABTI_THREAD_ID(OldState) == Teb->ClientId.UniqueThread);

	if (ABTI_STATE(OldState) == PENDING) {
		// handle the race where someone has alerted us between when we first checked
		// and now.
		goto HandlePending;
	}

	ASSERT (ABTI_STATE(OldState) == NORMAL);

	//
	// We're now committed to waiting.
	// Go ahead and call NtWaitForKeyedEvent.
	//

	GlobalKeyedEvent = KexRtlpGetGlobalKeyedEvent();

	Status = NtWaitForKeyedEvent(
		GlobalKeyedEvent,
		(PVOID) &TebExtension->AlertByThreadIdState,
		FALSE,
		Timeout);

	ASSERT (Status == STATUS_SUCCESS || Status == STATUS_TIMEOUT);

	if (Status == STATUS_SUCCESS) {
		// Someone else woke us up.
		ASSERT (ABTI_STATE(TebExtension->AlertByThreadIdState) == WAKING);
		Status = STATUS_ALERTED;
	} else {
		//
		// No one else woke us up.
		// We either timed out or NtWaitForKeyedEvent failed.
		// Perform Waiting -> Waking transition.
		//

		OldState = InterlockedExchange(
			&TebExtension->AlertByThreadIdState,
			MAKE_ABTI(Teb->ClientId.UniqueThread, WAKING));

		ASSERT (ABTI_THREAD_ID(OldState) == Teb->ClientId.UniqueThread);
		ASSERT (ABTI_STATE(OldState) == WAITING || ABTI_STATE(OldState) == WAKING);

		if (ABTI_STATE(OldState) == WAKING) {
			NTSTATUS BailoutStatus;

			//
			// Someone tried to wake us up but that attempt wasn't what actually woke
			// us up. Bail out the waker.
			//
			// This can deadlock if the waker gets terminated. I don't see a good
			// way of stopping that from happening in a proper way, since we can't
			// really tell the difference between a now-dead terminated waker and
			// a waker that has been preempted for some time.
			//

			BailoutStatus = NtWaitForKeyedEvent(
				GlobalKeyedEvent,
				(PVOID) &TebExtension->AlertByThreadIdState,
				FALSE,
				NULL);

			ASSERT (BailoutStatus == STATUS_SUCCESS);

			if (Status == STATUS_TIMEOUT) {
				Status = STATUS_ALERTED;
			}
		}
	}

	// Perform the WAKING -> NORMAL transition.
	// This is done as non-interlocked since wakers aren't allowed to change
	// state once we reach WAKING state.
	ASSERT (ABTI_STATE(TebExtension->AlertByThreadIdState) == WAKING);
	TebExtension->AlertByThreadIdState = MAKE_ABTI(Teb->ClientId.UniqueThread, NORMAL);

	ASSERT (Status == STATUS_ALERTED || (Timeout && Status == STATUS_TIMEOUT));
	return Status;
}

//
// On Windows 8+, this function returns:
//
//   - STATUS_ACCESS_DENIED if the thread is in another process.
//   - STATUS_INVALID_CID if the thread ID is invalid.
//   - STATUS_SUCCESS if successful.
//
// This is supposed to be a non-blocking function, unlike keyed events.
//
KEXAPI NTSTATUS NTAPI NtAlertThreadByThreadId(
	IN	HANDLE	UniqueThread)
{
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID ClientId;
	HANDLE ThreadHandle;
	THREAD_BASIC_INFORMATION BasicInformation;
	PTEB Teb;
	PTEB TargetTeb;
	PKEX_TEB_EXTENSION TargetTebExtension;
	HANDLE GlobalKeyedEvent;

	//
	// Get a handle to the target thread.
	//

	InitializeObjectAttributes(
		&ObjectAttributes,
		NULL,
		0,
		NULL,
		NULL);

	// UniqueProcess must be NULL. If we set it to our own process ID, then it
	// becomes impossible to distinguish between a valid thread ID that is in
	// another process vs. a non-existent thread ID.
	ClientId.UniqueProcess	= NULL;
	ClientId.UniqueThread	= UniqueThread;

	Status = NtOpenThread(
		&ThreadHandle,
		THREAD_QUERY_INFORMATION,
		&ObjectAttributes,
		&ClientId);

	ASSERT (Status == STATUS_SUCCESS ||
			Status == STATUS_INVALID_PARAMETER ||
			Status == STATUS_ACCESS_DENIED);

	if (!NT_SUCCESS(Status)) {
		if (Status == STATUS_INVALID_PARAMETER) {
			// NtOpenThread returns STATUS_INVALID_PARAMETER when the thread id
			// is not found with UniqueProcess == NULL.
			Status = STATUS_INVALID_CID;
		}

		return Status;
	}

	//
	// Query the thread's basic information.
	// This tells us which process the thread is in (so that we can return the
	// appropriate status code when it's in a different process), and most
	// importantly tells us the address of the TEB of that thread.
	//

	Status = NtQueryInformationThread(
		ThreadHandle,
		ThreadBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL);

	ASSERT (NT_SUCCESS(Status));
	SafeClose(ThreadHandle);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Teb = NtCurrentTeb();

	if (BasicInformation.ClientId.UniqueProcess != Teb->ClientId.UniqueProcess) {
		// target thread was in a different process
		return STATUS_ACCESS_DENIED;
	}

	TargetTeb = BasicInformation.TebBaseAddress;
	TargetTebExtension = (PKEX_TEB_EXTENSION) &TargetTeb[1];

	//
	// We now have the address of the target TEB extension which contains the
	// ABTI state. Keep in mind that all accesses to the target TEB extension
	// must be protected by SEH because the target thread could exit at any time,
	// causing the target's TEB and TEB extension to be deallocated.
	//
	// Note that the target's TEB will not be deallocated if the thread is
	// *terminated* (the memory is simply leaked in that case).
	//

	try {
		LONG OldState;
		//
		// Try the Normal -> Pending transition first.
		//

RetryCompareAndSwapLoop:
		OldState = InterlockedCompareExchange(
			&TargetTebExtension->AlertByThreadIdState,
			MAKE_ABTI(UniqueThread, PENDING),
			MAKE_ABTI(UniqueThread, NORMAL));

		if (ABTI_THREAD_ID(OldState) != UniqueThread) {
			// The thread exited, its TEB was deallocated and replaced with a
			// different memory mapping, probably the TEB of a new thread.
			// Or, the value of the ABTI state got corrupted. We can't really
			// determine which one of the two happened.
			KexDebugCheckpoint();
			return STATUS_INVALID_CID;
		}

		if (ABTI_STATE(OldState) == NORMAL || ABTI_STATE(OldState) == PENDING) {
			// We changed the state from NORMAL to PENDING, or the state was
			// already PENDING. We can exit now.
			return STATUS_SUCCESS;
		}

		//
		// Try the Waiting -> Waking transition.
		//

		if (ABTI_STATE(OldState) == WAKING) {
			// The state was already WAKING - the thread has already awakened or
			// will wake up soon.
			return STATUS_SUCCESS;
		}

		OldState = InterlockedCompareExchange(
			&TargetTebExtension->AlertByThreadIdState,
			MAKE_ABTI(UniqueThread, WAKING),
			MAKE_ABTI(UniqueThread, WAITING));

		if (ABTI_THREAD_ID(OldState) != UniqueThread) {
			KexDebugCheckpoint();
			return STATUS_INVALID_CID;
		}

		if (ABTI_STATE(OldState) == NORMAL) {
			// Thread has gone back to NORMAL.
			// Go back and try again.

			Status = NtYieldExecution();

			if (Status == STATUS_NO_YIELD_PERFORMED) {
				YieldProcessor();
			}

			goto RetryCompareAndSwapLoop;
		}

		if (ABTI_STATE(OldState) == PENDING || ABTI_STATE(OldState) == WAKING) {
			return STATUS_SUCCESS;
		}

		ASSERT (ABTI_STATE(OldState) == WAITING);
	} except (GetExceptionCode() != STATUS_BREAKPOINT &&
			  GetExceptionCode() != STATUS_ASSERTION_FAILURE) {

		// thread and its TEB no longer exists
		return STATUS_INVALID_CID;
	}

	//
	// We've now committed to waking up the target thread.
	//

	GlobalKeyedEvent = KexRtlpGetGlobalKeyedEvent();

	Status = NtReleaseKeyedEvent(
		GlobalKeyedEvent,
		(PVOID) &TargetTebExtension->AlertByThreadIdState,
		FALSE,
		NULL);

	ASSERT (Status == STATUS_SUCCESS);

	return Status;
}