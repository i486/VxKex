#include <NtDll.h>
#include "VxKexLdr.h"

#include <ShlObj.h>

//
// Other process-related utility functions which do not belong in psinfo.c
//

VOID CreateSuspendedProcess(
	IN	LPWSTR	lpszCmdLine)	// Full command-line of the process incl. executable name
{
	BOOL bSuccess;
	DWORD dwCpError;
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcInfo;

	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.lpReserved = NULL;
	StartupInfo.cbReserved2 = 0;
	StartupInfo.lpReserved2 = NULL;
	StartupInfo.lpTitle = NULL;
	
	// attach to parent console (if any) to allow child process to inherit it
	PreScanExeFile();

	bSuccess = CreateProcess(
		NULL,
		lpszCmdLine,
		NULL,
		NULL,
		TRUE,
		CREATE_SUSPENDED | DEBUG_ONLY_THIS_PROCESS,
		NULL,
		NULL,
		&StartupInfo,
		&ProcInfo);
	dwCpError = RtlGetLastWin32Error();

	PatchKernel32ImageVersionCheck(FALSE);

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