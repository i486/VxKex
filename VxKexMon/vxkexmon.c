#include <KexComm.h>
#include <KexData.h>
#include <NtDll.h>
#include "VxKexMon.h"

#include <Shlwapi.h>

//
// - VxKex Monitor Application
//
// The VxKex Monitor is an application which monitors the initialization of a remote
// process and controls DLL rewriting and other VxKex functions for the duration of
// process initialization. The Monitor is designed exclusively to be started by the
// CreateProcessInternalW hook contained within KERNELBA33.DLL.

// A single parameter is passed by the CPIW hook - the process ID of the suspended
// process to monitor, as a zero-padded uppercase hexadecimal with 8 digits and no
// preceding "0x". The process which is passed as a parameter must be suspended and
// newly created. After the monitor exits the process will be running. (If successful,
// of course.)
//
// The Monitor takes on the role that VxKexLdr (loader) used to have. However, because
// of severe constraints to application compatibility that the loader-approach brought
// about, the architecture is now changed. More VxKex functionality is moved into the
// base DLL (KERNELBA33.DLL) and the dependence on the VxKex loader is reduced.
//
// The monitor will attach to the process as a debugger, initialize VxKex structures
// and set up the correct PATH directories, monitor DLL loads, rewrite them as
// necessary, and then detach and exit as soon as process initialization is complete.
// All other former functions of VxKexLdr are the responsibility of the CPIW hook,
// except for those related to displaying UI and acting as an IFEO "debugger".
//
// - Why not use process initialization code injected into the process itself?
//
// This would work for the most part, but displaying error UI becomes difficult. The
// only function which displays UI before user32.dll is initialized is NtRaiseHardError.
// This function is not flexible and often results in very unclear error messages which
// have a great potential for confusing the user. Furthermore it does not allow for
// easy and simple bug reports, increasing the chance that the user will just ignore
// the problem and/or uninstall VxKex.
//
// Additional Notes:
//   - There are two VxKexMon executables in a 64-bit installation. These are
//     KexMon32.exe and KexMon64.exe for 32-bit and 64-bit target processes,
//     respectively. VxKexMon and target process bitness MUST be matched.
//

VOID EntryPoint(
	VOID)
{
	PCWSTR CommandLine;
	ULONG TargetProcessId;
	BOOLEAN KexDllDirIs64Bit;

	SetFriendlyAppName(L"VxKex Monitor");
	CommandLine = GetCommandLineWithoutImageName();

	// Parameter parsing/validation
	if (!ReadValidateCommandLine(CommandLine, &TargetProcessId, &KexDllDirIs64Bit)) {
		IncorrectUsageError();
	}

	MonitorRewriteImageImports(TargetProcessId, KexDllDirIs64Bit);
	ExitProcess(STATUS_SUCCESS);
}

