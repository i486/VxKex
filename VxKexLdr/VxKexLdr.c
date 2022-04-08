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
#include <stdarg.h>

#include "VaRw.h"
#include "resource.h"

#define APPNAME L"VxKexLdr"
#define FRIENDLYAPPNAME L"VxKex Loader"
#define EXCEPTION_WX86_BREAKPOINT 0x4000001F

typedef struct _KEXIFEOPARAMETERS {
	DWORD	dwEnableVxKex;
	WCHAR	szWinVerSpoof[6];
	DWORD	dwAlwaysShowDebug;
	DWORD	dwDisableForChild;
	DWORD	dwDisableAppSpecific;
	DWORD	dwWaitForChild;
	DWORD	dwDebuggerSpoof;
} KEXIFEOPARAMETERS, *PKEXIFEOPARAMETERS, *LPKEXIFEOPARAMETERS;

//
// Global vars
//

WCHAR g_szExeFullPath[MAX_PATH];
WCHAR g_szKexDir[MAX_PATH];
ULONG_PTR g_vaExeBase;
ULONG_PTR g_vaPebBase;
HANDLE g_hProc = NULL;
HANDLE g_hThread = NULL;
DWORD g_dwProcId;
DWORD g_dwThreadId;
BOOL g_bExe64 = -1;
KEXIFEOPARAMETERS g_KexIfeoParameters;

//
// Utility functions
//

VOID Pause(
	VOID)
{
	HWND hWndConsole = GetConsoleWindow();
	if (hWndConsole && IsWindowVisible(hWndConsole)) {
		PrintF(L"\nYou may now close the console window.");
		Sleep(INFINITE);
	}
}

NORETURN VOID Exit(
	IN	DWORD	dwExitCode)
{
	Pause();
	ExitProcess(dwExitCode);
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
			CriticalErrorBoxF(L"Internal error: %s(bPatch=TRUE) called inappropriately", __FUNCTION__);
#else
			return;
#endif
		}

		if (!lpdwFakeMajorVersion) {
			lpdwFakeMajorVersion = (LPDWORD) VirtualAlloc(NULL, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			if (!lpdwFakeMajorVersion) {
#ifdef _DEBUG
				CriticalErrorBoxF(L"Failed to allocate memory: %s", GetLastErrorAsString());
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
				CriticalErrorBoxF(L"Failed to allocate memory: %s", GetLastErrorAsString());
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
			CriticalErrorBoxF(L"Internal error: Pointer to faked subsystem version too large.");
#else
			return;
#endif
		}

		if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(L"kernel32.dll"), &k32ModInfo, sizeof(k32ModInfo)) == FALSE) {
#ifdef _DEBUG
			CriticalErrorBoxF(L"Failed to retrieve module information of KERNEL32.DLL: %s", GetLastErrorAsString());
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
	IN	HANDLE		hProc,
	IN	PULONG_PTR	lpProcessBaseAddress OPTIONAL,
	IN	PULONG_PTR	lpPebBaseAddress OPTIONAL)
{
	NTSTATUS st;
	PROCESS_BASIC_INFORMATION BasicInfo;

	st = NtQueryInformationProcess(
		hProc,
		ProcessBasicInformation,
		&BasicInfo,
		sizeof(BasicInfo),
		NULL);

	if (!SUCCEEDED(st)) {
		CriticalErrorBoxF(L"Failed to query process information.\nNTSTATUS error code: %#010I32x", st);
	} else {
		// The returned PebBaseAddress is in the virtual address space of the
		// created process, not ours. It is also always the native PEB (i.e. 64 bit, even
		// when the process is 32 bit).

		if (lpPebBaseAddress) {
			*lpPebBaseAddress = (ULONG_PTR) BasicInfo.PebBaseAddress;
		}

		if (lpProcessBaseAddress) {
			ReadProcessMemory(hProc, &BasicInfo.PebBaseAddress->ImageBaseAddress,
							  lpProcessBaseAddress, sizeof(*lpProcessBaseAddress), NULL);
		}
	}
}

BOOL ProcessIs64Bit(
	IN	HANDLE	hProc)
{
#ifndef _WIN64
	return FALSE;
#else
	ULONG_PTR vaPeBase;
	HANDLE hOldProc = g_hProc;
	ULONG_PTR vaCoffHdr;
	BOOL bPe64;

	GetProcessBaseAddressAndPebBaseAddress(hProc, &vaPeBase, NULL);

	g_hProc = hProc;
	vaCoffHdr = vaPeBase + VaReadDword(vaPeBase + 0x3C) + 4;
	bPe64 = (VaReadWord(vaCoffHdr) == 0x8664) ? TRUE : FALSE;
	g_hProc = hOldProc;

	return bPe64;
#endif
}

