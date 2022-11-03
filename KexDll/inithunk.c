///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     inithunk.c
//
// Abstract:
//
//     Contains the APC function which will be called during propagation.
//
// Author:
//
//     vxiiduu (03-Nov-2022)
//
// Revision History:
//
//     vxiiduu              03-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// KexDllInitializeThunk is called BEFORE LdrInitializeThunk, if it is called
// at all. This means you CANNOT call *ANY* NTDLL imports, directly or
// indirectly, until we can self-snap all NTDLL imports.
//
// Exception handling doesn't work here, either. So this code cannot contain
// any bugs or coding errors.
//
VOID NTAPI KexDllInitializeThunk(
	IN	PVOID	NormalContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2)
{
	NtCurrentPeb()->SubSystemData = (PVOID) 0xDEADBEEF;
	return;
}