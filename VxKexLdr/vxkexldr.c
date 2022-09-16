#include <NtDll.h>
#include "VxKexLdr.h"
#include "resource.h"

#include <Shlwapi.h>
#include <ShlObj.h>

#define FORCE_FLAG L"/FORCE "

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

BOOL SpawnProgramUnderLoader(
	IN	LPWSTR	lpszCmdLine,
	IN	BOOL	bCalledFromDialog,
	IN	BOOL	bDialogDebugChecked,
	IN	BOOL	bForce)
{
	// set up KexData (miscellaneous fields)
	g_KexData.ProcessInitializationComplete = FALSE;

	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", g_KexData.szKexDir, ARRAYSIZE(g_KexData.szKexDir))) {
		CriticalErrorBoxF(L"Could not find the VxKex installation directory. Please reinstall the software to fix this problem.");
	}
	
	if (!GetExeFullPathFromCmdLine(lpszCmdLine, g_szExeFullPath, MAX_PATH, &g_lpszExeBaseName)) {
		CriticalErrorBoxF(L"Could not identify the full path of the executable given by the command line:\r\n"
						  L"%s\r\n%#010I32x: %s", GetCommandLineWithoutImageName(), GetLastError(), GetLastErrorAsString());
	}

	KexReadIfeoParameters(g_szExeFullPath, &g_KexData.IfeoParameters);
	
	g_hLogFile = KexOpenLogFile(g_lpszExeBaseName);

	if (!bCalledFromDialog && !bForce && !g_KexData.IfeoParameters.dwEnableVxKex) {
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
		unless (g_KexData.IfeoParameters.DisableAppSpecific) {
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

// Monitors the child process for DLL imports.
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



#ifdef _M_X64
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

	// set ProcessInitializationComplete to TRUE
	g_KexData.ProcessInitializationComplete = TRUE;
	UpdateKexDataInChild();

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