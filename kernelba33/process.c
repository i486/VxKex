#include <KexComm.h>
#include <KexData.h>
#include <KexDll.h>
#include <BaseDll.h>
#include <NtDll.h>
#include <VxKexMon\VkmConstants.h>
#include "process.h"

#include <Shlwapi.h>

// I'm pretty sure it's important to have DECLSPEC_IMPORT here.
// So don't remove it.
DECLSPEC_IMPORT BOOL WINAPI CreateProcessInternalW(
	IN	HANDLE					hUserToken OPTIONAL,
	IN	LPCWSTR					lpszApplicationName OPTIONAL,
	IN	LPWSTR					lpszCommandLine OPTIONAL,
	IN	LPSECURITY_ATTRIBUTES	lpProcessAttributes OPTIONAL,
	IN	LPSECURITY_ATTRIBUTES	lpThreadAttributes OPTIONAL,
	IN	BOOL					bInheritHandles,
	IN	DWORD					dwCreationFlags,
	IN	LPVOID					lpEnvironment OPTIONAL,
	IN	LPCWSTR					lpszCurrentDirectory OPTIONAL,
	IN	LPSTARTUPINFOW			lpStartupInfo,
	OUT	LPPROCESS_INFORMATION	lpProcessInformation,
	OUT	PHANDLE					phRestrictedUserToken);


BYTE CpiwHook[] = {
#ifdef _M_X64
#  define CPIW_HOOKPROC_PTR_OFFSET (6)
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// JMP [FuncPtr]
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC	// FuncPtr: DQ 0xCCCCCCCCCCCCCCCC
#else
#  define CPIW_HOOKPROC_PTR_OFFSET (1)
	0x68, 0xCC, 0xCC, 0xCC, 0xCC,					// PUSH 0xCCCCCCCC
	0xC3,											// RET
		  0x00, 0x00, 0x00, 0x00					// (padding)
#endif
};

BYTE CpiwTrampoline[] = {
#ifdef _M_X64
#  define CPIW_CONTINUATION_PTR_OFFSET (20)
	0x00, 0x00, 0x00,								// MOV R11, RSP
	0x00,											// PUSH RBX
	0x00,											// PUSH RSI
	0x00,											// PUSH RDI
	0x00, 0x00,										// PUSH R12
	0x00, 0x00,										// PUSH R13
	0x00, 0x00,										// PUSH R14
	0x00, 0x00,										// PUSH R15
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// JMP [FuncPtr]
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC	// FuncPtr: DQ 0xCCCCCCCCCCCCCCCC
#else
#  define CPIW_CONTINUATION_PTR_OFFSET (11)
	0x00, 0x00, 0x00, 0x00, 0x00,					// PUSH imm32
	0x00, 0x00, 0x00, 0x00, 0x00,					// PUSH imm32
	0x68, 0xCC, 0xCC, 0xCC, 0xCC,					// PUSH 0xCCCCCCCC
	0xC3											// RET
#endif
};

STATIC BOOL (WINAPI *CreateProcessInternalWOriginal)(
	HANDLE, PCWSTR, PWSTR, PSECURITY_ATTRIBUTES, PSECURITY_ATTRIBUTES, BOOL, DWORD,
	PVOID, PCWSTR, LPSTARTUPINFOW, PPROCESS_INFORMATION, PHANDLE) = NULL;

