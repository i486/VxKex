///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tbs.c
//
// Abstract:
//
//     Implements extended TPM-related APIs.
//
// Author:
//
//     vxiiduu (16-Jul-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               16-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxmip.h"
#include <tbs.h>

//
// This is used to support Chromium versions 150 and higher. Those Chromium
// versions delay-load this function from tbs.dll, but Windows 7 prior to build
// 6.1.7601.19146 did not have Tbsi_GetDeviceInfo.
//
// This causes some kind of fast fail/debug assertion to go off even in release
// builds of Chromium. No doubt this was not intended but since all officially
// supported OSes have this function they probably didn't catch it.
//
TBS_RESULT WINAPI Ext_Tbsi_GetDeviceInfo(
	IN	UINT32	Size,
	OUT	PVOID	Info)
{
	STATIC TBS_RESULT (WINAPI *Tbsi_GetDeviceInfo)(UINT32 Size, PVOID Info) = NULL;
	STATIC BOOLEAN AlreadyInitialized = FALSE;

	if (!AlreadyInitialized) {
		HMODULE Tbs;

		Tbs = LoadLibrary(L"tbs.dll");

		if (Tbs != NULL) {
			Tbsi_GetDeviceInfo = (TBS_RESULT (WINAPI *)(UINT32, PVOID)) GetProcAddress(
				Tbs,
				"Tbsi_GetDeviceInfo");
		}

		AlreadyInitialized = TRUE;
	}

	if (!Tbsi_GetDeviceInfo) {
		return TBS_E_TPM_NOT_FOUND;
	}

	return Tbsi_GetDeviceInfo(Size, Info);
}