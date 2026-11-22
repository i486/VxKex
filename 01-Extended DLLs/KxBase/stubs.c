///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     stubs.c
//
// Abstract:
//
//     Forwarder stubs that do nothing except for calling the original function.
//     These exist because some stupid software such as Chromium is not
//     compatible with export forwarders.
//
//     Stubs are also needed to support Themida-protected applications.
//
// Author:
//
//     vxiiduu (09-Mar-2024)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu              09-Mar-2024  Initial creation.
//     vxiiduu              27-Apr-2026  Add more stubs to support Themida.
//     vxiiduu              02-May-2026  Move GetProcAddress to module.c.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

// required for Chromium
KXBASEAPI BOOL WINAPI Stub_DuplicateHandle(
	IN	HANDLE		SourceProcessHandle,
	IN	HANDLE		SourceHandle,
	IN	HANDLE		TargetProcessHandle,
	OUT	PHANDLE		TargetHandle,
	IN	ACCESS_MASK	DesiredAccess,
	IN	BOOL		Inherit,
	IN	ULONG		Options)
{
	//
	// Starting from Windows 8, a process handle which only has the
	// PROCESS_QUERY_LIMITED_INFORMATION access right is sufficient for
	// VirtualQueryEx. On Windows 7, PROCESS_QUERY_INFORMATION is required.
	//
	// Some applications rely on the newer behavior. In particular, the
	// Cygwin/MSYS2 runtime (3.5 and above) duplicates a handle to its own
	// process with limited query access and gives it to forked children,
	// which then call VirtualQueryEx on it. On Windows 7 that fails with
	// ERROR_ACCESS_DENIED and makes fork() fail for any process which has
	// private memory mappings.
	//
	// To emulate the newer behavior, when the caller duplicates a process
	// handle from the current process and asks for limited query access, we
	// try to add PROCESS_QUERY_INFORMATION as well. If that doesn't work
	// (e.g. the source handle doesn't have that access), we fall back to
	// exactly what the caller asked for.
	//
	// This isn't done if DUPLICATE_CLOSE_SOURCE is specified, because the
	// source handle is closed even when duplication fails, so there would
	// be nothing left to fall back to.
	//

	if (!(Options & (DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE)) &&
		(DesiredAccess & PROCESS_QUERY_LIMITED_INFORMATION) &&
		!(DesiredAccess & PROCESS_QUERY_INFORMATION) &&
		SourceProcessHandle == GetCurrentProcess()) {

		ULONG LastError;
		BOOL SourceIsProcessHandle;

		//
		// Make sure the source handle really is a process handle, since the
		// access mask bits mean different things for other object types.
		// GetProcessId only succeeds on process handles.
		//

		LastError = GetLastError();
		SourceIsProcessHandle = (GetProcessId(SourceHandle) != 0);
		SetLastError(LastError);

		if (SourceIsProcessHandle) {
			BOOL Success;

			Success = DuplicateHandle(
				SourceProcessHandle,
				SourceHandle,
				TargetProcessHandle,
				TargetHandle,
				DesiredAccess | PROCESS_QUERY_INFORMATION,
				Inherit,
				Options);

			if (Success) {
				return TRUE;
			}

			SetLastError(LastError);
		}
	}

	return DuplicateHandle(
		SourceProcessHandle,
		SourceHandle,
		TargetProcessHandle,
		TargetHandle,
		DesiredAccess,
		Inherit,
		Options);
}

// required for Themida
KXBASEAPI PVOID WINAPI Stub_VirtualAlloc(
	IN	PVOID		Address OPTIONAL,
	IN	SIZE_T		Size,
	IN	ULONG		AllocationType,
	IN	ULONG		ProtectionMask)
{
	return VirtualAlloc(
		Address,
		Size,
		AllocationType,
		ProtectionMask);
}