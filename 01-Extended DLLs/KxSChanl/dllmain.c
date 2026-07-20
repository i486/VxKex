///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     DLL initialization function.
//
// Author:
//
//     vxiiduu (09-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               09-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

PKEX_PROCESS_DATA KexData;

BOOL WINAPI DllMain(
	IN	PVOID		DllBase,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		KexDataInitialize(&KexData);
		LdrDisableThreadCalloutsForDll(DllBase);
	}

	return TRUE;
}