///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     gaiasync.c
//
// Abstract:
//
//     Implements asynchronous GetAddrInfoExW.
//
// Author:
//
//     vxiiduu (30-Jun-2026)
//
// Revision History:
//
//     vxiiduu              30-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxnetp.h"

//
// Data Types used for this feature.
//

typedef enum {
	GaiWaiting = 0,
	GaiCompletionInProgress,
	GaiCompleted
} TYPEDEF_TYPE_NAME(KXNETP_GAI_STATE);

typedef struct _KXNETP_GAI_ASYNC_CONTEXT *PKXNETP_GAI_ASYNC_CONTEXT;

typedef struct _KXNETP_GAI_ASYNC_CONTEXT {
	//
	// Starts off at GaiWaiting.
	// When the operation completes, times out, or gets canceled, this gets atomically
	// set to GaiCompletionInProgress by InterlockedCompareExchange. The first thread
	// that sets it to GaiCompletionInProgress is then responsible for signaling
	// Overlapped->hEvent or calling the completion routine. That thread then must set
	// it to GaiCompleted.
	//
	// If the thread that sets GaiCompleted is not the threadpool callback, then it must
	// call WakeByAddressSingle to signal GetAddrInfoExWAsyncThreadpoolCallback to wake
	// up (if it is sleeping).
	//
	// In that case, the threadpool callback must wait on this value using WaitOnAddress
	// until it becomes GaiCompleted before returning (which automatically frees the
	// structure).
	//

	VOLATILE KXNETP_GAI_STATE			State;

	union {
		struct {
			BOOLEAN						NamePresent:1;
			BOOLEAN						ServiceNamePresent:1;
			BOOLEAN						NspGuidPresent:1;
			BOOLEAN						HintsPresent:1;
			BOOLEAN						TimeoutPresent:1;
		};

		UCHAR							Flags;
	};

	// Copies of information passed to Ext_GetAddrInfoExW.
	WCHAR								Name[256];
	WCHAR								ServiceName[256];
	ULONG								Namespace;
	GUID								NspGuid;
	ADDRINFOEXW							Hints;
	PPADDRINFOEXW						AddrInfoResult;
	TIMEVAL								Timeout;
	LPOVERLAPPED						Overlapped;
	LPLOOKUPSERVICE_COMPLETION_ROUTINE	CompletionRoutine OPTIONAL;

	// Timer used to implement support for the Timeout parameter.
	// Will be NULL if no timeout was supplied by the caller.
	HANDLE								TimeoutTimer OPTIONAL;
} TYPEDEF_TYPE_NAME(KXNETP_GAI_ASYNC_CONTEXT);

typedef struct {
	// Initially points to the async context in the stack of the thread that called
	// Ext_GetAddrInfoExW.
	// After ThreadpoolCallbackReady is TRUE, points to the async context in the stack
	// of the threadpool callback.
	PKXNETP_GAI_ASYNC_CONTEXT	AsyncContext;

	// Whether the threadpool callback has finished copying AsyncContext into its
	// own stack.
	VOLATILE BOOLEAN			ThreadpoolCallbackReady;
} TYPEDEF_TYPE_NAME(KXNETP_GAI_ASYNC_HANDOFF);

//
// Function Definitions
//

