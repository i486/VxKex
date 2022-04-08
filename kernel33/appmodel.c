#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

#include "appmodel.h"

WINBASEAPI DWORD WINAPI ClosePackageInfo(
	IN	PACKAGE_INFO_REFERENCE	PackageInfoReference)
{
	return ERROR_NOT_SUPPORTED;
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