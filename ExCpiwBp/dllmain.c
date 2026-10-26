///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     This is the implementation of ExCpBp32.dll/ExCpBp64.dll, which is a
//     temporary DLL used during a new install of VxKex. This DLL is injected
//     into Explorer.
//
//     Our responsibility here is to call KexPatchCpiwSubsystemVersionCheck in
//     kexdll (which should already be installed into system32 by this point).
//     And then we need to return FALSE from DllMain in order to cause the
//     loader to automatically clean up and unload this DLL.
//
// Author:
//
//     vxiiduu (23-Oct-2026)
//
// Environment:
//
//     Inside Explorer.
//
// Revision History:
//
//     vxiiduu               23-Oct-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

BOOL WINAPI DllMain(
	IN	HMODULE	ModuleHandle,
	IN	ULONG	Reason,
	IN	PVOID	Reserved)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		KexPatchCpiwSubsystemVersionCheck();
	}

	return FALSE;
}