#include <Windows.h>
#include "VXLL.h"

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
	L"Invalid or unknown error code"													// VXL_MAXIMUM_ERROR_CODE
};

//
// Convert a VXLSTATUS enumeration value into a human-readable string.
//
PCWSTR VXLAPI VxlErrorLookup(
	IN		VXLSTATUS		Status)
{
	return ErrorLookupTable[min(Status, VXL_MAXIMUM_ERROR_CODE)];
}