VOID GetProcessImageFullPath(
	IN	HANDLE	hProc,
	OUT	LPWSTR	szFullPath)
{
	WCHAR szDosDevice[3] = L"A:";
	WCHAR szNtDevice[MAX_PATH];
	NTSTATUS st;
	struct {
		UNICODE_STRING us;
		WCHAR buf[MAX_PATH];
	} psinfo;

	// This returns a NT style path such as "\Device\HarddiskVolume3\Program Files\whatever.exe"
	// We can't just read it out of ProcessParameters because that value is not yet initialized.
	st = NtQueryInformationProcess(
		hProc,
		ProcessImageFileName,
		&psinfo,
		sizeof(psinfo),
		NULL);

	for (; szDosDevice[0] <= 'Z'; szDosDevice[0]++) {
		if (QueryDosDevice(szDosDevice, szNtDevice, ARRAYSIZE(szNtDevice) - 1)) {
			if (!wcsnicmp(szNtDevice, psinfo.us.Buffer, wcslen(szNtDevice))) {
				break;
			}
		}
	}

	if (szDosDevice[0] > 'Z') {
		CriticalErrorBoxF(L"%s was unable to resolve the DOS device name of the executable file '%s'.\n"
						  L"This may be caused by the file residing on a network share or unmapped drive. "
						  L"%s does not currently support this scenario, please ensure all executable files "
						  L"are on mapped drives with drive letters.",
						  FRIENDLYAPPNAME, FRIENDLYAPPNAME);
	}

	wcscpy_s(szFullPath, MAX_PATH, szDosDevice);
	wcscat_s(szFullPath, MAX_PATH, psinfo.us.Buffer + wcslen(szNtDevice));
}

DWORD GetParentProcessId(
	VOID)
{
	NTSTATUS st;
	PROCESS_BASIC_INFORMATION pbi;

	st = NtQueryInformationProcess(GetCurrentProcess(),
								   ProcessBasicInformation,
								   &pbi, sizeof(pbi), NULL);

	if (st == STATUS_SUCCESS) {
		return (DWORD) pbi.InheritedFromUniqueProcessId;
	} else {
		return -1;
	}
}

HANDLE OpenParentProcess(
	IN	DWORD	dwDesiredAccess)
{
	DWORD dwParentId = GetParentProcessId();
	
	if (dwParentId != -1) {
		return OpenProcess(dwDesiredAccess, FALSE, dwParentId);
	} else {
		return NULL;
	}
}

BOOL ChildProcessIsConsoleProcess(
	VOID)
{
	return VaReadDword(g_vaPebBase + FIELD_OFFSET(PEB, ImageSubsystem)) == IMAGE_SUBSYSTEM_WINDOWS_CUI;
}


// 1. Check if parent process is called cmd.exe or command.com
// 2. If so:
//      1. suspend the entire cmd.exe process
//      2. place a duplicated handle of our child process into the unused "SsHandle" member of parent PEB->Ldr
//      3. create a remote thread which loads CmdSus32.dll or CmdSus64.dll from KexDir
// The CmdSus.dll's DllMain procedure will:
//   1. Call NtWaitForSingleObject to wait for the child process to exit
//   2. Call NtResumeProcess on the cmd.exe process
//   3. Return FALSE to unload the DLL and exit thread.
VOID PerformCmdSusHack(
	VOID)
{
	HANDLE hParent;
	HANDLE hChild;
	WCHAR szParentFullName[MAX_PATH];
	BOOL bParent64;
	ULONG_PTR vaParentBase;
	ULONG_PTR vaParentPeb;
	ULONG_PTR vaParentPebLdr;
	ULONG_PTR vaDllFullPath;
	ULONG_PTR vaLoadLibraryW;
	WCHAR szDllFullPath[MAX_PATH];
	INT cbDllFullPath;

	hParent = OpenParentProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
								PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE |
								PROCESS_SUSPEND_RESUME | PROCESS_DUP_HANDLE);

	if (!hParent) {
		// we tried.
		return;
	}

	// Now to check the name of the process to see whether it's a recognized shell.
	// It must be inside the Windows directory as well.
	GetProcessImageFullPath(hParent, szParentFullName);

	if (!PathIsPrefix(SharedUserData->NtSystemRoot, szParentFullName)) {
		return;
	}

	PathStripPath(szParentFullName);

	unless (!wcsicmp(szParentFullName, L"CMD.EXE") || !wcsicmp(szParentFullName, L"POWERSHELL.EXE")) {
		return;
	}

	// step 1
	CHECKED(NtSuspendProcess(hParent) == STATUS_SUCCESS);

	if (!wcsicmp(szParentFullName, L"CMD.EXE")) {
		INPUT EnterKey;
		HANDLE hStdOut = NtCurrentPeb()->ProcessParameters->StandardOutput;
		CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
		DWORD dwDiscard;

		// We need to send an extra Enter key for CMD.EXE so that it doesn't chew up a line of
		// input that should have gone to the child process. Powershell doesn't appear to need
		// this, and if we send the Enter key anyway, it appears in the input stream of the child.
		EnterKey.type			= INPUT_KEYBOARD;
		EnterKey.ki.wVk			= VK_RETURN;
		EnterKey.ki.wScan		= MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
		EnterKey.ki.dwFlags		= 0;
		EnterKey.ki.time		= 0;
		EnterKey.ki.dwExtraInfo	= GetMessageExtraInfo();
		SendInput(1, &EnterKey, sizeof(INPUT));

		// We also need to move the cursor up by a column.
		GetConsoleScreenBufferInfo(hStdOut, &ScreenBufferInfo);
		ScreenBufferInfo.dwCursorPosition.X = 0;
		ScreenBufferInfo.dwCursorPosition.Y -= 1;
		SetConsoleCursorPosition(hStdOut, ScreenBufferInfo.dwCursorPosition);

		// Then erase the screen underneath the cursor.
		FillConsoleOutputCharacter(
			hStdOut, ' ',
			(ScreenBufferInfo.dwSize.Y - ScreenBufferInfo.dwCursorPosition.Y) * ScreenBufferInfo.dwSize.X,
			ScreenBufferInfo.dwCursorPosition, &dwDiscard);
	}

	// step 2
	bParent64 = ProcessIs64Bit(hParent);
	CHECKED(DuplicateHandle(GetCurrentProcess(), g_hProc, hParent, &hChild, SYNCHRONIZE, FALSE, 0));
	GetProcessBaseAddressAndPebBaseAddress(hParent, &vaParentBase, &vaParentPeb);