//
// Return TRUE if the PATH environment variable in the specified environment block
// contains any KexDllDir (Kex32 or Kex64). If the Environment parameter is NULL,
// use the current process environment.
//
STATIC BOOL PathContainsKexDllDir(
	IN	PVOID		Environment OPTIONAL,
	OUT	PBOOLEAN	KexDllDirIs64Bit OPTIONAL)
{
	HRESULT Result;
	ULONG PathLength;
	PWSTR PathValue;
	PWSTR MatchingSubString = NULL;
	SIZE_T KexDllDirLength;
	WCHAR KexDllDir32[MAX_PATH];
	WCHAR KexDllDir64[MAX_PATH];

	if (KexDllDirIs64Bit) {
		*KexDllDirIs64Bit = FALSE;
	}

	PathLength = GetEnvironmentVariableExW(L"Path", NULL, 0, Environment);
	
	if (PathLength == 0) {
		return FALSE;
	}

	PathValue = (PWSTR) StackAlloc(PathLength * sizeof(WCHAR));
	PathLength = GetEnvironmentVariableExW(L"Path", PathValue, PathLength, Environment);

	if (PathLength == 0) {
		return FALSE;
	}

	//
	// PathValue now contains the value of the "Path" environment variable.
	// We'll see whether it contains a KexDllDir. First, assemble KexDllDir32/64
	// strings so we can search for them.
	//

	Result = StringCchPrintf(KexDllDir64, ARRAYSIZE(KexDllDir64), L"%s\\Kex64", KexData->KexDir);
	if (FAILED(Result)) {
		SetLastError(WIN32_FROM_HRESULT(Result));
		return FALSE;
	}

	Result = StringCchLength(KexDllDir64, ARRAYSIZE(KexDllDir64), &KexDllDirLength);
	if (FAILED(Result)) {
		SetLastError(WIN32_FROM_HRESULT(Result));
		return FALSE;
	}

	MatchingSubString = StrStrIW(PathValue, KexDllDir64);

	if (!MatchingSubString) {
		Result = StringCchPrintf(KexDllDir32, ARRAYSIZE(KexDllDir32), L"%s\\Kex32", KexData->KexDir);
		if (FAILED(Result)) {
			SetLastError(WIN32_FROM_HRESULT(Result));
			return FALSE;
		}

		MatchingSubString = StrStrI(PathValue, KexDllDir32);
		if (!MatchingSubString) {
			return FALSE;
		}
	} else {
		*KexDllDirIs64Bit = TRUE;
	}

	//
	// Now that we have determined there is a KexDllDir somewhere in PATH, we must
	// ensure that there is a ';' character before and after it.
	//

	if (MatchingSubString != PathValue) {
		if (*(MatchingSubString - 1) != ';') {
			SetLastError(ERROR_SUCCESS);
			*KexDllDirIs64Bit = FALSE;
			return FALSE;
		}
	}

	MatchingSubString += KexDllDirLength;

	if (*MatchingSubString != ';' && *MatchingSubString != '\0') {
		SetLastError(ERROR_SUCCESS);
		*KexDllDirIs64Bit = FALSE;
		return FALSE;
	}

	return TRUE;
}

