#include "buildcfg.h"
#include "kxcomp.h"

//
// 28-07-2026: Replace old implementation with one that doesn't have any
// race condition (CoIncrementMTAUsage now waits for mta usage to actually
// be incremented before returning).
//

typedef enum {
	Initializing,		// The incrementer thread is still working to initialize COM.
	Waiting,			// The incrementer thread has initialized COM and is now sleeping.
	Waking,				// The incrementer thread should wake up, uninitialize COM, and exit.
} TYPEDEF_TYPE_NAME(CO_MTA_INCREMENT_STATE);

typedef struct _CO_MTA_INCREMENT_INFORMATION *PCO_MTA_INCREMENT_INFORMATION;

typedef struct _CO_MTA_INCREMENT_INFORMATION {
	// NULL when this structure is passed to the incrementer thread.
	// After the incrementer thread transitions from Initializing to Waiting,
	// this will be a pointer to the CO_MTA_INCREMENT_INFORMATION structure
	// copied to the stack of the incrementer thread.
	PCO_MTA_INCREMENT_INFORMATION	Handoff;
	CO_MTA_INCREMENT_STATE			State;
} TYPEDEF_TYPE_NAME(CO_MTA_INCREMENT_INFORMATION);

STATIC DWORD WINAPI KxCompMTAUsageIncrementerThread(
	IN	PVOID	Parameter)
{
	HRESULT Result;
	PCO_MTA_INCREMENT_INFORMATION HandoffInformation;
	CO_MTA_INCREMENT_INFORMATION Information;
	CO_MTA_INCREMENT_STATE UnwantedState;

	ASSERT (Parameter != NULL);

	//
	// Initialize COM on this thread.
	//
	
	Result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ASSERT (SUCCEEDED(Result));

	//
	// Tell the thread that started us that we've finished initializing COM.
	//

	HandoffInformation = (PCO_MTA_INCREMENT_INFORMATION) Parameter;
	ASSERT (HandoffInformation->Handoff == NULL);
	ASSERT (HandoffInformation->State == Initializing);

	HandoffInformation->Handoff = &Information;
	HandoffInformation->State = Waiting;
	Information = *HandoffInformation;
	
	RtlWakeAddressSingle(&HandoffInformation->State);

	//
	// Wait for someone to call CoDecrementMTAUsage.
	//

	UnwantedState = Waiting;

	do {
		RtlWaitOnAddress(
			&Information.State,
			&UnwantedState,
			sizeof(UnwantedState),
			NULL);
	} while (Information.State == UnwantedState);

	//
	// Someone called CoDecrementMTAUsage.
	//

	ASSERT (Information.State == Waking);
	CoUninitialize();

	return 0;
}

KXCOMAPI HRESULT WINAPI CoIncrementMTAUsage(
	OUT	PCO_MTA_USAGE_COOKIE	Cookie)
{
	HANDLE ThreadHandle;
	CO_MTA_INCREMENT_INFORMATION HandoffInformation;
	CO_MTA_INCREMENT_STATE UnwantedState;

	if (!Cookie) {
		return E_INVALIDARG;
	}

	//
	// Create incrementer thread.
	//

	HandoffInformation.Handoff = NULL;
	HandoffInformation.State = Initializing;

	ThreadHandle = CreateThread(
		NULL,
		0,
		KxCompMTAUsageIncrementerThread,
		&HandoffInformation,
		0,
		NULL);

	if (ThreadHandle == NULL) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	SafeClose(ThreadHandle);

	//
	// Wait for the incrementer thread to actually call CoInitializeEx.
	//

	UnwantedState = Initializing;

	do {
		RtlWaitOnAddress(
			&HandoffInformation.State,
			&UnwantedState,
			sizeof(HandoffInformation.State),
			NULL);
	} while (HandoffInformation.State == UnwantedState);

	//
	// The incrementer thread has called CoInitializeEx and given us a pointer
	// to its stack-allocated CO_MTA_INCREMENT_INFORMATION.
	//

	ASSERT (HandoffInformation.Handoff != NULL);
	ASSERT (HandoffInformation.State == Waiting);

	*Cookie = HandoffInformation.Handoff;
	return S_OK;
}

KXCOMAPI HRESULT WINAPI CoDecrementMTAUsage(
	IN	CO_MTA_USAGE_COOKIE		Cookie)
{
	PCO_MTA_INCREMENT_INFORMATION Information;

	if (!Cookie) {
		return E_INVALIDARG;
	}

	Information = (PCO_MTA_INCREMENT_INFORMATION) Cookie;

	ASSERT (Information->Handoff != NULL);
	ASSERT (Information->State >= Waiting);

	//
	// Tell the incrementer thread that it should now uninitialize COM and exit.
	//

	Information->State = Waking;
	RtlWakeAddressSingle(&Information->State);

	return S_OK;
}