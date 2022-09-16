#pragma once
#include <KexComm.h>
#include <KexData.h>

#define APPNAME L"VxKexLdr"
#define FRIENDLYAPPNAME L"VxKex Loader"

// Global variable extern declarations
extern WCHAR g_szExeFullPath[];
extern LPWSTR g_lpszExeBaseName;
extern ULONG_PTR g_vaExeBase;
extern ULONG_PTR g_vaPebBase;
extern HANDLE g_hProc;
extern HANDLE g_hThread;
extern DWORD g_dwProcId;
extern DWORD g_dwThreadId;
extern BOOL g_bExe64;
extern HANDLE g_hLogFile;
extern KEX_PROCESS_DATA g_KexData;

// dialog.c
HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPWSTR	lpszText);

INT_PTR CALLBACK DlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam);

// harderr.c
VOID VklHardError(
	IN	NTSTATUS	st,
	IN	DWORD		dw,
	IN	LPWSTR		lpsz1,
	IN	LPWSTR		lpsz2);

// k32patch.c
VOID PatchKernel32ImageVersionCheck(
	BOOL bPatch);

// kexdata.c
BOOL KexOpenIfeoKeyForExe(
	IN	LPCWSTR	lpszExeFullPath OPTIONAL,
	IN	REGSAM	samDesired,
	OUT	PHKEY	phkResult);

VOID CopyKexDataToChild(
	VOID);

VOID UpdateKexDataInChild(
	VOID);

// prescan.c
VOID PreScanExeFile(
	VOID);

// psinfo.c
VOID GetProcessBaseAddressAndPebBaseAddress(
	IN	HANDLE		hProc,
	IN	PULONG_PTR	lpProcessBaseAddress OPTIONAL,
	IN	PULONG_PTR	lpPebBaseAddress OPTIONAL);

BOOL ProcessIs64Bit(
	IN	HANDLE	hProc);

VOID GetProcessImageFullPath(
	IN	HANDLE	hProc,
	OUT	LPWSTR	szFullPath);

DWORD GetParentProcessId(
	VOID);

BOOL ChildProcessIsConsoleProcess(
	VOID);

ULONG_PTR GetEntryPointVa(
	IN	ULONG_PTR	vaPeBase);

// utility.c
HANDLE KexOpenLogFile(
	IN	LPCWSTR	szExeFullPath);

VOID LogF(
	IN	LPCWSTR lpszFmt, ...);

VOID Pause(
	VOID);

NORETURN VOID Exit(
	IN	DWORD	dwExitCode);

BOOL ShouldAllocConsole(
	VOID);

// vxkexldr.c
VOID RewriteImports(
	IN	LPCWSTR		lpszImageName OPTIONAL,
	IN	ULONG_PTR	vaPeBase);

BOOL ShouldRewriteImportsOfDll(
	IN	LPCWSTR	lpszDllName);

VOID PerformPostInitializationSteps(
	VOID);

VOID RewriteDllImports(
	VOID);

BOOL SpawnProgramUnderLoader(
	IN	LPWSTR	lpszCmdLine,
	IN	BOOL	bCalledFromDialog,
	IN	BOOL	bDialogDebugChecked,
	IN	BOOL	bForce);

NORETURN VOID EntryPoint(
	VOID);