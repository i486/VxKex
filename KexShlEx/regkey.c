///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     regkey.c
//
// Abstract:
//
//     Implements the "Open in Registry Editor" functionality.
//
// Author:
//
//     vxiiduu (24-Jun-2026)
//
// Environment:
//
//     Inside explorer.exe
//
// Revision History:
//
//     vxiiduu              24-Jun-2026  Initial creation.
//     vxiiduu              30-Jun-2026  Change CreateProcess to ShellExecute
//                                       in order to support UAC
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "KexShlEx.h"

BOOLEAN OpenIfeoRegKey(
	IN	HWND	ParentWindow OPTIONAL,
	IN	PCWSTR	ExeFullPath)
{
	typedef struct {
		ULONG	NameLength;
		WCHAR	Name[StringLiteralLength(L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT"
										 L"\\CurrentVersion\\Image File Execution Options\\") +
					 MAX_PATH +
					 StringLiteralLength(L"\\VxKex_0123456789ABCDEF") +
					 1];
	} TYPEDEF_TYPE_NAME(KEY_NAME_INFORMATION_BUFFER);

	NTSTATUS Status;
	BOOL Success;
	HRESULT Result;
	ULONG Win32Error;
	HANDLE KeyHandle;
	UNICODE_STRING ExeFullPathUS;
	KEY_NAME_INFORMATION_BUFFER KeyName;
	ULONG ReturnCb;
	WCHAR RegeditExePath[MAX_PATH];
	WCHAR RegeditRegPath[StringLiteralLength(L"Computer\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\"
											 L"Windows NT\\CurrentVersion\\Image File Execution Options\\") +
						 MAX_PATH +
						 StringLiteralLength(L"\\VxKex_0123456789ABCDEF") +
						 1];
	HINSTANCE ShellExecuteResult;

	ASSERT (ExeFullPath != NULL);

	//
	// Open the IFEO key.
	//

	RtlInitUnicodeString(&ExeFullPathUS, ExeFullPath);

	Status = LdrOpenImageFileOptionsKey(
		&ExeFullPathUS,
		FALSE,
		&KeyHandle);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	//
	// Find out the registry path of this key.
	// The path is returned as a KEY_NAME_INFORMATION structure (here extended as a
	// KEY_NAME_INFORMATION_BUFFER structure), the Name member of which is not null
	// terminated. Since we subtracted a sizeof(WCHAR) from the structure, we can
	// guarantee that it is null terminated.
	//

	KexRtlZeroMemory(&KeyName, sizeof(KeyName));

	Status = NtQueryKey(
		KeyHandle,
		KeyNameInformation,
		&KeyName,
		sizeof(KeyName) - sizeof(WCHAR),
		&ReturnCb);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (ReturnCb >= sizeof(ULONG));
	ASSERT (ReturnCb <= sizeof(KeyName) - sizeof(WCHAR));
	SafeClose(KeyHandle);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	//
	// Convert the registry path returned by NtQueryKey to the one expected by
	// Regedit.
	// NT format: \Registry\Machine\Software\Microsoft\...
	// Regedit format: Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\...
	// In other words we need to replace \Registry\Machine with
	// Computer\HKEY_LOCAL_MACHINE.
	//

	ASSERT (StringBeginsWithI(KeyName.Name, L"\\Registry\\Machine"));

	Result = StringCchCopy(
		RegeditRegPath,
		ARRAYSIZE(RegeditRegPath),
		L"Computer\\HKEY_LOCAL_MACHINE");

	ASSERT (SUCCEEDED(Result));

	Result = StringCchCat(
		RegeditRegPath,
		ARRAYSIZE(RegeditRegPath),
		&KeyName.Name[StringLiteralLength(L"\\Registry\\Machine")]);

	ASSERT (SUCCEEDED(Result));

	//
	// Write the regedit registry path (REG_SZ) to
	// HKCU\Software\Microsoft\Windows\CurrentVersion\Applets\Regedit\LastKey.
	//

	Win32Error = RegWriteString(
		HKEY_CURRENT_USER,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit",
		L"LastKey",
		RegeditRegPath);

	ASSERT (Win32Error == ERROR_SUCCESS);

	if (Win32Error != ERROR_SUCCESS) {
		return FALSE;
	}

	//
	// Determine the location of regedit.exe.
	// Usually %WinDir%\regedit.exe
	//

	GetSystemWindowsDirectory(RegeditExePath, ARRAYSIZE(RegeditExePath));

	Result = PathCchAppend(
		RegeditExePath,
		ARRAYSIZE(RegeditExePath),
		L"regedit.exe");

	ASSERT (SUCCEEDED(Result));

	//
	// Open regedit.
	// We use the /M flag so that it permits more than one regedit window
	// to be open.
	// ShellExecute is required to avoid ERROR_ELEVATION_REQUIRED if UAC is
	// enabled on the system.
	//

	ShellExecuteResult = ShellExecute(
		ParentWindow,
		L"open",
		RegeditExePath,
		L"/M",
		NULL,
		SW_SHOWNORMAL);

	Success = (INT) ShellExecuteResult > 32;
	ASSERT (Success);

	return Success;
}