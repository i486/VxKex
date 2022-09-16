#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <BaseDll.h>

#include "thread.h"

// (Get/Set)ThreadDescription is basically just used for setting a pretty name for threads
// so that you can view them in the debugger. It depends on NtSetInformationThread with
// THREADINFOCLASS=0x26 (ThreadNameInformation) which is available starting from Windows
// 10. We could sort of emulate this given some work (considering the simple nature of
// thread descriptions), but it is almost definitely not worth doing at the moment.

WINBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	hThread,
	IN	LPCWSTR	lpThreadDescription)
{
	ODS_ENTRY(L"(%p, \"%ws\")", hThread, lpThreadDescription);
	return S_OK;
}

WINBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	hThread,
	OUT	LPWSTR	*ppszThreadDescription)
{
	// The caller is supposed to call LocalFree on *ppszThreadDescription
	STATIC CONST WCHAR szNoDesc[] = L"<no description available>";
	ODS_ENTRY(L"(%p, %p)", hThread, ppszThreadDescription);
	*ppszThreadDescription = (LPWSTR) LocalAlloc(0, sizeof(szNoDesc));
	RtlCopyMemory(ppszThreadDescription, szNoDesc, sizeof(szNoDesc));
	return S_OK;
}

WINBASEAPI BOOL WINAPI GetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	OUT	LPVOID						ThreadInformation,
	IN	DWORD						ThreadInformationSize)
{
	NTSTATUS st;
	THREADINFOCLASS NtThreadInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationSize);

	if (ThreadInformationClass >= ThreadInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ThreadInformationClass) {
	case ThreadMemoryPriority:
		NtThreadInfoClass = ThreadPagePriority;
		break;
	case ThreadAbsoluteCpuPriority:
		NtThreadInfoClass = ThreadActualBasePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtQueryInformationThread(
		ThreadHandle,
		NtThreadInfoClass,
		ThreadInformation,
		ThreadInformationSize,
		NULL);
	
	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}

// Memory priority and "absolute CPU priority" are supported on Windows 7, but
// just not exposed via a Win32 API. On Windows 8 the existing functionality is
// exposed. This makes those two features easy. However, "dynamic code policy"
// and "power throttling" are Win10+ kernel features, so we cannot support them.
WINBASEAPI BOOL WINAPI SetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	IN	LPVOID						ThreadInformation,
	IN	DWORD						ThreadInformationSize)
{
	NTSTATUS st;
	THREADINFOCLASS NtThreadInfoClass;

	ODS_ENTRY(L"(%p, %d, %p, %I32u)", ThreadHandle, ThreadInformationClass, ThreadInformation, ThreadInformationSize);

	if (ThreadInformationClass >= ThreadInformationClassMax) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	switch (ThreadInformationClass) {
	case ThreadMemoryPriority:
		NtThreadInfoClass = ThreadPagePriority;
		break;
	case ThreadAbsoluteCpuPriority:
		NtThreadInfoClass = ThreadActualBasePriority;
		break;
	default:
		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	st = NtSetInformationThread(
		ThreadHandle,
		NtThreadInfoClass,
		ThreadInformation,
		ThreadInformationSize);

	if (NT_SUCCESS(st)) {
		return TRUE;
	} else {
		BaseSetLastNTError(st);
		return FALSE;
	}
}