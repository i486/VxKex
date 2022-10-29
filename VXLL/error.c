///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     error.c
//
// Abstract:
//
//     Contains routines for converting VXLSTATUS error codes to human-readable
//     text and converting Win32 error codes to VXLSTATUS error codes.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu              30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "VXLP.h"

static PCWSTR ErrorLookupTable[] = {
	L"The operation completed successfully",											// VXL_SUCCESS
	L"An unclassified error was encountered",											// VXL_FAILURE
	L"There are insufficient memory resources to complete this operation",				// VXL_OUT_OF_MEMORY
	L"The function is not implemented",													// VXL_NOT_IMPLEMENTED
	L"Insufficient credentials to perform this action",									// VXL_INSUFFICIENT_CREDENTIALS
	L"One or more parameters was invalid",												// VXL_INVALID_PARAMETER
	L"One or more parameters have a length exceeding the allowed maximum",				// VXL_INVALID_PARAMETER_LENGTH
	L"An invalid combination of parameters or flags was supplied",						// VXL_INVALID_PARAMETER_COMBINATION
	L"The size of the data area passed to a function is too small",						// VXL_INSUFFICIENT_BUFFER
	L"The file is already opened by another application",								// VXL_FILE_ALREADY_OPENED
	L"The file already exists",															// VXL_FILE_ALREADY_EXISTS
	L"An error was encountered while reading or writing to a file",						// VXL_FILE_IO_ERROR
	L"Could not obtain a lock on required regions of the file",							// VXL_FILE_SYNCHRONIZATION_ERROR
	L"The file was not opened in the correct manner for performing this operation",		// VXL_FILE_WRONG_MODE
	L"The file was not found",															// VXL_FILE_NOT_FOUND
	L"The file is corrupt",																// VXL_FILE_CORRUPT
	L"The file is not a valid VXL file",												// VXL_FILE_INVALID
	L"Cannot open the file",															// VXL_FILE_CANNOT_OPEN
	L"The log file belongs to another application",										// VXL_FILE_MISMATCHED_SOURCE_APPLICATION
	L"The specified source component was not found in the log file",					// VXL_FILE_SOURCE_COMPONENT_NOT_FOUND
	L"There are too many source components in the log file",							// VXL_FILE_SOURCE_COMPONENT_LIMIT_EXCEEDED
	L"The specified entry was not found in the log file",								// VXL_ENTRY_NOT_FOUND
	L"The specified entry is out of range",												// VXL_ENTRY_OUT_OF_RANGE
	L"The length of the log entry text is too long",									// VXL_ENTRY_TEXT_TOO_LONG
	L"The version of the library which created the log file does not match with the "
	L"current library version",															// VXL_VERSION_MISMATCH
	L"Invalid or unknown error code"													// VXL_MAXIMUM_ERROR_CODE
};

//
// Convert a VXLSTATUS enumeration value into a human-readable string.
//
PCWSTR VXLAPI VxlErrorLookup(
	IN		VXLSTATUS		Status)
{
	return ErrorLookupTable[min((ULONG) Status, VXL_MAXIMUM_ERROR_CODE)];
}

//
// Convert a Win32 error code to a VXLSTATUS error code.
// An optional Default value can be supplied, which is returned if no match
// can be found. If Default is zero, the default error code is VXL_FAILURE.
//
VXLSTATUS VXLAPI VxlTranslateWin32Error(
	IN	ULONG		Win32Error,
	IN	VXLSTATUS	Default OPTIONAL)
{
	switch (Win32Error) {
	case ERROR_SUCCESS:
		return VXL_SUCCESS;
	case ERROR_INVALID_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_PARAMETER:
	case ERROR_INVALID_FLAGS:
		return VXL_INVALID_PARAMETER;
	case ERROR_FILE_NOT_FOUND:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_INVALID_DRIVE:
		return VXL_FILE_NOT_FOUND;
	case ERROR_FILE_EXISTS:
		return VXL_FILE_ALREADY_EXISTS;
	case ERROR_ACCESS_DENIED:
		return VXL_INSUFFICIENT_CREDENTIALS;
	case ERROR_NOT_ENOUGH_MEMORY:
		return VXL_OUT_OF_MEMORY;
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		return VXL_FILE_ALREADY_OPENED;
	case ERROR_OPEN_FAILED:
	case ERROR_CANNOT_MAKE:
		return VXL_FILE_CANNOT_OPEN;
	case ERROR_WRITE_PROTECT:
	case ERROR_READ_FAULT:
	case ERROR_WRITE_FAULT:
		return VXL_FILE_IO_ERROR;
	case ERROR_INSUFFICIENT_BUFFER:
		return VXL_INSUFFICIENT_BUFFER;
	default:
		if (Default) {
			return Default;
		} else {
			return VXL_FAILURE;
		}
	}
}