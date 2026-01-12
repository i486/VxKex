///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     support.c
//
// Abstract:
//
//     Contains miscellaneous support routines used by KXBASE.
//
// Author:
//
//     vxiiduu (11-Feb-2022)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              11-Feb-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

//
// This function translates a Win32-style timeout (milliseconds) into a
// NT-style timeout (100-nanosecond intervals).
//
// The return value of this function can be directly passed to most
// NT APIs that accept a timeout parameter.
//
PLARGE_INTEGER BaseFormatTimeOut(
	OUT	PLARGE_INTEGER	TimeOut,
	IN	ULONG			Milliseconds)
{
	if (Milliseconds == INFINITE) {
		return NULL;
	}

	TimeOut->QuadPart = UInt32x32To64(Milliseconds, 10000);
	TimeOut->QuadPart *= -1;
	return TimeOut;
}