#ifdef _M_X64
	// if parent is 32 bit but the system is 64 bit, subtract 0x1000 from the returned PEB base,
	// which will get us the 32 bit PEB which the CmdSus DLL can actually read from.
	// Note: 0x1000 corresponds to the page size. Perhaps it would be better to get the "real"
	// page size from GetSystemInfo? Anyway it probably doesn't matter.
	if (!bParent64) {
		vaParentPeb -= 0x1000;
	}
#endif

	CHECKED(ReadProcessMemory(hParent, (LPVOID) (vaParentPeb + (bParent64 ? 0x18 : 0x0C)), &vaParentPebLdr, bParent64 ? sizeof(QWORD) : sizeof(DWORD), NULL));
	CHECKED(WriteProcessMemory(hParent, (LPVOID) (vaParentPebLdr + 0x08), &hChild, bParent64 ? sizeof(QWORD) : sizeof(DWORD), NULL));

	// step 3
	cbDllFullPath = (swprintf_s(szDllFullPath, ARRAYSIZE(szDllFullPath), L"%s\\CmdSus%d.dll", g_szKexDir, bParent64 ? 64 : 32) + 1) * sizeof(WCHAR);
	vaDllFullPath = (ULONG_PTR) VirtualAllocEx(hParent, NULL, cbDllFullPath, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	CHECKED(vaDllFullPath != 0);
	CHECKED(WriteProcessMemory(hParent, (LPVOID) vaDllFullPath, szDllFullPath, cbDllFullPath, NULL));

	if (bParent64) {
		// 64 bit loader, 64 bit parent process
		vaLoadLibraryW = (ULONG_PTR) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
		CHECKED(vaLoadLibraryW);
	} else {
#ifdef _M_X64
		// 64 bit loader, 32 bit parent process.
		// just start a helper process to find the 32-bit kernel32 LoadLibraryW
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcInfo;
		WCHAR szCmdSus32Exe[MAX_PATH];
		DWORD dwvaLoadLibraryW;

		GetStartupInfo(&StartupInfo);
		swprintf_s(szCmdSus32Exe, ARRAYSIZE(szCmdSus32Exe), L"%s\\CmdSus32.exe", g_szKexDir);
		CHECKED(CreateProcess(szCmdSus32Exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcInfo));
		NtClose(ProcInfo.hThread);
		NtWaitForSingleObject(ProcInfo.hProcess, FALSE, NULL);
		CHECKED(GetExitCodeProcess(ProcInfo.hProcess, &dwvaLoadLibraryW));
		NtClose(ProcInfo.hProcess);
		CHECKED(vaLoadLibraryW = dwvaLoadLibraryW);
#else
		// 32 bit loader and 32 bit parent process
		vaLoadLibraryW = (ULONG_PTR) GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
#endif
	}

	{
		// Create the remote thread, which will load CmdSus.dll and execute its DllMain routine.
		HANDLE hThread = CreateRemoteThread(hParent, NULL, 0, (LPTHREAD_START_ROUTINE) vaLoadLibraryW, (LPVOID) vaDllFullPath, 0, NULL);
		CHECKED(hThread);
		NtClose(hThread);
	}

	goto Exit;
Error:
	NtResumeProcess(hParent);
Exit:
	NtClose(hParent);
	return;
}

