///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     Helper application invoked by KexSetup to allow injection of 64-bit
//     excpbp64.dll into a 64-bit explorer.exe on 64-bit systems
//
// Author:
//
//     vxiiduu (22-Oct-2026)
//
// Environment:
//
//     Win32, launched from KexSetup, just after VxKex has been installed.
//
// Revision History:
//
//     vxiiduu               22-Oct-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>

NTSTATUS PatchAllExplorers(
	VOID);

NTSTATUS NTAPI EntryPoint(
	IN	PPEB	Peb)
{
	KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	return PatchAllExplorers();
}