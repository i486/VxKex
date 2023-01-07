///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for kernel33.
//     The DLLs "kernel32" and "kernelbase" are redirected to this DLL.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Environment:
//
//     Win32 mode.
//     This module can import from NTDLL, KERNEL32, and KERNELBASE.
//
// Revision History:
//
//     vxiiduu              07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "basedllp.h"

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	LdrDisableThreadCalloutsForDll(DllBase);
	return TRUE;
}