#include <Windows.h>
#include <Psapi.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <KexComm.h>
#include <NtDll.h>

#include "VaRw.h"
#include "resource.h"

#define APPNAME T("VxKexLdr")
#define FRIENDLYAPPNAME T("VxKex Loader")
#define EXCEPTION_WX86_BREAKPOINT 0x4000001F

//
// Global vars
//

TCHAR g_szExeFullPath[MAX_PATH];
ULONG_PTR g_vaExeBase;
ULONG_PTR g_vaPebBase;
HANDLE g_hProc = NULL;
HANDLE g_hThread = NULL;
DWORD g_dwProcId;
DWORD g_dwThreadId;
BOOL g_bExe64 = -1;

//
// Utility functions
//

VOID Pause(
	VOID)
{
	HWND hWndConsole = GetConsoleWindow();
	if (hWndConsole && IsWindowVisible(hWndConsole)) {
		cprintf(T("Press any key to continue . . . "));
		getch();
	}
}

NORETURN VOID Exit(
	IN	DWORD	dwExitCode)
{
	Pause();
	FreeConsole();
	ExitProcess(dwExitCode);
}

// FailExit() should only be called on failure.
NORETURN VOID FailExit(
	VOID)
{
	TerminateProcess(g_hProc, 0);
	Exit(0);
}

//
// Worker functions
//

