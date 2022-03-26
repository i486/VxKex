#pragma once
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <VxKex.h>
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <malloc.h>
#include <stdarg.h>

#define T TEXT
#define ASSUME __assume
#define NORETURN __declspec(noreturn)
#define STATIC static
#define INLINE _inline
#define VOLATILE volatile
#define DECLSPEC_EXPORT __declspec(dllexport)
#define CHECKED(x) if (!(x)) goto Error
#define tWinMain _tWinMain
#define getch _getch
#define cprintf _tcprintf
#define caprintf _cprintf
#define cwprintf _cwprintf
#define strlen _tcslen
#define stricmp _tcsicmp
#define strnicmp _tcsnicmp
#define strcpy_s _tcscpy_s
#define strcat_s _tcscat_s
#define scprintf _sctprintf
#define vscprintf _vsctprintf
#define vsprintf_s _vstprintf_s
#define sprintf_s _stprintf_s
#define snprintf_s _sntprintf_s
#define sscanf_s _stscanf_s
#define StackAlloc _alloca

#define until (condition) while (!(condition))
#define unless (condition) if (!(condition))

typedef LPVOID *PPVOID;
typedef unsigned __int64 QWORD, *PQWORD, *LPQWORD, **PPQWORD;
typedef LONG KPRIORITY;

EXTERN_C VOID SetFriendlyAppName(
	IN	LPCTSTR	lpszFriendlyName);

EXTERN_C LPCTSTR GetLastErrorAsString(
	VOID);

EXTERN_C VOID MessageBoxV(
	IN	UINT	uType	OPTIONAL,
	IN	LPCTSTR	lpszFmt,
	IN	va_list	ap);

EXTERN_C VOID ErrorBoxF(
	IN	LPCTSTR	lpszFmt, ...);

EXTERN_C NORETURN VOID CriticalErrorBoxF(
	IN	LPCTSTR	lpszFmt, ...);

EXTERN_C VOID InfoBoxF(
	IN	LPCTSTR	lpszFmt, ...);

EXTERN_C BOOL RegReadSz(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	OUT	LPTSTR	lpszData,
	IN	DWORD	dwcchData);

EXTERN_C BOOL RegWriteSz(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	IN	LPCTSTR	lpszData OPTIONAL);

EXTERN_C BOOL RegReadDw(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	OUT	LPDWORD	lpdwData);

EXTERN_C BOOL RegWriteDw(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName OPTIONAL,
	IN	DWORD	dwData);

EXTERN_C BOOL RegDelValue(
	IN	HKEY	hKey,
	IN	LPCTSTR	lpszSubKey,
	IN	LPCTSTR	lpszValueName);

HANDLE CreateTempFile(
	IN	LPCTSTR					lpszPrefix,
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

VOID AutoFree(
	IN	LPVOID	lpv);

#define __MAX_STACK_ALLOC_SIZE 1024
#define AutoAlloc(cb) (((cb) < __MAX_STACK_ALLOC_SIZE) ? __AutoStackAllocHelper(StackAlloc((cb) + 1)) : __AutoHeapAllocHelper((cb) + 1))

#ifdef UNICODE
#define WriteFileF WriteFileWF
#else
#define WriteFileF WriteFileAF
#endif