///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllrewrt.c
//
// Abstract:
//
//     Contains a data table indicating which DLLs are redirected to which.
//
// Author:
//
//     vxiiduu (06-Feb-2024)
//
// Environment:
//
//     N/A
//
// Revision History:
//
//     vxiiduu              06-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>

CONST PCWSTR KxCfgDllRedirects[][2] = {
	{L"ntdll",										L"kxnt"			},
	{L"kernel32",									L"kxbase"		},
	{L"kernelbase",									L"kxbase"		},
	{L"user32",										L"kxuser"		},
	{L"ole32",										L"kxcom"		},
	{L"shcore",										L"kxuser"		},
	{L"combase",									L"kxcom"		},
	{L"dxgi",										L"kxdx"			},
	{L"BluetoothApis",								L"kxuser"		},
	{L"powrprof",									L"kxbase"		},
	{L"bcrypt",										L"kxbase"		},
	{L"bcryptprimitives",							L"kxbase"		},
	{L"msvcrt",										L"msvw10"		},
	{L"dcomp",										L"dcow8"		},
	{L"userenv",									L"kxbase"		},

	{L"api-ms-win-appmodel-identity",				L"kxbase"		},
	{L"api-ms-win-appmodel-runtime",				L"kxbase"		},
	{L"api-ms-win-core-apiquery",					L"kxnt"			},
	{L"api-ms-win-core-atoms",						L"kxbase"		},
	{L"api-ms-win-core-crt",						L"msvw10"		},
	{L"api-ms-win-core-com",						L"kxcom"		},
	{L"api-ms-win-core-console",					L"kxbase"		},
	{L"api-ms-win-core-datetime",					L"kxbase"		},
	{L"api-ms-win-core-debug",						L"kxbase"		},
	{L"api-ms-win-core-delayload",					L"kxbase"		},
	{L"api-ms-win-core-errorhandling",				L"kxbase"		},
	{L"api-ms-win-core-fibers",						L"kxbase"		},
	{L"api-ms-win-core-file",						L"kxbase"		},
	{L"api-ms-win-core-handle",						L"kxbase"		},
	{L"api-ms-win-core-heap",						L"kxbase"		},
	{L"api-ms-win-core-heap-obsolete",				L"kxbase"		},
	{L"api-ms-win-core-interlocked",				L"kxbase"		},
	{L"api-ms-win-core-io",							L"kxbase"		},
	{L"api-ms-win-core-kernel32-legacy",			L"kxbase"		},
	{L"api-ms-win-core-largeinteger",				L"kxbase"		},
	{L"api-ms-win-core-libraryloader",				L"kxbase"		},
	{L"api-ms-win-core-localization",				L"kxbase"		},
	{L"api-ms-win-core-localization-obsolete",		L"kxbase"		},
	{L"api-ms-win-core-memory",						L"kxbase"		},
	{L"api-ms-win-core-path",						L"kxbase"		},
	{L"api-ms-win-core-privateprofile",				L"kxbase"		},
	{L"api-ms-win-core-processenvironment",			L"kxbase"		},
	{L"api-ms-win-core-processsnapshot",			L"kxbase"		},
	{L"api-ms-win-core-processthreads",				L"kxbase"		},
	{L"api-ms-win-core-processtopology",			L"kxbase"		},
	{L"api-ms-win-core-processtopology-obsolete",	L"kxbase"		},
	{L"api-ms-win-core-profile",					L"kxbase"		},
	{L"api-ms-win-core-psapi",						L"kxbase"		},
	{L"api-ms-win-core-quirks",						L"kxbase"		},
	{L"api-ms-win-core-registry",					L"advapi32"		},
	{L"api-ms-win-core-rtlsupport",					L"kxnt"			},
	{L"api-ms-win-core-shlwapi-legacy",				L"kxuser"		},
	{L"api-ms-win-core-shlwapi-obsolete",			L"kxuser"		},
	{L"api-ms-win-core-string-obsolete",			L"kxbase"		},
	{L"api-ms-win-core-sidebyside",					L"kxbase"		},
	{L"api-ms-win-core-string",						L"kxbase"		},
	{L"api-ms-win-core-string-obsolete",			L"kxbase"		},
	{L"api-ms-win-core-synch",						L"kxbase"		},
	{L"api-ms-win-core-sysinfo",					L"kxbase"		},
	{L"api-ms-win-core-systemtopology",				L"kxbase"		},
	{L"api-ms-win-core-threadpool",					L"kxbase"		},
	{L"api-ms-win-core-threadpool-legacy",			L"kxbase"		},
	{L"api-ms-win-core-threadpool-private",			L"kxbase"		},
	{L"api-ms-win-core-timezone",					L"kxbase"		},
	{L"api-ms-win-core-url",						L"kxuser"		},
	{L"api-ms-win-core-util",						L"kxbase"		},
	{L"api-ms-win-core-version",					L"version"		},
	{L"api-ms-win-core-versionansi",				L"version"		},
	{L"api-ms-win-core-windowserrorreporting",		L"kxbase"		},
	{L"api-ms-win-core-winrt",						L"kxcom"		},
	{L"api-ms-win-core-winrt-error",				L"kxcom"		},
	{L"api-ms-win-core-winrt-robuffer",				L"kxcom"		},
	{L"api-ms-win-core-winrt-string",				L"kxcom"		},
	{L"api-ms-win-core-wow64",						L"kxbase"		},
	{L"api-ms-win-devices-config",					L"cfgmgr32"		},
	//{L"api-ms-win-devices-query",					L"cfgmgr32"		},
	{L"api-ms-win-downlevel-kernel32",				L"kxbase"		},
	{L"api-ms-win-downlevel-ole32",					L"kxcom"		},
	{L"api-ms-win-eventing-classicprovider",		L"advapi32"		},
	{L"api-ms-win-eventing-provider",				L"advapi32"		},
	{L"api-ms-win-mm-time",							L"winmm"		},
	{L"api-ms-win-ntuser-sysparams",				L"kxuser"		},
	{L"api-ms-win-power-base",						L"kxbase"		},
	{L"api-ms-win-power-setting",					L"kxbase"		},
	{L"api-ms-win-security-base",					L"advapi32"		},
	{L"api-ms-win-security-lsalookup",				L"advapi32"		},
	{L"api-ms-win-security-sddl",					L"advapi32"		},
	{L"api-ms-win-security-systemfunctions",		L"advapi32"		},
	{L"api-ms-win-shcore-obsolete",					L"kxuser"		},
	{L"api-ms-win-shcore-scaling",					L"kxuser"		},
	{L"api-ms-win-shell-namespace",					L"kxuser"		},
	{L"ext-ms-win-kernel32-package-current",		L"kxbase"		},
	{L"ext-ms-win-uiacore",							L"uiautomationcore" },

	{L"xinput1_4",									L"xinput1_3"	},
};

CONST ULONG KxCfgNumberOfDllRedirects = ARRAYSIZE(KxCfgDllRedirects);