#include <Windows.h>
#include "VXLL.h"

//
// Convert a VXLSTATUS enumeration value into a human-readable string.
//
PCWSTR VXLAPI VxlErrorLookup(
	IN		VXLSTATUS		Status)
{
	switch (Status) {
	case VXL_SUCCESS:
		return L"The operation completed successfully";
	case VXL_FAILURE:
		return L"An unclassified error was encountered";
	case VXL_OUT_OF_MEMORY:
		return L"There are insufficient memory resources to complete this operation";
	case VXL_NOT_IMPLEMENTED:
		return L"The function is not implemented";
	case VXL_INSUFFICIENT_CREDENTIALS:
		return L"Insufficient credentials to perform this action";
	case VXL_INVALID_PARAMETER:
		return L"One or more parameters was invalid";
	case VXL_INVALID_PARAMETER_LENGTH:
		return L"One or more parameters have a length exceeding the allowed maximum";
	case VXL_INVALID_PARAMETER_COMBINATION:
		return L"An invalid combination of parameters or flags was supplied";
	case VXL_INSUFFICIENT_BUFFER:
		return L"The size of the data area passed to a function is too small";
	case VXL_FILE_ALREADY_OPENED:
		return L"The file is already opened by another application";
	case VXL_FILE_ALREADY_EXISTS:
		return L"The file already exists";
	case VXL_FILE_IO_ERROR:
		return L"An error was encountered while reading or writing to a file.";
	case VXL_FILE_SYNCHRONIZATION_ERROR:
		return L"Could not obtain a lock on required regions of the file.";
	case VXL_FILE_WRONG_MODE:
		return L"The file was not opened in the correct manner for performing this operation.";
	case VXL_FILE_NOT_FOUND:
		return L"The file was not found";
	case VXL_FILE_CORRUPT:
		return L"The file is corrupt";
	case VXL_FILE_INVALID:
		return L"The file is not a valid VXL file";
	case VXL_FILE_CANNOT_OPEN:
		return L"Cannot open the file";
	case VXL_FILE_MISMATCHED_SOURCE_APPLICATION:
		return L"The log file belongs to another application";
	case VXL_FILE_SOURCE_COMPONENT_NOT_FOUND:
		return L"The specified source component was not found in the log file";
	case VXL_FILE_SOURCE_COMPONENT_LIMIT_EXCEEDED:
		return L"There are too many source components in the log file";
	case VXL_ENTRY_NOT_FOUND:
		return L"The specified entry was not found in the log file";
	case VXL_ENTRY_OUT_OF_RANGE:
		return L"The specified entry is out of range";
	default:
		return L"Invalid error code supplied to VxlErrorLookup";
	}
}