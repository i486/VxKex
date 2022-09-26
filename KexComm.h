#pragma once
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//
// VxKex Common Components
// KexComm is a library of utility functions that are widely used across multiple
// components of VxKex.
//

#include <VxKex.h>
#include <KexData.h>

#include <Windows.h>
#include <strsafe.h>
#include <malloc.h>
#include <stdarg.h>

#define StatusBar_SetParts(StatusBarWindow, NumberOfParts, SizeArray) (SendMessage((StatusBarWindow), SB_SETPARTS, (NumberOfParts), (LPARAM) (SizeArray)))
#define StatusBar_SetText(StatusBarWindow, Index, Text) (SendMessage((StatusBarWindow), SB_SETTEXT, (Index), (LPARAM) (Text)))

#undef MAX_PATH
#define MAX_PATH ((SIZE_T) 260)

#ifndef __L
#  define __L(s) L ## s
#endif

#ifndef _L
#  define _L(s) __L(s)
#endif

#if defined(_DEBUG) || defined(RELEASE_ASSERTS_ENABLED)
#  define ASSERT(Condition) do { \
								if (!(Condition)) { \
									ErrorBoxF(L"Assertion failed in %s:%lu (function %s):\r\n" \
											  L"Condition: %s\r\n" \
											  L"A breakpoint exception will be raised in response to this event after you click OK.", \
											  _L(__FILE__), __LINE__, _L(__FUNCTION__), L#Condition); \
									__debugbreak(); \
								} \
							} while (0)
#else
#  define ASSERT(Condition) do { if (!(Condition)); } while (0)
#endif

#ifndef _DEBUG
#  define ASSUME __assume
#else
#  define ASSUME(...)
#endif

#define NORETURN __declspec(noreturn)
#define STATIC static
#define INLINE _inline
#define VOLATILE volatile
#define DECLSPEC_EXPORT __declspec(dllexport)
#define CHECKED(x) if (!(x)) goto Error
#define CONCAT(a,b) a##b
#define L(str) CONCAT(L,str)
#define WIN32_FROM_HRESULT(x) (HRESULT_CODE(x)) // be careful when using this - not all hresult codes map to a valid win32 error

#ifdef _M_X64
#  define PROCESS_IS_64BIT (TRUE)
#  define PROCESS_IS_32BIT (FALSE)
#  define PROCESSBITS (64)
#else
#  define PROCESS_IS_64BIT (FALSE)
#  define PROCESS_IS_32BIT (TRUE)
#  define PROCESSBITS (32)
#endif

#define scwprintf __scwprintf
#define snwprintf_s _snwprintf_s
#define vscwprintf _vscwprintf
#define wcsicmp _wcsicmp
#define wcsnicmp _wcsnicmp

#define StackAlloc _alloca

#define until(condition) while (!(condition))
#define unless(condition) if (!(condition))

typedef PVOID *PPVOID;
typedef CONST VOID *PCVOID;
typedef PCVOID *PPCVOID;
typedef unsigned __int64 QWORD, *PQWORD, *LPQWORD, **PPQWORD;
typedef LONG NTSTATUS;
typedef LONG KPRIORITY;

EXTERN_C VOID SetFriendlyAppName(
	IN	LPCWSTR	lpszFriendlyName);

EXTERN_C VOID SetApplicationWindow(
	IN	HWND	Window);

EXTERN_C LPCWSTR Win32ErrorAsString(
	IN	DWORD	dw);

EXTERN_C LPCWSTR NtStatusAsString(
	IN	NTSTATUS st);

EXTERN_C LPCWSTR GetLastErrorAsString(
	VOID);

EXTERN_C VOID MessageBoxV(
	IN	UINT	uType OPTIONAL,
	IN	LPCWSTR	lpszFmt,
	IN	va_list	ap);

