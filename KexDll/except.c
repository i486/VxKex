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
		case 1:
			AccessType = L"write";
		case 8:
			AccessType = L"execute";
		default:
			NOT_REACHED;
		}

		KexSrvLogErrorEvent(
			L"UNHANDLED EXCEPTION in %s\r\n\r\n"
			L"Exception code:                0x%08lx\r\n"
			L"Exception address:             0x%p\r\n\r\n"
			L"Attempt to %s the inaccessible location 0x%p.",
			FunctionName,
			ExceptionCode,
			ExceptionRecord->ExceptionAddress,
			AccessType,
			ExceptionRecord->ExceptionInformation[1]);
	} else {
		KexSrvLogErrorEvent(
			L"UNHANDLED EXCEPTION in %s\r\n\r\n"
			L"Exception code:                0x%08lx\r\n"
			L"Exception address:             0x%p",
			FunctionName,
			ExceptionCode,
			ExceptionRecord->ExceptionAddress);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}