#include <KexComm.h>
#include <KexData.h>
#include <BaseDll.h>
#include <NtDll.h>
#include "VxKexMon.h"

#include <Shlwapi.h>

BOOLEAN AdjustRemoteKexDllDir(
	IN	HANDLE	ProcessHandle,
	IN	ULONG	BitnessFrom,
	IN	ULONG	BitnessTo)
{
	PVOID Environment;
	PWSTR Path;
	ULONG PathLength;
	PWSTR PathKexDllDir;
	WCHAR KexDir[MAX_PATH];
	WCHAR ExistingKexDllDir[MAX_PATH];
	WCHAR NewKexDllDir[MAX_PATH];
	PWCH NewKexDllDirEnd;
	SIZE_T NewKexDllDirLength;
	HRESULT Result;
	BOOLEAN Success;

	Success = FALSE;

	//
	// param validation
	//
	unless ((BitnessFrom == 32 || BitnessFrom == 64) && (BitnessTo == 32 || BitnessTo == 64)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (BitnessFrom == BitnessTo) {
		// Nothing to do.
		return TRUE;
	}

	//
	// Fetch environment from the remote process
	//
	if (!CloneRemoteEnvironmentBlock(ProcessHandle, &Environment)) {
		return FALSE;
	}

	//
	// Extract Path variable from the copied environment
	//

	PathLength = GetEnvironmentVariableExW(L"Path", NULL, 0, Environment);
	if (PathLength == 0) {
		goto CleanupExit;
	}

	Path = (PWSTR) StackAlloc(PathLength * sizeof(WCHAR));

	PathLength = GetEnvironmentVariableExW(L"Path", Path, PathLength * sizeof(WCHAR), Environment);
	if (PathLength == 0) {
		goto CleanupExit;
	}

	//
	// Find where the KexDllDir is and replace it with one with the correct bitness
	//
	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", KexDir, ARRAYSIZE(KexDir))) {
		goto CleanupExit;
	}

	Result = StringCchPrintf(
		ExistingKexDllDir,
		ARRAYSIZE(ExistingKexDllDir),
		L"%s\\Kex%lu", KexDir, BitnessFrom);

	if (FAILED(Result)) {
		SetLastError(WIN32_FROM_HRESULT(Result));
		goto CleanupExit;
	}

	Result = StringCchPrintfEx(
		NewKexDllDir,
		ARRAYSIZE(NewKexDllDir),
		&NewKexDllDirEnd,
		NULL,
		0,
		L"%s\\Kex%lu", KexDir, BitnessTo);

	if (FAILED(Result)) {
		SetLastError(WIN32_FROM_HRESULT(Result));
		goto CleanupExit;
	}

	PathKexDllDir = StrStrIW(Path, ExistingKexDllDir);
	if (!PathKexDllDir) {
		SetLastError(ERROR_INVALID_DATA);
		goto CleanupExit;
	}

	NewKexDllDirLength = NewKexDllDirEnd - NewKexDllDir;
	CopyMemory(PathKexDllDir, NewKexDllDir, NewKexDllDirLength);
	
	if (!SetEnvironmentVariableExW(L"Path", Path, &Environment)) {
		goto CleanupExit;
	}

	//
	// Update environment of target process
	//
	if (!UpdateRemoteEnvironmentBlock(ProcessHandle, Environment)) {
		goto CleanupExit;
	}

	Success = TRUE;

CleanupExit:
	RtlDestroyEnvironment(Environment);
	return Success;
}

//
// Initialize a KEX_PROCESS_DATA structure and place it in the remote address
// space of a remote process, as well as filling out a SubSystemData pointer
// in the remote process's PEB as required by VxKex DLLs.
//
// You may also pass a KexDataOut pointer to obtain a copy of the structure
// for your own usage.
//
BOOLEAN InitializeRemoteKexData(
	IN	HANDLE				ProcessHandle,
	IN	PCWSTR				ProcessImageFullName,
	OUT	PKEX_PROCESS_DATA	KexDataOut OPTIONAL)
{
	KEX_PROCESS_DATA KexData;
	ULONG_PTR VaKexData;
	ULONG_PTR VaRemotePeb;
	ULONG_PTR VaSubSystemData;
	SIZE_T SizeOfKexData;
	NTSTATUS Status;

	//
	// Set up KexData structure
	//

	if (!KexReadIfeoParameters(ProcessImageFullName, &KexData.IfeoParameters)) {
		return FALSE;
	}

	if (!RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", KexData.KexDir, ARRAYSIZE(KexData.KexDir))) {
		return FALSE;
	}

	KexData.ProcessInitializationComplete = FALSE;

	//
	// Allocate memory and copy KexData into the remote process
	//

	SizeOfKexData = sizeof(KexData);
	Status = NtAllocateVirtualMemory(
		ProcessHandle,
		(PPVOID) &VaKexData,
		0,
		&SizeOfKexData,
		MEM_COMMIT,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	if (!VaWrite(ProcessHandle, VaKexData, &KexData, sizeof(KexData))) {
		return FALSE;
	}

	//
	// Place a pointer to KexData into PEB->SubSystemData in the remote process
	//

	if (!GetRemotePebVa(ProcessHandle, &VaRemotePeb)) {
		return FALSE;
	}

	VaSubSystemData = VaRemotePeb + FIELD_OFFSET(PEB, SubSystemData);

	if (!VaWriteP(ProcessHandle, VaSubSystemData, VaKexData)) {
		return FALSE;
	}

	if (KexDataOut) {
		*KexDataOut = KexData;
	}

	return TRUE;
}

BOOLEAN UpdateRemoteKexData(
	IN	HANDLE				ProcessHandle,
	IN	PKEX_PROCESS_DATA	NewKexData)
{
	ULONG_PTR VaRemotePeb;
	ULONG_PTR VaSubSystemData;
	ULONG_PTR VaRemoteKexData;

	if (!NewKexData) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!GetRemotePebVa(ProcessHandle, &VaRemotePeb)) {
		return FALSE;
	}

	VaSubSystemData = VaRemotePeb + FIELD_OFFSET(PEB, SubSystemData);

	if (!VaRead(ProcessHandle, VaSubSystemData, &VaRemoteKexData, sizeof(ULONG_PTR))) {
		return FALSE;
	}

	if (!VaWrite(ProcessHandle, VaRemoteKexData, NewKexData, sizeof(*NewKexData))) {
		return FALSE;
	}

	return TRUE;
}