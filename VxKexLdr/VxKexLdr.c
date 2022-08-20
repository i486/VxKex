#include <Windows.h>
#include <WindowsX.h>
#include <Psapi.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <conio.h>
#include <stdio.h>
#include <stdarg.h>

#include <KexComm.h>
#include <KexData.h>
#include <NtDll.h>

#include "VaRw.h"
#include "resource.h"

#define APPNAME L"VxKexLdr"
#define FRIENDLYAPPNAME L"VxKex Loader"
#define EXCEPTION_WX86_BREAKPOINT 0x4000001F

//
// Global vars
//

WCHAR g_szExeFullPath[MAX_PATH];
LPCWSTR g_lpszExeBaseName = NULL;
ULONG_PTR g_vaExeBase;
ULONG_PTR g_vaPebBase;
HANDLE g_hProc = NULL;
HANDLE g_hThread = NULL;
DWORD g_dwProcId;
DWORD g_dwThreadId;
BOOL g_bExe64 = -1;
HANDLE g_hLogFile = NULL;
KEX_PROCESS_DATA g_KexData;

//
// Utility functions
//

VOID LogF(
	IN	LPCWSTR lpszFmt, ...)
{
	SIZE_T cch;
	LPWSTR lpszText;
	DWORD dwDiscard;

#ifndef _DEBUG
	ASSUME(g_hLogFile == NULL);
#endif

	if (g_hLogFile || g_KexData.IfeoParameters.dwAlwaysShowDebug) {
		va_list ap;
		va_start(ap, lpszFmt);
		cch = vscwprintf(lpszFmt, ap) + 1;
		lpszText = (LPWSTR) StackAlloc(cch * sizeof(WCHAR));
		vswprintf_s(lpszText, cch, lpszFmt, ap);
		va_end(ap);
	}

	WriteConsole(NtCurrentPeb()->ProcessParameters->StandardOutput, lpszText, (DWORD) cch - 1, &dwDiscard, NULL);

	if (g_hLogFile) {
		WriteFile(g_hLogFile, lpszText, (DWORD) (cch - 1) * sizeof(WCHAR), &dwDiscard, NULL);
	}
}

VOID Pause(
	VOID)
{
	HWND hWndConsole = GetConsoleWindow();
	if (hWndConsole && IsWindowVisible(hWndConsole)) {
		PrintF(L"\r\nYou may now close the console window.");
		Sleep(INFINITE);
	}
}

