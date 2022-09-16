#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>
#include <BaseDll.h>

LPWSTR GetCommandLineWithoutImageName(
	VOID)
{
	BOOL bDoubleQuoted = FALSE;
	LPWSTR lpszRaw = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;

	while (*lpszRaw > ' ' || (*lpszRaw && bDoubleQuoted)) {
		if (*lpszRaw == '"') {
			bDoubleQuoted = !bDoubleQuoted;
		}

		lpszRaw++;
	}

	while (*lpszRaw && *lpszRaw <= ' ') {
		lpszRaw++;
	}

	return lpszRaw;
}

BOOLEAN IsOperatingSystem64Bit(
	VOID)
{
#ifdef _M_X64
	// Only 64-bit OS can run 64-bit programs. So if we are 64-bit, the OS is also
	// 64-bit.
	return TRUE;
#else
	BOOL CurrentProcessIsWow64Process;

	// I don't believe IsWow64Process can fail for the current process.
	// If you believe otherwise, feel free to report a bug.
	IsWow64Process(GetCurrentProcess(), &CurrentProcessIsWow64Process);

	if (CurrentProcessIsWow64Process) {
		// 32 bit OSes do not have the concept of "WOW64". So if we, a 32 bit process,
		// are a WOW64 process, that means the OS is 64 bit.
		return TRUE;
	} else {
		return FALSE;
	}
#endif
}

// Return TRUE/FALSE for success/failure and return result of calculation to Is64Bit
// pointer.
BOOLEAN IsProcess64Bit(
	IN	HANDLE		ProcessHandle,
	OUT	PBOOLEAN	Is64Bit)
{
	BOOLEAN OperatingSystemIs64Bit;
	BOOL ProcessIsWow64Process;

	if (!Is64Bit) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	OperatingSystemIs64Bit = IsOperatingSystem64Bit();

	if (!OperatingSystemIs64Bit) {
		// 32bit OS can't run 64 bit apps
		*Is64Bit = FALSE;
		return TRUE;
	}

	if (!IsWow64Process(ProcessHandle, &ProcessIsWow64Process)) {
		return FALSE;
	}

	if (ProcessIsWow64Process) {
		*Is64Bit = FALSE;
	} else {
		*Is64Bit = TRUE;
	}

	return TRUE;
}

// Find the size in bytes of an environment block (pass NULL for current process
// environment). This size includes all null terminators.
ULONG SizeOfEnvironmentBlock(
	IN	PVOID	Environment OPTIONAL)
{
	ULONG EnvironmentSize;
	PWCHAR EnvironmentWchars;
	BOOLEAN PebLocked = FALSE;

	if (!Environment) {
		//
		// Make sure nothing else changes the environment while we are reading
		// it.
		//
		RtlAcquirePebLock();
		PebLocked = TRUE;
		Environment = NtCurrentPeb()->ProcessParameters->Environment;
	}

	//
	// Scan through the environment block until we reach the end.
	//

	EnvironmentWchars = (PWCHAR) Environment;

	until (EnvironmentWchars[0] == '\0' && EnvironmentWchars[1] == '\0') {
		EnvironmentWchars++;
	}

	//
	// EnvironmentWchars pointer is now pointing to the two NULL characters at
	// the end of the environment block.
	//

	EnvironmentWchars += 2;
	EnvironmentSize = (ULONG) ((PBYTE) EnvironmentWchars - (PBYTE) Environment);

	if (PebLocked) {
		RtlReleasePebLock();
	}

	return EnvironmentSize;
}

// Make a copy of an environment block. Pass NULL as the Source parameter to
// use the current process environment block.
// You must pass the Destination environment block to **RtlDestroyEnvironment**
// after you are done with it.
BOOLEAN CloneEnvironmentBlock(
	IN	PVOID	Source OPTIONAL,
	OUT	PVOID	*Destination)
{
	NTSTATUS Status;
	SIZE_T EnvironmentSize;
	PVOID NewEnvironmentBlock;
	BOOLEAN PebLocked = FALSE;

	if (!Destination) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (!Source) {
		//
		// Lock the process environment while we're dealing with it
		//
		RtlAcquirePebLock();
		PebLocked = TRUE;
		Source = NtCurrentPeb()->ProcessParameters->Environment;
	}

	//
	// Find the size of the environment block and allocate the appropriate size
	// of memory for it
	//
	EnvironmentSize = SizeOfEnvironmentBlock(Source);
	
	Status = NtAllocateVirtualMemory(
		NtCurrentProcess(),
		&NewEnvironmentBlock,
		0,
		&EnvironmentSize,
		MEM_COMMIT,
		PAGE_READWRITE);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	//
	// Copy the source environment block into the new block
	//
	CopyMemory(NewEnvironmentBlock, Source, EnvironmentSize);

	//
	// Unlock environment if we locked it earlier
	//
	if (PebLocked) {
		RtlReleasePebLock();
	}

	*Destination = NewEnvironmentBlock;
	return TRUE;
}

// Same as GetEnvironmentVariableW except that it lets you specify which environment
// block to read values from. Dunno why they didn't put this function in Windows,
// considering it's so easy to implement.
DWORD GetEnvironmentVariableExW(
	IN	PCWSTR	Name,
	OUT	PWSTR	Buffer OPTIONAL,
	IN	ULONG	BufferSize,
	IN	PVOID	Environment OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING Name_U;
	UNICODE_STRING Value;
	ULONG Size;

	if (BufferSize > UNICODE_STRING_MAX_CHARS - 1) {
		Size = UNICODE_STRING_MAX_BYTES - sizeof(WCHAR);
	} else {
		if (BufferSize > 0) {
			Size = (BufferSize - 1) * sizeof(WCHAR);
		} else {
			Size = 0;
		}
	}

	Status = RtlInitUnicodeStringEx(&Name_U, Name);
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return 0;
	}

	Value.Buffer = Buffer;
	Value.Length = 0;
	Value.MaximumLength = (USHORT) Size;

	Status = RtlQueryEnvironmentVariable_U(Environment,
										   &Name_U,
										   &Value);
	if (NT_SUCCESS(Status) && BufferSize == 0) {
		Status = STATUS_BUFFER_TOO_SMALL;
	}

	if (NT_SUCCESS(Status)) {
		Buffer[Value.Length / sizeof(WCHAR)] = L'\0';
		return Value.Length / sizeof(WCHAR);
	} else {
		if (Status == STATUS_BUFFER_TOO_SMALL) {
			return Value.Length / sizeof(WCHAR) + 1;
		} else {
			BaseSetLastNTError(Status);
			return 0;
		}
	}
}

// same as SetEnvironmentVariableW but you can specify the environment block
// pass a null value for environment to use the process environment yada yada
BOOL SetEnvironmentVariableExW(
	IN	PCWSTR	Name,
	IN	PCWSTR	Value OPTIONAL,
	IN	PVOID	*Environment OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING Name_U;
	UNICODE_STRING Value_U;

	Status = RtlInitUnicodeStringEx(&Name_U, Name);
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	if (Value) {
		Status = RtlInitUnicodeStringEx(&Value_U, Value);
		if (!NT_SUCCESS(Status)) {
			BaseSetLastNTError(Status);
			return FALSE;
		}

		Status = RtlSetEnvironmentVariable(Environment, &Name_U, &Value_U);
	} else {
		Status = RtlSetEnvironmentVariable(Environment, &Name_U, NULL);
	}

	if (NT_SUCCESS(Status)) {
		return TRUE;
	} else {
		BaseSetLastNTError(Status);
		return FALSE;
	}
}