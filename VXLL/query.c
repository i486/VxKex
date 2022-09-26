#include <Windows.h>
#include <strsafe.h>
#include "VXLL.h"
#include "VXLP.h"

#define BUFFER_SIZE_CHECK(NumberOfBytes) if (*BufferSize < (NumberOfBytes)) do { \
											 *BufferSize = (ULONG) (NumberOfBytes); \
											 Status = VXL_INSUFFICIENT_BUFFER; \
											 goto Error; \
										 } while (0)

//
// Query information about a log file identified by the LogHandle parameter.
// If *BufferSize is 0 or too small, this function will return VXL_INSUFFICIENT_BUFFER
// and place the correct buffer size in *BufferSize.
//
VXLSTATUS VXLAPI VxlQueryLogInformation(
	IN		VXLHANDLE		LogHandle,
	IN		VXLLOGINFOCLASS	InformationClass,
	OUT		PVOID			Buffer,
	IN OUT	PULONG			BufferSize)
{
	VXLSTATUS Status;
	HRESULT Result;
	SIZE_T Length;

	if (!LogHandle || !BufferSize) {
		return VXL_INVALID_PARAMETER;
	}

	if (!Buffer && *BufferSize != 0) {
		// if buffer is NULL then the size must be 0
		return VXL_INVALID_PARAMETER;
	}

	//
	// Retrieve header information from file in case another application/thread has
	// modified the data
	//
	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		Status = VxlpAcquireFileLock(LogHandle);
		if (VXL_FAILED(Status)) {
			return Status;
		}
	}

	switch (InformationClass) {
	case LogLibraryVersion:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.Version;
		break;
	case LogNumberOfCriticalEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityCritical];
		break;
	case LogNumberOfErrorEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityError];
		break;
	case LogNumberOfWarningEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityWarning];
		break;
	case LogNumberOfInformationEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityInformation];
		break;
	case LogNumberOfDetailEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityDetail];
		break;
	case LogNumberOfDebugEvents:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityDebug];
		break;
	case LogNumberOfEntries:
		BUFFER_SIZE_CHECK(sizeof(ULONG));
		*((PULONG) Buffer) = LogHandle->Header.EventSeverityTypeCount[LogSeverityCritical] +
							 LogHandle->Header.EventSeverityTypeCount[LogSeverityError] +
							 LogHandle->Header.EventSeverityTypeCount[LogSeverityWarning] +
							 LogHandle->Header.EventSeverityTypeCount[LogSeverityInformation] +
							 LogHandle->Header.EventSeverityTypeCount[LogSeverityDetail] +
							 LogHandle->Header.EventSeverityTypeCount[LogSeverityDebug];
		break;
	case LogSourceApplication:
		Result = StringCchLength(LogHandle->Header.SourceApplication,
								 ARRAYSIZE(LogHandle->Header.SourceApplication),
								 &Length);
		if (FAILED(Result)) {
			Status = VXL_FILE_CORRUPT;
			goto Error;
		}

		Length++; // for null terminator
		BUFFER_SIZE_CHECK(Length * sizeof(WCHAR));

		Result = StringCchCopy((PWSTR) Buffer,
							   *BufferSize / sizeof(WCHAR),
							   LogHandle->Header.SourceApplication);
		if (FAILED(Result)) {
			Status = VXL_FAILURE;
			goto Error;
		}

		break;
	case LogSourceComponents: {
		//
		// Returns a double-null-terminated series of null-terminated strings, much like
		// a Windows environment block.
		//
		ULONG Index;
		SIZE_T TotalLength;
		PWSTR DestinationString;

		Index = 0;
		TotalLength = 0;
		DestinationString = (PWSTR) Buffer;

		//
		// Find total required length of the buffer.
		//
		do {
			Result = StringCchLength(LogHandle->Header.SourceComponents[Index],
									 ARRAYSIZE(LogHandle->Header.SourceComponents[Index]),
									 &Length);
			if (FAILED(Result)) {
				Status = VXL_FILE_CORRUPT;
				goto Error;
			}

			TotalLength += Length;
			TotalLength++; // for null terminator
		} while (Length > 0 && Index < ARRAYSIZE(LogHandle->Header.SourceComponents));

		TotalLength++; // for final null terminator
		BUFFER_SIZE_CHECK(TotalLength * sizeof(WCHAR));

		//
		// Copy strings into the caller-supplied buffer.
		//
		Index = 0;

		do {
			Result = StringCchCopyEx(DestinationString,
									 TotalLength,
									 LogHandle->Header.SourceComponents[Index],
									 &DestinationString,
									 &TotalLength,
									 0);
			if (FAILED(Result)) {
				Status = VXL_INSUFFICIENT_BUFFER;
				goto Error;
			}

			++DestinationString;
			--TotalLength;
		} while (Length > 0 && Index < ARRAYSIZE(LogHandle->Header.SourceComponents));

		// add final null terminator
		*DestinationString = '\0';
							  } break;
	default:
		Status = VXL_NOT_IMPLEMENTED;
		goto Error;
	}

	Status = VXL_SUCCESS;

Error:
	if (LogHandle->OpenFlags & VXL_OPEN_WRITE_ONLY) {
		VxlpReleaseFileLock(LogHandle);
	}

	return Status;
}