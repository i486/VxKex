///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     BaseDll.h
//
// Abstract:
//
//     Win32 base API (i.e. kernel32 and kernelbase)
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu               07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>

#pragma region Structures and Typedefs

typedef struct _KERNELBASE_GLOBAL_DATA {
	PUNICODE_STRING			BaseDefaultPath;
	PRTL_CRITICAL_SECTION	BaseDefaultPathLock;
	PUNICODE_STRING			BaseDllDirectory;
	PRTL_CRITICAL_SECTION	BaseDllDirectoryLock;
	PULONG					BaseSearchPathMode;
	PRTL_SRWLOCK			BaseSearchPathModeLock;

	//
	// There are more entries past this, but should not be used as they
	// change between OS builds and don't contain anything interesting.
	// Mostly just NLS crap, locale tables and whatever. There are also
	// some function pointers to some pretty irrelevant functions.
	//
} TYPEDEF_TYPE_NAME(KERNELBASE_GLOBAL_DATA);

#pragma endregion

#pragma region thread.c

WINBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	ThreadHandle,
	OUT	PPWSTR	ThreadDescription);

WINBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	ThreadHandle,
	IN	PCWSTR	ThreadDescription);

#pragma endregion