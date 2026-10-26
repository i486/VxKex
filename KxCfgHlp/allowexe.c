///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     allowexe.c
//
// Abstract:
//
//     Contains a routine for determining whether the user is allowed to
//     enable VxKex for a particular EXE. This code used to be scattered
//     throughout KexCfg and KexShlEx etc. but was centralized here.
//
// Author:
//
//     vxiiduu (25-Oct-2026)
//
// Environment:
//
//     inside KexShlEx or KexCfg
//
// Revision History:
//
//     vxiiduu               25-Oct-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>

//
// ExeFullPath must be an absolute Win32 path.
//
KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgIsUserAllowedToEnableVxKexForFile(
	IN	PCWSTR	FullPath)
{
	BOOLEAN Success;
	STATIC WCHAR WinDir[MAX_PATH] = L"";
	STATIC WCHAR KexDir[MAX_PATH] = L"";

	ASSERT (!PathIsRelative(FullPath));

	if (WinDir[0] == '\0') {
		GetSystemWindowsDirectory(WinDir, ARRAYSIZE(WinDir));
	}

	if (KexDir[0] == '\0') {
		Success = KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));
		ASSERT (Success);

		if (!Success) {
			return TRUE;
		}
	}

	if (PathIsPrefix(KexDir, FullPath)) {
		// Exe is in the VxKex installation directory. Not permitted.
		return FALSE;
	}

	if (PathIsPrefix(WinDir, FullPath)) {
		PCWSTR BaseName;

		// Exe is in the Windows directory.
		BaseName = PathFindFileName(FullPath);

		if (StringEqualI(BaseName, L"powershell.exe") ||
			StringEqualI(BaseName, L"powershell_ise.exe")) {

			// Allow enabling VxKex for Powershell. PS 5.1 benefits from VxKex, even
			// though it already runs on Win7, because many of its built-in features
			// (Update-Help, Invoke-WebRequest) require KxSChanl to work properly.
			return TRUE;
		}

		// Otherwise, cannot enable for Windows exe.
		return FALSE;
	}

	// Otherwise, enabling VxKex is allowed.
	return TRUE;
}