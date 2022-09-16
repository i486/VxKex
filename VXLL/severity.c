#include <Windows.h>
#include "VXLL.h"

//
// Convert a VXLSEVERITY enumeration value into a human-readable string.
// You can obtain a long description by passing TRUE for the LongDescription
// parameter.
//
PCWSTR VXLAPI VxlSeverityLookup(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription)
{
	if (LongDescription) {
		switch (Severity) {
		case LogSeverityCritical:
			return L"The application encountered a critical error and cannot continue";
		case LogSeverityError:
			return L"The application encountered a non-critical error which affected its function";
		case LogSeverityWarning:
			return L"An event occurred which should be investigated but does not affect the immediate functioning of the application";
		case LogSeverityInformation:
			return L"An informational message which is not a cause for concern";
		case LogSeverityDetail:
			return L"An informational message, generated in large quantities, which is not a cause for concern";
		case LogSeverityDebug:
			return L"Information useful only to the developer of the application";
		default:
			return L"An invalid or unknown severity value";
		}
	} else {
		switch (Severity) {
		case LogSeverityCritical:
			return L"Critical";
		case LogSeverityError:
			return L"Error";
		case LogSeverityWarning:
			return L"Warning";
		case LogSeverityInformation:
			return L"Information";
		case LogSeverityDetail:
			return L"Detail";
		case LogSeverityDebug:
			return L"Debug";
		default:
			return L"Unknown";
		}
	}
}