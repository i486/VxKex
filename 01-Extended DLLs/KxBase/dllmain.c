///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for KXBASE.
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
//     vxiiduu              10-Feb-2024  Rename to KXBASE (from kernel33).
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>

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

		//
		// Add the Kex32/Kex64 directory to the Base default DLL load path.
		// This path is applicable when LoadLibrary(Ex) is called, and it is also
		// part of the path used for resolving static imports of those libraries
		// which were loaded using LoadLibrary(Ex).
		//
		
		KxBaseAddKex3264ToBaseDefaultPath();

		//
		// If we are doing SharedUserData-based version spoofing, we need to
		// hook GetSystemTime and GetSystemTimeAsFileTime.
		//

		if (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA) {
			KexHkInstallBasicHook(GetSystemTime, KxBasepGetSystemTimeHook, NULL);
			KexHkInstallBasicHook(GetSystemTimeAsFileTime, KxBasepGetSystemTimeAsFileTimeHook, NULL);
		}

		//
		// Hook HeapCreate. See KxBasepHeapCreateHook to understand why.
		//

		KexHkInstallBasicHook(HeapCreate, KxBasepHeapCreateHook, NULL);

		//
		// Use AddDllDirectory to ensure that Kex32/Kex64 is always in the DLL
		// search path.
		//

		{
			WCHAR Kex3264Dir[MAX_PATH];

			StringCchPrintf(
				Kex3264Dir,
				ARRAYSIZE(Kex3264Dir),
				L"%wZ\\Kex%d",
				&KexData->KexDir,
				KexRtlCurrentProcessBitness());

			Ext_AddDllDirectory(Kex3264Dir);
		}

		//
		// Patch subsystem version check inside CreateProcessInternalW.
		//

		KexPatchCpiwSubsystemVersionCheck();
	}

	return TRUE;
}