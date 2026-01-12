#include "buildcfg.h"
#include "kxbasep.h"
#include <KexStrSafe.h>

//
// Iertutil.dll contains a version check which will end up causing errors (failure
// to open files with ShellExecute including with shell context menus) if the reported
// OS version is higher than 6.1.
//
// This function un-spoofs the OS version for iertutil, and preserves normal behavior
// for all other situations.
//

KXBASEAPI BOOL WINAPI Ext_GetVersionExA(
	IN OUT	POSVERSIONINFOA	VersionInfo)
{
	BOOL Success;

	Success = GetVersionExA(VersionInfo);

	if (Success && AshModuleBaseNameIs(ReturnAddress(), L"iertutil")) {
		KexLogDebugEvent(L"Unspoofing OS version for iertutil.dll compatibility.");
		VersionInfo->dwMajorVersion = 6;
		VersionInfo->dwMinorVersion = 1;
	}

	return Success;
}