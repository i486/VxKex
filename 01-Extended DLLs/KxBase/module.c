///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     module.c
//
// Abstract:
//
//     Thjs file contains functions related to DLL loading.
//     The main purpose of the files in here is to rewrite the names of DLLs
//     which the application requests to dynamically load.
//
// Author:
//
//     Author (10-Feb-2024)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              10-Feb-2024    Initial creation.
//     vxiiduu              02-Mar-2024    Fix GetModuleHandleExW logging when
//										   GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
//										   flag is passed.
//     vxiiduu              13-Mar-2024    Move most of the code here to kexldr.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>

//
// These two utility functions make use of an unused field in the TEB.
// Their purpose is to set KexLdrShouldRewriteDll to 1 whenever
// Ext_GetModuleHandle(Ex)(A/W) or Ext_LoadLibrary(Ex)(A/W) is present in
// the call stack.
//
// When this happens, it means that an EXE or DLL outside of WinDir and KexDir
// has called GetModuleHandle or LoadLibrary. It signals to Ext_LdrLoadDll
// and Ext_LdrGetDllHandle so that they can avoid rewriting imports when it
// isn't desired.
//

STATIC INLINE VOID InterceptedKernelBaseLoaderCallEntry(
	OUT	PBOOLEAN	ReEntrant)
{
	PTEB Teb;

	Teb = NtCurrentTeb();
	*ReEntrant = Teb->KexLdrShouldRewriteDll;
	Teb->KexLdrShouldRewriteDll = TRUE;
}

STATIC INLINE VOID InterceptedKernelBaseLoaderCallReturn(
	IN	BOOLEAN		ReEntrant)
{
	if (!ReEntrant) {
		NtCurrentTeb()->KexLdrShouldRewriteDll = FALSE;
	}
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = GetModuleHandleA(ModuleName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	//
	// APPSPECIFICHACK: Chromium-based software uses a bootleg knockoff version of
	// GetProcAddress that fails miserably and crashes the whole app when we rewrite
	// NTDLL, because their shitty implementation doesn't work properly with
	// the export forwarders in KxNt. Neither does it properly work with stubs,
	// because they actually scan the instruction code of system calls.
	//
	if ((KexData->Flags & KEXDATA_FLAG_CHROMIUM) &&
		ModuleName != NULL &&
		StringEqual(ModuleName, L"ntdll.dll")) {

		KexLogDebugEvent(L"Not rewriting NTDLL for Chromium compatibility");
		return (HMODULE) KexData->SystemDllBase;
	}

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = GetModuleHandleW(ModuleName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	BOOL Success;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	Success = GetModuleHandleExA(Flags, ModuleName, ModuleHandleOut);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return Success;
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	BOOL Success;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	Success = GetModuleHandleExW(Flags, ModuleName, ModuleHandleOut);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return Success;
}

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameA(
	IN	HMODULE	ModuleHandle,
	OUT	PSTR	FileName,
	IN	ULONG	FileNameCch)
{
	return GetModuleFileNameA(ModuleHandle, FileName, FileNameCch);
}

KXBASEAPI ULONG WINAPI Ext_GetModuleFileNameW(
	IN	HMODULE	ModuleHandle,
	OUT	PWSTR	FileName,
	IN	ULONG	FileNameCch)
{
	return GetModuleFileNameW(ModuleHandle, FileName, FileNameCch);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryA(
	IN	PCSTR	FileName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = LoadLibraryA(FileName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = LoadLibraryW(FileName);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = LoadLibraryExA(FileName, FileHandle, Flags);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	HMODULE ModuleHandle;
	BOOLEAN ReEntrant;

	InterceptedKernelBaseLoaderCallEntry(&ReEntrant);
	ModuleHandle = LoadLibraryExW(FileName, FileHandle, Flags);
	InterceptedKernelBaseLoaderCallReturn(ReEntrant);

	return ModuleHandle;
}

KXBASEAPI HMODULE WINAPI LoadPackagedLibrary(
	IN	PCWSTR	LibFileName,
	IN	ULONG	Reserved)
{
	RTL_PATH_TYPE PathType;
	ULONG Index;

	if (Reserved) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	PathType = RtlDetermineDosPathNameType_U(LibFileName);

	if (PathType != RtlPathTypeRelative) {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}

	for (Index = 0; LibFileName[Index] != '\0'; ++Index) {
		if (LibFileName[Index] == '.' && LibFileName[Index+1] == '.' &&
			(LibFileName[Index+2] == '\\' || LibFileName[Index+2] == '/')) {

			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return NULL;
		}

		do {
			++Index;
		} until (LibFileName[Index] == '\0' ||
				 LibFileName[Index] == '\\' ||
				 LibFileName[Index] == '/');
	}

	// On Windows 8 this would be the point where this function would call
	// LoadLibraryExW with the undocumented flag 0x04. However, this flag and
	// its underlying implementation inside LdrLoadDll is not present on Windows
	// 7, so we will just return an error straight away (as documented).
	RtlSetLastWin32Error(APPMODEL_ERROR_NO_PACKAGE);
	return NULL;
}

STATIC NTSTATUS BasepGetDllDirectoryProcedure(
	IN		PCSTR	ProcedureName,
	IN OUT	PPVOID	ProcedureAddress)
{
	NTSTATUS Status;

	Status = STATUS_SUCCESS;

	ASSERT (ProcedureName != NULL);
	ASSERT (ProcedureAddress != NULL);

	if (!*ProcedureAddress) {
		ANSI_STRING ProcedureNameAS;

		Status = RtlInitAnsiStringEx(&ProcedureNameAS, ProcedureName);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Status = LdrGetProcedureAddress(
			KexData->BaseDllBase,
			&ProcedureNameAS,
			0,
			ProcedureAddress);

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"%hs is not available on this computer\r\n\r\n"
				L"This function is only available on Windows 7 with the KB2533623 "
				L"security update.", ProcedureName);

			BaseSetLastNTError(Status);
		}
	}

	if (NT_SUCCESS(Status)) {
		ASSUME (*ProcedureAddress != NULL);
	}

	return Status;
}

KXBASEAPI DLL_DIRECTORY_COOKIE WINAPI Ext_AddDllDirectory(
	IN	PCWSTR	NewDirectory)
{
	STATIC DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory) (PCWSTR) = NULL;

	BasepGetDllDirectoryProcedure("AddDllDirectory", (PPVOID) &AddDllDirectory);

	if (AddDllDirectory) {
		return AddDllDirectory(NewDirectory);
	} else {
		return NULL;
	}
}

KXBASEAPI BOOL WINAPI Ext_RemoveDllDirectory(
	IN	DLL_DIRECTORY_COOKIE	Cookie)
{
	STATIC BOOL (WINAPI *RemoveDllDirectory) (DLL_DIRECTORY_COOKIE) = NULL;

	BasepGetDllDirectoryProcedure("RemoveDllDirectory", (PPVOID) &RemoveDllDirectory);

	if (RemoveDllDirectory) {
		return RemoveDllDirectory(Cookie);
	} else {
		return FALSE;
	}
}

KXBASEAPI BOOL WINAPI Ext_SetDefaultDllDirectories(
	IN	ULONG	DirectoryFlags)
{
	STATIC BOOL (WINAPI *SetDefaultDllDirectories) (ULONG) = NULL;

	BasepGetDllDirectoryProcedure("SetDefaultDllDirectories", (PPVOID) &SetDefaultDllDirectories);

	if (SetDefaultDllDirectories) {
		return SetDefaultDllDirectories(DirectoryFlags);
	} else {
		return FALSE;
	}
}