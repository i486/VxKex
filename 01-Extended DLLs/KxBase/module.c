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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"
#include <KexW32ML.h>

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleA(
	IN	PCSTR	ModuleName)
{
	BOOLEAN Success;
	HMODULE ModuleHandle;

	Success = Ext_GetModuleHandleExA(
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		ModuleName,
		&ModuleHandle);

	if (Success) {
		return ModuleHandle;
	} else {
		return GetModuleHandleA(ModuleName);
	}
}

KXBASEAPI HMODULE WINAPI Ext_GetModuleHandleW(
	IN	PCWSTR	ModuleName)
{
	BOOLEAN Success;
	HMODULE ModuleHandle;

	Success = Ext_GetModuleHandleExW(
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		ModuleName,
		&ModuleHandle);

	if (Success) {
		return ModuleHandle;
	} else {
		return GetModuleHandleW(ModuleName);
	}
}

//
// ANSI thunk to Ext_GetModuleHandleExW
//
KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExA(
	IN	ULONG	Flags,
	IN	PCSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	if (Flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
		goto BailOut;
	}

	if (ModuleName) {
		WCHAR ModuleNameUnicode[MAX_PATH];
		ULONG ModuleNameCch;

		ModuleNameCch = MultiByteToWideChar(
			CP_ACP,
			0,
			ModuleName,
			-1,
			ModuleNameUnicode,
			ARRAYSIZE(ModuleNameUnicode));

		if (ModuleNameCch == 0) {
			goto BailOut;
		}

		return Ext_GetModuleHandleExW(Flags, ModuleNameUnicode, ModuleHandleOut);
	}

BailOut:
	return GetModuleHandleExA(Flags, ModuleName, ModuleHandleOut);
}

