///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for KXBASE.
//
// Author:
//
//     vxiiduu (10-Feb-2022)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              10-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxuserp.h"

PKEX_PROCESS_DATA KexData = NULL;

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		LdrDisableThreadCalloutsForDll(DllBase);

		KexDataInitialize(&KexData);
		KexLogDebugEvent(L"DllMain called with DLL_PROCESS_ATTACH");
	}

	return TRUE;
}