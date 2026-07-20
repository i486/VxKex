///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     stubs.c
//
// Abstract:
//
//     Another stub file to satisfy shitty code that doesn't support forwarders
//
// Author:
//
//     vxiiduu (02-Jul-2026)
//
// Revision History:
//
//     vxiiduu              02-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxnetp.h"

//
// At least one Steam API emulator requires this to be a stub function,
// not a forwarder.
// I have no idea why it has a problem with WSASetLastError specifically
// because every other forwarder in VxKex works just fine.
//
// Here's the details from one of them:
//
//   File name						steam_api64.dll
//   File size						3,575,808 bytes
//   File description				Steam Client API
//   File version					8.33.9.23
//   Product name					Steam Client API
//   Product version				01.00.00.01
//   Copyright						Copyright (C) 2007
//   Language						Chinese (Simplified, PRC)
//   Original filename				steam_api.dll
//
// I believe it is the ALI213 emulator.
//
KXNETAPI VOID WINAPI Stub_WSASetLastError(
	IN	INT	LastError)
{
	WSASetLastError(LastError);
}