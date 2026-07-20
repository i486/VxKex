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
//     vxiiduu              22-Feb-2026  Remove qt6 kerning hack.
//     vxiiduu              19-May-2026  Remove outdated comment about protected
//                                       function macros.
//                                       Move around ASH detection stuff.
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

//
// Data type definitions
//

typedef enum _KEX_DWRITE_IMPLEMENTATION {
	DWriteNoImplementation,
	DWriteWindows10Implementation
} TYPEDEF_TYPE_NAME(KEX_DWRITE_IMPLEMENTATION);

typedef struct _KEX_DLL_REWRITE_UNDO_ENTRY {
	PVOID		Location;			// The location at which to write the undo data to undo the DLL rewrite.
	ULONG		NumberOfBytes;		// Number of bytes to write at the location.
	BYTE		UndoData[];			// Bytes to write at the location.
} TYPEDEF_TYPE_NAME(KEX_DLL_REWRITE_UNDO_ENTRY);

typedef struct _KEX_DLL_REWRITE_UNDO_LIST {
	ULONG		BytesAllocated;
	ULONG		BytesUsed;
	PVOID		UndoEntries;
} TYPEDEF_TYPE_NAME(KEX_DLL_REWRITE_UNDO_LIST);

//
// Protected Function Macros.
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

ULONG KexDllProtectedFunctionExceptionFilter(
	IN	PCWSTR				FunctionName,
	IN	NTSTATUS			ExceptionCode,
	IN	PEXCEPTION_POINTERS	ExceptionPointers);

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

//
// ash.c
//

VOID AshInitialize(
	VOID);

//
// ashcrsup.c
//

NTSTATUS AshSetIsChromiumProcess(
	VOID);

NTSTATUS AshPerformChromiumDetectionFromModuleExports(
	IN	PCVOID	ModuleBase);

//
// ashdetec.c
//

NTSTATUS AshSetIsQt6Process(
	VOID);

NTSTATUS AshSetIsDotnetProcess(
	VOID);

VOID AshDllLoadNotification(
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData);

//
// ashselec.c
//

NTSTATUS AshSelectDWriteImplementation(
	IN	KEX_DWRITE_IMPLEMENTATION	Implementation);

//
// avrf.c
//

NTSTATUS KexDisableAVrf(
	VOID);

//
// dllnotif.c
//

VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context);

//
// dllpath.c
//

NTSTATUS KexpAddKex3264ToDllPath(
	VOID);

//
// dllrewrt.c
//

NTSTATUS KexInitializeDllRewrite(
	VOID);

BOOLEAN KexDoesDllRewriteEntryExist(
	IN	PCUNICODE_STRING		DllName);

BOOLEAN KexShouldRewriteStaticImportsOfDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName);

NTSTATUS KexRewriteImageImportDirectory(
	IN		PVOID						ImageBase,
	IN		PCUNICODE_STRING			BaseImageName,
	IN		PCUNICODE_STRING			FullImageName);

NTSTATUS KexAddDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName);

NTSTATUS KexRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName);

NTSTATUS KexAddUpdateRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName OPTIONAL);

NTSTATUS KexApplyUserDllRewrite(
	IN	PWSTR	RewriteSpec);

//
// initapc.c
//

VOID NTAPI KexPostInitializationApcRoutine(
	IN	PVOID	NormalContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2);

//
// kexdata.c
//

EXTERN PKEX_PROCESS_DATA KexData;

//
// kexhe.c
//

NTSTATUS NTAPI Ext_NtRaiseHardError(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

NORETURN VOID KexHeErrorBox(
	IN	PCWSTR	ErrorMessage);

//
// kexrtlp.c
//

HANDLE KexRtlpGetGlobalKeyedEvent(
	VOID);

//
// logging.c
//

NTSTATUS KexOpenVxlLogForCurrentApplication(
	OUT	PVXLHANDLE	LogHandle);

//
// ntalrtid.c
//

EXTERN FORCEINLINE VOID KexAlertByThreadIdThreadAttach(
	VOID);

//
// rtlrng.c
//

NTSTATUS KexRtlInitializeKsec(
	VOID);

//
// verspoof.c
//

VOID KexApplyVersionSpoof(
	VOID);

//
// vxlpriv.c
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

