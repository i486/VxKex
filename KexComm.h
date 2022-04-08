#pragma once
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <VxKex.h>
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <malloc.h>
#include <stdarg.h>

#undef MAX_PATH
#define MAX_PATH ((SIZE_T) 260)

#define ASSUME __assume
#define NORETURN __declspec(noreturn)
#define STATIC static
#define INLINE _inline
#define VOLATILE volatile
#define DECLSPEC_EXPORT __declspec(dllexport)
#define CHECKED(x) if (!(x)) goto Error
#define CONCAT(a,b) a##b
#define L(str) CONCAT(L,str)

#define snwprintf_s _snwprintf_s
#define vscwprintf _vscwprintf
#define wcsicmp _wcsicmp
#define wcsnicmp _wcsnicmp

#define StackAlloc _alloca

#define until(condition) while (!(condition))
#define unless(condition) if (!(condition))
#define otherwise else
#define and &&
#define or ||
#define not !
#define is ==
#define is_not !=

typedef LPVOID *PPVOID;
typedef unsigned __int64 QWORD, *PQWORD, *LPQWORD, **PPQWORD;
typedef LONG KPRIORITY;

EXTERN_C VOID SetFriendlyAppName(
	IN	LPCWSTR	lpszFriendlyName);

EXTERN_C LPCWSTR GetLastErrorAsString(
	VOID);

EXTERN_C VOID MessageBoxV(
	IN	UINT	uType	OPTIONAL,
	IN	LPCWSTR	lpszFmt,
	IN	va_list	ap);

EXTERN_C VOID ErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C NORETURN VOID CriticalErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C VOID InfoBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C BOOL RegReadSz(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	OUT	LPWSTR	lpszData,
	IN	DWORD	dwcchData);

EXTERN_C BOOL RegWriteSz(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	IN	LPCWSTR	lpszData OPTIONAL);

EXTERN_C BOOL RegReadDw(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	OUT	LPDWORD	lpdwData);

EXTERN_C BOOL RegWriteDw(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	IN	DWORD	dwData);

EXTERN_C BOOL RegDelValue(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName);

VOID PrintF(
	IN	LPCWSTR lpszFmt, ...);

HANDLE CreateTempFile(
	IN	LPCWSTR					lpszPrefix,
	IN	DWORD					dwDesiredAccess,
	IN	DWORD					dwShareMode,
	IN	LPSECURITY_ATTRIBUTES	lpSecurityAttributes);

BOOL WriteFileWF(
	IN	HANDLE	hFile,
	IN	LPCWSTR	lpszFmt, ...);

LPVOID __AutoHeapAllocHelper(
	IN	SIZE_T	cb);

LPVOID __AutoStackAllocHelper(
	IN	LPVOID	lpv);

VOID __AutoFreeHelper(
	IN	LPVOID	lpv);

LPWSTR GetCommandLineWithoutImageName(
	VOID);

FORCEINLINE LPVOID DefHeapAlloc(
	IN	SIZE_T	cb)
{
	return HeapAlloc(GetProcessHeap(), 0, cb);
}

FORCEINLINE VOID DefHeapFree(
	IN	LPVOID	lpBase)
{
	HeapFree(GetProcessHeap(), 0, lpBase);
}

FORCEINLINE LPVOID DefHeapReAlloc(
	IN	LPVOID	lpBase,
	IN	SIZE_T	cbNew)
{
	return HeapReAlloc(GetProcessHeap(), 0, lpBase, cbNew);
}

// application can override this
#ifndef __MAX_STACK_ALLOC_SIZE
#  define __MAX_STACK_ALLOC_SIZE 1024
#endif

#define AutoAlloc(cb) (((cb) < __MAX_STACK_ALLOC_SIZE) ? __AutoStackAllocHelper(StackAlloc((cb) + 1)) : __AutoHeapAllocHelper((cb) + 1))
#define AutoFree(lpv) do { __AutoFreeHelper(lpv); lpv = NULL; } while(0)

#ifdef UNICODE
#define WriteFileF WriteFileWF
#else
#define WriteFileF WriteFileAF
#endif