//
// Call to complete, cancel, or time out a asynchronous GetAddrInfoEx operation.
// Returns TRUE if the operation was completed.
// Returns FALSE if the operation was not completed due to a multithreaded race.
//
STATIC BOOLEAN GetAddrInfoExCompleteAsyncOperation(
	IN OUT	PKXNETP_GAI_ASYNC_CONTEXT	AsyncContext,
	IN		PADDRINFOEXW				AddrInfoResult OPTIONAL,
	IN		INT							GaiReturnCode,
	IN		BOOLEAN						SignalThreadpoolCallback,
	IN		BOOLEAN						SynchronousTimerDeletion)
{
	BOOL Success;
	KXNETP_GAI_STATE OldState;

	//
	// Set state to completion in progress, unless someone else has already won the
	// race and initiated a cancellation or timeout.
	//

	OldState = (KXNETP_GAI_STATE) InterlockedCompareExchange(
		(VOLATILE LONG *) &AsyncContext->State,
		GaiCompletionInProgress,
		GaiWaiting);

	if (OldState != GaiWaiting) {
		// We lost.
		return FALSE;
	}

	//
	// We're now in charge of either calling the completion routine or signaling the
	// event in the OVERLAPPED structure.
	//

	ASSERT (AsyncContext->State == GaiCompletionInProgress);

	*AsyncContext->AddrInfoResult = AddrInfoResult;
	AsyncContext->Overlapped->Internal = GaiReturnCode;
	AsyncContext->Overlapped->Pointer = AsyncContext->AddrInfoResult;

	//
	// Delete the timeout timer if there is one present.
	// This has to be done before the completion routine is called or event is
	// signaled.
	// We will use a blocking delete (INVALID_HANDLE_VALUE) if the call to this
	// function did not originate from a timer callback, to make extra sure that
	// no timeout callback can run after we've completed the async operation.
	//

	if (AsyncContext->TimeoutTimer) {
		ULONG RetryCount;

		RetryCount = 0;

RetryDeleteTimer:
		Success = DeleteTimerQueueTimer(
			NULL,
			AsyncContext->TimeoutTimer,
			SynchronousTimerDeletion ? INVALID_HANDLE_VALUE : NULL);

		++RetryCount;

		if (!Success) {
			ULONG LastError;

			LastError = GetLastError();

			if (LastError == ERROR_IO_PENDING) {
				// "If the error code is ERROR_IO_PENDING, it is not necessary
				// to call this function again."
				Success = TRUE;
			} else {
				if (RetryCount < 100) {
					// "For any other error, you should retry the call."
					goto RetryDeleteTimer;
				}
			}
		}

		ASSERT (Success);
		AsyncContext->TimeoutTimer = NULL;
	}

	//
	// Call completion routine or signal event.
	//

	if (AsyncContext->CompletionRoutine) {
		AsyncContext->CompletionRoutine(
			GaiReturnCode,
			0,
			AsyncContext->Overlapped);
	} else if (AsyncContext->Overlapped->hEvent) {
		Success = SetEvent(AsyncContext->Overlapped->hEvent);
		ASSERT (Success);
	} else {
		NOT_REACHED;
	}

	//
	// If necessary, signal the threadpool callback so that it can return, if it
	// was waiting.
	//

	AsyncContext->State = GaiCompleted;

	if (SignalThreadpoolCallback) {
		RtlWakeAddressSingle((PVOID) &AsyncContext->State);
	}

	KexLogDebugEvent(
		L"Asynchronous GetAddrInfoEx completed (%s)\r\n\r\n"
		L"AsyncContext:       0x%p\r\n"
		L"AddrInfoResult:     0x%p\r\n"
		L"GaiReturnCode:      %d",
		AsyncContext->Name,
		AsyncContext,
		AddrInfoResult,
		GaiReturnCode);

	return TRUE;
}

//
// Timer callback function to time out an asynchronous GetAddrInfoExW.
// This timer callback is called when the operation times out. If the caller did
// not specify a timeout, this callback will never be called.
//
STATIC VOID CALLBACK GetAddrInfoExAsyncTimeoutCallback(
	IN OUT	PVOID	Parameter,
	IN		BOOLEAN	TimerOrWaitFired)
{
	PKXNETP_GAI_ASYNC_CONTEXT AsyncContext;

	ASSERT (Parameter != NULL);
	
	if (TimerOrWaitFired == FALSE) {
		return;
	}

	AsyncContext = (PKXNETP_GAI_ASYNC_CONTEXT) Parameter;

	//
	// Complete the asynchronous operation.
	//
	// This works even when the async operation times out before the threadpool
	// callback runs. The threadpool callback will still end up calling GetAddrInfoEx,
	// but the results will be discarded.
	//
	// We will specify that the timer deletion is not synchronous, because that would
	// cause a deadlock.
	//

	GetAddrInfoExCompleteAsyncOperation(
		AsyncContext,
		NULL,
		WSAETIMEDOUT,
		TRUE,
		FALSE);
}

