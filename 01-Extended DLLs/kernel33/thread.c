///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     thread.c
//
// Abstract:
//
//     Extended functions for dealing with threads.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu               07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "basedllp.h"

//
// Retrieves the description that was assigned to a thread by calling
// SetThreadDescription.
//
// Parameters:
//
//   ThreadHandle
//     A handle to the thread for which to retrieve the description. The handle
//     must have THREAD_QUERY_LIMITED_INFORMATION access.
//
//   ThreadDescription
//     A Unicode string that contains the description of the thread.
//
// Return value:
//
//   If the function succeeds, the return value is the HRESULT that denotes a
//   successful operation. If the function fails, the return value is an HRESULT
//   that denotes the error.
//
// Remarks:
//
//   The description for a thread can change at any time. For example, a different
//   thread can change the description of a thread of interest while you try to
//   retrieve that description.
//
//   Thread descriptions do not need to be unique.
//
//   To free the memory for the thread description, call the LocalFree method.
//
WINBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	ThreadHandle,
	OUT	PPWSTR	ThreadDescription)
{
	NTSTATUS Status;
	PUNICODE_STRING Description;
	ULONG DescriptionCb;

	*ThreadDescription = NULL;
	Description = NULL;
	DescriptionCb = 64 * sizeof(WCHAR);

	//
	// The thread description can be changed at any time by another
	// thread. Therefore, we need to put the code in the loop in case
	// the length changes.
	//

	while (TRUE) {
		Description = (PUNICODE_STRING) SafeAlloc(BYTE, DescriptionCb);

		if (!Description) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = NtQueryInformationThread(
			ThreadHandle,
			ThreadNameInformation,
			&Description,
			DescriptionCb,
			&DescriptionCb);

		//
		// If the call succeeded, or if there was a failure caused by
		// anything other than our buffer being too small, break out of
		// the loop.
		//

		if (Status != STATUS_INFO_LENGTH_MISMATCH &&
			Status != STATUS_BUFFER_TOO_SMALL &&
			Status != STATUS_BUFFER_OVERFLOW) {
			break;
		}

		SafeFree(Description);
	}

	if (NT_SUCCESS(Status)) {
		PWSTR ReturnDescription;
		ULONG DescriptionCch;

		//
		// Shift the buffer of the UNICODE_STRING backwards onto its
		// base address, and make sure it's null terminated.
		//

		ReturnDescription = (PWSTR) Description;
		DescriptionCch = Description->Length / sizeof(WCHAR);
		RtlMoveMemory(ReturnDescription, Description->Buffer, Description->Length);
		ReturnDescription[DescriptionCch] = L'\0';

		*ThreadDescription = ReturnDescription;
		Description = NULL;
	}

	SafeFree(Description);
	return HRESULT_FROM_NT(Status);
}

//
// Assigns a description to a thread.
//
// Parameters:
//
//   ThreadHandle
//     A handle for the thread for which you want to set the description.
//     The handle must have THREAD_SET_LIMITED_INFORMATION access.
//
//   ThreadDescription
//     A Unicode string that specifies the description of the thread.
//
// Return value:
//
//   If the function succeeds, the return value is the HRESULT that denotes
//   a successful operation. If the function fails, the return value is an
//   HRESULT that denotes the error.
//
// Remarks:
//
//   The description of a thread can be set more than once; the most recently
//   set value is used. You can retrieve the description of a thread by
//   calling GetThreadDescription.
//
WINBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	ThreadHandle,
	IN	PCWSTR	ThreadDescription)
{
	NTSTATUS Status;
	UNICODE_STRING Description;

	Status = RtlInitUnicodeStringEx(&Description, ThreadDescription);
	
	if (NT_SUCCESS(Status)) {
		Status = NtSetInformationThread(
			ThreadHandle,
			ThreadNameInformation,
			&Description,
			sizeof(Description));
	}

	return HRESULT_FROM_NT(Status);
}