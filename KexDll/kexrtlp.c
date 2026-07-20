///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexrtlp.c
//
// Abstract:
//
//     Private functions of KexDll. These functions are not to be exported.
//
// Author:
//
//     vxiiduu (03-Jul-2026)
//
// Revision History:
//
//     vxiiduu              03-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Implements lazy-initialization of the GlobalKeyedEvent member of KexData.
// Apps that don't use WaitOnAddress or NtAlertThreadByThreadId etc. don't
// need that keyed event, so we can sometimes avoid creating it.
//
HANDLE KexRtlpGetGlobalKeyedEvent(
	VOID)
{
	NTSTATUS Status;
	HANDLE KeyedEventHandle;
	HANDLE OldValue;

	if (KexData->GlobalKeyedEvent != NULL) {
		// Already initialized
		ASSERT (VALID_HANDLE(KexData->GlobalKeyedEvent));
		return KexData->GlobalKeyedEvent;
	}

	Status = NtCreateKeyedEvent(
		&KeyedEventHandle,
		KEYEDEVENT_WAKE | KEYEDEVENT_WAIT,
		NULL,
		0);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	OldValue = InterlockedCompareExchangePointer(
		&KexData->GlobalKeyedEvent,
		KeyedEventHandle,
		NULL);

	if (OldValue != NULL) {
		// we lost the race
		SafeClose(KeyedEventHandle);
	}

	return KexData->GlobalKeyedEvent;
}