///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllrewrt.c
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

		// Logging here must be wrapped in more SEH because VxlWriteLogEx is itself
		// a protected function - in case of failure inside that function, it will
		// result in infinite recursion.
		try {
			KexLogErrorEvent(
				L"UNHANDLED EXCEPTION in %s\r\n\r\n"
				L"Exception code:                0x%08lx\r\n"
				L"Exception address:             0x%p\r\n\r\n"
				L"Attempt to %s the inaccessible location 0x%p.",
				FunctionName,
				ExceptionCode,
				ExceptionRecord->ExceptionAddress,
				AccessType,
				ExceptionRecord->ExceptionInformation[1]);
		} except (EXCEPTION_EXECUTE_HANDLER) {}
	} else {
		try {
			KexLogErrorEvent(
				L"UNHANDLED EXCEPTION in %s\r\n\r\n"
				L"Exception code:                0x%08lx\r\n"
				L"Exception address:             0x%p",
				FunctionName,
				ExceptionCode,
				ExceptionRecord->ExceptionAddress);
		} except (EXCEPTION_EXECUTE_HANDLER) {}
	}

	return EXCEPTION_EXECUTE_HANDLER;
}