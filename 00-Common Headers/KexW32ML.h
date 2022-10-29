///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexW32ML.h
//
// Abstract:
//
//     VxKex Win32-mode General Library Header file
//
// Author:
//
//     vxiiduu (02-Oct-2022)
//
// Environment:
//
//     This header is automatically included in EXE targets.
//     It may be used in DLLs which are to be loaded in kex processes.
//     Do not use in native DLLs.
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef KEX_ENV_NATIVE
#  error This header file cannot be used in a native mode project.
#endif

#ifndef KW32MLDECLSPEC
#  define KW32MLDECLSPEC DECLSPEC_IMPORT
#  pragma comment(lib, "KexW32ML.lib")
#endif

#define KW32MLAPI WINAPI

#define GetLastErrorAsString() Win32ErrorAsString(GetLastError())
#define NtStatusAsString(Status) Win32ErrorAsString(RtlNtStatusToDosError(Status))

KW32MLDECLSPEC EXTERN_C LONGLONG KW32MLAPI CompareFileTimes(
	IN	FILETIME	FileTime1,
	IN	FILETIME	FileTime2);

KW32MLDECLSPEC EXTERN_C PWSTR KW32MLAPI GetCommandLineWithoutImageName(
	VOID);

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI PathReplaceIllegalCharacters(
	IN	PWSTR	Path,
	IN	WCHAR	ReplacementCharacter,
	IN	BOOLEAN	AllowPathSeparators);

KW32MLDECLSPEC EXTERN_C PCWSTR KW32MLAPI Win32ErrorAsString(
	IN	ULONG	Win32ErrorCode);

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegReadI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PULONG	Data);

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegWriteI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	ULONG	Data);