KXBASEAPI BOOL WINAPI Ext_GetModuleHandleExW(
	IN	ULONG	Flags,
	IN	PCWSTR	ModuleName,
	OUT	HMODULE	*ModuleHandleOut)
{
	NTSTATUS Status;

	Status = STATUS_SUCCESS;

	if (Flags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) {
		KexLogDebugEvent(
			L"GetModuleHandle* called: 0x%p\r\n\r\n"
			L"Flags: 0x%08lx\r\n"
			L"ModuleHandleOut: 0x%p",
			ModuleName,
			Flags,
			ModuleHandleOut);

		// Nothing to rewrite
		goto BailOut;
	} else {
		KexLogDebugEvent(
			L"GetModuleHandle* called: %s\r\n\r\n"
			L"Flags: 0x%08lx\r\n"
			L"ModuleHandleOut: 0x%p",
			ModuleName,
			Flags,
			ModuleHandleOut);
	}

	if (ModuleName) {
		UNICODE_STRING ModuleNameUS;
		UNICODE_STRING RewrittenModuleName;
		WCHAR RewrittenModuleNameBuffer[MAX_PATH + 1];

		RtlInitEmptyUnicodeString(
			&RewrittenModuleName,
			RewrittenModuleNameBuffer,
			sizeof(RewrittenModuleNameBuffer));

		Status = RtlInitUnicodeStringEx(&ModuleNameUS, ModuleName);
		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		Status = KexRewriteDllPath(
			&ModuleNameUS,
			&RewrittenModuleName);

		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		Status = KexRtlNullTerminateUnicodeString(&RewrittenModuleName);
		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		ModuleName = RewrittenModuleName.Buffer;
	}

BailOut:

	if (!NT_SUCCESS(Status) &&
		Status != STATUS_STRING_MAPPER_ENTRY_NOT_FOUND &&	// DLL doesn't need to be rewritten
		Status != STATUS_DLL_NOT_IN_SYSTEM_ROOT) {			// Isn't a Windows DLL

		KexLogWarningEvent(
			L"Failed to rewrite ModuleName: %s\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			ModuleName,
			KexRtlNtStatusToString(Status),
			Status);
	}

	return GetModuleHandleExW(Flags, ModuleName, ModuleHandleOut);
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
	return Ext_LoadLibraryExA(FileName, NULL, 0);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryW(
	IN	PCWSTR	FileName)
{
	return Ext_LoadLibraryExW(FileName, NULL, 0);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExA(
	IN	PCSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	if (FileName) {
		WCHAR FileNameUnicode[MAX_PATH];
		ULONG FileNameCch;

		FileNameCch = MultiByteToWideChar(
			CP_ACP,
			0,
			FileName,
			-1,
			FileNameUnicode,
			ARRAYSIZE(FileNameUnicode));

		if (FileNameCch == 0) {
			goto BailOut;
		}

		return Ext_LoadLibraryExW(FileNameUnicode, FileHandle, Flags);
	}

BailOut:
	return LoadLibraryExA(FileName, FileHandle, Flags);
}

KXBASEAPI HMODULE WINAPI Ext_LoadLibraryExW(
	IN	PCWSTR	FileName,
	IN	HANDLE	FileHandle,
	IN	ULONG	Flags)
{
	NTSTATUS Status;
	HMODULE ModuleHandle;
	STATIC ULONG Counter = 0;

	Status = STATUS_SUCCESS;

	if (Flags & (DONT_RESOLVE_DLL_REFERENCES |
				 LOAD_LIBRARY_AS_DATAFILE |
				 LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE |
				 LOAD_LIBRARY_AS_IMAGE_RESOURCE)) {

		// They are probably trying to load resources.
		// In which case, we won't rewrite.
		goto BailOut;
	}

	if (FileName) {
		UNICODE_STRING FileNameUS;
		UNICODE_STRING RewrittenFileName;
		WCHAR RewrittenFileNameBuffer[MAX_PATH + 1];

		RtlInitEmptyUnicodeString(
			&RewrittenFileName,
			RewrittenFileNameBuffer,
			sizeof(RewrittenFileNameBuffer));

		Status = RtlInitUnicodeStringEx(
			&FileNameUS,
			FileName);

		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		Status = KexRewriteDllPath(
			&FileNameUS,
			&RewrittenFileName);

		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		Status = KexRtlNullTerminateUnicodeString(&RewrittenFileName);
		if (!NT_SUCCESS(Status)) {
			goto BailOut;
		}

		FileName = RewrittenFileNameBuffer;

		//
		// At this point we have changed the DLL.
		//
	}

	if (!NT_SUCCESS(Status) &&
		Status != STATUS_STRING_MAPPER_ENTRY_NOT_FOUND &&
		Status != STATUS_DLL_NOT_IN_SYSTEM_ROOT) {

		KexLogWarningEvent(
			L"Failed to rewrite FileName: %s\r\n\r\n"
			L"NTSTATUS error code: %s (0x%08lx)",
			FileName,
			KexRtlNtStatusToString(Status),
			Status);
	}

BailOut:
	ModuleHandle = LoadLibraryExW(FileName, FileHandle, Flags);

	if (ModuleHandle) {
		KexLogDebugEvent(
			L"Dynamically loaded %s successfully.\r\n\r\n"
			L"Flags:         0x%08lx\r\n"
			L"FileHandle:    0x%08lx\r\n"
			L"The module handle is 0x%p.",
			FileName,
			Flags,
			FileHandle,
			ModuleHandle);
	} else {
		KexLogWarningEvent(
			L"Failed to dynamically load %s.\r\n\r\n"
			L"Flags:         0x%08lx\r\n"
			L"FileHandle:    0x%08lx\r\n"
			L"(%d) %s",
			FileName,
			Flags,
			FileHandle,
			GetLastError(), GetLastErrorAsString());
	}

	return ModuleHandle;
}

KXBASEAPI DLL_DIRECTORY_COOKIE WINAPI Ext_AddDllDirectory(
	IN	PCWSTR	NewDirectory)
{
	HMODULE Kernel32;
	DLL_DIRECTORY_COOKIE (WINAPI *AddDllDirectory)(PCWSTR NewDirectory);

	KexLogDebugEvent(L"AddDllDirectory was called: %s", NewDirectory);

	if (KexData->IfeoParameters.DisableDllDirectory) {
		KexLogDetailEvent(L"AddDllDirectory disabled by user setting.");
		return (DLL_DIRECTORY_COOKIE) 0x12345678;
	}

	Kernel32 = GetModuleHandle(L"kernel32");
	AddDllDirectory = (DLL_DIRECTORY_COOKIE (WINAPI *)(PCWSTR NewDirectory)) GetProcAddress(Kernel32, "AddDllDirectory");

	if (!AddDllDirectory) {
		KexLogInformationEvent(
			L"AddDllDirectory is not available on this computer.\r\n\r\n"
			L"This function is only available on Windows 7 with the KB2533623 security update.");
		SetLastError(ERROR_NOT_SUPPORTED);
		return NULL;
	}

	return AddDllDirectory(NewDirectory);
}

KXBASEAPI BOOL WINAPI Ext_RemoveDllDirectory(
	IN	DLL_DIRECTORY_COOKIE	Cookie)
{
	HMODULE Kernel32;
	BOOL (WINAPI *RemoveDllDirectory)(DLL_DIRECTORY_COOKIE Cookie);

	KexLogDebugEvent(L"RemoveDllDirectory was called: 0x%p", Cookie);

	if (KexData->IfeoParameters.DisableDllDirectory) {
		KexLogDetailEvent(L"RemoveDllDirectory disabled by user setting.");
		return TRUE;
	}

	Kernel32 = GetModuleHandle(L"kernel32");
	RemoveDllDirectory = (BOOL (WINAPI *)(DLL_DIRECTORY_COOKIE)) GetProcAddress(Kernel32, "RemoveDllDirectory");

	if (!RemoveDllDirectory) {
		KexLogInformationEvent(
			L"RemoveDllDirectory is not available on this computer.\r\n\r\n"
			L"This function is only available on Windows 7 with the KB2533623 security update.");
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	return RemoveDllDirectory(Cookie);
}

KXBASEAPI BOOL WINAPI Ext_SetDefaultDllDirectories(
	IN	ULONG	DirectoryFlags)
{
	HMODULE Kernel32;
	BOOL (WINAPI *SetDefaultDllDirectories)(ULONG DirectoryFlags);

	KexLogDetailEvent(
		L"SetDefaultDllDirectories was called.\r\n\r\n"
		L"DirectoryFlags = 0x%08lx%s\r\n"
		L"%s%s%s%s%s%s",
		DirectoryFlags,
		DirectoryFlags ? L", which represents the following value(s):" : L"",
		DirectoryFlags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR ? L"LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR\r\n" : L"",
		DirectoryFlags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR ? L"LOAD_LIBRARY_SEARCH_APPLICATION_DIR\r\n" : L"",
		DirectoryFlags & LOAD_LIBRARY_SEARCH_USER_DIRS ? L"LOAD_LIBRARY_SEARCH_USER_DIRS\r\n" : L"",
		DirectoryFlags & LOAD_LIBRARY_SEARCH_SYSTEM32 ? L"LOAD_LIBRARY_SEARCH_SYSTEM32\r\n" : L"",
		DirectoryFlags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS ? L"LOAD_LIBRARY_SEARCH_DEFAULT_DIRS\r\n" : L"",
		DirectoryFlags & LOAD_LIBRARY_SAFE_CURRENT_DIRS ? L"LOAD_LIBRARY_SAFE_CURRENT_DIRS\r\n" : L"");

	if (KexData->IfeoParameters.DisableDllDirectory) {
		KexLogDetailEvent(L"SetDefaultDllDirectories disabled by user setting.");
		return TRUE;
	}

	Kernel32 = GetModuleHandle(L"kernel32");
	SetDefaultDllDirectories = (BOOL (WINAPI *)(ULONG)) GetProcAddress(Kernel32, "SetDefaultDllDirectories");

	if (!SetDefaultDllDirectories) {
		KexLogInformationEvent(
			L"SetDefaultDllDirectories is not available on this computer.\r\n\r\n"
			L"This function is only available on Windows 7 with the KB2533623 security update.");
		SetLastError(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	//
	// We will add LOAD_LIBRARY_SEARCH_USER_DIRS to the list of flags,
	// so that the dll directory we added during DllMain will be available.
	// This isn't necessary for LoadLibrary/GetModuleHandle, but it is necessary
	// for successfully loading rewritten imports of any future DLLs.
	//

	if (!(DirectoryFlags & LOAD_LIBRARY_SEARCH_USER_DIRS)) {
		KexLogDebugEvent(L"Adding LOAD_LIBRARY_SEARCH_USER_DIRS to DirectoryFlags.");
		DirectoryFlags |= LOAD_LIBRARY_SEARCH_USER_DIRS;
	}

	return SetDefaultDllDirectories(DirectoryFlags);
}

KXBASEAPI FARPROC WINAPI Ext_GetProcAddress(
	IN	HMODULE		ModuleHandle,
	IN	PCSTR		ProcedureName)
{
	FARPROC ProcedureAddress;
	WCHAR ModuleFullPath[MAX_PATH];
	PCWSTR ModuleName;

	GetModuleFileName(ModuleHandle, ModuleFullPath, ARRAYSIZE(ModuleFullPath));
	ModuleName = PathFindFileName(ModuleFullPath);

	ProcedureAddress = GetProcAddress(ModuleHandle, ProcedureName);

	if (ProcedureAddress) {
		if (IS_INTRESOURCE(ProcedureName)) {
			KexLogDebugEvent(
				L"GetProcAddress of #%hu from %s succeeded",
				ProcedureName, ModuleName);
		} else {
			KexLogDebugEvent(
				L"GetProcAddress of %hs from %s succeeded",
				ProcedureName, ModuleName);
		}
	} else {
		if (IS_INTRESOURCE(ProcedureName)) {
			KexLogWarningEvent(
				L"GetProcAddress of #%hu from %s failed\r\n\r\n"
				L"(%d) %s",
				ProcedureName, ModuleName,
				GetLastError(), GetLastErrorAsString());
		} else {
			KexLogWarningEvent(
				L"GetProcAddress of %hs from %s failed\r\n\r\n"
				L"(%d) %s",
				ProcedureName, ModuleName,
				GetLastError(), GetLastErrorAsString());
		}
	}

	return ProcedureAddress;
}