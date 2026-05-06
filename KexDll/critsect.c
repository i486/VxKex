///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     critsect.c
//
// Abstract:
//
//     Extended APIs related to critical sections.
//
// Author:
//
//     vxiiduu (16-Feb-2026)
//
// Revision History:
//
//     vxiiduu              16-Feb-2026  Initial creation.
//     vxiiduu              17-Feb-2026  Make critical sections not have debug
//                                       info by default (perf bonus?)
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE		((ULONG) 0x08000000)
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO	((ULONG) 0x10000000)

KEXAPI NTSTATUS NTAPI Ext_RtlInitializeCriticalSectionEx(
	IN	PRTL_CRITICAL_SECTION	CriticalSection,
	IN	ULONG					SpinCount,
	IN	ULONG					Flags)
{
	//
	// Check for invalid flag combinations.
	//

	if ((Flags & RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO) &&
		(Flags & RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)) {

		return STATUS_INVALID_PARAMETER_3;
	}

	//
	// Make critical sections not have debug info by default.
	// Creating a debug info structure for a critical section requires a
	// heap allocation. Since InitializeCriticalSection is called often,
	// this causes a small performance degradation, which Microsoft fixed
	// in Win8+.
	//

	if (Flags & RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO) {
		// Do nothing except remove the "force debug info" flag (so that later
		// code does not log it as an anomaly).
		// On Win7, by default, debug info is always created.
		Flags &= ~RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO;
	} else {
		// If user did not specify debug info must be created, then
		// we will not create debug info.
		Flags |= RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO;
	}

	//
	// Remove unrecognized Flags values, if present.
	//
	// Flag values recognized in Windows 7:
	//   RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO
	//   RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN
	//   RTL_CRITICAL_SECTION_FLAG_STATIC_INIT
	//
	// Flag values not recognized in Windows 7 (added in later versions):
	//   RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE	(0x08000000)
	//   RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO	(0x10000000)
	//

	if (Flags & RTL_CRITICAL_SECTION_FLAG_RESERVED) {
		KexLogDebugEvent(
			L"RtlInitializeCriticalSectionEx called with unrecognized flags\r\n\r\n"
			L"SpinCount: %lu\r\n"
			L"Flags:     0x%08lx",
			SpinCount,
			Flags);

		Flags &= ~RTL_CRITICAL_SECTION_FLAG_RESERVED;
	}

	return RtlInitializeCriticalSectionEx(
		CriticalSection,
		SpinCount,
		Flags);
}

KEXAPI NTSTATUS NTAPI Ext_RtlInitializeCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection)
{
	return Ext_RtlInitializeCriticalSectionEx(
		CriticalSection,
		0,
		0);
}

KEXAPI NTSTATUS NTAPI Ext_RtlInitializeCriticalSectionAndSpinCount(
	IN	PRTL_CRITICAL_SECTION	CriticalSection,
	IN	ULONG					SpinCount)
{
	return Ext_RtlInitializeCriticalSectionEx(
		CriticalSection,
		SpinCount & 0xFFFFFF,
		0);
}