//
// Threadpool callback function to support *asynchronous* GetAddrInfoExW.
//
// This function:
//   - copies the async context into its own stack-allocated structure
//   - sets the async context state to GaiWaiting and use WakeByAddressSingle to let
//     Ext_GetAddrInfoExW return
//
//   - calls GetAddrInfoExW (non-asynchronous)
//   - uses InterlockedCompareExchange to see if completion (timeout or cancel) is in
//     progress, if so, call WaitOnAddress to wait until the completion has finished
//     and then return.
//
//   - sets the Overlapped->Internal to the WSA status code of the operation
//   - sets the Overlapped->Pointer to the PPADDRINFOEXW pointer originally specified
//     by the application
//   - signals Overlapped->hEvent or calls CompletionRoutine in the async context
//
//   - returns.
//
STATIC VOID CALLBACK GetAddrInfoExAsyncThreadpoolCallback(
	IN OUT	PTP_CALLBACK_INSTANCE	Instance,
	IN OUT	PVOID					Parameter OPTIONAL)
{
	BOOLEAN Success;
	KXNETP_GAI_ASYNC_CONTEXT AsyncContext;
	PKXNETP_GAI_ASYNC_HANDOFF AsyncHandoff;
	PADDRINFOEXW AddrInfoResult;
	INT GaiReturnCode;

	ASSERT (Parameter != NULL);

	//
	// Copy the async context into our stack.
	// Change the pointer in the handoff structure to the one in our stack.
	//

	AsyncHandoff = (PKXNETP_GAI_ASYNC_HANDOFF) Parameter;
	AsyncContext = *AsyncHandoff->AsyncContext;

	if (KexIsDebugBuild) {
		WCHAR NspGuidString[64];

		StringFromGUID2(&AsyncContext.NspGuid, NspGuidString, ARRAYSIZE(NspGuidString));

		KexLogDebugEvent(
			L"Asynchronous GetAddrInfoEx started (%s)\r\n\r\n"
			L"&AsyncContext:      0x%p\r\n"
			L"Name:               \"%s\"\r\n"
			L"ServiceName:        \"%s\"\r\n"
			L"Namespace:          %lu\r\n"
			L"NspGuid:            %s\r\n"
			L"HintsPresent:       %s\r\n"
			L"Timeout:            %d seconds + %d microseconds (0,0 means no timeout)\r\n"
			L"CompletionRoutine:  0x%p\r\n"
			L"Overlapped->hEvent: 0x%p",
			AsyncContext.Name,
			&AsyncContext,
			AsyncContext.Name,
			AsyncContext.ServiceName,
			AsyncContext.Namespace,
			NspGuidString,
			BOOLEAN_AS_STRING(AsyncContext.HintsPresent),
			AsyncContext.Timeout.tv_sec,
			AsyncContext.Timeout.tv_usec,
			AsyncContext.CompletionRoutine,
			AsyncContext.Overlapped->hEvent);
	}

	//
	// Set up timer if required.
	//

	if (AsyncContext.TimeoutPresent) {
		BOOL Success;
		ULONGLONG TimeoutMs;

		TimeoutMs = AsyncContext.Timeout.tv_sec * 1000ULL;
		TimeoutMs += (AsyncContext.Timeout.tv_usec + 999ULL) / 1000ULL;

		if (TimeoutMs > ULONG_MAX) {
			// Clamp to prevent overflows.
			TimeoutMs = ULONG_MAX;
		}

		Success = CreateTimerQueueTimer(
			&AsyncContext.TimeoutTimer,
			NULL,
			GetAddrInfoExAsyncTimeoutCallback,
			&AsyncContext,
			(ULONG) TimeoutMs,
			0,
			0);

		ASSERT (Success);
	}

	//
	// Signal the caller that we've finished copying, and it can return now.
	//

	AsyncHandoff->AsyncContext = &AsyncContext;
	AsyncHandoff->ThreadpoolCallbackReady = TRUE;
	RtlWakeAddressSingle((PVOID) &AsyncHandoff->ThreadpoolCallbackReady);
	
	// This pointer is no longer valid after RtlWakeByAddressSingle returns.
	AsyncHandoff = NULL;

	//
	// Call GetAddrInfoExW in synchronous mode. This is a blocking call.
	//

	GaiReturnCode = GetAddrInfoExW(
		AsyncContext.NamePresent ? AsyncContext.Name : NULL,
		AsyncContext.ServiceNamePresent ? AsyncContext.ServiceName : NULL,
		AsyncContext.Namespace,
		AsyncContext.NspGuidPresent ? &AsyncContext.NspGuid : NULL,
		AsyncContext.HintsPresent ? &AsyncContext.Hints : NULL,
		&AddrInfoResult,
		NULL,
		NULL,
		NULL,
		NULL);

	//
	// Try to complete the operation.
	//

	Success = GetAddrInfoExCompleteAsyncOperation(
		&AsyncContext,
		AddrInfoResult,
		GaiReturnCode,
		FALSE,
		TRUE);

	if (!Success) {
		KXNETP_GAI_STATE UnwantedState;

		//
		// We lost the race and another thread has cancelled or timed out the operation
		// before us.
		//
		// If the GetAddrInfoExW call returned successfully, since we are not passing
		// the returned PADDRINFOEXW to the caller, we must free it.
		//

		if (GaiReturnCode == NO_ERROR) {
			FreeAddrInfoExW(AddrInfoResult);
			AddrInfoResult = NULL;
		}

		//
		// Someone else has already initiated a cancellation or timeout.
		// We have to wait until they have completed the asynchronous operation.
		//

		UnwantedState = GaiCompletionInProgress;

		until (AsyncContext.State != UnwantedState) {
			NTSTATUS Status;

			Status = RtlWaitOnAddress(
				&AsyncContext.State,
				&UnwantedState,
				sizeof(AsyncContext.State),
				NULL);

			ASSERT (Status == STATUS_SUCCESS);
		}
	}

	//
	// We may now return, since the asynchronous operation has completed.
	//

	ASSERT (AsyncContext.State == GaiCompleted);
	return;
}