// 1. Add the appropriate Kex(32/64) to PATH
// 2. Patch kernel32 CreateProcess if the subsystem version is incorrect
// 3. Set g_bExe64 correctly
VOID PreScanExeFile(
	VOID)
{
	WCHAR szPathAppend[MAX_PATH + 7];
	LPWSTR szPath;
	DWORD dwcchPath;
	HMODULE hModExe;
	PIMAGE_DOS_HEADER lpDosHdr;
	PIMAGE_NT_HEADERS lpNtHdr;
	PIMAGE_FILE_HEADER lpFileHdr;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	
	union {
		PIMAGE_OPTIONAL_HEADER Hdr;
		PIMAGE_OPTIONAL_HEADER32 Hdr32;
		PIMAGE_OPTIONAL_HEADER64 Hdr64;
	} lpOptHdr;

	hModExe = LoadLibraryEx(g_szExeFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (!hModExe) {
		CriticalErrorBoxF(L"Failed to scan the executable file \"%s\": %#010I32x: %s",
						  g_szExeFullPath, GetLastError(), GetLastErrorAsString());
	}

	lpDosHdr = (PIMAGE_DOS_HEADER) ((LPBYTE) hModExe);

	if (lpDosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
		// When you specify LOAD_LIBRARY_AS_DATAFILE and the DLL/EXE is not already
		// loaded, the returned HMODULE is offset by +1 from the real beginning of
		// the DOS header.
		// But when the EXE is already loaded (which can only happen when you use
		// VxKexLdr to load VxKexLdr), the returned HMODULE is not offset.
		lpDosHdr = (PIMAGE_DOS_HEADER) (((LPBYTE) hModExe) - 1);
	}

	lpNtHdr = (PIMAGE_NT_HEADERS) (((LPBYTE) lpDosHdr) + lpDosHdr->e_lfanew);
	lpFileHdr = &lpNtHdr->FileHeader;
	lpOptHdr.Hdr = &lpNtHdr->OptionalHeader;
	g_bExe64 = (lpFileHdr->Machine == 0x8664);

	if (g_bExe64) {
		MajorSubsystemVersion = lpOptHdr.Hdr64->MajorSubsystemVersion;
		MinorSubsystemVersion = lpOptHdr.Hdr64->MinorSubsystemVersion;
	} else {
		MajorSubsystemVersion = lpOptHdr.Hdr32->MajorSubsystemVersion;
		MinorSubsystemVersion = lpOptHdr.Hdr32->MinorSubsystemVersion;
	}

	FreeLibrary(hModExe);

	// If we would fail BasepIsImageVersionOk (also called BasepCheckImageVersion in some versions)
	// due to the image subsystem versions being too high, then patch it to succeed. It will
	// be unpatched after the CreateProcess call.
	if ((MajorSubsystemVersion > SharedUserData->NtMajorVersion) ||
		((MajorSubsystemVersion == SharedUserData->NtMajorVersion) && (MinorSubsystemVersion > SharedUserData->NtMinorVersion))) {
		PatchKernel32ImageVersionCheck(TRUE);
	}

	// Append correct extended DLL directory to PATH environment variable.
	swprintf_s(szPathAppend, ARRAYSIZE(szPathAppend), L";%s\\Kex%d", g_szKexDir, g_bExe64 ? 64 : 32);
	dwcchPath = GetEnvironmentVariable(L"Path", NULL, 0) + (DWORD) wcslen(szPathAppend);
	szPath = (LPWSTR) StackAlloc(dwcchPath * sizeof(WCHAR));
	GetEnvironmentVariable(L"Path", szPath, dwcchPath);
	wcscat_s(szPath, dwcchPath, szPathAppend);
	SetEnvironmentVariable(L"Path", szPath);
}

VOID CreateSuspendedProcess(
	IN	LPWSTR	lpszCmdLine)	// Full command-line of the process incl. executable name
{
	BOOL bSuccess;
	DWORD dwCpError;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;

	GetStartupInfo(&StartupInfo);
	StartupInfo.lpTitle = NULL;

	PreScanExeFile();

	// attach to parent console (if any) to allow child process to inherit it
	AttachConsole(ATTACH_PARENT_PROCESS);

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

	// unpatch kernel32 if it was patched before
	PatchKernel32ImageVersionCheck(FALSE);

	if (!bSuccess) {
		if (dwCpError == ERROR_BAD_EXE_FORMAT) {
			// Windows doesn't provide any pre-defined error message for this scenario.
			// Therefore to prevent user confusion we have to display our own error message.
			CriticalErrorBoxF(L"Failed to create process: %#010I32x: The executable file is invalid.", dwCpError);
		} else if (dwCpError = ERROR_ELEVATION_REQUIRED && !IsUserAnAdmin()) {
			// Debugging elevated processes as a non-elevated process isn't allowed so we need to re-exec
			// ourselves as admin.
			WCHAR szVxKexLdr[MAX_PATH];
			LPCWSTR lpszCommandLine = GetCommandLine();
			LPCWSTR lpszSubStr;
			GetModuleFileName(NULL, szVxKexLdr, ARRAYSIZE(szVxKexLdr));
			lpszSubStr = StrStrI(lpszCommandLine, szVxKexLdr);

			if (lpszSubStr == lpszCommandLine || lpszSubStr == lpszCommandLine + 1) {
				lpszCommandLine = lpszSubStr + wcslen(szVxKexLdr);
			}

			if (*lpszCommandLine == '\"') {
				++lpszCommandLine;
			}

			ShellExecute(NULL, L"runas", szVxKexLdr, lpszCommandLine, NULL, TRUE);
			ExitProcess(0);
		} else if (dwCpError == 0) {
			// This can happen, for example, if you try to run winload.exe or ntoskrnl.exe
			CriticalErrorBoxF(L"Failed to create process: Unspecified error.");
		}

		CriticalErrorBoxF(L"Failed to create process: %#010I32x: %s", dwCpError, GetLastErrorAsString());
	}

	g_hProc = ProcInfo.hProcess;
	g_hThread = ProcInfo.hThread;
	g_dwProcId = ProcInfo.dwProcessId;
	g_dwThreadId = ProcInfo.dwThreadId;
	GetProcessBaseAddressAndPebBaseAddress(g_hProc, &g_vaExeBase, &g_vaPebBase);

	if (ChildProcessIsConsoleProcess()) {
		PerformCmdSusHack(); // must be done before freeing console
	}

	// we don't need this console anymore, a separate one will be allocated later if needed
	FreeConsole();
}

VOID CreateNormalProcess(
	IN	LPWSTR	lpszCmdLine)
{
	CreateSuspendedProcess(lpszCmdLine);
	DebugSetProcessKillOnExit(FALSE);
	DebugActiveProcessStop(g_dwProcId);
	NtResumeThread(g_hThread, NULL);
}

ULONG_PTR GetEntryPointVa(
	IN	ULONG_PTR	vaPeBase)
{
	CONST ULONG_PTR vaPeSig = vaPeBase + VaReadDword(vaPeBase + 0x3C);
	CONST ULONG_PTR vaCoffHdr = vaPeSig + 4;
	CONST ULONG_PTR vaOptHdr = vaCoffHdr + 20;
	return vaPeBase + VaReadDword(vaOptHdr + 16);
}

BOOL KexOpenIfeoKeyForExe(
	IN	LPCWSTR	lpszExeFullPath OPTIONAL,
	IN	REGSAM	samDesired,
	OUT	PHKEY	phkResult)
{
	WCHAR szKexIfeoKey[54 + MAX_PATH] = L"SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\";
	LSTATUS lStatus;

	if (lpszExeFullPath) {
		wcscat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), lpszExeFullPath);
	}

	lStatus = RegOpenKeyEx(HKEY_CURRENT_USER, szKexIfeoKey, 0, samDesired, phkResult);

	if (lStatus != ERROR_SUCCESS) {
		SetLastError(lStatus);
		return FALSE;
	} else {
		return TRUE;
	}
}