// The process creation hook procedure has the following responsibilities:
// ...TODO
WINBASEAPI BOOL WINAPI HOOK_FUNCTION(CreateProcessInternalW) (
	IN	HANDLE					UserToken OPTIONAL,
	IN	LPCWSTR					ApplicationName OPTIONAL,
	IN	LPWSTR					CommandLine OPTIONAL,
	IN	LPSECURITY_ATTRIBUTES	ProcessAttributes OPTIONAL,
	IN	LPSECURITY_ATTRIBUTES	ThreadAttributes OPTIONAL,
	IN	BOOL					InheritHandles,
	IN	DWORD					CreationFlags,
	IN	LPVOID					Environment OPTIONAL,
	IN	LPCWSTR					CurrentDirectory OPTIONAL,
	IN	LPSTARTUPINFOW			StartupInfo,
	OUT	LPPROCESS_INFORMATION	ProcessInformation,
	OUT	PHANDLE					RestrictedUserToken OPTIONAL)
{
	BOOLEAN KexDllDirIs64Bit;
	BOOLEAN ChildProcessIs64Bit;
	BOOLEAN MustFreeEnvironment = FALSE;
	BOOLEAN ConvertedEnvironment = FALSE;
	BOOLEAN CpiwSuccess;
	HANDLE ProcessHandle;
	ULONG ProcessId;
	NTSTATUS Status;
	HRESULT Result;
	WCHAR VkmFullPath[MAX_PATH];
	WCHAR VkmCommandLine[1 + MAX_PATH + 1 + 1 + VKM_ARGUMENT_LENGTH]; // Two quotes and a space
	STARTUPINFO VkmStartupInfo;
	PROCESS_INFORMATION VkmProcessInformation;

	ODS_ENTRY(L"(%p, L\"%ws\", L\"%ws\", %p, %p, %d, 0x%08x, %p, L\"%ws\", %p, %p, %p)",
			  UserToken, ApplicationName, CommandLine, ProcessAttributes,
			  ThreadAttributes, InheritHandles, CreationFlags, Environment,
			  CurrentDirectory, StartupInfo, ProcessInformation, RestrictedUserToken);

	//
	// Note that, for all errors that occur in this code, you should log the error
	// and then jump to the original CPIW code. Do not return an error to the caller
	// under any circumstances just because we failed to launch a child process under
	// VxKex, unless the failure occurred after the first modified call to CPIW.
	//

	if (KexData->IfeoParameters.DisableForChild) {
		goto DoNotUseVxKex;
	}

	if (CreationFlags & (CREATE_SUSPENDED | DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS |
						 CREATE_PROTECTED_PROCESS | CREATE_SEPARATE_WOW_VDM |
						 CREATE_SHARED_WOW_VDM)) {
		//
		// Don't try and do any of this stuff if the caller wants to specify any
		// "weird" flags for the child process. It's likely to cause big problems
		// and errors.
		//
		goto DoNotUseVxKex;
	}

	//
	// Convert environment to Unicode, if it's an ANSI environment block.
	// This code is adapted from base\win32\client\process.c:2265
	//

	if (Environment && !(CreationFlags & CREATE_UNICODE_ENVIRONMENT)) {
		PCHAR s;
		STRING Ansi;
		UNICODE_STRING Unicode;
		SIZE_T AllocationSize;

		Ansi.Buffer = s = (PCHAR) Environment;
		while (*s || *(s+1)) { // find end of block
			++s;
		}

		Ansi.Length = (USHORT) (s - Ansi.Buffer) + 1;
		Ansi.MaximumLength = Ansi.Length + 1;
		Unicode.MaximumLength = Ansi.MaximumLength * sizeof(WCHAR);
		AllocationSize = Unicode.MaximumLength;
	
		Status = NtAllocateVirtualMemory(
			NtCurrentProcess(),
			(PVOID *) &Unicode.Buffer,
			0,
			&AllocationSize,
			MEM_COMMIT,
			PAGE_READWRITE);

		if (!NT_SUCCESS(Status)) {
			goto DoNotUseVxKex;
		}

		Unicode.MaximumLength = (USHORT) AllocationSize;

		Status = RtlAnsiStringToUnicodeString(&Unicode, &Ansi, FALSE);
		if (!NT_SUCCESS(Status)) {
			AllocationSize = 0;
			NtFreeVirtualMemory(
				NtCurrentProcess(),
				(PVOID *) &Unicode.Buffer,
				&AllocationSize,
				MEM_RELEASE);

			goto DoNotUseVxKex;
		}

		ConvertedEnvironment = TRUE;
	}

	//
	// Set up KexDLL directory in PATH env var
	//

	if (!PathContainsKexDllDir(Environment, &KexDllDirIs64Bit)) {
		//
		// If the caller supplied an environment block which doesn't contain a
		// Path environment variable which contains a KexDllDir, we need to make
		// a copy of the environment block and add a KexDllDir to the Path variable
		// inside that copied environment block (or add a Path variable if it
		// doesn't already exist).
		//
		WCHAR KexDllDir[MAX_PATH];
		PWSTR Path;
		ULONG PathLength;
		ULONG PathBufferCch;
		SIZE_T KexDllDirLength;
		
		// Clone environment block if it wasn't already cloned earlier
		if (!MustFreeEnvironment) {
			if (!CloneEnvironmentBlock(Environment, &Environment)) {
				goto DoNotUseVxKex;
			}

			MustFreeEnvironment = TRUE;
		}

		Result = StringCchPrintf(KexDllDir, ARRAYSIZE(KexDllDir), L"%s\\Kex%d", KexData->KexDir, PROCESSBITS);
		if (FAILED(Result)) {
			goto DoNotUseVxKex;
		}

		KexDllDirIs64Bit = PROCESS_IS_64BIT ? TRUE : FALSE;

		//
		// Get existing Path variable, if it exists. Otherwise allocate one.
		//

		PathLength = GetEnvironmentVariableExW(L"Path", NULL, 0, Environment);
		if (PathLength == 0 && GetLastError() != ERROR_ENVVAR_NOT_FOUND) {
			goto DoNotUseVxKex;
		}

		// calculate length of the KexDllDir
		Result = StringCchLength(KexDllDir, ARRAYSIZE(KexDllDir), &KexDllDirLength);
		if (FAILED(Result)) {
			goto DoNotUseVxKex;
		}
		
		// add KexDllDir length to allocation so we can tack that onto the end,
		// in addition to a ';' before and '\0' after
		PathBufferCch = PathLength + (ULONG) KexDllDirLength + 2;
		Path = (PWSTR) StackAlloc(PathBufferCch * sizeof(WCHAR));

		PathLength = GetEnvironmentVariableExW(L"Path", Path, PathLength, Environment);
		if (PathLength == 0) {
			goto DoNotUseVxKex;
		}

		//
		// Append KexDllDir to Path
		//

		Result = StringCchPrintf(Path, PathBufferCch, L"%s;%s", Path, KexDllDir);
		if (FAILED(Result)) {
			goto DoNotUseVxKex;
		}

		if (!SetEnvironmentVariableExW(L"Path", Path, &Environment)) {
			goto DoNotUseVxKex;
		}
	}

	//
	// Now, without further ado, let's get this process started.
	//

	if (ConvertedEnvironment) {
		CreationFlags |= CREATE_UNICODE_ENVIRONMENT;
	}

	CreationFlags |= CREATE_SUSPENDED;

	CpiwSuccess = CreateProcessInternalWOriginal(
		UserToken,
		ApplicationName,
		CommandLine,
		ProcessAttributes,
		ThreadAttributes,
		InheritHandles,
		CreationFlags,
		Environment,
		CurrentDirectory,
		StartupInfo,
		ProcessInformation,
		RestrictedUserToken);

	if (!CpiwSuccess) {
		goto DoNotUseVxKex;
	}

	if (MustFreeEnvironment) {
		RtlDestroyEnvironment(Environment);
		MustFreeEnvironment = FALSE;
	}

	ProcessHandle = ProcessInformation->hProcess;
	ProcessId = ProcessInformation->dwProcessId;

	//
	// Past this point, any errors should goto PostProcessCreationFailure. This
	// terminates the child process and then retries without using VxKex.
	//

	//
	// Now we have done as much as we can from here. We need to start the VxKexMon
	// executable (of the correct bitness) and tell it to work on the child process.
	// VxKexMon needs to know the process ID of the child process and the bitness of
	// the KexDllDir that we put inside the child process.
	//
	// We also need to start the correct bitness of monitor (KexMon32.exe/KexMon64.exe)
	// based on the bitness of the child process.
	//

	if (!IsProcess64Bit(ProcessHandle, &ChildProcessIs64Bit)) {
		goto PostProcessCreationFailure;
	}

	Result = StringCchPrintf(VkmFullPath, ARRAYSIZE(VkmFullPath),
							 L"%s\\KexMon%d.exe", KexData->KexDir,
							 ChildProcessIs64Bit ? 64 : 32);
	if (FAILED(Result)) {
		goto PostProcessCreationFailure;
	}

	Result = StringCchPrintf(VkmCommandLine, ARRAYSIZE(VkmCommandLine),
							 VKM_CMDLINE_FORMAT_STRING,
							 VkmFullPath, ProcessId, KexDllDirIs64Bit);
	if (FAILED(Result)) {
		goto PostProcessCreationFailure;
	}

	// Inherit StartupInfo from the current process except for certain problematic values
	// in dwFlags, etc. which we don't want.
	GetStartupInfo(&VkmStartupInfo);
	VkmStartupInfo.lpReserved		= NULL;
	VkmStartupInfo.lpTitle			= NULL;
	VkmStartupInfo.dwFlags			= 0;
	VkmStartupInfo.cbReserved2		= 0;
	VkmStartupInfo.lpReserved2		= NULL;

	CpiwSuccess = CreateProcessInternalWOriginal(
		NULL,
		VkmFullPath,
		VkmCommandLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&VkmStartupInfo,
		&VkmProcessInformation,
		NULL);

	if (!CpiwSuccess) {
		goto PostProcessCreationFailure;
	}

	CloseHandle(VkmProcessInformation.hProcess);
	CloseHandle(VkmProcessInformation.hThread);

	return TRUE;

PostProcessCreationFailure:
	TerminateProcess(ProcessHandle, 0);

DoNotUseVxKex:
	if (MustFreeEnvironment) {
		RtlDestroyEnvironment(Environment);
		MustFreeEnvironment = FALSE;
	}

	return CreateProcessInternalWOriginal(
		UserToken,
		ApplicationName,
		CommandLine,
		ProcessAttributes,
		ThreadAttributes,
		InheritHandles,
		CreationFlags,
		Environment,
		CurrentDirectory,
		StartupInfo,
		ProcessInformation,
		RestrictedUserToken);
}

