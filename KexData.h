#pragma once

typedef struct _KEX_IFEO_PARAMETERS {
	DWORD	dwEnableVxKex;
	WCHAR	szWinVerSpoof[6];
	DWORD	dwAlwaysShowDebug;
	DWORD	dwDisableForChild;
	DWORD	dwDisableAppSpecific;
	DWORD	dwWaitForChild;
	DWORD	dwDebuggerSpoof;
} KEX_IFEO_PARAMETERS, *PKEX_IFEO_PARAMETERS, *LPKEX_IFEO_PARAMETERS;

// KexData is basically like a "PEB" but contains specific data related to VxKex.
// The definition of this structure is NOT fixed or stable or documented for
// external usages, since it is created dynamically on process creation and not
// used outside of VxKex.
//
// A pointer to a process's KEX_PROCESS_DATA is stored in Peb->SubSystemData.

typedef struct _KEX_PROCESS_DATA {
	KEX_IFEO_PARAMETERS	IfeoParameters;
	WCHAR				szKexDir[MAX_PATH];					// full Win32 path to KexDir e.g. C:\Program Files\VxKex
} KEX_PROCESS_DATA, *PKEX_PROCESS_DATA, *LPKEX_PROCESS_DATA;