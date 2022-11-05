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

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

ULONG KexDllProtectedFunctionExceptionFilter(
	IN	PCWSTR				FunctionName,
	IN	NTSTATUS			ExceptionCode,
	IN	PEXCEPTION_POINTERS	ExceptionPointers);

//
// Protected Function Macros should be used on every function in KexDll.
// Usage of PROTECTED_FUNCTION(_END(_NOLOG)) wraps each function with SEH
// so that errors can be logged or reported to the user instead of causing
// a crash.
//
// Protected function macros should be used on all functions except:
//
//   - trivial functions (e.g. mapping a function to its *Ex variant)
//   - functions that do not return (NORETURN)
//   - functions that must not have their behavior changed for compatibility
//     reasons (e.g. extended API functions)
//
// PROTECTED_FUNCTION_END_NOLOG causes the error not to be logged. This
// macro should only be used on functions that are directly involved in
// logging errors, e.g. KexSrvSendMessage, in order to avoid an infinite
// loop and stack overflow.
//
// PROTECTED_FUNCTION_END_BOOLEAN returns FALSE instead of a NTSTATUS.
//

#define PROTECTED_FUNCTION { try

#define PROTECTED_FUNCTION_END \
	except (KexDllProtectedFunctionExceptionFilter(__FUNCTIONW__, GetExceptionCode(), GetExceptionInformation())) { \
		return GetExceptionCode(); \
	}}

#define PROTECTED_FUNCTION_END_BOOLEAN \
	except (KexDllProtectedFunctionExceptionFilter(__FUNCTIONW__, GetExceptionCode(), GetExceptionInformation())) { \
		return FALSE; \
	}}

#define PROTECTED_FUNCTION_END_VOID \
	except (KexDllProtectedFunctionExceptionFilter(__FUNCTIONW__, GetExceptionCode(), GetExceptionInformation())) { \
		return; \
	}}

#define PROTECTED_FUNCTION_END_NOLOG except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }}
#define PROTECTED_FUNCTION_END_NOLOG_BOOLEAN except (EXCEPTION_EXECUTE_HANDLER) { return FALSE; }}

extern PKEX_PROCESS_DATA KexData;

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

NTSTATUS KexInitializePropagation(
	VOID);

NTSTATUS KexDisableAVrf(
	VOID);

VOID NTAPI KexDllInitializeThunk(
	IN	PVOID	NormalContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2);

NTSTATUS NTAPI KexpNtOpenKeyExHook(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

NTSTATUS NTAPI KexpNtOpenKeyHook(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes);