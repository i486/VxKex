///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     stubs.c
//
// Abstract:
//
//     Forwarder stubs that do nothing except for calling the original function.
//     These exist because some stupid software such as THEMIDA is not
//     compatible with export forwarders.
//
// Author:
//
//     vxiiduu (27-Apr-2026)
//
// Environment:
//
//     Native
//
// Revision History:
//
//     vxiiduu              27-Apr-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

NTSTATUS NTAPI Stub_Ext_RtlInitializeCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection)
{
	return Ext_RtlInitializeCriticalSection(
		CriticalSection);
}

NTSTATUS NTAPI Stub_Ext_NtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength)
{
	return Ext_NtSetInformationThread(
		ThreadHandle,
		ThreadInformationClass,
		ThreadInformation,
		ThreadInformationLength);
}

NTSTATUS NTAPI Stub_NtQueryVirtualMemory(
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL)
{
	return KexNtQueryVirtualMemory(
		ProcessHandle,
		BaseAddress,
		MemoryInformationClass,
		MemoryInformation,
		MemoryInformationLength,
		ReturnLength);
}

NTSTATUS NTAPI Stub_Ext_NtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL)
{
	return Ext_NtQueryInformationProcess(
		ProcessHandle,
		ProcessInformationClass,
		ProcessInformation,
		ProcessInformationLength,
		ReturnLength);
}

NTSTATUS NTAPI Stub_LdrUnloadDll(
	IN	PVOID				DllHandle)
{
	//
	// 32-bit WinDbg uses some kind of Unicows or whatever library which has
	// its own bootleg GetProcAddress (nicknamed "GetProcAddressInternal" in
	// the symbols) sdktools\unicows\delay\resolve.c, which is used for delay-
	// loaded APIs.
	//
	// Of course as is typical these bootleg GetProcAddress functions do not handle
	// export forwarders and this causes Windbg to fail to show the file open dialog
	// because GetOpenFileName and GetSaveFileName (among a few other shell-related
	// functions) are delay loaded.
	//
	// Long story short, none of these delay loaded functions will work unless the
	// function being delay-loaded isn't export-forwarded AND LdrUnloadDll isn't
	// export-forwarded. LdrUnloadDll isn't even called, it looks like they are just
	// trying to load it to test for its presence. Probably some legacy code that
	// tries to determine if it's running on Win9x, which is funny because new
	// versions of WinDbg no longer run on anything less than Windows 11.
	//

	return LdrUnloadDll(DllHandle);
}