// The purpose of this function is to avoid the subsystem version check performed by kernel32.dll
// as a part of the CreateProcess() routine.
// There is a function called BasepCheckImageVersion (on XP, it's called BasepIsImageVersionOk) which
// queries two memory addresses:
//   0x7ffe026c: Subsystem Major Version (DWORD)
//   0x7ffe0270: Subsystem Minor Version (DWORD)
// These two values are part of the USER_SHARED_DATA structure, which is read-only. However we can
// patch the memory values themselves to point to our own fake subsystem image numbers.
// It turns out that we can simply scan the in-memory kernel32.dll for these two addresses (works
// on XP, Vista and 7 - x86, x64 and WOW64) and only one result will come up. What a stroke of luck.
// Then we rewrite those addresses to point at our fake numbers.
VOID PatchKernel32ImageVersionCheck(
	BOOL bPatch)
{
	static BOOL bAlreadyPatched = FALSE;
	static LPDWORD lpMajorAddress = NULL; // addresses within kernel32.dll that we are patching
	static LPDWORD lpMinorAddress = NULL; // ^
	static LPDWORD lpdwFakeMajorVersion = NULL;
	static LPDWORD lpdwFakeMinorVersion = NULL;
	DWORD dwOldProtect;

	if (bPatch) {
		// patch
		MODULEINFO k32ModInfo;
		LPBYTE lpPtr;
		LPBYTE lpEndPtr;

		if (bAlreadyPatched) {
			// this shouldn't happen
#ifdef _DEBUG
			CriticalErrorBoxF(T("Internal error: %s(bPatch=TRUE) called inappropriately"), __FUNCTION__);
#else
			return;
#endif
		}

		if (!lpdwFakeMajorVersion) {
			lpdwFakeMajorVersion = (LPDWORD) VirtualAlloc(NULL, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			if (!lpdwFakeMajorVersion) {
#ifdef _DEBUG
				CriticalErrorBoxF(T("Failed to allocate memory: %s"), GetLastErrorAsString());
#else
				return;
#endif
			}

			*lpdwFakeMajorVersion = 0xFFFFFFFF;
			VirtualProtect(lpdwFakeMajorVersion, sizeof(DWORD), PAGE_READONLY, &dwOldProtect);
		}

		if (!lpdwFakeMinorVersion) {
			lpdwFakeMinorVersion = (LPDWORD) VirtualAlloc(NULL, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			if (!lpdwFakeMinorVersion) {
#ifdef _DEBUG
				CriticalErrorBoxF(T("Failed to allocate memory: %s"), GetLastErrorAsString());
#else
				return;
#endif
			}

			*lpdwFakeMinorVersion = 0xFFFFFFFF;
			VirtualProtect(lpdwFakeMinorVersion, sizeof(DWORD), PAGE_READONLY, &dwOldProtect);
		}


		if ((DWORD_PTR) lpdwFakeMajorVersion > 0xFFFFFFFC || (DWORD_PTR) lpdwFakeMinorVersion > 0xFFFFFFFC) {
			// this shouldn't happen on win7 and earlier, they only started adding ASLR stuff
			// in windows 8 IIRC (and we don't care about those)
#ifdef _DEBUG
			CriticalErrorBoxF(T("Internal error: Pointer to faked subsystem version too large."));
#else
			return;
#endif
		}

		if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(T("kernel32.dll")), &k32ModInfo, sizeof(k32ModInfo)) == FALSE) {
			// give up and hope that the .exe file the user wants to launch doesn't need this
			// workaround
#ifdef _DEBUG
			CriticalErrorBoxF(T("Failed to retrieve module information of KERNEL32.DLL: %s"), GetLastErrorAsString());
#else
			return;
#endif
		}

		lpEndPtr = (LPBYTE) k32ModInfo.lpBaseOfDll + k32ModInfo.SizeOfImage;

		for (lpPtr = (LPBYTE) k32ModInfo.lpBaseOfDll; lpPtr < (lpEndPtr - sizeof(DWORD)); ++lpPtr) {
			LPDWORD lpDw = (LPDWORD) lpPtr;

			if (lpMajorAddress && lpMinorAddress) {
				// we have already patched both required values
				break;
			}

			if (*lpDw == 0x7FFE026C) {
				DWORD dwOldProtect;
				lpMajorAddress = lpDw;
				VirtualProtect(lpDw, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
				*lpDw = (DWORD) lpdwFakeMajorVersion;
				VirtualProtect(lpDw, sizeof(DWORD), dwOldProtect, &dwOldProtect);
			} else if (*lpDw == 0x7FFE0270) {
				DWORD dwOldProtect;
				lpMinorAddress = lpDw;
				VirtualProtect(lpDw, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
				*lpDw = (DWORD) lpdwFakeMinorVersion;
				VirtualProtect(lpDw, sizeof(DWORD), dwOldProtect, &dwOldProtect);
			}
		}

		bAlreadyPatched = TRUE;
	} else if (bAlreadyPatched) {
		// unpatch
		VirtualProtect(lpMajorAddress, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
		*lpMajorAddress = 0x7FFE026C;
		VirtualProtect(lpMajorAddress, sizeof(DWORD), dwOldProtect, &dwOldProtect);

		VirtualProtect(lpMinorAddress, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
		*lpMinorAddress = 0x7FFE0270;
		VirtualProtect(lpMinorAddress, sizeof(DWORD), dwOldProtect, &dwOldProtect);
		
		lpMajorAddress = NULL;
		lpMinorAddress = NULL;
		VirtualFree(lpdwFakeMajorVersion, 0, MEM_RELEASE); lpdwFakeMajorVersion = NULL;
		VirtualFree(lpdwFakeMinorVersion, 0, MEM_RELEASE); lpdwFakeMinorVersion = NULL;
		bAlreadyPatched = FALSE;
	}
}


VOID GetProcessBaseAddressAndPebBaseAddress(
	IN	HANDLE	hProc)
{
	NTSTATUS st;
	PROCESS_BASIC_INFORMATION BasicInfo;

	// Using GetModuleInformation will fail with ERROR_INVALID_HANDLE if Peb(child)->Ldr
	// is NULL, which is the case when we start a suspended process - since the Ldr struct
	// is initialized by NTDLL. See nt5src\Win2K3\sdktools\psapi\process.c for more info.
	// Anyway, that means we must use NtQueryInformationProcess.

	st = NtQueryInformationProcess(
		hProc,
		ProcessBasicInformation,
		&BasicInfo,
		sizeof(BasicInfo),
		NULL);

	if (!SUCCEEDED(st)) {
		CriticalErrorBoxF(T("Failed to query process information.\nNTSTATUS error code: %#010I32x"), st);
	} else {
		// The returned PebBaseAddress is in the virtual address space of the
		// created process, not ours.
		g_vaPebBase = (ULONG_PTR) BasicInfo.PebBaseAddress;
		ReadProcessMemory(hProc, &BasicInfo.PebBaseAddress->ImageBaseAddress, &g_vaExeBase, sizeof(g_vaExeBase), NULL);
	}
}

BOOL ProcessIs32Bit(
	IN	HANDLE	hProc)
{
#ifndef _WIN64
	return TRUE;
#else
	// IsWow64Process doesn't work. Always returns FALSE.
	WORD w;
	GetProcessBaseAddressAndPebBaseAddress((g_hProc = hProc));
	w = VaReadWord(g_vaExeBase + VaReadDword(g_vaExeBase + 0x3C) + 4);
	return (w == 0x014C);
#endif
}

// this allows programs to load our DLLs
// avoids having to add entries to clutter up the system PATH
VOID AppendKex3264ToPath(
	IN	INT	bits)
{
	TCHAR szKexDir[MAX_PATH];
	TCHAR szPathAppend[MAX_PATH * 2];
	LPTSTR szPath;
	DWORD dwcchPath;

	RegReadSz(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("KexDir"), szKexDir, ARRAYSIZE(szKexDir));
	sprintf_s(szPathAppend, ARRAYSIZE(szPathAppend), T(";%s\\Kex%d"), szKexDir, bits);
	dwcchPath = GetEnvironmentVariable(T("Path"), NULL, 0) + (DWORD) strlen(szPathAppend);
	szPath = (LPTSTR) StackAlloc(dwcchPath * sizeof(TCHAR));
	GetEnvironmentVariable(T("Path"), szPath, dwcchPath);
	strcat_s(szPath, dwcchPath, szPathAppend);
	SetEnvironmentVariable(T("Path"), szPath);
}

VOID CreateSuspendedProcess(
	IN	LPTSTR	lpszCmdLine)	// Full command-line of the process incl. executable name
{
	BOOL bSuccess;
	BOOL bRetryPatchKernel32 = FALSE;
	BOOL bRetryAppendPath = FALSE;
	DWORD dwCpError;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;

	GetStartupInfo(&StartupInfo);

RetryCreateProcess:
	bSuccess = CreateProcess(
		NULL,
		lpszCmdLine,
		NULL,
		NULL,
		FALSE,
		CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS,
		NULL,
		NULL,
		&StartupInfo,
		&ProcInfo);
	dwCpError = GetLastError();

	PatchKernel32ImageVersionCheck(FALSE);

	if (!bSuccess) {
		if (dwCpError == ERROR_BAD_EXE_FORMAT) {
			if (!bRetryPatchKernel32) {
				// Since patching kernel32 carries a performance penalty (have to scan through memory), we
				// only patch kernel32 if CreateProcess fails (which, in most cases, is due to not passing
				// the image subsystem version check).
				bRetryPatchKernel32 = TRUE;
				PatchKernel32ImageVersionCheck(TRUE);
				goto RetryCreateProcess;
			} else {
				// Windows doesn't provide any pre-defined error message for this scenario.
				// Therefore to prevent user confusion we have to display our own error message.
				CriticalErrorBoxF(T("Failed to create process: %#010I32x: The executable file is invalid."), dwCpError);
			}
		} else if (dwCpError = ERROR_ELEVATION_REQUIRED && !IsUserAnAdmin()) {
			// Debugging elevated processes as a non-elevated process isn't allowed so we need to re-exec
			// ourselves as admin.
			TCHAR szVxKexLdr[MAX_PATH];
			LPCTSTR lpszCommandLine = GetCommandLine();
			LPCTSTR lpszSubStr;
			GetModuleFileName(NULL, szVxKexLdr, ARRAYSIZE(szVxKexLdr));
			lpszSubStr = StrStrI(lpszCommandLine, szVxKexLdr);

			if (lpszSubStr == lpszCommandLine || lpszSubStr == lpszCommandLine + 1) {
				lpszCommandLine = lpszSubStr + lstrlen(szVxKexLdr);
			}

			if (*lpszCommandLine == '\"') {
				++lpszCommandLine;
			}

			ShellExecute(NULL, T("runas"), szVxKexLdr, lpszCommandLine, NULL, TRUE);
			ExitProcess(0);
		}

		CriticalErrorBoxF(T("Failed to create process: %#010I32x: %s"), dwCpError, GetLastErrorAsString());
	}

	if (!bRetryAppendPath) {
		// The most reliable way to find out whether any given process is 32 bits is just to
		// start it and check. It's bloat but it works.
		// In the future maybe we will find a way to inject environment variables into a started
		// process and avoid having to start the process twice.
		g_bExe64 = !ProcessIs32Bit(ProcInfo.hProcess);
		AppendKex3264ToPath(g_bExe64 ? 64 : 32);
		DebugActiveProcessStop(ProcInfo.dwProcessId); // avoid spurious debug events later
		TerminateProcess(ProcInfo.hProcess, 0);
		CloseHandle(ProcInfo.hProcess);
		CloseHandle(ProcInfo.hThread);
		bRetryAppendPath = TRUE;
		goto RetryCreateProcess;
	}

	DebugSetProcessKillOnExit(FALSE);
	g_hProc = ProcInfo.hProcess;
	g_hThread = ProcInfo.hThread;
	g_dwProcId = ProcInfo.dwProcessId;
	g_dwThreadId = ProcInfo.dwThreadId;
}

ULONG_PTR GetEntryPointVa(
	IN	ULONG_PTR	vaPeBase)
{
	CONST ULONG_PTR vaPeSig = vaPeBase + VaReadDword(vaPeBase + 0x3C);
	CONST ULONG_PTR vaCoffHdr = vaPeSig + 4;
	CONST ULONG_PTR vaOptHdr = vaCoffHdr + 20;
	return vaPeBase + VaReadDword(vaOptHdr + 16);
}

LPCTSTR KexpGetIfeoKey(
	IN	LPCTSTR	lpszExeFullPath OPTIONAL)
{
	static TCHAR szKexIfeoKey[54 + MAX_PATH];

	if (lpszExeFullPath == NULL) {
		lpszExeFullPath = g_szExeFullPath;
	}

	strcpy_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), T("SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\"));
	strcat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), lpszExeFullPath);
	return szKexIfeoKey;
}

BOOL KexQueryIfeoDw(
	IN	LPCTSTR	lpszExeFullPath OPTIONAL,
	IN	LPCTSTR	lpszValueName,
	OUT	LPDWORD	lpdwResult)
{
	return RegReadDw(HKEY_CURRENT_USER, KexpGetIfeoKey(lpszExeFullPath), lpszValueName, lpdwResult);
}

BOOL KexQueryIfeoSz(
	IN	LPCTSTR	lpszExeFullPath OPTIONAL,
	IN	LPCTSTR	lpszValueName,
	OUT	LPTSTR	lpszResult,
	IN	DWORD	dwcchResult)
{
	return RegReadSz(HKEY_CURRENT_USER, KexpGetIfeoKey(lpszExeFullPath), lpszValueName, lpszResult, dwcchResult);
}

BOOL ShouldAllocConsole(
	IN	LPCTSTR	lpszExeFullPath OPTIONAL)
{
	DWORD dwShouldAllocConsole;

	if (lpszExeFullPath) {
		if (RegReadDw(HKEY_CURRENT_USER, T("SOFTWARE\\VXsoft\\VxKexLdr"), T("ShowDebugInfoByDefault"), &dwShouldAllocConsole)) {
			return dwShouldAllocConsole;
		} else {
			return FALSE;
		}
	} else {
		if (KexQueryIfeoDw(lpszExeFullPath, T("AlwaysShowDebug"), &dwShouldAllocConsole)) {
			return dwShouldAllocConsole;
		} else {
			return FALSE;
		}
	}
}

BOOL ShouldDetachAfterDllRewrite(
	IN	LPCTSTR	lpszExeFullPath)
{
	DWORD dwShouldNotDetach;
	
	if (KexQueryIfeoDw(lpszExeFullPath, T("WaitForChild"), &dwShouldNotDetach)) {
		return !dwShouldNotDetach;
	} else {
		return TRUE;
	}
}

// user32.dll -> user33.dll etc etc
VOID RewriteImports(
	IN	LPCTSTR		lpszImageName OPTIONAL,
	IN	ULONG_PTR	vaPeBase)
{
	CONST ULONG_PTR vaPeSig = vaPeBase + VaReadDword(vaPeBase + 0x3C);
	CONST ULONG_PTR vaCoffHdr = vaPeSig + 4;
	CONST ULONG_PTR vaOptHdr = vaCoffHdr + 20;
	CONST BOOL bPe64 = (VaReadWord(vaCoffHdr) == 0x8664) ? TRUE : FALSE;
	CONST ULONG_PTR vaOptHdrWin = vaOptHdr + (bPe64 ? 24 : 28);
	CONST ULONG_PTR vaOptHdrDir = vaOptHdr + (bPe64 ? 112 : 96);
	CONST ULONG_PTR vaImportDir = vaOptHdr + (bPe64 ? 120 : 104);
	CONST ULONG_PTR ulImportDir = VaReadPtr(vaImportDir);
	CONST ULONG_PTR vaImportTbl = vaPeBase + (ulImportDir & 0x00000000FFFFFFFF);
	
	HKEY hKeyDllRewrite;
	ULONG_PTR idxIntoImportTbl;
	LSTATUS lStatus;

	if (g_bExe64 != bPe64) {
		// Provide a more useful error message to the user than the cryptic shit
		// which ntdll displays by default.
		CriticalErrorBoxF(T("The %d-bit DLL '%s' was loaded into a %d-bit application.\n")
						  T("Architecture of DLLs and applications must match for successful program operation."),
						  bPe64 ? 64 : 32, lpszImageName ? lpszImageName : T("(unknown)"), g_bExe64 ? 64 : 32);
	}

	if (ulImportDir == 0) {
		cprintf(T("    This DLL contains no import directory - skipping\n"));
		return;
	}

	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite"),
						   0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKeyDllRewrite);

	if (lStatus != ERROR_SUCCESS) {
		SetLastError(lStatus);
		CriticalErrorBoxF(T("Failed to open DLL rewrite registry key: %s"), GetLastErrorAsString());
	}

	for (idxIntoImportTbl = 0;; ++idxIntoImportTbl) {
		CONST ULONG_PTR ulPtr = vaImportTbl + (idxIntoImportTbl * 20);
		CONST DWORD rvaDllName = VaReadDword(ulPtr + 12);
		CONST ULONG_PTR vaDllName = vaPeBase + rvaDllName;
#ifdef UNICODE
		CHAR originalDllNameData[MAX_PATH * sizeof(TCHAR)];
#endif
		TCHAR szOriginalDllName[MAX_PATH];
		TCHAR szRewriteDllName[MAX_PATH];
		DWORD dwMaxPath = MAX_PATH;

		if (rvaDllName == 0) {
			// Reached end of imports
			break;
		}

#ifdef UNICODE
		VaReadSzA(vaDllName, originalDllNameData, ARRAYSIZE(originalDllNameData));
		MultiByteToWideChar(CP_UTF8, 0, (LPCSTR) originalDllNameData, -1, szOriginalDllName, ARRAYSIZE(szOriginalDllName));
#else
		VaReadSzA(vaDllName, szOriginalDllName, ARRAYSIZE(szOriginalDllName));
#endif

		lStatus = RegQueryValueEx(hKeyDllRewrite, szOriginalDllName, NULL, NULL,
								  (LPBYTE) szRewriteDllName, &dwMaxPath);

		if (lStatus == ERROR_SUCCESS) {
			CONST DWORD dwOriginalLength = (DWORD) strlen(szOriginalDllName);
			CONST DWORD dwRewriteLength = (DWORD) strlen(szRewriteDllName);

			if (dwRewriteLength > dwOriginalLength) {
				// IMPORTANT NOTE: When you add more rewrite DLLs to the registry, the length of the
				// rewrite DLL name MUST BE LESS THAN OR EQUAL to the length of the original DLL name.
				cprintf(T("    WARNING: The rewrite DLL name %s for original DLL %s is too long. ")
						T("It is %u bytes but the maximum is %u. Skipping.\n"),
						szRewriteDllName, szOriginalDllName, dwRewriteLength, dwOriginalLength);
			} else {
				cprintf(T("    Rewriting DLL import %s -> %s\n"), szOriginalDllName, szRewriteDllName);
				VaWriteSzA(vaDllName, szRewriteDllName);
			}
		} else if (lStatus == ERROR_MORE_DATA) {
			// The data for the registry entry was too long
			cprintf(T("    WARNING: The rewrite DLL registry entry for the DLL %s ")
					T("contains invalid data. Skipping.\n"), szOriginalDllName);
		} else {
			// No rewrite entry was found
			cprintf(T("    Found DLL import %s - not rewriting\n"), szOriginalDllName);
		}
	}

	RegCloseKey(hKeyDllRewrite);
}

BOOL ShouldRewriteImportsOfDll(
	IN	LPCTSTR	lpszDllName)
{
	TCHAR szKexDir[MAX_PATH];
	TCHAR szWinDir[MAX_PATH];
	TCHAR szDllDir[MAX_PATH];
	LSTATUS lStatus;
	HKEY hKeyMainConf;

	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("SOFTWARE\\VXsoft\\" APPNAME),
						   0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &hKeyMainConf);
	if (lStatus != ERROR_SUCCESS) {
		SetLastError(lStatus);
		CriticalErrorBoxF(T("Could not open configuration registry key: %s"), GetLastErrorAsString());
	} else {
		DWORD dwMaxPath = MAX_PATH;
		lStatus = RegQueryValueEx(hKeyMainConf, T("KexDir"), NULL, NULL,
								  (LPBYTE) szKexDir, &dwMaxPath);
	}

	if (!lpszDllName || !strlen(lpszDllName) || PathIsRelative(lpszDllName)) {
		// just be safe and don't rewrite it, since we don't even know what it is
		// for relative path: usually only NTDLL is specified as a relative path,
		// and we don't want to rewrite that either (especially since it has no
		// imports anyway)
		return FALSE;
	}

	GetWindowsDirectory(szWinDir, ARRAYSIZE(szWinDir));
	strcpy_s(szDllDir, ARRAYSIZE(szDllDir), lpszDllName);
	PathRemoveFileSpec(szDllDir);

	if (PathIsPrefix(szWinDir, szDllDir) || PathIsPrefix(szKexDir, szDllDir)) {
		// don't rewrite any imports if the particular DLL is in the Windows folder
		// or the kernel extensions folder
		return FALSE;
	}

	return TRUE;
}