NORETURN VOID Exit(
	IN	DWORD	dwExitCode)
{
	if (g_hLogFile) {
		NtClose(g_hLogFile);
	}

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
		// when the process is 32 bit). If you want the 32 bit PEB of a wow64 process,
		// then subtract 0x1000 from the address.

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
	LPWSTR lpszFullPath;
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

	lpszFullPath = ConvertDeviceHarddiskToDosPath(psinfo.us.Buffer);

	if (!lpszFullPath) {
		CriticalErrorBoxF(L"%s was unable to resolve the DOS device name of the executable file '%s'.\r\n"
						  L"This may be caused by the file residing on a network share or unmapped drive. "
						  L"%s does not currently support this scenario, please ensure all executable files "
						  L"are on mapped drives with drive letters.",
						  FRIENDLYAPPNAME, FRIENDLYAPPNAME);
	}

	wcscpy_s(szFullPath, MAX_PATH, lpszFullPath);
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
	cbDllFullPath = (swprintf_s(szDllFullPath, ARRAYSIZE(szDllFullPath), L"%s\\CmdSus%d.dll", g_KexData.szKexDir, bParent64 ? 64 : 32) + 1) * sizeof(WCHAR);
	vaDllFullPath = (ULONG_PTR) VirtualAllocEx(hParent, NULL, cbDllFullPath, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	CHECKED(vaDllFullPath != 0);
	CHECKED(WriteProcessMemory(hParent, (LPVOID) vaDllFullPath, szDllFullPath, cbDllFullPath, NULL));

	if (bParent64) {
		// 64 bit loader, 64 bit parent process
		vaLoadLibraryW = (ULONG_PTR) &LoadLibraryW;
	} else {
#ifdef _M_X64
		// 64 bit loader, 32 bit parent process.
		// just start a helper process to find the 32-bit kernel32 LoadLibraryW
		STARTUPINFO StartupInfo;
		PROCESS_INFORMATION ProcInfo;
		WCHAR szCmdSus32Exe[MAX_PATH];
		DWORD dwvaLoadLibraryW;

		GetStartupInfo(&StartupInfo);
		swprintf_s(szCmdSus32Exe, ARRAYSIZE(szCmdSus32Exe), L"%s\\CmdSus32.exe", g_KexData.szKexDir);
		CHECKED(CreateProcess(szCmdSus32Exe, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcInfo));
		NtClose(ProcInfo.hThread);
		NtWaitForSingleObject(ProcInfo.hProcess, FALSE, NULL);
		CHECKED(GetExitCodeProcess(ProcInfo.hProcess, &dwvaLoadLibraryW));
		NtClose(ProcInfo.hProcess);
		CHECKED(vaLoadLibraryW = dwvaLoadLibraryW);
#else
		// 32 bit loader and 32 bit parent process
		vaLoadLibraryW = (ULONG_PTR) &LoadLibraryW;
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
	swprintf_s(szPathAppend, ARRAYSIZE(szPathAppend), L";%s\\Kex%d", g_KexData.szKexDir, g_bExe64 ? 64 : 32);
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
	SIZE_T cbProcThreadAttributeList;
	STARTUPINFOEX StartupInfoEx;
	PROCESS_INFORMATION ProcInfo;
	HANDLE hParentProcess = OpenParentProcess(PROCESS_CREATE_PROCESS);
	LPWSTR lpszApplicationName;

	// This "ProcThreadAttribute" stuff is a Vista+ feature that allows you to pretend
	// to the child process, that its parent process is our parent process.
	// You can check the behavior of this code through process explorer.
	InitializeProcThreadAttributeList(NULL, 1, 0, &cbProcThreadAttributeList);
	GetStartupInfo(&StartupInfoEx.StartupInfo);

	lpszApplicationName = StartupInfoEx.StartupInfo.lpReserved;

	if (lpszApplicationName && *lpszApplicationName == '\0') {
		lpszApplicationName = NULL;
	}

	StartupInfoEx.StartupInfo.cb = sizeof(StartupInfoEx);
	StartupInfoEx.StartupInfo.lpReserved = NULL;
	StartupInfoEx.StartupInfo.cbReserved2 = 0;
	StartupInfoEx.StartupInfo.lpReserved2 = NULL;
	StartupInfoEx.StartupInfo.lpTitle = NULL;
	StartupInfoEx.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST) StackAlloc(cbProcThreadAttributeList);

	InitializeProcThreadAttributeList(StartupInfoEx.lpAttributeList, 1, 0, &cbProcThreadAttributeList);
	UpdateProcThreadAttribute(StartupInfoEx.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
							  &hParentProcess, sizeof(hParentProcess), NULL, NULL);

	PreScanExeFile();

	// attach to parent console (if any) to allow child process to inherit it
	AttachConsole(ATTACH_PARENT_PROCESS);

	bSuccess = CreateProcess(
		lpszApplicationName,
		lpszCmdLine,
		NULL,
		NULL,
		TRUE,
		CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS | EXTENDED_STARTUPINFO_PRESENT,
		NULL,
		NULL,
		&StartupInfoEx.StartupInfo,
		&ProcInfo);
	dwCpError = RtlGetLastWin32Error();

	// unpatch kernel32 if it was patched before
	PatchKernel32ImageVersionCheck(FALSE);

	// free procthreadattribute shit
	DeleteProcThreadAttributeList(StartupInfoEx.lpAttributeList);
	NtClose(hParentProcess);

	if (!bSuccess) {
		if (dwCpError == ERROR_BAD_EXE_FORMAT) {
			// Windows doesn't provide any pre-defined error message for this scenario.
			// Therefore to prevent user confusion we have to display our own error message.
			CriticalErrorBoxF(L"Failed to create process: %#010I32x: The executable file is invalid.", dwCpError);
		} else if (dwCpError == ERROR_ELEVATION_REQUIRED && !IsUserAnAdmin()) {
			// Debugging elevated processes as a non-elevated process isn't allowed so we need to re-exec
			// ourselves as admin. With the /FISH flag, the child process will simply open its parent (us)
			// and "fish" the parameters out of our address space. This means that we need to wait around
			// until the child finishes - we will do Sleep(1000). The 1 second timeout is in case our elevated
			// VxKexLdr process somehow fails. But usually the child will terminate us when it's done.
			WCHAR szVxKexLdr[MAX_PATH];
			StringCchPrintf(szVxKexLdr, ARRAYSIZE(szVxKexLdr), L"%s\\VxKexLdr.exe", g_KexData.szKexDir);
			ShellExecute(NULL, L"runas", szVxKexLdr, L"/FISH", NULL, TRUE);
#ifdef _DEBUG
			// In debugging mode we don't want shit disappearing under us when in the debugger.
			Sleep(INFINITE);
#else
			Sleep(1000);
#endif
			ExitProcess(0);
		} else if (dwCpError == 0) {
			// This can happen, for example, if you try to run winload.exe or ntoskrnl.exe
			CriticalErrorBoxF(L"Failed to create process: Unspecified error.");
		}

		RtlSetLastWin32Error(dwCpError);
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

#ifdef _DEBUG
	// allow other debuggers to debug the child process after we exit
	DebugSetProcessKillOnExit(FALSE);
#endif
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
	return dwShouldAllocConsole || g_KexData.IfeoParameters.dwAlwaysShowDebug;
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
		// APPSPECIFICHACK
		unless (g_KexData.IfeoParameters.dwDisableAppSpecific) {
			if (!wcsicmp(g_lpszExeBaseName, L"MAYA.EXE") ||
				!wcsicmp(g_lpszExeBaseName, L"MAYABATCH.EXE")) {
				// Ignore bitness mismatch for Maya
				goto SkipBitnessMismatchError;
			}
		}

		// Provide a more useful error message to the user than the cryptic shit
		// which ntdll displays by default.
		CriticalErrorBoxF(L"The %d-bit DLL '%s' was loaded into a %d-bit application.\r\n"
						  L"Architecture of DLLs and applications must match for successful program operation.",
						  bPe64 ? 64 : 32, lpszImageName ? lpszImageName : L"(unknown)", g_bExe64 ? 64 : 32);
	}

SkipBitnessMismatchError:

	if (ulImportDir == 0) {
		LogF(L"    This DLL contains no import directory - skipping\r\n");
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
				LogF(L"    WARNING: The rewrite DLL name %s for original DLL %s is too long. "
						L"It is %u bytes but the maximum is %u. Skipping.\r\n",
						szRewriteDllName, szOriginalDllName, dwRewriteLength, dwOriginalLength);
			} else {
				bAtLeastOneDllWasRewritten = TRUE;
				LogF(L"    Rewriting DLL import %s -> %s\r\n", szOriginalDllName, szRewriteDllName);
				VaWriteSzA(vaDllName, szRewriteDllName);
			}
		} else if (lStatus == ERROR_MORE_DATA) {
			// The data for the registry entry was too long
			LogF(L"    WARNING: The rewrite DLL registry entry for the DLL %s "
					L"contains invalid data. Skipping.\r\n", szOriginalDllName);
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
			LogF(L"    Found a Bound Import Directory - zeroing\r\n");
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

	if (PathIsPrefix(SharedUserData->NtSystemRoot, szDllDir) || PathIsPrefix(g_KexData.szKexDir, szDllDir)) {
		// don't rewrite any imports if the particular DLL is in the Windows folder
		// or the kernel extensions folder
		return FALSE;
	}

	return TRUE;
}

VOID CopyKexDataToChild(
	VOID)
{
	ULONG_PTR vaChildKexData;
	ULONG_PTR vaSubSystemData;

	// allocate memory in child process for KexData structure
	vaChildKexData = (ULONG_PTR) VirtualAllocEx(g_hProc, NULL, sizeof(KEX_PROCESS_DATA), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!vaChildKexData) {
		CriticalErrorBoxF(L"Failed to allocate memory inside the child process to store VxKex environment data.");
	}

	// write the data
	VaWrite(vaChildKexData, &g_KexData, sizeof(KEX_PROCESS_DATA));

	// put the pointer to the data in Peb->SubSystemData
	vaSubSystemData = g_vaPebBase - (g_bExe64 ? 0 : 0x1000) + (g_bExe64 ? 0x28 : 0x14);
	VaWrite(vaSubSystemData, &vaChildKexData, (g_bExe64 ? sizeof(QWORD) : sizeof(DWORD)));
}

// fake version info, etc.
VOID PerformPostInitializationSteps(
	VOID)
{
#ifdef _M_X64
	// 32 bit peb if the process is 32 bit - otherwise version spoofing will have no effect
	ULONG_PTR vaProcessNativePebBase = g_vaPebBase - (g_bExe64 ? 0 : 0x1000);
#else
	ULONG_PTR vaProcessNativePebBase = g_vaPebBase;
#endif

	LogF(L"[KE] Performing post-initialization steps\r\n");

	// APPSPECIFICHACK
	unless (g_KexData.IfeoParameters.dwDisableAppSpecific) {		
		if (!wcsicmp(g_lpszExeBaseName, L"ONEDRIVESETUP.EXE")) {
			LogF(L"[KE] Skipping user-defined Windows version spoof due to app-specific hack\r\n");
			goto SkipWinVerSpoof;
		}
	}

	if (wcscmp(g_KexData.IfeoParameters.szWinVerSpoof, L"NONE")) {
		ULONG_PTR vaMajorVersion = vaProcessNativePebBase + (g_bExe64 ? 0x118 : 0xA4);
		ULONG_PTR vaMinorVersion = vaMajorVersion + sizeof(ULONG);
		ULONG_PTR vaBuildNumber = vaMinorVersion + sizeof(ULONG);
		ULONG_PTR vaCSDVersion = vaBuildNumber + sizeof(USHORT);
		ULONG_PTR vaCSDVersionUS = vaProcessNativePebBase + (g_bExe64 ? 0x2E8 : 0x1F0);
		UNICODE_STRING usFakeCSDVersion;
#ifdef _M_X64
		UNICODE_STRING32 us32FakeCSDVersion;
#endif
		LPVOID lpUnicodeString = &usFakeCSDVersion;
		DWORD dwSizeofUnicodeString = sizeof(UNICODE_STRING);

		// buffer points to its own length (which is 0, the null terminator)
		// conveniently, all windows versions 8 and above have no service packs
		usFakeCSDVersion.Length = 0;
		usFakeCSDVersion.MaximumLength = 0;
		usFakeCSDVersion.Buffer = (PWSTR) vaCSDVersionUS;

#ifdef _M_X64
		// 32 bits - if we just use sizeof(UNICODE_STRING) directly, we will clobber activation
		// context data, which breaks a lot of apps.
		if (!g_bExe64) {
			us32FakeCSDVersion.Length = 0;
			us32FakeCSDVersion.MaximumLength = 0;
			us32FakeCSDVersion.Buffer = (DWORD) vaCSDVersionUS;

			lpUnicodeString = &us32FakeCSDVersion;
			dwSizeofUnicodeString = sizeof(UNICODE_STRING32);
		}
#endif

		if (!wcsicmp(g_KexData.IfeoParameters.szWinVerSpoof, L"WIN8")) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 2);
			VaWriteWord(vaBuildNumber, 9200);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, lpUnicodeString, dwSizeofUnicodeString);
		} else if (!wcsicmp(g_KexData.IfeoParameters.szWinVerSpoof, L"WIN81")) {
			VaWriteDword(vaMajorVersion, 6);
			VaWriteDword(vaMinorVersion, 3);
			VaWriteWord(vaBuildNumber, 9600);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, lpUnicodeString, dwSizeofUnicodeString);
		} else if (!wcsicmp(g_KexData.IfeoParameters.szWinVerSpoof, L"WIN10")) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 19044);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, lpUnicodeString, dwSizeofUnicodeString);
		} else if (!wcsicmp(g_KexData.IfeoParameters.szWinVerSpoof, L"WIN11")) {
			VaWriteDword(vaMajorVersion, 10);
			VaWriteDword(vaMinorVersion, 0);
			VaWriteWord(vaBuildNumber, 22000);
			VaWriteWord(vaCSDVersion, 0);
			VaWrite(vaCSDVersionUS, lpUnicodeString, dwSizeofUnicodeString);
		}

		LogF(L"[KE]    Faked Windows version: %s\r\n", g_KexData.IfeoParameters.szWinVerSpoof);
	}

