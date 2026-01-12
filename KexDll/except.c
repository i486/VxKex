///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     except.c
//
// Abstract:
//
//     Contains the exception filter for general protected functions in KexDll.
//
// Author:
//
//     vxiiduu (30-Oct-2022)
//
// Revision History:
//
//     vxiiduu              30-Oct-2022  Initial creation.
//     vxiiduu              23-Feb-2024  VxlWriteLogEx is no longer a protected
//                                       function - remove extra SEH wrapping
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This function cannot contain any errors or bugs.
//
ULONG KexDllProtectedFunctionExceptionFilter(
	IN	PCWSTR				FunctionName,
	IN	NTSTATUS			ExceptionCode,
	IN	PEXCEPTION_POINTERS	ExceptionPointers)
{
	PEXCEPTION_RECORD ExceptionRecord;

	if (ExceptionCode == STATUS_ASSERTION_FAILURE ||
		ExceptionCode == STATUS_BREAKPOINT) {

		// Do not catch assertions and breakpoints.
		// Otherwise it would be harder to debug.
		return EXCEPTION_CONTINUE_SEARCH;
	}

	ExceptionRecord = ExceptionPointers->ExceptionRecord;

	if (ExceptionCode == STATUS_ACCESS_VIOLATION) {
		PCWSTR AccessType;

		switch (ExceptionRecord->ExceptionInformation[0]) {
		case 0:
			AccessType = L"read";
			break;
		case 1:
			AccessType = L"write";
			break;
		case 8:
			AccessType = L"execute";
			break;
		default:
			NOT_REACHED;
		}

		KexLogErrorEvent(
			L"UNHANDLED EXCEPTION in %s\r\n\r\n"
			L"Exception code:    %s (0x%08lx)\r\n"
			L"Exception address: 0x%p\r\n\r\n"
			L"Attempt to %s the inaccessible location 0x%p.",
			FunctionName,
			KexRtlNtStatusToString(ExceptionCode), ExceptionCode,
			ExceptionRecord->ExceptionAddress,
			AccessType,
			ExceptionRecord->ExceptionInformation[1]);
	} else {
		KexLogErrorEvent(
			L"UNHANDLED EXCEPTION in %s\r\n\r\n"
			L"Exception code:    %s (0x%08lx)\r\n"
			L"Exception address: 0x%p",
			FunctionName,
			KexRtlNtStatusToString(ExceptionCode), ExceptionCode,
			ExceptionRecord->ExceptionAddress);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}