// TODO: Make an XP compatible version
LPTSTR GetFilePathFromHandle(
	IN	HANDLE	hFile)
{
	static TCHAR szPath[MAX_PATH + 4];

	if (!GetFinalPathNameByHandle(hFile, szPath, ARRAYSIZE(szPath), 0)) {
		return NULL;
	}

	if (szPath[0] == '\\' && szPath[1] == '\\' && szPath[2] == '?' && szPath[3] == '\\') {
		return szPath + 4;
	} else {
		return szPath;
	}
}

// fake version info, etc.
VOID PerformPostInitializationSteps(
	VOID)
{
	TCHAR szFakedVersion[6];
	DWORD dwDebuggerSpoof;

	cprintf(T("[KE] Performing post-initialization steps\n"));

	if (KexQueryIfeoSz(g_szExeFullPath, T("WinVerSpoof"), szFakedVersion, ARRAYSIZE(szFakedVersion)) == TRUE) {
		ULONG_PTR vaMajorVersion = g_vaPebBase + (g_bExe64 ? 0x118 : 0xA4);
		ULONG_PTR vaMinorVersion = vaMajorVersion + sizeof(ULONG);
		ULONG_PTR vaBuildNumber = vaMinorVersion + sizeof(ULONG);
		ULONG_PTR vaCSDVersion = vaBuildNumber + sizeof(USHORT);
		ULONG_PTR vaCSDVersionUS = g_vaPebBase + (g_bExe64 ? 0x2E8 : 0x1F0);
		UNICODE_STRING usFakeCSDVersion;

		// buffer points to its own length (which is 0, the null terminator)
		usFakeCSDVersion.Length = 0;
		usFakeCSDVersion.MaximumLength = 0;
		usFakeCSDVersion.Buffer = (PWSTR) vaCSDVersionUS;

		if (!lstrcmpi(szFakedVersion, T("WIN8"))) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 2);
			VaWriteWord(vaBuildNumber, 9200);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!lstrcmpi(szFakedVersion, T("WIN81"))) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 3);
			VaWriteWord(vaBuildNumber, 9600);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!lstrcmpi(szFakedVersion, T("WIN10"))) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 19044);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!lstrcmpi(szFakedVersion, T("WIN11"))) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 22000);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		}

		cprintf(T("[KE]    Faked Windows version: %s\n"), szFakedVersion);
	}

	if (KexQueryIfeoDw(g_szExeFullPath, T("DebuggerSpoof"), &dwDebuggerSpoof)) {
		ULONG_PTR vaDebuggerPresent = g_vaPebBase + FIELD_OFFSET(PEB, BeingDebugged);
		VaWriteByte(vaDebuggerPresent, !dwDebuggerSpoof);
		cprintf(T("[KE]    Spoofed debugger presence\n"));
	}
}

