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
#  define KW32MLDECLSPEC
#  pragma comment(lib, "KexW32ML.lib")
#endif

#define KW32MLAPI WINAPI

#define GetLastErrorAsString() Win32ErrorAsString(GetLastError())
#define NtStatusAsString(Status) Win32ErrorAsString(RtlNtStatusToDosErrorNoTeb(Status))

// Use CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) instead
#pragma deprecated(CoInitialize)

KW32MLDECLSPEC LONGLONG KW32MLAPI CompareFileTimes(
	IN	FILETIME	FileTime1,
	IN	FILETIME	FileTime2);

KW32MLDECLSPEC PWSTR KW32MLAPI GetCommandLineWithoutImageName(
	VOID);

KW32MLDECLSPEC BOOLEAN KW32MLAPI PathReplaceIllegalCharacters(
	IN	PWSTR	Path,
	IN	WCHAR	ReplacementCharacter,
	IN	BOOLEAN	AllowPathSeparators);

KW32MLDECLSPEC PCWSTR KW32MLAPI Win32ErrorAsString(
	IN	ULONG	Win32ErrorCode);

KW32MLDECLSPEC ULONG KW32MLAPI RegReadI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PULONG	Data);

KW32MLDECLSPEC ULONG KW32MLAPI RegWriteI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	ULONG	Data);

KW32MLDECLSPEC ULONG KW32MLAPI RegReadString(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PWSTR	Buffer,
	IN	ULONG	BufferCch);

KW32MLDECLSPEC ULONG KW32MLAPI RegWriteString(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	PCWSTR	Data);

KW32MLDECLSPEC BOOLEAN KW32MLAPI RegReOpenKey(
	IN OUT	PHKEY		KeyHandle,
	IN		ACCESS_MASK	NewAccessMask,
	IN		HANDLE		TransactionHandle OPTIONAL);

KW32MLDECLSPEC EXTERN_C BOOLEAN KW32MLAPI SupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile,
	IN	HANDLE	TransactionHandle OPTIONAL);

KW32MLDECLSPEC HANDLE KW32MLAPI CreateSimpleTransaction(
	IN	PCWSTR	Description);