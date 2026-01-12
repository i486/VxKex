///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     vxlsever.c
//
// Abstract:
//
//     Contains a routine to convert a VXLSEVERITY enumeration value into a
//     human-readable string.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              12-Nov-2022  Move into KexDll
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

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
KEXAPI PCWSTR NTAPI VxlSeverityToText(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription)
{
	return SeverityLookupTable[min((ULONG) Severity, LogSeverityMaximumValue)][!!LongDescription];
}