EXTERN_C VOID ErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C NORETURN VOID CriticalErrorBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C VOID WarningBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C VOID InfoBoxF(
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C VOID MessageBoxF(
	IN	PCWSTR Format, ...);

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

EXTERN_C BOOL RegReadBoolean(
	IN	HKEY		Key,
	IN	PCWSTR		SubKey OPTIONAL,
	IN	PWSTR		Value OPTIONAL,
	OUT	PBOOLEAN	Data);

EXTERN_C BOOL RegWriteDw(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName OPTIONAL,
	IN	DWORD	dwData);

EXTERN_C BOOL RegDelValue(
	IN	HKEY	hKey,
	IN	LPCWSTR	lpszSubKey,
	IN	LPCWSTR	lpszValueName);

EXTERN_C LONGLONG CompareFileTimes(
	IN	FILETIME	FileTime1,
	IN	FILETIME	FileTime2);

EXTERN_C VOID PrintF(
	IN	LPCWSTR lpszFmt, ...);

EXTERN_C BOOLEAN PathReplaceIllegalCharacters(
	IN	PWSTR	Path,
	IN	WCHAR	ReplacementCharacter,
	IN	BOOLEAN	AllowPathSeparators);

EXTERN_C HANDLE CreateTempFile(
	IN	LPCWSTR					lpszPrefix,
	IN	DWORD					dwDesiredAccess,
	IN	DWORD					dwShareMode,
	IN	LPSECURITY_ATTRIBUTES	lpSecurityAttributes);

EXTERN_C BOOL WriteFileWF(
	IN	HANDLE	hFile,
	IN	LPCWSTR	lpszFmt, ...);

EXTERN_C LPWSTR ConvertDeviceHarddiskToDosPath(
	IN	LPWSTR lpszPath);

EXTERN_C LPWSTR GetFilePathFromHandle(
	IN	HANDLE	hFile);

EXTERN_C LPVOID __AutoHeapAllocHelper(
	IN	SIZE_T	cb);

EXTERN_C LPVOID __AutoStackAllocHelper(
	IN	LPVOID	lpv);

EXTERN_C VOID __AutoFreeHelper(
	IN	LPVOID	lpv);

EXTERN_C LPWSTR GetCommandLineWithoutImageName(
	VOID);

EXTERN_C NORETURN VOID KexRaiseHardError(
	IN	PCWSTR	WindowTitle OPTIONAL,
	IN	PCWSTR	BugLink OPTIONAL,
	IN	PCWSTR	Format OPTIONAL, ...);

EXTERN_C BOOL KexOpenIfeoKeyForExe(
	IN	PCWSTR	ExeFullPath OPTIONAL,
	IN	REGSAM	DesiredSam,
	OUT	PHKEY	Key);

EXTERN_C BOOL KexReadIfeoParameters(
	IN	PCWSTR					ExeFullPath,
	OUT	PKEX_IFEO_PARAMETERS	KexIfeoParameters);

EXTERN_C DWORD GetEnvironmentVariableExW(
	IN	PCWSTR	Name,
	OUT	PWSTR	Buffer OPTIONAL,
	IN	ULONG	BufferSize,
	IN	PVOID	Environment OPTIONAL);

EXTERN_C BOOL SetEnvironmentVariableExW(
	IN	PCWSTR	Name,
	IN	PCWSTR	Value OPTIONAL,
	IN	PVOID	*Environment OPTIONAL);

EXTERN_C BOOLEAN CloneEnvironmentBlock(
	IN	PVOID	Source OPTIONAL,
	OUT	PVOID	*Destination);

EXTERN_C BOOLEAN CloneEnvironmentBlockConvertAnsiToUnicode(
	IN	PVOID	Source OPTIONAL,
	OUT	PVOID	*Destination);

EXTERN_C ULONG SizeOfEnvironmentBlockW(
	IN	PVOID	Environment OPTIONAL);

EXTERN_C BOOLEAN IsOperatingSystem64Bit(
	VOID);

EXTERN_C BOOLEAN IsProcess64Bit(
	IN	HANDLE		ProcessHandle,
	OUT	PBOOLEAN	Is64Bit);

EXTERN_C HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPWSTR	lpszText);

EXTERN_C BOOLEAN SetWindowTextF(
	IN	HWND	Window,
	IN	PCWSTR	Format,
	IN	...);

EXTERN_C VOID ListView_SetCheckedStateAll(
	IN	HWND	Window,
	IN	BOOLEAN	Checked);

EXTERN_C BOOLEAN SetDlgItemTextF(
	IN	HWND	Window,
	IN	INT		ItemId,
	IN	PCWSTR	Format,
	IN	...);

EXTERN_C VOID StatusBar_SetTextF(
	IN	HWND	Window,
	IN	INT		Index,
	IN	PCWSTR	Format,
	IN	...);

EXTERN_C VOID CenterWindow(
	IN	HWND	Window,
	IN	HWND	ParentWindow OPTIONAL);

EXTERN_C ULONG ContextMenu(
	IN	HWND	Window,
	IN	USHORT	MenuId,
	IN	PPOINT	ClickPoint);

EXTERN_C VOID SetWindowIcon(
	IN	HWND	Window,
	IN	USHORT	IconId);

EXTERN_C INT __scwprintf(
	IN	PCWSTR Format,
	IN	...);

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

FORCEINLINE SIZE_T DefHeapSize(
	IN	LPCVOID	BaseAddress)
{
	return HeapSize(GetProcessHeap(), 0, BaseAddress);
}

// application can override this
#ifndef __MAX_STACK_ALLOC_SIZE
#  define __MAX_STACK_ALLOC_SIZE 1024
#endif

#define RVA_TO_VA(base, rva) ((LPVOID) (((LPBYTE) (base)) + (rva)))

#define AutoAlloc(cb) (((cb) < __MAX_STACK_ALLOC_SIZE) ? __AutoStackAllocHelper(StackAlloc((cb) + 1)) : __AutoHeapAllocHelper((cb) + 1))
#define AutoFree(lpv) do { __AutoFreeHelper(lpv); lpv = NULL; } while(0)

#ifdef UNICODE
#  define WriteFileF WriteFileWF
#else
#  define WriteFileF WriteFileAF
#endif