// Monitors the process for DLL imports.
VOID RewriteDllImports(
	VOID)
{
	DEBUG_EVENT DbgEvent;
	BOOL bResumed = FALSE;
	CONST ULONG_PTR vaEntryPoint = GetEntryPointVa(g_vaExeBase);
	CONST BYTE btOriginal = VaReadByte(vaEntryPoint);

	cprintf(T("[KE ProcId=%lu, ThreadId=%lu]\tProcess entry point: %p, Original byte: 0x%02X\n"),
			g_dwProcId, g_dwThreadId, vaEntryPoint, btOriginal);
	VaWriteByte(vaEntryPoint, 0xCC);

	DbgEvent.dwProcessId = g_dwProcId;
	DbgEvent.dwThreadId = g_dwThreadId;

	while (TRUE) {
		if (!bResumed) {
			ResumeThread(g_hThread);
			bResumed = TRUE;
		} else {
			ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
		}

		WaitForDebugEvent(&DbgEvent, INFINITE);

		if (DbgEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
			cprintf(T("[CP ProcId=%lu]\t\tProcess created.\n"), DbgEvent.dwProcessId);
			CloseHandle(DbgEvent.u.CreateProcessInfo.hFile);
			CloseHandle(DbgEvent.u.CreateProcessInfo.hThread);
			CloseHandle(DbgEvent.u.CreateProcessInfo.hProcess);
		} else if (DbgEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
			HANDLE hDllFile = DbgEvent.u.LoadDll.hFile;
			LPTSTR lpszDllName;

			if (!hDllFile || (lpszDllName = GetFilePathFromHandle(hDllFile)) == NULL) {
				continue;
			}

			cprintf(T("[LD ProcId=%lu, ThreadId=%lu]\tDLL loaded: %s"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszDllName);

			if (ShouldRewriteImportsOfDll(lpszDllName)) {
				cprintf(T(" (imports will be rewritten)\n"));
				PathStripPath(lpszDllName);
				RewriteImports(lpszDllName, (ULONG_PTR) DbgEvent.u.LoadDll.lpBaseOfDll);
			} else {
				cprintf(T(" (imports will NOT be rewritten)\n"));
			}
		} else if (DbgEvent.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
			cprintf(T("[UL ProcId=%lu, ThreadId=%lu]\tThe DLL at %p was unloaded.\n"),
					DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.u.UnloadDll.lpBaseOfDll);
		} else if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			cprintf(T("[EP ProcId=%lu]\t\tAttached process has exited.\n"), DbgEvent.dwProcessId);

			if (DbgEvent.dwProcessId == g_dwProcId) {
				DWORD dwExitCode;
				GetExitCodeProcess(g_hProc, &dwExitCode);
				Exit(dwExitCode);
			}
		} else if (DbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&
				   (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT ||
				    DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_WX86_BREAKPOINT) &&
				   DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID) vaEntryPoint) {
			CONTEXT Ctx;
#ifdef _WIN64
			WOW64_CONTEXT W64Ctx;
#endif

			// Must decrement EIP/RIP, restore the original byte, detach the debugger and
			// resume program execution. Then exit the loader.
			cprintf(T("[KE ProcId=%lu, ThreadId=%lu]\tProcess has finished loading.\n"),
					DbgEvent.dwProcessId, DbgEvent.dwThreadId);

			if (g_bExe64) {
#ifdef _WIN64	// 64 bit loader running 64 bit program
				Ctx.ContextFlags = CONTEXT_CONTROL;
				GetThreadContext(g_hThread, &Ctx);
				Ctx.Rip--;
				SetThreadContext(g_hThread, &Ctx);
#endif
			} else {
#ifdef _WIN64	// 64 bit loader running 32 bit program
				W64Ctx.ContextFlags = CONTEXT_CONTROL;
				Wow64GetThreadContext(g_hThread, &W64Ctx);
				W64Ctx.Eip--;
				Wow64SetThreadContext(g_hThread, &W64Ctx);
#else			// 32 bit loader running 32 bit program
				Ctx.ContextFlags = CONTEXT_CONTROL;
				GetThreadContext(g_hThread, &Ctx);
				Ctx.Eip--;
				SetThreadContext(g_hThread, &Ctx);
#endif
			}

			VaWriteByte(vaEntryPoint, btOriginal);
			PerformPostInitializationSteps();
			ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, DBG_CONTINUE);

			if (ShouldDetachAfterDllRewrite(g_szExeFullPath)) {
				DebugActiveProcessStop(g_dwProcId);
				cprintf(T("[KE] Detached from process.\n"));
				return;
			}
		} else if (DbgEvent.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
			HANDLE hProc = NULL;
			LPTSTR lpszBuf = NULL;
			SIZE_T BufSize = DbgEvent.u.DebugString.nDebugStringLength;

			if ((lpszBuf = (LPTSTR) malloc(BufSize)) == NULL) {
				continue;
			}
			
			if ((hProc = OpenProcess(PROCESS_VM_READ, FALSE, DbgEvent.dwProcessId)) == NULL) {
				continue;
			}

			if (ReadProcessMemory(hProc, DbgEvent.u.DebugString.lpDebugStringData, lpszBuf, BufSize, NULL) == FALSE) {
				continue;
			}

			CloseHandle(hProc);

#ifdef UNICODE
			cprintf(T("[DS ProcId=%lu, ThreadId=%lu]\t%S\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszBuf);
#else
			cprintf(T("[DS ProcId=%lu, ThreadId=%lu]\t%s\n"), DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszBuf);
#endif

			free(lpszBuf);
		} else {
			cprintf(T("[?? ProcId=%lu, ThreadId=%lu]\tUnknown debug event received: %lu\n"),
					DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
		}
	}
}

VOID GetProcessImageFullPath(
	IN	HANDLE	hProc,
	OUT	LPTSTR	szFullPath)
{
	TCHAR szImageFileName[MAX_PATH];
	TCHAR szDosDevice[3] = T("A:");
	TCHAR szNtDevice[MAX_PATH];

	// This returns a NT style path such as "\Device\HarddiskVolume3\Program Files\whatever.exe"
	// which is pretty much the verbatim output of NtQueryInformationProcess ProcessImageFileName.
	GetProcessImageFileName(hProc, szImageFileName, ARRAYSIZE(szImageFileName));

	for (; szDosDevice[0] <= 'Z'; szDosDevice[0]++) {
		if (QueryDosDevice(szDosDevice, szNtDevice, ARRAYSIZE(szNtDevice) - 1)) {
			if (!strnicmp(szNtDevice, szImageFileName, strlen(szNtDevice))) {
				break;
			}
		}
	}

	if (szDosDevice[0] > 'Z') {
		CriticalErrorBoxF(T("%s was unable to resolve the DOS device name of the executable file '%s'.\n")
						  T("This may be caused by the file residing on a network share or unmapped drive. ")
						  T("%s does not currently support this scenario, please ensure all executable files ")
						  T("are on mapped drives with drive letters."),
						  FRIENDLYAPPNAME, FRIENDLYAPPNAME);
	}

	strcpy_s(g_szExeFullPath, ARRAYSIZE(g_szExeFullPath), szDosDevice);
	strcat_s(g_szExeFullPath, ARRAYSIZE(g_szExeFullPath), szImageFileName + strlen(szNtDevice));
}

BOOL SpawnProgramUnderLoader(
	IN	LPTSTR	lpszCmdLine,
	IN	BOOL	bCalledFromDialog,
	IN	BOOL	bForce)
{
	DWORD dwEnableVxKex;

	CreateSuspendedProcess(lpszCmdLine);
	GetProcessImageFullPath(g_hProc, g_szExeFullPath);

	if (!bCalledFromDialog && !bForce && (!KexQueryIfeoDw(g_szExeFullPath, T("EnableVxKex"), &dwEnableVxKex) || !dwEnableVxKex)) {
		// Since the IFEO in HKLM doesn't discriminate by image path and only by name, this check has to
		// be performed - if we aren't actually supposed to be enabled for this executable, just resume
		// the thread and be done with it.
		ResumeThread(g_hThread);
		return FALSE;
	}

	if (!bCalledFromDialog && (ShouldAllocConsole(NULL) || ShouldAllocConsole(g_szExeFullPath))) {
		// You can't call cprintf() anywhere before this. If you do, all console output
		// fails to work.
		AllocConsole();
	}

	GetProcessBaseAddressAndPebBaseAddress(g_hProc);
	cprintf(T("The process is %d-bit and its base address is %p (PEB base address: %p)\n"),
			g_bExe64 ? 64 : 32, g_vaExeBase, g_vaPebBase);
	RewriteImports(NULL, g_vaExeBase);
	RewriteDllImports();

	return TRUE;
}

HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPTSTR	lpszText)
{
	TOOLINFO ToolInfo;
	HWND hWndTool;
	HWND hWndTip;

	if (!iToolID || !hDlg || !lpszText) {
		return NULL;
	}

	// Get the window of the tool.
	hWndTool = GetDlgItem(hDlg, iToolID);

	// Create the tooltip.
	hWndTip = CreateWindowEx(
		0, TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL, 
		NULL, NULL);

	if (!hWndTool || !hWndTip) {
		return NULL;
	}

	// Associate the tooltip with the tool.
	ZeroMemory(&ToolInfo, sizeof(ToolInfo));
	ToolInfo.cbSize = sizeof(ToolInfo);
	ToolInfo.hwnd = hDlg;
	ToolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ToolInfo.uId = (UINT_PTR) hWndTool;
	ToolInfo.lpszText = lpszText;
	SendMessage(hWndTip, TTM_ADDTOOL, 0, (LPARAM) &ToolInfo);

	return hWndTip;
}

