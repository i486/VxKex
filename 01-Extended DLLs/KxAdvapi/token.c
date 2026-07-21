///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     token.c
//
// Abstract:
//
//     Contains functions related to security tokens.
//
// Author:
//
//     vxiiduu (12-Jul-2026)
//
// Revision History:
//
//     vxiiduu              12-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxadvapip.h"

//
// This Ext_ function only applies to programs that load from advapi32.
// There is an OpenProcessToken export in kernel32 as well, so if this is
// used for anything other than Vulkan, make sure to take care of that.
//
KXADVAPI BOOL WINAPI Ext_OpenProcessToken(
	IN	HANDLE	ProcessHandle,
	IN	ULONG	DesiredAccess,
	OUT	PHANDLE	TokenHandle)
{
	if (ProcessHandle == NtCurrentProcess() &&
		DesiredAccess == (TOKEN_QUERY | TOKEN_QUERY_SOURCE) &&
		AshModuleBaseNameIs(ReturnAddress(), L"vulkan-1.dll")) {

		//
		// The Vulkan runtime loader ignores environment variables for "security"
		// reasons if running as administrator. VxKex uses the environment variables
		// for disabling Vulkan for apps that shouldn't have it enabled; therefore
		// we have to get rid of this behavior by always failing.
		//
		// vulkan-1.dll only calls OpenProcessToken in this context.
		//

		*TokenHandle = NULL;
		return FALSE;
	}

	return OpenProcessToken(ProcessHandle, DesiredAccess, TokenHandle);
}