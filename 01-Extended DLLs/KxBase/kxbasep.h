///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexdllp.h
//
// Abstract:
//
//     Private header file for KxBase.
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu              07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxBase.h>

EXTERN PKEX_PROCESS_DATA KexData;
EXTERN HANDLE KsecDD;

//
// module.c
//

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName);

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName);

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut);

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut);

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameA(
	IN	HMODULE	ModuleHandle,
	OUT	PSTR	FileName,
	IN	ULONG	FileNameCch);

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameW(
	IN	HMODULE	ModuleHandle,
	OUT	PWSTR	FileName,
	IN	ULONG	FileNameCch);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryA(
	IN	PCSTR	FileName);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags);

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags);

//
// time.c
//

KXBASEAPI VOID WINAPI KxBasepGetSystemTimeAsFileTimeHook(
	OUT	PFILETIME	SystemTimeAsFileTime);

KXBASEAPI VOID WINAPI KxBasepGetSystemTimeHook(
	OUT	PSYSTEMTIME	SystemTime);

//
// support.c
//

PLARGE_INTEGER BaseFormatTimeOut(
	OUT	PLARGE_INTEGER	TimeOut,
	IN	ULONG			Milliseconds);

HANDLE WINAPI BaseGetNamedObjectDirectory(
	VOID);

HANDLE WINAPI BaseGetUntrustedNamedObjectDirectory(
	VOID);

PVOID BaseGetBaseDllHandle(
	VOID);

//
// dllpath.c
//

VOID KxBaseAddKex3264ToBaseDefaultPath(
	VOID);

//
// crypto.c
//

NTSTATUS BaseInitializeCrypto(
	VOID);