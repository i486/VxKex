///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntthread.c
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
//     vxiiduu               05-Jul-2026  Remove AlertByThreadId functions
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI Ext_NtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status;

	if (ThreadInformationClass <= ThreadIdealProcessorEx) {

		//
		// fall through and call original NtQueryInformationThread
		//

		NOTHING;

	} else if (ThreadInformationClass == ThreadNameInformation) {
		OBJECT_BASIC_INFORMATION ThreadHandleInformation;

		//
		// Check for THREAD_QUERY_LIMITED_INFORMATION access.
		//

		Status = NtQueryObject(
			ThreadHandle,
			ObjectBasicInformation,
			&ThreadHandleInformation,
			sizeof(ThreadHandleInformation),
			NULL);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		unless (ThreadHandleInformation.GrantedAccess & THREAD_QUERY_LIMITED_INFORMATION) {
			return STATUS_ACCESS_DENIED;
		}

		//
		// TODO: Implement ThreadNameInformation here.
		//

		return STATUS_INVALID_INFO_CLASS;
	} else {
		KexLogWarningEvent(
			L"NtQueryInformationThread called with an unsupported extended information class %d",
			ThreadInformationClass);
	}

	return NtQueryInformationThread(
		ThreadHandle,
		ThreadInformationClass,
		ThreadInformation,
		ThreadInformationLength,
		ReturnLength);
} PROTECTED_FUNCTION_END

KEXAPI NTSTATUS NTAPI Ext_NtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength) PROTECTED_FUNCTION
{
	//
	// TODO: Implement ThreadNameInformation
	//

	return NtSetInformationThread(
		ThreadHandle,
		ThreadInformationClass,
		ThreadInformation,
		ThreadInformationLength);
} PROTECTED_FUNCTION_END