BOOL ShouldAllocConsole(
	VOID)
{
	DWORD dwShouldAllocConsole = FALSE;
	RegReadDw(HKEY_CURRENT_USER, L"SOFTWARE\\VXsoft\\VxKexLdr", L"ShowDebugInfoByDefault", &dwShouldAllocConsole);
	return dwShouldAllocConsole || g_KexIfeoParameters.dwAlwaysShowDebug;
}

// user32.dll -> user33.dll etc etc
VOID RewriteImports(
	IN	LPCWSTR		lpszImageName OPTIONAL,
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
	
	BOOL bAtLeastOneDllWasRewritten = FALSE;
	HKEY hKeyDllRewrite;
	ULONG_PTR idxIntoImportTbl;
	LSTATUS lStatus;

	if (g_bExe64 != bPe64) {
		// Provide a more useful error message to the user than the cryptic shit
		// which ntdll displays by default.
		CriticalErrorBoxF(L"The %d-bit DLL '%s' was loaded into a %d-bit application.\n"
						  L"Architecture of DLLs and applications must match for successful program operation.",
						  bPe64 ? 64 : 32, lpszImageName ? lpszImageName : L"(unknown)", g_bExe64 ? 64 : 32);
	}

	if (ulImportDir == 0) {
		PrintF(L"    This DLL contains no import directory - skipping\n");
		return;
	}

	lStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite",
						   0, KEY_QUERY_VALUE, &hKeyDllRewrite);

	if (lStatus != ERROR_SUCCESS) {
		SetLastError(lStatus);
		CriticalErrorBoxF(L"Failed to open DLL rewrite registry key: %s", GetLastErrorAsString());
	}

	for (idxIntoImportTbl = 0;; ++idxIntoImportTbl) {
		CONST ULONG_PTR ulPtr = vaImportTbl + (idxIntoImportTbl * 20);
		CONST DWORD rvaDllName = VaReadDword(ulPtr + 12);
		CONST ULONG_PTR vaDllName = vaPeBase + rvaDllName;
#ifdef UNICODE
		CHAR originalDllNameData[MAX_PATH * sizeof(WCHAR)];
#endif
		WCHAR szOriginalDllName[MAX_PATH];
		WCHAR szRewriteDllName[MAX_PATH];
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
			CONST DWORD dwOriginalLength = (DWORD) wcslen(szOriginalDllName);
			CONST DWORD dwRewriteLength = (DWORD) wcslen(szRewriteDllName);

			if (dwRewriteLength > dwOriginalLength) {
				// IMPORTANT NOTE: When you add more rewrite DLLs to the registry, the length of the
				// rewrite DLL name MUST BE LESS THAN OR EQUAL to the length of the original DLL name.
				PrintF(L"    WARNING: The rewrite DLL name %s for original DLL %s is too long. "
						L"It is %u bytes but the maximum is %u. Skipping.\n",
						szRewriteDllName, szOriginalDllName, dwRewriteLength, dwOriginalLength);
			} else {
				bAtLeastOneDllWasRewritten = TRUE;
				PrintF(L"    Rewriting DLL import %s -> %s\n", szOriginalDllName, szRewriteDllName);
				VaWriteSzA(vaDllName, szRewriteDllName);
			}
		} else if (lStatus == ERROR_MORE_DATA) {
			// The data for the registry entry was too long
			PrintF(L"    WARNING: The rewrite DLL registry entry for the DLL %s "
					L"contains invalid data. Skipping.\n", szOriginalDllName);
		} else {
			// No rewrite entry was found
			PrintF(L"    Found DLL import %s - not rewriting\n", szOriginalDllName);
		}
	}

	if (bAtLeastOneDllWasRewritten) {
		// A Bound Import Directory will cause process initialization to fail. So we simply zero
		// it out.
		// Bound imports are a performance optimization, but basically we can't use it because
		// the bound import addresses are dependent on the "real" function addresses within the
		// imported DLL - and since we have replaced one or more imported DLLs, these pre-calculated
		// function addresses are no longer valid, so we just have to delete it.

		CONST ULONG_PTR vaBoundImportDir = vaOptHdr + (bPe64 ? 200 : 184);
		CONST DWORD dwBoundImportDirRva = VaReadDword(vaBoundImportDir);

		if (dwBoundImportDirRva) {
			PrintF(L"    Found a Bound Import Directory - zeroing\n");
			VaWriteDword(vaBoundImportDir, 0);
		}
	}

	RegCloseKey(hKeyDllRewrite);
}

