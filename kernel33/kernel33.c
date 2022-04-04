#include <Windows.h>
#include <KexDll.h>
#include <BaseDll.h>

#include "k32defs.h"

//
// EXPORTED FUNCTIONS
//

BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
	}

	return TRUE;
}

WINBASEAPI BOOL WINAPI GetCurrentPackageId(
	IN OUT	LPDWORD	lpdwBufferLength,
	OUT		LPBYTE	buffer)
{
	ODS_ENTRY();

	if (!lpdwBufferLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*lpdwBufferLength && !buffer) {
		return ERROR_INVALID_PARAMETER;
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}