BOOLEAN MonitorRewriteImageImports(
	IN	ULONG		TargetProcessId,
	IN	BOOLEAN		KexDllDirIs64Bit)
{
	BOOL Success;
	DWORD LastError;
	DEBUG_EVENT Event;
	KEX_PROCESS_DATA KexData;

	ULONG_PTR VaExeImageBase;				// virtual address of the start of the .exe file used to create the process
	ULONG_PTR VaEntryPoint;
	ULONG_PTR VaNtRaiseHardError;
	
	BYTE EntryPointOriginal;
	BYTE NtRaiseHardErrorOriginal;
	
	HANDLE ProcessHandle;
	HANDLE ThreadHandle;					// handle to the first thread in the process (main thread)
	
	WCHAR ImageFullName[MAX_PATH];
	PCWSTR ImageBaseName;

	Success = DebugActiveProcess(TargetProcessId);
	if (!Success) {
		return FALSE;
	}

	while (TRUE) {
		DWORD EventCode;
		DWORD ProcessId;
		DWORD ThreadId;

		CHECKED(WaitForDebugEvent(&Event, INFINITE));
		EventCode = Event.dwDebugEventCode;
		ProcessId = Event.dwProcessId;
		ThreadId = Event.dwThreadId;
		
		switch (EventCode) {
		case CREATE_PROCESS_DEBUG_EVENT: {
			CREATE_PROCESS_DEBUG_INFO *DebugInfo = &Event.u.CreateProcessInfo;

			//
			// Get full path of the executable file.
			//
			CHECKED(GetFinalPathNameByHandle(DebugInfo->hFile, ImageFullName, ARRAYSIZE(ImageFullName), 0));
			CloseHandle(DebugInfo->hFile);

			ImageBaseName		= PathFindFileName(ImageFullName);
			ProcessHandle		= DebugInfo->hProcess;
			ThreadHandle		= DebugInfo->hThread;
			VaExeImageBase		= (ULONG_PTR) DebugInfo->lpBaseOfImage;
			VaEntryPoint		= GetRemoteImageEntryPoint(ProcessHandle, VaExeImageBase);
			VaNtRaiseHardError	= (ULONG_PTR) GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtRaiseHardError");

			//
			// Set breakpoints on EXE entry point and hard error function. Exceptions will be raised
			// (and handled by the exception handler further below) when execution is transferred
			// to the relevant functions.
			//
			CHECKED(VaRead(ProcessHandle, VaEntryPoint, &EntryPointOriginal, 1));
			CHECKED(VaRead(ProcessHandle, VaNtRaiseHardError, &NtRaiseHardErrorOriginal, 1));
			CHECKED(VaWrite1(ProcessHandle, VaEntryPoint, 0xCC));
			CHECKED(VaWrite1(ProcessHandle, VaNtRaiseHardError, 0xCC));

			//
			// Initialize child's KexData and %Path%
			//
			CHECKED(AdjustRemoteKexDllDir(ProcessHandle, KexDllDirIs64Bit ? 64 : 32, PROCESSBITS));
			CHECKED(InitializeRemoteKexData(ProcessHandle, ImageFullName, &KexData));
		} break;
		case CREATE_THREAD_DEBUG_EVENT: {
			CREATE_THREAD_DEBUG_INFO *DebugInfo = &Event.u.CreateThread;
		} break;
		case EXIT_PROCESS_DEBUG_EVENT: {
			EXIT_PROCESS_DEBUG_INFO *DebugInfo = &Event.u.ExitProcess;
		} break;
		case EXIT_THREAD_DEBUG_EVENT: {
			EXIT_THREAD_DEBUG_INFO *DebugInfo = &Event.u.ExitThread;
		} break;
		case LOAD_DLL_DEBUG_EVENT: {
			LOAD_DLL_DEBUG_INFO *DebugInfo = &Event.u.LoadDll;
			WCHAR DllFullName[MAX_PATH];
			PCWSTR DllBaseName;

			CHECKED(GetFinalPathNameByHandle(DebugInfo->hFile, ImageFullName, ARRAYSIZE(ImageFullName), 0));
			DllBaseName = PathFindFileName(DllFullName);

			if (ShouldRewriteImportsOfDll(DllFullName)) {
				RewriteImageImportDirectory(ProcessHandle, (ULONG_PTR) DebugInfo->lpBaseOfDll, DllBaseName);
			}
		} break;
		case UNLOAD_DLL_DEBUG_EVENT: {
			UNLOAD_DLL_DEBUG_INFO *DebugInfo = &Event.u.UnloadDll;
		} break;
		case EXCEPTION_DEBUG_EVENT: {
			EXCEPTION_DEBUG_INFO *DebugInfo = &Event.u.Exception;
			PEXCEPTION_RECORD ExceptionRecord = &DebugInfo->ExceptionRecord;

			if (ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
				if ((ULONG_PTR) ExceptionRecord->ExceptionAddress == VaEntryPoint) {
					// Unpatch the entry point & hard error, continue the process and detach
					CHECKED(VaWrite1(ProcessHandle, VaEntryPoint, EntryPointOriginal));
					CHECKED(VaWrite1(ProcessHandle, VaNtRaiseHardError, NtRaiseHardErrorOriginal));
					CHECKED(ContinueDebugEvent(ProcessId, ThreadId, DBG_CONTINUE));
					CHECKED(DebugActiveProcessStop(ProcessId));
					return TRUE;
				}
				
				if ((ULONG_PTR) ExceptionRecord->ExceptionAddress == VaNtRaiseHardError) {
					VkmHardError(ImageBaseName, ProcessHandle, ThreadHandle);
					SetLastError(ERROR_PROCESS_ABORTED);
					return FALSE;
				}
			}
		} break;
		case OUTPUT_DEBUG_STRING_EVENT: {
			OUTPUT_DEBUG_STRING_INFO *DebugInfo = &Event.u.DebugString;
		} break;
		case RIP_EVENT: {
			RIP_INFO *DebugInfo = &Event.u.RipInfo;
		} break;
		}
		
		CHECKED(ContinueDebugEvent(ProcessId, ThreadId, DBG_EXCEPTION_NOT_HANDLED));
	}

Error:
	LastError = GetLastError();
	DebugActiveProcessStop(TargetProcessId);
	TerminateProcess(ProcessHandle, 0);
	SetLastError(LastError);
	return FALSE;
}

BOOLEAN ShouldRewriteImportsOfDll(
	IN	PCWSTR		DllFullName)
{
}

// Rewrite the import directory of a single PE image in a given process.
BOOLEAN RewriteImageImportDirectory(
	IN	HANDLE		Process,
	IN	ULONG_PTR	ImageBase,
	IN	PCWSTR		ImageBaseName OPTIONAL)
{
}