BOOL ShouldRewriteImportsOfDll(
	IN	LPCWSTR	lpszDllName)
{
	WCHAR szDllDir[MAX_PATH];

	if (!lpszDllName || !wcslen(lpszDllName) || PathIsRelative(lpszDllName)) {
		// just be safe and don't rewrite it, since we don't even know what it is
		// for relative path: usually only NTDLL is specified as a relative path,
		// and we don't want to rewrite that either (especially since it has no
		// imports anyway)
		return FALSE;
	}

	wcscpy_s(szDllDir, ARRAYSIZE(szDllDir), lpszDllName);
	PathRemoveFileSpec(szDllDir);

	if (PathIsPrefix(SharedUserData->NtSystemRoot, szDllDir) || PathIsPrefix(g_szKexDir, szDllDir)) {
		// don't rewrite any imports if the particular DLL is in the Windows folder
		// or the kernel extensions folder
		return FALSE;
	}

	return TRUE;
}

// TODO: Make an XP compatible version
LPWSTR GetFilePathFromHandle(
	IN	HANDLE	hFile)
{
	static WCHAR szPath[MAX_PATH + 4];

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
	WCHAR szFakedVersion[6];

	PrintF(L"[KE] Performing post-initialization steps\n");

	if (wcscmp(g_KexIfeoParameters.szWinVerSpoof, L"NONE")) {
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

		if (!wcsicmp(szFakedVersion, L"WIN8")) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 2);
			VaWriteWord(vaBuildNumber, 9200);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!wcsicmp(szFakedVersion, L"WIN81")) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 3);
			VaWriteWord(vaBuildNumber, 9600);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!wcsicmp(szFakedVersion, L"WIN10")) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 19044);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		} else if (!wcsicmp(szFakedVersion, L"WIN11")) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 22000);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, &usFakeCSDVersion, sizeof(usFakeCSDVersion));
		}

		PrintF(L"[KE]    Faked Windows version: %s\n", szFakedVersion);
	}

	if (g_KexIfeoParameters.dwDebuggerSpoof) {
		ULONG_PTR vaDebuggerPresent = g_vaPebBase + FIELD_OFFSET(PEB, BeingDebugged);
		VaWriteByte(vaDebuggerPresent, !g_KexIfeoParameters.dwDebuggerSpoof);
		PrintF(L"[KE]    Spoofed debugger presence\n");
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

	PrintF(L"[KE ProcId=%lu, ThreadId=%lu]\tProcess entry point: %p, Original byte: 0x%02X\n",
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
			PrintF(L"[CP ProcId=%lu]\t\tProcess created.\n", DbgEvent.dwProcessId);
			NtClose(DbgEvent.u.CreateProcessInfo.hFile);
			NtClose(DbgEvent.u.CreateProcessInfo.hThread);
			NtClose(DbgEvent.u.CreateProcessInfo.hProcess);
		} else if (DbgEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
			HANDLE hDllFile = DbgEvent.u.LoadDll.hFile;
			LPWSTR lpszDllName;

			if (!hDllFile || (lpszDllName = GetFilePathFromHandle(hDllFile)) == NULL) {
				continue;
			}

			PrintF(L"[LD ProcId=%lu, ThreadId=%lu]\tDLL loaded: %s", DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszDllName);

			if (ShouldRewriteImportsOfDll(lpszDllName)) {
				PrintF(L" (imports will be rewritten)\n");
				PathStripPath(lpszDllName);
				RewriteImports(lpszDllName, (ULONG_PTR) DbgEvent.u.LoadDll.lpBaseOfDll);
			} else {
				PrintF(L" (imports will NOT be rewritten)\n");
			}
		} else if (DbgEvent.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
			PrintF(L"[UL ProcId=%lu, ThreadId=%lu]\tThe DLL at %p was unloaded.\n",
					DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.u.UnloadDll.lpBaseOfDll);
		} else if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			DWORD dwProcId = DbgEvent.dwProcessId;
			DWORD dwExitCode = DbgEvent.u.ExitProcess.dwExitCode;

			PrintF(L"[EP ProcId=%lu]\t\tAttached process has exited (code %#010I32x).\n", dwExitCode, dwProcId);

			if (dwProcId == g_dwProcId) {
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
			PrintF(L"[KE ProcId=%lu, ThreadId=%lu]\tProcess has finished loading.\n",
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

			if (!g_KexIfeoParameters.dwWaitForChild) {
				DebugSetProcessKillOnExit(FALSE);
				DebugActiveProcessStop(g_dwProcId);
				PrintF(L"[KE] Detached from process.\n");
				return;
			}
		} else if (DbgEvent.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
			HANDLE hProc = NULL;
			LPWSTR lpszBuf = NULL;
			SIZE_T BufSize = DbgEvent.u.DebugString.nDebugStringLength;

			if ((lpszBuf = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, BufSize)) == NULL) {
				continue;
			}
			
			if ((hProc = OpenProcess(PROCESS_VM_READ, FALSE, DbgEvent.dwProcessId)) == NULL) {
				continue;
			}

			if (ReadProcessMemory(hProc, DbgEvent.u.DebugString.lpDebugStringData, lpszBuf, BufSize, NULL) == FALSE) {
				continue;
			}

			NtClose(hProc);

			PrintF(L"[DS ProcId=%lu, ThreadId=%lu]\t%hs\n", DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszBuf);
			HeapFree(GetProcessHeap(), 0, lpszBuf);
		} else {
			PrintF(L"[?? ProcId=%lu, ThreadId=%lu]\tUnknown debug event received: %lu\n",
					DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.dwDebugEventCode);
		}
	}
}

