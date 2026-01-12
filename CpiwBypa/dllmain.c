///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     This DLL is a very small stub that gets loaded into Explorer at startup
//     if the user hasn't disabled CPIW bypass for Explorer.
//
//     The mechanism of loading is by being registered as a browser helper
//     object (BHO). This means it can get loaded into Internet Explorer and
//     potentially other processes as well, so we have to check whether we are
//     actually loaded into %SystemRoot%\explorer.exe. KxCfgHlp does set some
//     registry keys to stop this from happening but it's better safe than
//     sorry.
//
//     Its job is to patch CreateProcessInternalW to bypass the subsystem
//     version check and then get unloaded (by returning FALSE from dllmain).
//
// Author:
//
//     Author (DD-MMM-YYYY)
//
// Environment:
//
//     WHERE CAN THIS PROGRAM/LIBRARY/CODE RUN?
//
// Revision History:
//
//     Author               DD-MMM-YYYY  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>

BOOL WINAPI DllMain(
	IN	HMODULE		DllHandle,
	IN	ULONG		Reason,
	IN	PCONTEXT	Context OPTIONAL)
{
	WCHAR KexDllFullPath[MAX_PATH];
	PPEB Peb;
	HMODULE KexDll;
	NTSTATUS (NTAPI *pKexPatchCpiwSubsystemVersionCheck)(VOID);

	if (KexIsDebugBuild) {
		WCHAR ExplorerFullPath[MAX_PATH];
		WCHAR ExeFullPath[MAX_PATH];

		//
		// Check if we are loaded into Windows Explorer or KexCfg.
		//

		GetModuleFileName(NULL, ExeFullPath, ARRAYSIZE(ExeFullPath));
		GetWindowsDirectory(ExplorerFullPath, ARRAYSIZE(ExplorerFullPath));
		PathCchAppend(ExplorerFullPath, ARRAYSIZE(ExplorerFullPath), L"explorer.exe");

		if (!StringEqualI(ExeFullPath, ExplorerFullPath) &&
			!StringEqualI(PathFindFileName(ExeFullPath), L"KexCfg.exe")) {

			// Neither explorer nor kexcfg, this shouldn't happen.
			ASSERT (FALSE);
		}
	}

	//
	// Check if we have already patched CPIW subsystem check in this process.
	// See comment further down.
	//

	Peb = NtCurrentPeb();

	if (Peb->SpareBits0 & 1) {
		// Already patched.
		return FALSE;
	}

	//
	// We will go ahead with the patching. First, load KexDll so we can use its
	// routine to patch the required locations in kernel32.
	//

	GetSystemDirectory(KexDllFullPath, ARRAYSIZE(KexDllFullPath));
	PathCchAppend(KexDllFullPath, ARRAYSIZE(KexDllFullPath), L"KexDll.dll");
	KexDll = LoadLibrary(KexDllFullPath);

	if (!KexDll) {
		return FALSE;
	}

	pKexPatchCpiwSubsystemVersionCheck = (NTSTATUS (NTAPI *) (VOID)) GetProcAddress(
		KexDll,
		"KexPatchCpiwSubsystemVersionCheck");

	if (!pKexPatchCpiwSubsystemVersionCheck) {
		return FALSE;
	}

	pKexPatchCpiwSubsystemVersionCheck();

	//
	// Signal to potential future instances of this DLL that we've already patched
	// the version check, and prevent us from trying to patch it again.
	//
	// I chose the spare bits in the PEB bitfield because it seemed like a good place
	// to stash a boolean value.
	//

	Peb->SpareBits0 |= 1;

	FreeLibrary(KexDll);
	return FALSE;
}