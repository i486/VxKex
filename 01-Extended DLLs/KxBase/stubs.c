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