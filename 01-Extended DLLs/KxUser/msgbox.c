///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     msgbox.c
//
// Abstract:
//
//     Implements extended messagebox functions.
//     They can be used to automatically dismiss nag messages from applications,
//     if the application continues execution after the message box.
//
// Author:
//
//     vxiiduu (30-Jun-2026)
//
// Revision History:
//
//     vxiiduu              30-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI INT WINAPI Ext_MessageBoxA(
	IN	HWND	Window OPTIONAL,
	IN	PCSTR	Text OPTIONAL,
	IN	PCSTR	Caption OPTIONAL,
	IN	UINT	Type)
{
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		//
		// APPSPECIFICHACK: Get rid of nag message for osu!lazer.
		// This is unlikely to actually fix online multiplayer, but it will squash the
		// nag message.
		//
		// Online multiplayer for osu!lazer seems to use some sort of weird obfuscated
		// "anti-cheat" DLL. Reverse engineering it and fixing it is beyond the scope
		// of this project. It probably checks SharedUserData or PEB for the Windows
		// version, which cannot be spoofed without breaking .NET.
		//

		if (AshExeBaseNameIs(L"osu!.exe")) {
			if (Caption && StringEqualA(Caption, "Unsupported operating system")) {
				return IDCANCEL;
			}
		}
	}

	return MessageBoxA(Window, Text, Caption, Type);
}