SkipWinVerSpoof:
	if (g_KexData.IfeoParameters.dwDebuggerSpoof) {
		ULONG_PTR vaDebuggerPresent = vaProcessNativePebBase + FIELD_OFFSET(PEB, BeingDebugged);
		VaWriteByte(vaDebuggerPresent, !g_KexData.IfeoParameters.dwDebuggerSpoof);
		LogF(L"[KE]    Spoofed debugger presence\r\n");
	}
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

STATIC NTSTATUS stHe = 0;
STATIC DWORD dwHe = 0;
STATIC LPWSTR lpszHe1 = NULL;
STATIC LPWSTR lpszHe2 = NULL;

// The possible NTSTATUS codes are:
//   STATUS_IMAGE_MACHINE_TYPE_MISMATCH		- someone tried running an ARM/other architecture binary or dll (%s)
//   STATUS_INVALID_IMAGE_FORMAT			- PE is corrupt (%s)
//   STATUS_DLL_NOT_FOUND					- a DLL is missing (%s)
//   STATUS_ORDINAL_NOT_FOUND				- DLL ordinal function is missing (%d, %s)
//   STATUS_ENTRYPOINT_NOT_FOUND			- DLL function is missing (%s, %s)
INT_PTR CALLBACK HeDlgProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	WCHAR szErrStmt[MAX_PATH + 86];
	STATIC HWND hWndDebugLog = NULL;

	switch (uMsg) {
	case WM_INITDIALOG:
		switch (stHe) {
		case STATUS_DLL_NOT_FOUND:
			SetWindowText(hWnd, L"DLL Not Found");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required %s was not found while attempting to launch %s.",
							lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_ENTRYPOINT_NOT_FOUND:
			SetWindowText(hWnd, L"Missing Function In DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required function %s was not found in %s while attempting to launch %s.",
							lpszHe1, lpszHe2, g_lpszExeBaseName);
			break;
		case STATUS_ORDINAL_NOT_FOUND:
			SetWindowText(hWnd, L"Missing Function In DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"The required ordinal function #%s was not found in %s while attempting to launch %s.",
							dwHe, lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_INVALID_IMAGE_FORMAT:
			SetWindowText(hWnd, L"Invalid or Corrupt Executable or DLL");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"%s is invalid or corrupt. %s cannot start.",
							lpszHe1, g_lpszExeBaseName);
			break;
		case STATUS_IMAGE_MACHINE_TYPE_MISMATCH:
			SetWindowText(hWnd, L"Processor Architecture Mismatch");
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"%s is designed for a different processor architecture. %s cannot start.",
							lpszHe1, g_lpszExeBaseName);
			break;
		default:
			StringCchPrintf(szErrStmt, ARRAYSIZE(szErrStmt),
							L"An unrecoverable fatal error has occurred during the initialization of %s.\r\n"
							L"The error code is 0x%08x: %s",
							g_lpszExeBaseName, stHe, NtStatusAsString(stHe));
			break;
		}

		SetDlgItemText(hWnd, IDERRSTATEMENT, szErrStmt);
		SendDlgItemMessage(hWnd, IDERRICON, STM_SETICON, (WPARAM) LoadIcon(NULL, IDI_ERROR), 0);
		hWndDebugLog = GetDlgItem(hWnd, IDDEBUGLOG);

		// give our edit control a monospaced font
		SendMessage(hWndDebugLog, WM_SETFONT, (WPARAM) CreateFont(
					-MulDiv(8, GetDeviceCaps(GetDC(hWndDebugLog), LOGPIXELSY), 72),
					0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
					CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
					L"Consolas"), MAKELPARAM(TRUE, 0));

		// populate the edit control with the contents of the log
		// the edit control is read-write in case the user wants to censor some private info
		if (g_hLogFile) {
			HANDLE hMapping;
			LPCWSTR lpszDocument;
			CHECKED(hMapping = CreateFileMapping(g_hLogFile, NULL, PAGE_READONLY, 0, 0, NULL));
			CHECKED(lpszDocument = (LPCWSTR) MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0));
			Edit_SetText(hWndDebugLog, lpszDocument);
			UnmapViewOfFile(lpszDocument);
			CloseHandle(hMapping);
		} else {
Error:
			// whoops...
			Edit_SetText(hWndDebugLog, L"No log file is available.");
		}

		CreateToolTip(hWnd, IDBUGREPORT, L"Opens the GitHub issue reporter in your browser");
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, 0);
		} else if (LOWORD(wParam) == IDCOPYCLIPBOARD) {
			if (OpenClipboard(hWndDebugLog)) {
				CONST INT cchDocument = GetWindowTextLength(hWndDebugLog) + 1;
				HGLOBAL hglbDocument = GlobalAlloc(GMEM_MOVEABLE, cchDocument * sizeof(WCHAR));
				LPWSTR lpszDocument;

				if (hglbDocument == NULL) {
					CloseClipboard();
					break;
				}

				lpszDocument = (LPWSTR) GlobalLock(hglbDocument);
				GetWindowText(hWndDebugLog, lpszDocument, cchDocument);
				GlobalUnlock(hglbDocument);

				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, hglbDocument);
				CloseClipboard();
				GlobalFree(hglbDocument);
			}
		} else if (LOWORD(wParam) == IDBUGREPORT) {
			WCHAR szUrl[512];

			// TODO: we can add text to issue body as well using ?body=whatever
			switch (stHe) {
			case STATUS_DLL_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-dll-not-found--error.md&title=%s+not+found+when+trying+to+run+%s",
					lpszHe1, g_lpszExeBaseName);
				break;
			case STATUS_ORDINAL_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-missing-function--error.md&title=Ordinal+%d+not+found+in+%s+when+trying+to+run+%s",
					dwHe, lpszHe1, g_lpszExeBaseName);
				break;
			case STATUS_ENTRYPOINT_NOT_FOUND:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?template=-missing-function--error.md&title=Function+%s+not+found+in+%s+when+trying+to+run+%s",
					lpszHe1, lpszHe2, g_lpszExeBaseName);
				break;
			default:
				StringCchPrintf(szUrl, ARRAYSIZE(szUrl),
					L"https://github.com/vxiiduu/VxKex/issues/new?title=Error+when+running+%s",
					g_lpszExeBaseName);
				break;
			}

			ShellExecute(hWnd, L"open", szUrl, NULL, NULL, SW_SHOWNORMAL);
		}

		break;
	}

	return FALSE;
}

