#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

#include "fileopcr.h"

WINBASEAPI HANDLE WINAPI CreateFile2(
	IN	LPCWSTR								lpszFileName,
	IN	DWORD								dwDesiredAccess,
	IN	DWORD								dwShareMode,
	IN	DWORD								dwCreationDisposition,
	IN	LPCREATEFILE2_EXTENDED_PARAMETERS	lpCreateExParams OPTIONAL)
{
	ODS_ENTRY(L"(\"%ws\", %I32u, %I32u, %I32u, %p)", lpszFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, lpCreateExParams);

	if (lpCreateExParams == NULL) {
		return CreateFileW(lpszFileName, dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, 0, NULL);
	}

	return CreateFileW(
		lpszFileName,
		dwDesiredAccess,
		dwShareMode,
		lpCreateExParams->lpSecurityAttributes,
		dwCreationDisposition,
		lpCreateExParams->dwFileAttributes | lpCreateExParams->dwFileFlags | lpCreateExParams->dwSecurityQosFlags,
		lpCreateExParams->hTemplateFile);
}