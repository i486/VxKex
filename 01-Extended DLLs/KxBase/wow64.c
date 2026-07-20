#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI IsWow64Process2(
	IN	HANDLE	ProcessHandle,
	OUT	PUSHORT	ProcessMachine,
	OUT	PUSHORT	NativeMachine)
{
	NTSTATUS Status;

	Status = RtlWow64GetProcessMachines(ProcessHandle, ProcessMachine, NativeMachine);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}

KXBASEAPI HRESULT WINAPI IsWow64GuestMachineSupported(
	IN	USHORT	WowGuestMachine,
	OUT	PBOOL	MachineIsSupported)
{
	NTSTATUS Status;
	BOOLEAN IsSupported;

	Status = RtlWow64IsWowGuestMachineSupported(WowGuestMachine, &IsSupported);

	if (!NT_SUCCESS(Status)) {
		return HRESULT_FROM_NT(Status);
	}

	*MachineIsSupported = IsSupported;
	return S_OK;
}

//
// The Microsoft documentation for this function is useless, wrong, and should be
// completely ignored.
//
// WinHTTP.dll, Wininet.dll, and RPCSS.dll in Windows 10 uses this function apparently
// to modify the behavior of registry functions (with respect to WOW64 key redirection
// and so on).
// Combase in Windows 10 seems to use the function solely to determine the current
// default guest machine in the following way:
//
// GuestMachine = Wow64SetThreadDefaultGuestMachine(0);
// Wow64SetThreadDefaultGuestMachine(GuestMachine);
// ... other use of GuestMachine here ...
//
// This function returns a USHORT which represents the previous value of the
// default guest machine.
// This function accepts and returns an IMAGE_FILE_MACHINE_* value. The return
// value can be 0 (or IMAGE_FILE_MACHINE_UNKNOWN), WinHTTP seems to check for
// this.
// The function performs no validation whatsoever on the input value, it just
// sets a field in the TEB to whatever the parameter is.
//
// TODO: Run some tests on Win10 to figure out what this function does.
//
KXBASEAPI USHORT WINAPI Wow64SetThreadDefaultGuestMachine(
	IN	USHORT	WowGuestMachine)
{
	KexDebugCheckpoint();
	return IMAGE_FILE_MACHINE_UNKNOWN;
}

KXBASEAPI UINT WINAPI GetSystemWow64Directory2A(
	OUT	PSTR	Buffer,
	IN	UINT	Cch,
	IN	USHORT	ImageFileMachineType)
{
	HRESULT Result;
	CHAR Directory[MAX_PATH];
	PCSTR Wow64Directory;

	if (KexRtlOperatingSystemBitness() == 32) {
		RtlSetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}

	switch (ImageFileMachineType) {
	case IMAGE_FILE_MACHINE_TARGET_HOST:
		Wow64Directory = "\\system32";
		break;
	case IMAGE_FILE_MACHINE_I386:
		Wow64Directory = "\\SysWOW64";
		break;
	default:
		RtlSetLastWin32Error(ERROR_BAD_ARGUMENTS);
		return 0;
	}

	GetSystemWindowsDirectoryA(Directory, ARRAYSIZE(Directory));
	Result = StringCchCatA(Directory, ARRAYSIZE(Directory), Wow64Directory);
	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopyA(Buffer, Cch, Directory);
	
	if (FAILED(Result)) {
		RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	return (UINT) strlen(Directory);
}

KXBASEAPI UINT WINAPI GetSystemWow64Directory2W(
	OUT	PWSTR	Buffer,
	IN	UINT	Cch,
	IN	USHORT	ImageFileMachineType)
{
	HRESULT Result;
	WCHAR Directory[MAX_PATH];
	PCWSTR Wow64Directory;

	if (KexRtlOperatingSystemBitness() == 32) {
		RtlSetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
		return 0;
	}

	switch (ImageFileMachineType) {
	case IMAGE_FILE_MACHINE_TARGET_HOST:
		Wow64Directory = L"\\system32";
		break;
	case IMAGE_FILE_MACHINE_I386:
		Wow64Directory = L"\\SysWOW64";
		break;
	default:
		RtlSetLastWin32Error(ERROR_BAD_ARGUMENTS);
		return 0;
	}

	GetSystemWindowsDirectoryW(Directory, ARRAYSIZE(Directory));
	Result = StringCchCatW(Directory, ARRAYSIZE(Directory), Wow64Directory);
	ASSERT (SUCCEEDED(Result));

	Result = StringCchCopyW(Buffer, Cch, Directory);
	
	if (FAILED(Result)) {
		RtlSetLastWin32Error(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	return (UINT) wcslen(Directory);
}