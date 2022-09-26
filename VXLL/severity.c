#include <Windows.h>
#include "VXLL.h"

static PCWSTR SeverityLookupTable[][2] = {
	{L"Critical",		L"The application encountered a critical error and cannot continue"},
	{L"Error",			L"The application encountered a non-critical error which affected its function"},
	{L"Warning",		L"An event occurred which should be investigated but does not affect the immediate functioning of the application"},
	{L"Information",	L"An informational message which is not a cause for concern"},
	{L"Detail",			L"An informational message, generated in large quantities, which is not a cause for concern"},
	{L"Debug",			L"Information useful only to the developer of the application"},
	{L"Unknown",		L"An invalid or unknown severity value"}
};

//
// Convert a VXLSEVERITY enumeration value into a human-readable string.
// You can obtain a long description by passing TRUE for the LongDescription
// parameter.
//
PCWSTR VXLAPI VxlSeverityLookup(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription)
{
	return SeverityLookupTable[min(Severity, LogSeverityMaximumValue)][!!LongDescription];
}