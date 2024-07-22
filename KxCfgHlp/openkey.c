///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     openkey.c
//
// Abstract:
//
//     Contains functions for opening the VxKex HKLM/HKCU key.
//
// Author:
//
//     vxiiduu (02-Feb-2024)
//
// Environment:
//
//     Win32 mode. This code must be able to run without KexDll, as it is used
//     in KexSetup. This code must function properly when run under WOW64.
//
// Revision History:
//
//     vxiiduu              02-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>

//
// Open the HKLM or HKCU VxKex key.
// This function does not allow WOW64 redirection.
// Returns NULL on failure. Call GetLastError for more information.
//
KXCFGDECLSPEC HKEY KXCFGAPI KxCfgOpenVxKexRegistryKey(
	IN	BOOLEAN		PerUserKey,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL)
{
	return KxCfgpOpenKey(
		PerUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
		L"Software\\VXsoft\\VxKex",
		DesiredAccess,
		TransactionHandle);
}

//
// Open the legacy, pre-rewrite VxKex key.
// This function does not allow WOW64 redirection.
// Returns NULL on failure. Call GetLastError for more information.
//
KXCFGDECLSPEC HKEY KXCFGAPI KxCfgOpenLegacyVxKexRegistryKey(
	IN	BOOLEAN		PerUserKey,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL)
{
	return KxCfgpOpenKey(
		PerUserKey ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE,
		L"Software\\VXsoft\\VxKexLdr",
		DesiredAccess,
		TransactionHandle);
}