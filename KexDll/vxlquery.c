///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     vxlquery.c
//
// Abstract:
//
//     Contains the public routines for querying log file information.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              12-Nov-2022  Convert to v3 + native API
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

#pragma warning(disable:4267)

NTSTATUS NTAPI VxlQueryInformationLog(
	IN		VXLHANDLE		LogHandle,
	IN		VXLLOGINFOCLASS	LogInformationClass,
	OUT		PVOID			Buffer OPTIONAL,
	IN OUT	PULONG			BufferSize)
{
	ULONG RequiredBufferSize;

	if (!LogHandle || !BufferSize) {
		return STATUS_INVALID_PARAMETER;
	}

	if (!Buffer && *BufferSize != 0) {
		return STATUS_INVALID_PARAMETER_MIX;
	}

	switch (LogInformationClass) {
	case LogLibraryVersion:
		if (*BufferSize <= sizeof(ULONG)) {
			RequiredBufferSize = sizeof(ULONG);
			break;
		} else {
			*(PULONG) Buffer = VXLL_VERSION;
			return STATUS_SUCCESS;
		}
	case LogNumberOfCriticalEvents:
	case LogNumberOfErrorEvents:
	case LogNumberOfWarningEvents:
	case LogNumberOfInformationEvents:
	case LogNumberOfDetailEvents:
	case LogNumberOfDebugEvents:
	case LogTotalNumberOfEvents:
		RequiredBufferSize = sizeof(ULONG);
		break;
	case LogSourceApplication:
		// we will figure this out later
		RequiredBufferSize = 0;
		break;
	default:
		return STATUS_INVALID_INFO_CLASS;
	}

	if (*BufferSize < RequiredBufferSize) {
		*BufferSize = RequiredBufferSize;
		return STATUS_BUFFER_TOO_SMALL;
	}

	try {
		RtlAcquireSRWLockShared(&LogHandle->Lock);

		switch (LogInformationClass) {
		case LogNumberOfCriticalEvents:
		case LogNumberOfErrorEvents:
		case LogNumberOfWarningEvents:
		case LogNumberOfInformationEvents:
		case LogNumberOfDetailEvents:
		case LogNumberOfDebugEvents:
			*(PULONG) Buffer = LogHandle->Header->EventSeverityTypeCount[LogInformationClass - 1];
			break;
		case LogTotalNumberOfEvents:
			*(PULONG) Buffer = VxlpGetTotalLogEntryCount(LogHandle);
			break;
		case LogSourceApplication:
			RequiredBufferSize = wcslen(LogHandle->Header->SourceApplication) * sizeof(WCHAR);

			if (*BufferSize < RequiredBufferSize) {
				*BufferSize = RequiredBufferSize;
				return STATUS_BUFFER_TOO_SMALL;
			}

			RtlCopyMemory(Buffer, LogHandle->Header->SourceApplication, RequiredBufferSize);
			break;
		default:
			NOT_REACHED;
		}
	} finally {
		RtlReleaseSRWLockShared(&LogHandle->Lock);
	}

	return STATUS_SUCCESS;
}