VOID VklHardError(
	IN	NTSTATUS	st,
	IN	DWORD		dw,
	IN	LPWSTR		lpsz1,
	IN	LPWSTR		lpsz2)
{
	stHe = st;
	dwHe = dw;
	lpszHe1 = lpsz1;
	lpszHe2 = lpsz2;

	LogF(L"    st=0x%08x,dw=%I32d,lpsz1=%s,lpsz2=%s\r\n", st, dw, lpsz1, lpsz2);
	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG2), NULL, HeDlgProc);
}

// Monitors the process for DLL imports.
VOID RewriteDllImports(
	VOID)
{
	DEBUG_EVENT DbgEvent;
	BOOL bResumed = FALSE;
	CONST ULONG_PTR vaEntryPoint = GetEntryPointVa(g_vaExeBase);
	CONST ULONG_PTR vaNtRaiseHardError = (ULONG_PTR) &NtRaiseHardError;
	CONST BYTE btOriginal = VaReadByte(vaEntryPoint);
	CONST BYTE btOriginalHardError = VaReadByte(vaNtRaiseHardError);

	LogF(L"[KE ProcId=%lu, ThreadId=%lu]\tProcess entry point: 0x%p, Original byte: 0x%02X\r\n",
			g_dwProcId, g_dwThreadId, vaEntryPoint, btOriginal);
	VaWriteByte(vaEntryPoint, 0xCC);

	// The hard error handler is an additional debugging aid for development.
	// It traps a call to NtRaiseHardError, which is usually called when a DLL is missing, or
	// a function in a particular DLL is missing. Then it can be put into the log file, and an
	// appropriate informational message box can be displayed to the user.
	// The information which is inside the hard error message cannot be obtained through any
	// other means, since NTDLL raises an exception /after/ deleting all the information which
	// was sent to NtRaiseHardError.
	LogF(L"[KE ProcId=%lu, ThreadId=%lu]\tAdding hard error handler at 0x%p. Original byte: 0x%02X\r\n",
		 g_dwProcId, g_dwThreadId, vaNtRaiseHardError, btOriginalHardError);
	VaWriteByte(vaNtRaiseHardError, 0xCC);

	DbgEvent.dwProcessId = g_dwProcId;
	DbgEvent.dwThreadId = g_dwThreadId;

	while (TRUE) {
		if (!bResumed) {
			NtResumeThread(g_hThread, 0);
			bResumed = TRUE;
		} else {
			ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
		}

		WaitForDebugEvent(&DbgEvent, INFINITE);

		if (DbgEvent.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
			LogF(L"[CP ProcId=%lu]\t\tProcess created.\r\n", DbgEvent.dwProcessId);
			NtClose(DbgEvent.u.CreateProcessInfo.hFile);
			NtClose(DbgEvent.u.CreateProcessInfo.hThread);
			NtClose(DbgEvent.u.CreateProcessInfo.hProcess);
		} else if (DbgEvent.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
			HANDLE hDllFile = DbgEvent.u.LoadDll.hFile;
			LPWSTR lpszDllName;

			if (!hDllFile || (lpszDllName = GetFilePathFromHandle(hDllFile)) == NULL) {
				continue;
			}

			LogF(L"[LD ProcId=%lu, ThreadId=%lu]\tDLL loaded: %s\r\n", DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszDllName);

			if (ShouldRewriteImportsOfDll(lpszDllName)) {
				PathStripPath(lpszDllName);
				RewriteImports(lpszDllName, (ULONG_PTR) DbgEvent.u.LoadDll.lpBaseOfDll);
			}
		} else if (DbgEvent.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
			LogF(L"[UL ProcId=%lu, ThreadId=%lu]\tThe DLL at %p was unloaded.\r\n",
					DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.u.UnloadDll.lpBaseOfDll);
		} else if (DbgEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
			DWORD dwProcId = DbgEvent.dwProcessId;
			DWORD dwExitCode = DbgEvent.u.ExitProcess.dwExitCode;

			LogF(L"[EP ProcId=%lu]\t\tAttached process has exited (code %#010I32x).\r\n", dwExitCode, dwProcId);

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
			LogF(L"[KE ProcId=%lu, ThreadId=%lu]\tProcess has finished loading.\r\n",
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

			if (!g_KexData.IfeoParameters.dwWaitForChild) {
				VaWriteByte(vaNtRaiseHardError, btOriginalHardError);
				DebugSetProcessKillOnExit(FALSE);
				DebugActiveProcessStop(g_dwProcId);
				LogF(L"[KE] Detached from process.\r\n");
				return;
			}
		} else if (DbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&
				   (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT ||
				    DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_WX86_BREAKPOINT) &&
				   DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress == (PVOID) vaNtRaiseHardError) {
			CONTEXT Ctx;
			UINT i;
			HANDLE hProc;
			NTSTATUS st;
			WCHAR sz1[MAX_PATH];
			WCHAR sz2[MAX_PATH];
			DWORD dw;

			LogF(L"[HE ProcId=%lu, ThreadId=%lu]\tHard Error has been raised.\r\n", DbgEvent.dwProcessId, DbgEvent.dwThreadId);

			// Now we need to read the parameters of the Hard Error.
			// Here is a quick reference for the information we need to code this.
			// We don't need to bother with ValidResponseOptions and Response.
			// TODO: add 32 bit support.
			//
			// NTSTATUS NtRaiseHardError (						64 bit			32 bit
			//		IN NTSTATUS ErrorStatus,					RCX				[ESP]
			//		IN ULONG NumberOfParameters,				RDX				[ESP+4]
			//		IN ULONG UnicodeStringParameterMask,		R8				[ESP+8]
			//		IN PULONG_PTR Parameters,					R9				[ESP+12]
			//		IN ULONG ValidResponseOptions,
			//		OUT PULONG Response);
			//
			// The "Parameters" parameter is a pointer to an array of pointers to
			// UNICODE_STRING structures.

#ifdef _WIN64
			Ctx.ContextFlags = CONTEXT_INTEGER;
			GetThreadContext(g_hThread, &Ctx);
			st = (NTSTATUS) Ctx.Rcx;
			LogF(L"    Status code: 0x%I32x - %s", st, NtStatusAsString(st));

			if (Ctx.Rdx > 0) {
				for (i = 0; i < Ctx.Rdx; ++i) {
					if (Ctx.R8 & (QWORD) (1 << i)) {
						VaReadSzW(VaReadPtr(VaReadPtr(Ctx.R9 + i * sizeof(PVOID)) + FIELD_OFFSET(UNICODE_STRING, Buffer)),
								  i ? sz2 : sz1, MAX_PATH);
					} else {
						dw = VaReadDword(VaReadPtr(Ctx.R9));
					}
				}
			}
#endif

			VklHardError(st, dw, sz1, sz2);
			
			if ((hProc = OpenProcess(PROCESS_TERMINATE, FALSE, DbgEvent.dwProcessId))) {
				LogF(L"[KE] Terminating process.");
				TerminateProcess(hProc, 0);
				return;
			}
		} else if (DbgEvent.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
			HANDLE hProc = NULL;
			SIZE_T cbBuf = min(DbgEvent.u.DebugString.nDebugStringLength, 1024);
			LPSTR lpszBuf = (LPSTR) StackAlloc(cbBuf);
			
			if (DbgEvent.dwProcessId = g_dwProcId) {
				hProc = g_hProc;
			} else if ((hProc = OpenProcess(PROCESS_VM_READ, FALSE, DbgEvent.dwProcessId)) == NULL) {
				continue;
			}

			if (ReadProcessMemory(hProc, DbgEvent.u.DebugString.lpDebugStringData, lpszBuf, cbBuf, NULL) == FALSE) {
				continue;
			}

			if (DbgEvent.dwProcessId != g_dwProcId) {
				NtClose(hProc);
			}

			LogF(L"[DS ProcId=%lu, ThreadId=%lu]\t%hs\r\n", DbgEvent.dwProcessId, DbgEvent.dwThreadId, lpszBuf);

			// Don't know why, but calling this once here (and once again
			// at the beginning of the loop) is necessary to avoid debug
			// strings being printed twice. Very strange. Maybe I'm calling
			// it wrong or whatever.
			ContinueDebugEvent(DbgEvent.dwProcessId, DbgEvent.dwThreadId, DBG_CONTINUE);
		} else if (DbgEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
			LogF(L"[EX ProcId=%lu, ThreadId=%lu]\t%sxception occurred at 0x%p\r\n", DbgEvent.dwProcessId, DbgEvent.dwThreadId,
				 DbgEvent.u.Exception.dwFirstChance ? L"First-chance e"
													: (DbgEvent.u.Exception.ExceptionRecord.ExceptionFlags & EXCEPTION_NONCONTINUABLE ? L"Noncontinuable e"
																																	  : L"E"),
				 DbgEvent.u.Exception.ExceptionRecord.ExceptionAddress);
			// Exception codes are NTSTATUSes
			LogF(L"    Exception code: 0x%08x: %s", DbgEvent.u.Exception.ExceptionRecord.ExceptionCode,
														NtStatusAsString(DbgEvent.u.Exception.ExceptionRecord.ExceptionCode));

			if (DbgEvent.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION
				&& DbgEvent.u.Exception.ExceptionRecord.NumberParameters == 2) {
				LogF(L"    Attempted to %s the address 0x%p\r\n",
					 DbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[0] ? L"write" : L"read",
					 DbgEvent.u.Exception.ExceptionRecord.ExceptionInformation[1]);
			}
		} else if (DbgEvent.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
			LogF(L"[TH ProcId=%lu, ThreadId=%lu]\tThread created (starting address: 0x%p)\r\n",
				 DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.u.CreateThread.lpStartAddress);
		} else if (DbgEvent.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
			LogF(L"[TH ProcId=%lu, ThreadId=%lu]\tThread exited (exit code: 0x%08I32x)\r\n",
				 DbgEvent.dwProcessId, DbgEvent.dwThreadId, DbgEvent.u.ExitThread.dwExitCode);
		} else {
			LogF(L"[?? ProcId=%lu, ThreadId=%lu]\tUnknown debug event received: %lu\r\n",
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
	IN	LPCWSTR					szExeFullPath,
	OUT	LPKEX_IFEO_PARAMETERS	lpKexIfeoParameters)
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

HANDLE KexOpenLogFile(
	IN	LPCWSTR	szExeFullPath)
{
	HANDLE hLogFile;
	WCHAR szLogFileName[MAX_PATH] = L"";

	CHECKED(GetTempPath(ARRAYSIZE(szLogFileName), szLogFileName));
	CHECKED(SUCCEEDED(StringCchCat(szLogFileName, ARRAYSIZE(szLogFileName), L"VxKexLog\\")));

	CreateDirectory(szLogFileName, NULL);

	CHECKED(SUCCEEDED(StringCchCat(szLogFileName, ARRAYSIZE(szLogFileName), g_lpszExeBaseName)));
	CHECKED(PathRenameExtension(szLogFileName, L".log"));

	hLogFile = CreateFile(
		szLogFileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL);

	CHECKED(hLogFile != INVALID_HANDLE_VALUE);
	return hLogFile;

Error:
#ifdef _DEBUG
	WarningBoxF(L"Unable to open the log file %s: %s", szLogFileName, GetLastErrorAsString());
#endif // ifdef _DEBUG

	return NULL;
}

BOOL SpawnProgramUnderLoader(
	IN	LPWSTR	lpszCmdLine,
	IN	BOOL	bCalledFromDialog,
	IN	BOOL	bDialogDebugChecked,
	IN	BOOL	bCpiw,
	IN	BOOL	bForce)
{
	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", g_KexData.szKexDir, ARRAYSIZE(g_KexData.szKexDir))) {
		CriticalErrorBoxF(L"Could not find the VxKex installation directory. Please reinstall the software to fix this problem.");
	}
	
	GetExeFullPathFromCmdLine(lpszCmdLine, g_szExeFullPath);
	g_lpszExeBaseName = wcsrchr(g_szExeFullPath, '\\') + 1;

	if (bCpiw) {
		// parent process will pass its KexData through the lpReserved2 of STARTUPINFO
		PKEX_PROCESS_DATA pInheritedKexData = (PKEX_PROCESS_DATA) NtCurrentPeb()->ProcessParameters->RuntimeData.Buffer;
		DWORD cbInheritedKexData = NtCurrentPeb()->ProcessParameters->RuntimeData.Length;

		if (cbInheritedKexData == sizeof(KEX_PROCESS_DATA) && pInheritedKexData != NULL) {
			// the child process will inherit the parent's KexData
			CopyMemory(&g_KexData, pInheritedKexData, sizeof(g_KexData));

			// ugly hack pt. 3 (see kernelba33/process.c CreateProcessInternalW)
			g_KexData.IfeoParameters.dwEnableVxKex = TRUE;

			// except that we will NOT wait for this child process to exit, since
			// hooked CreateProcessInternalW depends on VxKexLdr exiting to pass
			// handles etc. back to the parent
			g_KexData.IfeoParameters.dwWaitForChild = FALSE;
		} else {
			CriticalErrorBoxF(L"StartupInfo contains no or invalid KexData");
		}
	} else {
		// if we weren't started from another kex-enabled process
		KexReadIfeoParameters(g_szExeFullPath, &g_KexData.IfeoParameters);
	}
	
	g_hLogFile = KexOpenLogFile(g_szExeFullPath);

	if (!bCalledFromDialog && !bForce && !bCpiw && !g_KexData.IfeoParameters.dwEnableVxKex) {
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

	LogF(L"Process image full path: %s\r\n", g_szExeFullPath);
	LogF(L"Process command line: %s\r\n", lpszCmdLine);
	LogF(L"The process is %d-bit and its base address is %p (PEB base address: %p)\r\n",
			g_bExe64 ? 64 : 32, g_vaExeBase, g_vaPebBase);

	LogF(L"[KE] Copying KexData structure to child process\r\n");
	CopyKexDataToChild();

	LogF(L"[KE] Rewriting imports of executable file\r\n");
	RewriteImports(g_lpszExeBaseName, g_vaExeBase);
	RewriteDllImports();

	return TRUE;
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
			SpawnProgramUnderLoader(szFilename, TRUE, bShouldShowDebugInfo, FALSE, TRUE);

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

// There are 3 things we need to get out of the parent process and
// put into this process before terminating it:
//
// Peb->ProcessParameters->CommandLine (lpCmdLine)
// Peb->ProcessParameters->ShellInfo (STARTUPINFO lpReserved)
// Peb->ProcessParameters->RuntimeData (STARTUPINFO lpReserved2 and cbReserved2)
//
// The usually tedious task of reading PEB values out of another process is simplified
// somewhat because we can clobber as many global variables as we want and also assume
// that the parent process is the same architecture as us. So we can use FIELD_OFFSET
// macro instead of hard coding offsets.
VOID VklFishParameters(
	VOID)
{
	HANDLE hParent;
	ULONG_PTR vaPebBase;
	ULONG_PTR vaProcessParameters;
	ULONG_PTR vaCommandLine;
	ULONG_PTR vaShellInfo;
	ULONG_PTR vaRuntimeData;

	PVOID CommandLineBuffer;
	PVOID ShellInfoBuffer;
	PVOID RuntimeDataBuffer;

	CHECKED(hParent = OpenParentProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ));

	GetProcessBaseAddressAndPebBaseAddress(hParent, NULL, &vaPebBase);
	vaProcessParameters = vaPebBase + FIELD_OFFSET(PEB, ProcessParameters);
	CHECKED(ReadProcessMemory(hParent, (LPCVOID) vaProcessParameters, &vaProcessParameters, sizeof(vaProcessParameters), NULL));
	vaCommandLine = vaProcessParameters + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, CommandLine);
	vaShellInfo = vaProcessParameters + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, ShellInfo);
	vaRuntimeData = vaProcessParameters + FIELD_OFFSET(RTL_USER_PROCESS_PARAMETERS, RuntimeData);

	CHECKED(ReadProcessMemory(hParent, (LPCVOID) vaCommandLine, &NtCurrentPeb()->ProcessParameters->CommandLine, sizeof(UNICODE_STRING), NULL));
	CHECKED(ReadProcessMemory(hParent, (LPCVOID) vaShellInfo, &NtCurrentPeb()->ProcessParameters->ShellInfo, sizeof(UNICODE_STRING), NULL));
	CHECKED(ReadProcessMemory(hParent, (LPCVOID) vaRuntimeData, &NtCurrentPeb()->ProcessParameters->RuntimeData, sizeof(UNICODE_STRING), NULL));

	CHECKED(CommandLineBuffer = DefHeapAlloc(NtCurrentPeb()->ProcessParameters->CommandLine.Length));
	CHECKED(ShellInfoBuffer = DefHeapAlloc(NtCurrentPeb()->ProcessParameters->ShellInfo.Length));
	CHECKED(RuntimeDataBuffer = DefHeapAlloc(NtCurrentPeb()->ProcessParameters->RuntimeData.Length));

	CHECKED(ReadProcessMemory(hParent, NtCurrentPeb()->ProcessParameters->CommandLine.Buffer, CommandLineBuffer, NtCurrentPeb()->ProcessParameters->CommandLine.Length, NULL));
	CHECKED(ReadProcessMemory(hParent, NtCurrentPeb()->ProcessParameters->ShellInfo.Buffer, ShellInfoBuffer, NtCurrentPeb()->ProcessParameters->ShellInfo.Length, NULL));
	CHECKED(ReadProcessMemory(hParent, NtCurrentPeb()->ProcessParameters->RuntimeData.Buffer, RuntimeDataBuffer, NtCurrentPeb()->ProcessParameters->RuntimeData.Length, NULL));

	NtCurrentPeb()->ProcessParameters->CommandLine.Buffer = (PWSTR) CommandLineBuffer;
	NtCurrentPeb()->ProcessParameters->ShellInfo.Buffer = (PWSTR) ShellInfoBuffer;
	NtCurrentPeb()->ProcessParameters->RuntimeData.Buffer = (PWSTR) RuntimeDataBuffer;

	NtTerminateProcess(hParent, 0);
	NtClose(hParent);

	return;
Error:
	CriticalErrorBoxF(L"Failed to retrieve parameters from parent process: %s", GetLastErrorAsString());
}

#define FORCE_FLAG L"/FORCE "
#define CPIW_FLAG L"/CPIW "
#define FISH_FLAG L"/FISH"

NORETURN VOID EntryPoint(
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
		CONST SIZE_T cchCpiwFlag = wcslen(CPIW_FLAG);
		CONST SIZE_T cchFishFlag = wcslen(FISH_FLAG);
		BOOL bCpiw = FALSE;
		BOOL bForce = FALSE;

		if (!wcsnicmp(lpszCmdLine, FISH_FLAG, cchFishFlag)) {
			// do proper initialization of variables and then do a soft re-exec
			VklFishParameters();
			EntryPoint();
		}
		
		if (!wcsnicmp(lpszCmdLine, CPIW_FLAG, cchCpiwFlag)) {
			bCpiw = TRUE;
			lpszCmdLine += cchCpiwFlag;
		}

		if (!wcsnicmp(lpszCmdLine, FORCE_FLAG, cchForceFlag)) {
			// This flag is used to bypass the usual checking that we're "supposed" to launch
			// the program with VxKex enabled. For example, the shell extended context menu entry
			// will use this flag, to let the user try out VxKex without having to go into the
			// property sheet and (possibly) accept the UAC dialog etc etc.
			bForce = TRUE;
			lpszCmdLine += cchForceFlag;
		}

		SpawnProgramUnderLoader(lpszCmdLine, FALSE, FALSE, bCpiw, bForce);

		if (bCpiw) {
			HANDLE hParent = OpenParentProcess(PROCESS_DUP_HANDLE);
			LPPROCESS_INFORMATION lpProcInfo = (LPPROCESS_INFORMATION) DefHeapAlloc(sizeof(PROCESS_INFORMATION));

#ifdef _M_X64
			if ((ULONG_PTR) lpProcInfo > 0xFFFFFFFF) {
				CriticalErrorBoxF(L"Pointer to process information is too large.");
			}
#endif

			lpProcInfo->dwProcessId = g_dwProcId;
			lpProcInfo->dwThreadId = g_dwThreadId;
			DuplicateHandle(GetCurrentProcess(), g_hProc, hParent, &lpProcInfo->hProcess,
							0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
			DuplicateHandle(GetCurrentProcess(), g_hThread, hParent, &lpProcInfo->hThread,
							0, FALSE, DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS);
			NtClose(hParent);

			// deliberately do NOT free lpProcInfo. The parent process needs to read it.
			Exit((DWORD) lpProcInfo);
		}
	}

	Exit(0);
}