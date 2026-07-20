///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxbasep.h
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
//     vxiiduu              04-May-2026  Add BaseIsConsoleAnsiSupportEnabled
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxBase.h>

EXTERN PKEX_PROCESS_DATA KexData;
EXTERN HANDLE KsecDD;

GEN_STD_TYPEDEFS(DYNAMIC_TIME_ZONE_INFORMATION);

typedef struct _REG_TZI_FORMAT {
	LONG		Bias;
	LONG		StandardBias;
	LONG		DaylightBias;
	SYSTEMTIME	StandardDate;
	SYSTEMTIME	DaylightDate;
} TYPEDEF_TYPE_NAME(REG_TZI_FORMAT);

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
// conansi.c
//

BOOLEAN BaseIsConsoleAnsiSupportEnabled(
	IN	HANDLE	ConsoleHandle);

BOOLEAN BaseEnableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle);

BOOLEAN BaseDisableConsoleAnsiSupport(
	IN	HANDLE	ConsoleHandle);

BOOL WriteConsoleWithEscapeSequencesAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWrittenOut OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode);

//
// consup.c
//

BOOLEAN VTGetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	OUT	PCOORD	Position);

BOOLEAN VTSetCursorPosition(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	AbsolutePosition);

BOOLEAN VTSetCursorHorizontalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	HorizontalPosition);

BOOLEAN VTSetCursorVerticalPosition(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	VerticalPosition);

BOOLEAN VTMoveCursorRelative(
	IN	HANDLE	ConsoleHandle,
	IN	COORD	RelativeMovement);

BOOLEAN VTMoveCursorVerticalRelativeWithHorizontalReset(
	IN	HANDLE	ConsoleHandle,
	IN	SHORT	RelativeMovement);

BOOLEAN GetConsoleTextAttribute(
	IN	HANDLE	ConsoleHandle,
	OUT	PWORD	Attribute);

BOOLEAN InjectStringToConsoleInput(
	IN	HANDLE	ConsoleHandle,
	IN	PCWSTR	String);

BOOL WriteConsoleAorW(
	IN	HANDLE	ConsoleHandle,
	IN	PCVOID	Buffer,
	IN	ULONG	CchToWrite,
	OUT	PULONG	CchWritten OPTIONAL,
	IN	PVOID	Reserved OPTIONAL,
	IN	BOOLEAN	Unicode);

BOOLEAN IsConsoleOutputHandle(
	IN	HANDLE	Handle);

BOOLEAN IsConsoleInputHandle(
	IN	HANDLE	Handle);

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