VOID DllMain_HookCreateProcess(
	VOID)
{
	ULONG OldMemoryProtection;

	ODS_ENTRY();

	if (KexData->IfeoParameters.DisableForChild) {
		// do nothing
		return;
	}

	// Fill out function pointer in hook code
	*((PULONG_PTR) (CpiwHook+CPIW_HOOKPROC_PTR_OFFSET)) = (ULONG_PTR) &HOOK_FUNCTION(CreateProcessInternalW);

	// Read original bytes from CreateProcessInternalW into the trampoline
	CopyMemory(CpiwTrampoline, CreateProcessInternalW, sizeof(CpiwHook));

	// Fill out jump address in trampoline
	*((PULONG_PTR) (CpiwTrampoline+CPIW_CONTINUATION_PTR_OFFSET)) = (ULONG_PTR) CreateProcessInternalW + sizeof(CpiwHook);

	// Allocate some executable memory for the trampoline
	*((PPVOID) &CreateProcessInternalWOriginal) = VirtualAlloc(NULL, sizeof(CpiwTrampoline), MEM_COMMIT, PAGE_READWRITE);

	if (CreateProcessInternalWOriginal == NULL) {
		// log error
		return;
	}

	// copy trampoline into allocated block
	CopyMemory(CreateProcessInternalWOriginal, CpiwTrampoline, sizeof(CpiwTrampoline));
	
	// Set trampoline to read-execute
	if (!VirtualProtect(CreateProcessInternalWOriginal, sizeof(CpiwTrampoline), PAGE_EXECUTE_READ, &OldMemoryProtection)) {
		// log error
	}

	// Write hook to CreateProcessInternalW
	if (VirtualProtect(CreateProcessInternalW, sizeof(CpiwHook), PAGE_READWRITE, &OldMemoryProtection)) {
		CopyMemory(CreateProcessInternalW, CpiwHook, sizeof(CpiwHook));

		if (!VirtualProtect(CreateProcessInternalW, sizeof(CpiwHook), OldMemoryProtection, &OldMemoryProtection)) {
			// log error
		}
	} else {
		// log error
	}
}

