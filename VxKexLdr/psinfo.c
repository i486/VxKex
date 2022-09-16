#include <NtDll.h>
#include "VxKexLdr.h"

//
// Routines which retrieve information about a process.
// None of the routines here should change or "write" to a process.
//

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

// Given a process handle, return TRUE if that process is a 64-bit process, or FALSE otherwise.
BOOL ProcessIs64Bit(
	IN	HANDLE	hProc)
{
#ifndef _WIN64
	// 32-bit systems cannot run 64-bit code, so always return FALSE.
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

// Given a process handle, place the full Win32 path of the .exe file used to create that
// process in the szFullPath parameter.
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

// Return the PID of the parent of the current VxKexLdr process.
// If unsuccessful, return -1.
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

ULONG_PTR GetEntryPointVa(
	IN	ULONG_PTR	vaPeBase)
{
	CONST ULONG_PTR vaPeSig = vaPeBase + VaReadDword(vaPeBase + 0x3C);
	CONST ULONG_PTR vaCoffHdr = vaPeSig + 4;
	CONST ULONG_PTR vaOptHdr = vaCoffHdr + 20;
	return vaPeBase + VaReadDword(vaOptHdr + 16);
}