//
// Added to support osu!lazer.
// Only GetAddrInfoExW needs to be here because asynchronous lookups are not
// supported for the ANSI version of the function (it returns WSAEINVAL on Win10
// if Timeout, Overlapped, CompletionRoutine, or EventHandle is non-NULL).
//

KXNETAPI INT WSAAPI Ext_GetAddrInfoExW(
	IN	PCWSTR								Name OPTIONAL,
	IN	PCWSTR								ServiceName OPTIONAL,
	IN	ULONG								Namespace,
	IN	LPGUID								NspGuid OPTIONAL,
	IN	PCADDRINFOEXW						Hints OPTIONAL,
	OUT	PPADDRINFOEXW						AddrInfoResult,
	IN	PTIMEVAL							Timeout OPTIONAL,
	IN	LPOVERLAPPED						Overlapped OPTIONAL,
	IN	LPLOOKUPSERVICE_COMPLETION_ROUTINE	CompletionRoutine OPTIONAL,
	OUT	PHANDLE								CancelHandle OPTIONAL)
{
	BOOL Success;
	HRESULT Result;
	TP_CALLBACK_ENVIRON CallbackEnvironment;
	KXNETP_GAI_ASYNC_CONTEXT AsyncContext;
	KXNETP_GAI_ASYNC_HANDOFF AsyncHandoff;

	if (!Timeout && !Overlapped && !CompletionRoutine && !CancelHandle) {
		// Not an asynchronous operation. No need to do anything, just call
		// original function.
		return GetAddrInfoExW(
			Name,
			ServiceName,
			Namespace,
			NspGuid,
			Hints,
			AddrInfoResult,
			NULL,
			NULL,
			NULL,
			NULL);
	}

	//
	// Validate parameters.
	//

	ASSERT (AddrInfoResult != NULL);
	*AddrInfoResult = NULL;

	if (!Name && !ServiceName) {
		KexDebugCheckpoint();
		SetLastError(WSAHOST_NOT_FOUND);
		return WSAHOST_NOT_FOUND;
	}

	if (!Overlapped) {
		// All asynchronous lookups must have an Overlapped parameter
		KexDebugCheckpoint();
		SetLastError(WSAEINVAL);
		return WSAEINVAL;
	}

	Overlapped->Internal = WSAEINPROGRESS;

	//
	// The completion routine and the hEvent member of OVERLAPPED are mutually
	// exclusive, and one of them must be specified.
	//

	unless (!!CompletionRoutine ^ !!Overlapped->hEvent) {
		KexDebugCheckpoint();
		SetLastError(WSAEINVAL);
		return WSAEINVAL;
	}

	//
	// Validate the contents of the Timeout struct if it is supplied.
	//

	if (Timeout) {
		// This is a great example of why unix using "int" everywhere is a really
		// stupid and bad idea. You have to constantly check whether things are
		// negative. Why not just use ULONG...
		// Anyway, basically, neither of the values can be negative and the total
		// timeout can't be zero.
		if (Timeout->tv_sec < 0 ||
			Timeout->tv_usec < 0 ||
			(Timeout->tv_sec == 0 && Timeout->tv_usec == 0)) {

			KexDebugCheckpoint();
			SetLastError(WSAEINVAL);
			return WSAEINVAL;
		}
	}

	//
	// Validate Hints struct if supplied.
	//

	if (Hints) {
		if (Hints->ai_addrlen || Hints->ai_canonname || Hints->ai_addr || Hints->ai_next) {
			// as documented by MS
			KexDebugCheckpoint();
			SetLastError(WSANO_RECOVERY);
			return WSANO_RECOVERY;
		}

		if (!Name) {
			if ((Hints->ai_flags & AI_CANONNAME) || (Hints->ai_flags & AI_FQDN)) {
				// these flags don't make sense if a name isn't supplied to us
				KexDebugCheckpoint();
				SetLastError(WSAEINVAL);
				return WSAEINVAL;
			}
		}

		switch (Hints->ai_family) {
		case AF_UNSPEC:
		case AF_INET:
		case AF_INET6:
			break;
		default:
			KexDebugCheckpoint();
			SetLastError(WSAEAFNOSUPPORT);
			return WSAEAFNOSUPPORT;
		}
	}

	//
	// The real GetAddrInfoExW does some additional validation but they require
	// lots of code and are not worth the effort.
	//
	// Set up the context for the asynchronous task.
	// Most of the information supplied through pointers must be *copied* into the
	// asynchronous context, since that is what Windows does, and it's very
	// possible that after we return all those pointers become invalid.
	//

	KexRtlZeroMemory(&AsyncContext, sizeof(AsyncContext));
	
	if (Name) {
		Result = StringCchCopy(
			AsyncContext.Name,
			ARRAYSIZE(AsyncContext.Name),
			Name);

		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			SetLastError(WSAENAMETOOLONG);
			return WSAENAMETOOLONG;
		}

		AsyncContext.NamePresent = TRUE;
	}

	if (ServiceName) {
		Result = StringCchCopy(
			AsyncContext.ServiceName,
			ARRAYSIZE(AsyncContext.ServiceName),
			ServiceName);

		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			SetLastError(WSAENAMETOOLONG);
			return WSAENAMETOOLONG;
		}

		AsyncContext.ServiceNamePresent = TRUE;
	}

	if (NspGuid) {
		AsyncContext.NspGuid = *NspGuid;
		AsyncContext.NspGuidPresent = TRUE;
	}

	if (Hints) {
		AsyncContext.Hints = *Hints;
		AsyncContext.HintsPresent = TRUE;
	}

	if (Timeout) {
		AsyncContext.Timeout = *Timeout;
		AsyncContext.TimeoutPresent = TRUE;
	}

	Overlapped->Pointer				= NULL;

	AsyncContext.Namespace			= Namespace;
	AsyncContext.AddrInfoResult		= AddrInfoResult;
	AsyncContext.Overlapped			= Overlapped;
	AsyncContext.CompletionRoutine	= CompletionRoutine;

	//
	// Start the threadpool callback.
	//

	AsyncHandoff.AsyncContext				= &AsyncContext;
	AsyncHandoff.ThreadpoolCallbackReady	= FALSE;

	InitializeThreadpoolEnvironment(&CallbackEnvironment);
	SetThreadpoolCallbackRunsLong(&CallbackEnvironment);

	Success = TrySubmitThreadpoolCallback(
		GetAddrInfoExAsyncThreadpoolCallback,
		&AsyncHandoff,
		&CallbackEnvironment);

	ASSERT (Success);
	DestroyThreadpoolEnvironment(&CallbackEnvironment);

	if (!Success) {
		SetLastError(WSASYSCALLFAILURE);
		return WSASYSCALLFAILURE;
	}

	//
	// Wait for the threadpool callback to finish copying.
	// Do in a loop to guard against spurious wakes.
	//

	until (AsyncHandoff.ThreadpoolCallbackReady) {
		NTSTATUS Status;
		BOOLEAN False;

		STATIC_ASSERT(sizeof(False) == sizeof(AsyncHandoff.ThreadpoolCallbackReady));
		
		False = FALSE;

		Status = RtlWaitOnAddress(
			&AsyncHandoff.ThreadpoolCallbackReady,
			&False,
			sizeof(AsyncHandoff.ThreadpoolCallbackReady),
			NULL);

		ASSERT (Status == STATUS_SUCCESS);

		if (Status != STATUS_SUCCESS) {
			SetLastError(WSASYSCALLFAILURE);
			return WSASYSCALLFAILURE;
		}
	}

	//
	// Return the cancel handle to the caller if the caller wants it.
	//

	if (CancelHandle) {
		// Note that AsyncContext is now pointing to the version in the threadpool
		// callback's stack.
		ASSERT (AsyncHandoff.AsyncContext != &AsyncContext);
		*CancelHandle = (HANDLE) AsyncHandoff.AsyncContext;
	}

	SetLastError(WSA_IO_PENDING);
	return WSA_IO_PENDING;
}

KXNETAPI INT WSAAPI GetAddrInfoExCancel(
	IN	PHANDLE	Handle)
{
	BOOLEAN Success;
	PKXNETP_GAI_ASYNC_CONTEXT AsyncContext;

	if (Handle == NULL || *Handle == NULL) {
		return WSA_INVALID_HANDLE;
	}

	AsyncContext = (PKXNETP_GAI_ASYNC_CONTEXT) *Handle;
	*Handle = NULL;

	Success = GetAddrInfoExCompleteAsyncOperation(
		AsyncContext,
		NULL,
		WSA_E_CANCELLED,
		TRUE,
		TRUE);

	if (!Success) {
		return WSA_INVALID_HANDLE;
	}

	return NO_ERROR;
}

KXNETAPI INT WSAAPI GetAddrInfoExOverlappedResult(
	IN	LPOVERLAPPED Overlapped)
{
	if (Overlapped == NULL) {
		return WSAEINVAL;
	}

	ASSERT (Overlapped->Internal <= ULONG_MAX);
	return (INT) Overlapped->Internal;
}