// GetProcessMitigationPolicy and SetProcessMitigationPolicy rely on NtSetInformationProcess
// values that are not present on Windows 7 (namely, ProcessMitigationPolicy (0x34)).
// However, there is an UpdateProcThreadAttribute function, which can set the
// PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY attribute. TODO: Look into this more. It is likely
// to be supported in Windows 7 but just undocumented. In fact I would say that at least
// ProcessASLRPolicy can be supported, in addition to ProcessDEPPolicy.
// Also see https://theryuu.github.io/ifeo-mitigationoptions.txt
// Also see https://support.microsoft.com/en-us/topic/an-update-is-available-for-the-aslr-feature-in-windows-7-or-in-windows-server-2008-r2-aec38646-36f1-08e8-32d2-6374d3c83d9e

WINBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	HANDLE						hProcess,
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY(L"(%p, %d, %p, %Iu)", hProcess, MitigationPolicy, lpBuffer, dwLength);

	switch (MitigationPolicy) {
	case ProcessDEPPolicy: {
		PPROCESS_MITIGATION_DEP_POLICY lpPolicy = (PPROCESS_MITIGATION_DEP_POLICY) lpBuffer;
		if (dwLength < sizeof(PROCESS_MITIGATION_DEP_POLICY)) {
			RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
			return FALSE;
		}

		lpPolicy->Permanent = !!GetSystemDEPPolicy();
						   }
		break;
	case ProcessASLRPolicy:
	case ProcessDynamicCodePolicy:
	case ProcessStrictHandleCheckPolicy:
	case ProcessSystemCallDisablePolicy:
	case ProcessMitigationOptionsMask:
	case ProcessExtensionPointDisablePolicy:
	case ProcessControlFlowGuardPolicy:
	case ProcessSignaturePolicy:
	case ProcessFontDisablePolicy:
	case ProcessImageLoadPolicy:
	case ProcessSystemCallFilterPolicy:
	case ProcessPayloadRestrictionPolicy:
	case ProcessChildProcessPolicy:
	case ProcessSideChannelIsolationPolicy:
	case ProcessUserShadowStackPolicy:
	case ProcessRedirectionTrustPolicy:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	default:
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	RtlSetLastWin32Error(ERROR_SUCCESS);
	return TRUE;
}

WINBASEAPI BOOL WINAPI SetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	IN	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY(L"(%u, %p, %Iu)", MitigationPolicy, lpBuffer, dwLength);
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

WINBASEAPI BOOL WINAPI GetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	OUT	LPVOID						ProcessInformation,
	IN	DWORD						ProcessInformationSize)
{
	NTSTATUS st;
	PROCESSINFOCLASS NtProcessInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationSize);

	if (ProcessInformationClass >= ProcessInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ProcessInformationClass) {
	case ProcessMemoryPriority:
		NtProcessInfoClass = ProcessPagePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtQueryInformationProcess(
		ProcessHandle,
		NtProcessInfoClass,
		ProcessInformation,
		ProcessInformationSize,
		NULL);

	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}

WINBASEAPI BOOL WINAPI SetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	IN	LPVOID						ProcessInformation,
	IN	DWORD						ProcessInformationSize)
{
	NTSTATUS st;
	PROCESSINFOCLASS NtProcessInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationSize);

	if (ProcessInformationClass >= ProcessInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ProcessInformationClass) {
	case ProcessMemoryPriority:
		NtProcessInfoClass = ProcessPagePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtSetInformationProcess(
		ProcessHandle,
		NtProcessInfoClass,
		ProcessInformation,
		ProcessInformationSize);

	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}