VOID GetFirstCmdLineToken(
	IN	LPCWSTR	lpszCmdLine,
	OUT	LPWSTR	lpszFirstCmdLineToken)
{
	BOOL bDoubleQuoted = FALSE;

	while (*lpszCmdLine > ' ' || (*lpszCmdLine && bDoubleQuoted)) {
		if (*lpszCmdLine == '"') {
			if (bDoubleQuoted) {
				break;
			}

			bDoubleQuoted = !bDoubleQuoted;
		} else {
			*lpszFirstCmdLineToken++ = *lpszCmdLine;
		}

		lpszCmdLine++;
	}

	// Ashamedly, I forgot this and spent 10 minutes debugging wtf was going on.
	*lpszFirstCmdLineToken = '\0';
}

VOID GetExeFullPathFromCmdLine(
	IN	LPCWSTR	lpszCmdLine,
	OUT	LPWSTR	lpszExeFullPath)
{
	WCHAR szFirstCmdLineToken[MAX_PATH];
	GetFirstCmdLineToken(lpszCmdLine, szFirstCmdLineToken);

	if (PathIsRelative(szFirstCmdLineToken)) {
		if (!SearchPath(NULL, szFirstCmdLineToken, L".exe", MAX_PATH, lpszExeFullPath, NULL)) {
			CriticalErrorBoxF(L"Unable to locate \"%s\": %s",
							  szFirstCmdLineToken, GetLastErrorAsString());
		}
	} else {
		wcscpy_s(lpszExeFullPath, MAX_PATH, szFirstCmdLineToken);
	}
}

VOID KexReadIfeoParameters(
	IN	LPCWSTR				szExeFullPath,
	OUT	LPKEXIFEOPARAMETERS	lpKexIfeoParameters)
{
	HKEY hKey;

	ZeroMemory(lpKexIfeoParameters, sizeof(*lpKexIfeoParameters));
	wcscpy_s(lpKexIfeoParameters->szWinVerSpoof, ARRAYSIZE(lpKexIfeoParameters->szWinVerSpoof), L"NONE");
	CHECKED(KexOpenIfeoKeyForExe(szExeFullPath, KEY_QUERY_VALUE, &hKey));

	RegReadSz(hKey, NULL, L"WinVerSpoof", lpKexIfeoParameters->szWinVerSpoof, ARRAYSIZE(lpKexIfeoParameters->szWinVerSpoof));
	RegReadDw(hKey, NULL, L"EnableVxKex",			&lpKexIfeoParameters->dwEnableVxKex);
	RegReadDw(hKey, NULL, L"AlwaysShowDebug",		&lpKexIfeoParameters->dwAlwaysShowDebug);
	RegReadDw(hKey, NULL, L"DisableForChild",		&lpKexIfeoParameters->dwDisableForChild);
	RegReadDw(hKey, NULL, L"DisableAppSpecific",	&lpKexIfeoParameters->dwDisableAppSpecific);
	RegReadDw(hKey, NULL, L"WaitForChild",			&lpKexIfeoParameters->dwWaitForChild);
	RegReadDw(hKey, NULL, L"DebuggerSpoof",			&lpKexIfeoParameters->dwDebuggerSpoof);

Error:
	RegCloseKey(hKey);
	return;
}

BOOL SpawnProgramUnderLoader(
	IN	LPWSTR	lpszCmdLine,
	IN	BOOL	bCalledFromDialog,
	IN	BOOL	bDialogDebugChecked,
	IN	BOOL	bForce)
{
	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", g_szKexDir, ARRAYSIZE(g_szKexDir))) {
		CriticalErrorBoxF(L"Could not find the VxKex installation directory. Please reinstall the software to fix this problem.");
	}
	
	GetExeFullPathFromCmdLine(lpszCmdLine, g_szExeFullPath);
	KexReadIfeoParameters(g_szExeFullPath, &g_KexIfeoParameters);

	if (!bCalledFromDialog && !bForce && !g_KexIfeoParameters.dwEnableVxKex) {
		// Since the IFEO in HKLM doesn't discriminate by image path and only by name, this check has to
		// be performed - if we aren't actually supposed to be enabled for this executable, just start
		// the process normally and be done with it.
		// Note: on Vista or 7+ (don't remember which) there's some sort of filter that can be used to
		// make HKLM IFEO only apply to a certain path. Maybe it's worth investigating.
		CreateNormalProcess(lpszCmdLine);
		return FALSE;
	}

	CreateSuspendedProcess(lpszCmdLine);

	if ((!bCalledFromDialog && ShouldAllocConsole()) || (bCalledFromDialog && bDialogDebugChecked)) {
		WCHAR szConsoleTitle[MAX_PATH + 25];
		AllocConsole();
		swprintf_s(szConsoleTitle, ARRAYSIZE(szConsoleTitle), L"VxKexLdr Debug Console (%s)", g_szExeFullPath);
		SetConsoleTitle(L"VxKexLdr Debug Console");
	}

	PrintF(L"Process command line: %s\n", lpszCmdLine);
	PrintF(L"The process is %d-bit and its base address is %p (PEB base address: %p)\n",
			g_bExe64 ? 64 : 32, g_vaExeBase, g_vaPebBase);

	RewriteImports(NULL, g_vaExeBase);
	RewriteDllImports();

	return TRUE;
}

