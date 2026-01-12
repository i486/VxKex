///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexdllp.h
//
// Abstract:
//
//     Private header file for KexDll.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

//
// These flags are the same as the ones in WinUser.h.
// Pass to KexMessageBox.
//

#define MB_ICONERROR			0x00000010
#define MB_ICONQUESTION			0x00000020
#define MB_ICONEXCLAMATION		0x00000030
#define MB_ICONINFORMATION		0x00000040

#define MB_OK					0x00000000

ULONG KexDllProtectedFunctionExceptionFilter(
	IN	PCWSTR				FunctionName,
	IN	NTSTATUS			ExceptionCode,
	IN	PEXCEPTION_POINTERS	ExceptionPointers);

//
// Protected Function Macros should be used on every function in KexDll.
// Usage of PROTECTED_FUNCTION(_END(_NOLOG)) wraps each function with SEH.
// It is particularly useful for creating syscall implementations or
// wrappers, since "real" syscalls never crash (unless there's a bug in the
// kernel).
//
// PROTECTED_FUNCTION_END_NOLOG causes the error not to be logged. This
// macro should only be used on functions that are directly involved in
// logging errors in order to avoid an infinite loop and stack overflow.
//
// PROTECTED_FUNCTION_END_BOOLEAN returns FALSE instead of a NTSTATUS.
//

#if DISABLE_PROTECTED_FUNCTION == FALSE
#  define PROTECTED_FUNCTION { try

#  define PROTECTED_FUNCTION_END \
	except (KexDllProtectedFunctionExceptionFilter(__FUNCTIONW__, GetExceptionCode(), GetExceptionInformation())) { \
		return GetExceptionCode(); \
	}}
#else
#  define PROTECTED_FUNCTION
#  define PROTECTED_FUNCTION_END
#endif

EXTERN PKEX_PROCESS_DATA KexData;

VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context);

NTSTATUS KexInitializeDllRewrite(
	VOID);

BOOLEAN KexShouldRewriteImportsOfDll(
	IN	PCUNICODE_STRING	BaseDllName,
	IN	PCUNICODE_STRING	FullDllName);

NTSTATUS KexRewriteImageImportDirectory(
	IN	PVOID				ImageBase,
	IN	PCUNICODE_STRING	BaseImageName,
	IN	PCUNICODE_STRING	FullImageName);

NTSTATUS KexHeInstallHandler(
	VOID);

NORETURN VOID KexHeErrorBox(
	IN	PCWSTR	ErrorMessage);

NTSTATUS KexpAddKex3264ToDllPath(
	VOID);

NTSTATUS KexDisableAVrf(
	VOID);

VOID KexApplyVersionSpoof(
	VOID);

NTSTATUS KexOpenVxlLogForCurrentApplication(
	OUT	PVXLHANDLE	LogHandle);

VOID AshApplyQt6EnvironmentVariableHacks(
	VOID);

VOID AshApplyQBittorrentEnvironmentVariableHacks(
	VOID);

//
// Logging (VXL)
//

NTSTATUS VxlpFlushLogFileHeader(
	IN	VXLHANDLE			LogHandle);

ULONG VxlpGetTotalLogEntryCount(
	IN	VXLHANDLE			LogHandle);

NTSTATUS VxlpFindOrCreateSourceComponentIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceComponent,
	OUT	PUCHAR				SourceComponentIndex);

NTSTATUS VxlpFindOrCreateSourceFileIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceFile,
	OUT	PUCHAR				SourceFileIndex);

NTSTATUS VxlpFindOrCreateSourceFunctionIndex(
	IN	VXLHANDLE			LogHandle,
	IN	PCWSTR				SourceFunction,
	OUT	PUCHAR				SourceFunctionIndex);

NTSTATUS VxlpBuildIndex(
	IN	VXLHANDLE			LogHandle);

//
// System Service Extensions/Hooks
//

NTSTATUS NTAPI KexpNtQueryInformationThreadHook(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

NTSTATUS NTAPI KexpNtSetInformationThreadHook(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);