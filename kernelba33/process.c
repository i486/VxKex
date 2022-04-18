#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <BaseDll.h>

#include "process.h"

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

#ifdef _M_X64
BYTE CpiwHookedBytes[] = {
	0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,				// JMP [FuncPtr]
	0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC	// FuncPtr: DQ 0xCCCCCCCCCCCCCCCC
};
#else
BYTE CpiwHookedBytes[] = {
	0x68, 0xCC, 0xCC, 0xCC, 0xCC,					// PUSH 0xCCCCCCCC
	0xC3											// RET
};
#endif

CRITICAL_SECTION CpiwLock;
BYTE CpiwOriginalBytes[sizeof(CpiwHookedBytes)];

// The process creation hook procedure has the following responsibilities:
//   - rewrite the arguments so that VxKexLdr is invoked (just like if the child process
//     has VxKex enabled)
//   - wait for VxKexLdr to finish
//   - pass correct data back to caller
WINBASEAPI BOOL WINAPI HOOK_FUNCTION(CreateProcessInternalW) (
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
	OUT	PHANDLE					phRestrictedUserToken OPTIONAL)
{
	STATIC WCHAR szVxKexLdr[MAX_PATH];
	LPWSTR lpszModifiedCommandLine = lpszCommandLine;
	SIZE_T cchModifiedCommandLine = 0;
	BOOL bCpiwSuccess;
	STARTUPINFOEX StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	DWORD dwOldProtect;
	DWORD dwLastError;
	DWORD vaProcessInformation;
	
	ODS_ENTRY(L"(%p, L\"%ws\", L\"%ws\", %p, %p, %d, 0x%08x, %p, L\"%ws\", %p, %p, %p)",
			  hUserToken, lpszApplicationName, lpszCommandLine, lpProcessAttributes,
			  lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
			  lpszCurrentDirectory, lpStartupInfo, lpProcessInformation, phRestrictedUserToken);

	if (lpStartupInfo->cb >= sizeof(STARTUPINFO)) {
		// make a copy of the startupinfo so we can modify it
		CopyMemory(&StartupInfo, lpStartupInfo, lpStartupInfo->cb);
	} else {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	StringCchPrintfW(szVxKexLdr, ARRAYSIZE(szVxKexLdr), L"%s\\VxKexLdr.exe", KexGetKexData()->szKexDir);

	if (lpszApplicationName || lpszCommandLine) {
		cchModifiedCommandLine += wcslen(L"/CPIW ");

		if (lpszApplicationName) {
			cchModifiedCommandLine += wcslen(lpszApplicationName);

			if (lpszCommandLine) {
				// make space for the ' ' space character
				cchModifiedCommandLine++;
			}

			// make space for two '"' characters
			cchModifiedCommandLine++;
		}

		if (lpszCommandLine) {
			cchModifiedCommandLine += wcslen(lpszCommandLine);
		}

		cchModifiedCommandLine += 1; // null terminator and pos
		lpszModifiedCommandLine = (LPWSTR) StackAlloc(cchModifiedCommandLine * sizeof(WCHAR));

		StringCchPrintfW(lpszModifiedCommandLine, cchModifiedCommandLine, L"\"%s\" /CPIW %s",
						 szVxKexLdr, lpszCommandLine ? lpszCommandLine : L"");
	} else {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// pass KexData and Application name through
	StartupInfo.StartupInfo.lpReserved = (LPWSTR) lpszApplicationName;
	StartupInfo.StartupInfo.cbReserved2 = sizeof(KEX_PROCESS_DATA);
	StartupInfo.StartupInfo.lpReserved2 = (LPBYTE) KexGetKexData();

	// ugly hack
	// Basically some sort of C-runtime bullshit is getting involved here, and if the first DWORD
	// of lpReserved2 isn't 0, it will insert some garbage into the data and create problems. Since
	// the first DWORD of lpReserved2 doesn't turn out to be that significant we will temporarily
	// set it to zero to keep the Cnile retard happy. No idea whats actually going on but it fixes
	// the problem. :shrug:
	KexGetKexData()->IfeoParameters.dwEnableVxKex = FALSE;

	EnterCriticalSection(&CpiwLock); {
		// unhook
		VirtualProtect(&CreateProcessInternalW, sizeof(CpiwOriginalBytes), PAGE_READWRITE, &dwOldProtect);
		CopyMemory(&CreateProcessInternalW, CpiwOriginalBytes, sizeof(CpiwOriginalBytes));
		VirtualProtect(&CreateProcessInternalW, sizeof(CpiwOriginalBytes), dwOldProtect, &dwOldProtect);
		// call
		bCpiwSuccess = CreateProcessInternalW(
			hUserToken,
			szVxKexLdr,
			lpszModifiedCommandLine,
			lpProcessAttributes,
			lpThreadAttributes,
			bInheritHandles,
			dwCreationFlags,
			lpEnvironment,
			lpszCurrentDirectory,
			&StartupInfo.StartupInfo,
			lpProcessInformation,
			phRestrictedUserToken);
		dwLastError = RtlGetLastWin32Error();
		// re-hook
		VirtualProtect(&CreateProcessInternalW, sizeof(CpiwHookedBytes), PAGE_READWRITE, &dwOldProtect);
		CopyMemory(&CreateProcessInternalW, CpiwHookedBytes, sizeof(CpiwHookedBytes));
		VirtualProtect(&CreateProcessInternalW, sizeof(CpiwHookedBytes), dwOldProtect, &dwOldProtect);
	} LeaveCriticalSection(&CpiwLock);

	// ugly hack pt. 2
	KexGetKexData()->IfeoParameters.dwEnableVxKex = TRUE;

	// Alright now wait for VxKex to exit, and gather its process exit code. This is a pointer
	// within VxKexLdr address space to a PROCESS_INFORMATION structure which can be read and
	// returned to the caller.
	WaitForSingleObject(lpProcessInformation->hProcess, INFINITE);
	GetExitCodeProcess(lpProcessInformation->hProcess, &vaProcessInformation);

	if (vaProcessInformation == 0) {
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	ReadProcessMemory(lpProcessInformation->hProcess,
					  (LPCVOID) vaProcessInformation,
					  &ProcessInformation,
					  sizeof(PROCESS_INFORMATION),
					  NULL);

	NtClose(lpProcessInformation->hThread);
	NtClose(lpProcessInformation->hProcess);
	*lpProcessInformation = ProcessInformation;

	RtlSetLastWin32Error(dwLastError);
	return bCpiwSuccess;
}

VOID DllMain_HookCreateProcess(
	VOID)
{
	PULONG_PTR pulHookFunction;
	DWORD dwOldProtect;

	ODS_ENTRY();

	if (KexGetKexData()->IfeoParameters.dwDisableForChild) {
		// do nothing
		return;
	}

#ifdef _M_X64
	pulHookFunction = (PULONG_PTR) &CpiwHookedBytes[6];
#else
	pulHookFunction = (PULONG_PTR) &CpiwHookedBytes[1];
#endif

	InitializeCriticalSection(&CpiwLock);
	*pulHookFunction = (ULONG_PTR) HOOK_FUNCTION(CreateProcessInternalW);
	CopyMemory(CpiwOriginalBytes, &CreateProcessInternalW, sizeof(CpiwOriginalBytes));
	VirtualProtect(&CreateProcessInternalW, sizeof(CpiwHookedBytes), PAGE_READWRITE, &dwOldProtect);
	CopyMemory(&CreateProcessInternalW, CpiwHookedBytes, sizeof(CpiwHookedBytes));
	VirtualProtect(&CreateProcessInternalW, sizeof(CpiwHookedBytes), dwOldProtect, &dwOldProtect);
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