HWND CreateToolTip(
	IN	HWND	hDlg,
	IN	INT		iToolID,
	IN	LPWSTR	lpszText)
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
	CONST WCHAR szUserSpecificConfigKey[] = L"SOFTWARE\\VXsoft\\" APPNAME;
	WCHAR szFilename[MAX_PATH + 2] = L"";
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
				hKeyUserSpecificConfig, L"LastCmdLine",
				0, NULL, (LPBYTE) szFilename, &dwcbFilename);
			lStatus = RegQueryValueEx(
				hKeyUserSpecificConfig, L"ShowDebugInfo",
				0, NULL, (LPBYTE) &bShouldShowDebugInfo, &dwcbShouldShowDebugInfo);

			if (lStatus != ERROR_SUCCESS) {
				bShouldShowDebugInfo = 0;
			}

			CheckDlgButton(hWnd, IDCHKDEBUG, bShouldShowDebugInfo ? BST_CHECKED : BST_UNCHECKED);
			RegCloseKey(hKeyUserSpecificConfig);
			SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		}

		CreateToolTip(hWnd, IDCHKDEBUG, L"Creates a console window to display additional debugging information");
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
			ofn.lpstrFilter			= L"Applications (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
			ofn.lpstrFile			= szFilename + 1;
			ofn.nMaxFile			= MAX_PATH;
			ofn.lpstrInitialDir		= L"C:\\Program Files\\";
			ofn.Flags				= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
			ofn.lpstrDefExt			= L"exe";
			
			if (GetOpenFileName(&ofn)) {				
				// Give the filename some quotes.
				szFilename[0] = '\"';
				wcscat_s(szFilename, ARRAYSIZE(szFilename), L"\"");
				SetDlgItemText(hWnd, IDFILEPATH, szFilename);
			}

			break;
		} else if (LOWORD(wParam) == IDOK) {
			GetDlgItemText(hWnd, IDFILEPATH, szFilename, dwcbFilename);
			bShouldShowDebugInfo = IsDlgButtonChecked(hWnd, IDCHKDEBUG);
			EndDialog(hWnd, 0);
			SpawnProgramUnderLoader(szFilename, TRUE, bShouldShowDebugInfo, TRUE);

			// save dialog information in the registry
			lStatus = RegCreateKeyEx(
				HKEY_CURRENT_USER,
				szUserSpecificConfigKey,
				0, NULL, 0, KEY_WRITE, NULL,
				&hKeyUserSpecificConfig, NULL);

			if (lStatus == ERROR_SUCCESS) {
				RegSetValueEx(
					hKeyUserSpecificConfig,
					L"LastCmdLine", 0, REG_SZ,
					(LPBYTE) szFilename,
					(DWORD) wcslen(szFilename) * sizeof(WCHAR));
				RegSetValueEx(
					hKeyUserSpecificConfig,
					L"ShowDebugInfo", 0, REG_DWORD,
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
		szFilename[0] = '"';
		DragQueryFile((HDROP) wParam, 0, szFilename + 1, MAX_PATH);
		StringCchCat(szFilename, ARRAYSIZE(szFilename), L"\"");
		SetDlgItemText(hWnd, IDFILEPATH, szFilename);
		DragFinish((HDROP) wParam);
		break;
	}
	
	return FALSE;
}

#define FORCE_FLAG L"/FORCE "

VOID EntryPoint(
	VOID)
{
	HINSTANCE hInstance = (HINSTANCE) NtCurrentPeb()->ImageBaseAddress;
	LPWSTR lpszCmdLine = GetCommandLineWithoutImageName();

	SetFriendlyAppName(FRIENDLYAPPNAME);
	hInstance = (HINSTANCE) NtCurrentPeb()->ImageBaseAddress;
	lpszCmdLine = GetCommandLineWithoutImageName();

	if (*lpszCmdLine == '\0') {
		InitCommonControls();
		DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	} else {
		CONST SIZE_T cchForceFlag = wcslen(FORCE_FLAG);
		BOOL bForce = FALSE;

		if (!wcsnicmp(lpszCmdLine, FORCE_FLAG, cchForceFlag)) {
			// This flag is used to bypass the usual checking that we're "supposed" to launch
			// the program with VxKex enabled. For example, the shell extended context menu entry
			// will use this flag, to let the user try out VxKex without having to go into the
			// property sheet and (possibly) accept the UAC dialog etc etc.
			bForce = TRUE;
			lpszCmdLine += cchForceFlag;
		}

		SpawnProgramUnderLoader(lpszCmdLine, FALSE, FALSE, bForce);
	}

	Exit(0);
}