INT_PTR CALLBACK DlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	LSTATUS lStatus;
	HKEY hKeyUserSpecificConfig;
	CONST TCHAR szUserSpecificConfigKey[] = T("SOFTWARE\\VXsoft\\") APPNAME;
	TCHAR szFilename[MAX_PATH + 2] = T("");
	DWORD dwcbFilename = sizeof(szFilename);
	BOOL bShouldShowDebugInfo;
	DWORD dwcbShouldShowDebugInfo = sizeof(bShouldShowDebugInfo);

	switch (uMsg) {
	case WM_INITDIALOG:
		// read last successful command line from registry
		lStatus = RegOpenKeyEx(
			HKEY_CURRENT_USER,
			szUserSpecificConfigKey,
			0, KEY_QUERY_VALUE | KEY_WOW64_64KEY,
			&hKeyUserSpecificConfig);

		if (lStatus == ERROR_SUCCESS) {
			RegQueryValueEx(
				hKeyUserSpecificConfig, T("LastCmdLine"),
				0, NULL, (LPBYTE) szFilename, &dwcbFilename);
			lStatus = RegQueryValueEx(
				hKeyUserSpecificConfig, T("ShowDebugInfo"),
				0, NULL, (LPBYTE) &bShouldShowDebugInfo, &dwcbShouldShowDebugInfo);

			if (lStatus != ERROR_SUCCESS) {
				bShouldShowDebugInfo = 0;
			}

			CheckDlgButton(hWnd, IDCHKDEBUG, bShouldShowDebugInfo ? BST_CHECKED : BST_UNCHECKED);
			RegCloseKey(hKeyUserSpecificConfig);
			SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		}

		CreateToolTip(hWnd, IDCHKDEBUG, T("Creates a console window to display additional debugging information"));
		DragAcceptFiles(hWnd, TRUE);
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDBROWSE) {
			OPENFILENAME ofn;

			// If user was typing in the edit box and pressed enter, just run the program
			// instead of displaying the file selector (which is confusing and unintended)
			if (GetFocus() == GetDlgItem(hWnd, IDFILEPATH)) {
				PostMessage(hWnd, WM_COMMAND, IDOK, 0);
				break;
			}

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize			= sizeof(ofn);
			ofn.hwndOwner			= hWnd;
			ofn.lpstrFilter			= T("Applications (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
			ofn.lpstrFile			= szFilename + 1;
			ofn.nMaxFile			= MAX_PATH;
			ofn.lpstrInitialDir		= T("C:\\Program Files\\");
			ofn.Flags				= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt			= T("exe");
			
			if (GetOpenFileName(&ofn)) {				
				// Give the filename some quotes.
				szFilename[0] = '\"';
				strcat_s(szFilename, ARRAYSIZE(szFilename), T("\""));
				SetDlgItemText(hWnd, IDFILEPATH, szFilename);
			}

			break;
		} else if (LOWORD(wParam) == IDOK) {
			GetDlgItemText(hWnd, IDFILEPATH, szFilename, dwcbFilename);
			bShouldShowDebugInfo = IsDlgButtonChecked(hWnd, IDCHKDEBUG);

			if (bShouldShowDebugInfo) {
				AllocConsole();
			}

			EndDialog(hWnd, 0);
			SpawnProgramUnderLoader(szFilename, TRUE, TRUE);

			// save dialog information in the registry
			lStatus = RegCreateKeyEx(
				HKEY_CURRENT_USER,
				szUserSpecificConfigKey,
				0, NULL, 0, KEY_WRITE, NULL,
				&hKeyUserSpecificConfig, NULL);

			if (lStatus == ERROR_SUCCESS) {
				RegSetValueEx(
					hKeyUserSpecificConfig,
					T("LastCmdLine"), 0, REG_SZ,
					(LPBYTE) szFilename,
					(DWORD) strlen(szFilename) * sizeof(TCHAR));
				RegSetValueEx(
					hKeyUserSpecificConfig,
					T("ShowDebugInfo"), 0, REG_DWORD,
					(LPBYTE) &bShouldShowDebugInfo,
					sizeof(bShouldShowDebugInfo));
				RegCloseKey(hKeyUserSpecificConfig);
			}

			Exit(0);
		} else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, 0);
		} else if (LOWORD(wParam) == IDFILEPATH) {
			if (HIWORD(wParam) == EN_CHANGE) {
				INT iTextLength = GetWindowTextLength(GetDlgItem(hWnd, IDFILEPATH));
				EnableWindow(GetDlgItem(hWnd, IDOK), !!iTextLength);
			}
		}

		break;
	case WM_DROPFILES:
		DragQueryFile((HDROP) wParam, 0, szFilename + 1, MAX_PATH);
		strcpy_s(szFilename + strlen(szFilename), 2, T("\""));
		SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		DragFinish((HDROP) wParam);
		break;
	}
	
	return FALSE;
}

#define FORCE_FLAG T("/FORCE ")

INT APIENTRY tWinMain(
	IN	HINSTANCE	hInstance,
	IN	HINSTANCE	hPrevInstance,
	IN	LPTSTR		lpszCmdLine,
	IN	INT			iCmdShow)
{
	SetFriendlyAppName(FRIENDLYAPPNAME);

	if (strlen(lpszCmdLine) == 0) {
		InitCommonControls();
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	} else {
		CONST SIZE_T cchForceFlag = strlen(FORCE_FLAG);
		BOOL bForce = FALSE;

		if (!strnicmp(lpszCmdLine, FORCE_FLAG, cchForceFlag)) {
			// This flag is used to bypass the usual checking that we're "supposed" to launch
			// the program with VxKex enabled. For example, the shell extended context menu entry
			// will use this flag, to let the user try out VxKex without having to go into the
			// property sheet and (possibly) accept the UAC dialog etc etc.
			bForce = TRUE;
			lpszCmdLine += cchForceFlag;
		}

		SpawnProgramUnderLoader(lpszCmdLine, FALSE, bForce);
	}

	Exit(0);
}