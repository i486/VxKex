#include <Windows.h>
#include <KexDll.h>

#include "k32func.h"
#include "k32defs.h"

BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInstance);
		DllMain_InitWoa();
	}

	return TRUE;
}

//
// EXPORTED FUNCTIONS
//

WINBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(
	OUT	LPFILETIME	lpSystemTimeAsFileTime)
{
	ODS_ENTRY();
	GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}

WINBASEAPI BOOL WINAPI GetCurrentPackageId(
	IN OUT	LPDWORD	lpdwBufferLength,
	OUT		LPBYTE	buffer)
{
	ODS_ENTRY();
	return APPMODEL_ERROR_NO_PACKAGE;
}

// Memory priority etc. not supported by win7
// TODO: Investigate using undocumented NTAPI to do this properly.
WINBASEAPI BOOL WINAPI SetProcessInformation(
	IN	HANDLE						hProcess,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	IN	LPVOID						ProcessInformation,
	IN	DWORD						ProcessInformationSize)
{
	ODS_ENTRY();
	return TRUE;
}

// TODO: Use NtSetInformationThread to properly emulate memory priority
WINBASEAPI BOOL WINAPI SetThreadInformation(
	IN	HANDLE						hThread,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	IN	LPVOID						ThreadInformation,
	IN	DWORD						ThreadInformationSize)
{
	ODS_ENTRY();
	return TRUE;
}

// (Get/Set)ThreadDescription is basically just used for setting a pretty name for threads
// so that you can view them in the debugger.

WINBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	hThread,
	IN	LPCWSTR	lpThreadDescription)
{
	ODS_ENTRY();
	return S_OK;
}

WINBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	hThread,
	OUT	LPWSTR	*ppszThreadDescription)
{
	// The caller is supposed to call LocalFree on *ppszThreadDescription
	STATIC CONST WCHAR szNoDesc[] = L"<no description available>";
	ODS_ENTRY();
	*ppszThreadDescription = (PWSTR) LocalAlloc(0, sizeof(szNoDesc));
	RtlCopyMemory(ppszThreadDescription, szNoDesc, sizeof(szNoDesc));
	return S_OK;
}

WINBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	HANDLE						hProcess,
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY();

	switch (MitigationPolicy) {
	case ProcessDEPPolicy: {
		PPROCESS_MITIGATION_DEP_POLICY lpPolicy = (PPROCESS_MITIGATION_DEP_POLICY) lpBuffer;
		if (dwLength < sizeof(PROCESS_MITIGATION_DEP_POLICY)) {
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
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
		ZeroMemory(lpBuffer, dwLength);
		break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	SetLastError(ERROR_SUCCESS);
	return TRUE;
}

WINBASEAPI BOOL WINAPI SetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	IN	LPVOID						lpBuffer,
	IN	SIZE_T						dwLength)
{
	ODS_ENTRY();
	return TRUE;
}

WINBASEAPI BOOL WINAPI PrefetchVirtualMemory(
	IN	HANDLE						hProcess,
	IN	ULONG_PTR					NumberOfEntries,
	IN	PWIN32_MEMORY_RANGE_ENTRY	VirtualAddresses,
	IN	ULONG						Flags)
{
	ODS_ENTRY();

	// Since the PrefetchVirtualMemory function can never be necessary for correct operation
	// of applications, it is treated as a strong hint by the system and is subject to usual
	// physical memory constraints where it can completely or partially fail under
	// low-memory conditions.
	SetLastError(ERROR_NOT_SUPPORTED);
	return FALSE;
}

WINBASEAPI BOOL GetOverlappedResultEx(
	IN	HANDLE			hFile,
	IN	LPOVERLAPPED	lpOverlapped,
	OUT	LPDWORD			lpNumberOfBytesTransferred,
	IN	DWORD			dwMilliseconds,
	IN	BOOL			bAlertable)
{
	ODS_ENTRY();
	return GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, (dwMilliseconds == INFINITE));
}