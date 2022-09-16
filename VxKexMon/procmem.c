#include <KexComm.h>
#include <BaseDll.h>
#include <NtDll.h>

//
// Functions for reading and writing memory of a remote process.
//

BOOLEAN VaRead(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PVOID		Buffer,
	IN	SIZE_T		Size)
{
	NTSTATUS Status;

	if (Size == 0 || Buffer == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	Status = NtReadVirtualMemory(
		ProcessHandle,
		(PVOID) VirtualAddress,
		Buffer,
		Size,
		NULL);

	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}

BOOLEAN VaReadSzA(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PSTR		Buffer,
	IN	SIZE_T		MaxCch,
	OUT	PSIZE_T		Cch OPTIONAL)
{
	CHAR Character;

	if (Cch) {
		*Cch = 0;
	}

	if (!Buffer || !MaxCch) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	do {
		BOOL Success;

		Success = VaRead(ProcessHandle, VirtualAddress, &Character, sizeof(Character));
		if (!Success) {
			return FALSE;
		}

		if (Cch) {
			++(*Cch);
		}

		*Buffer++ = Character;
	} while (Character != '\0' && --MaxCch != 0);

	return TRUE;
}

BOOLEAN VaReadSzW(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PWSTR		Buffer,
	IN	SIZE_T		MaxCch,
	OUT	PSIZE_T		Cch OPTIONAL)
{
	WCHAR Character;

	if (Cch) {
		*Cch = 0;
	}

	if (!Buffer || !MaxCch) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	do {
		BOOLEAN Success;

		Success = VaRead(ProcessHandle, VirtualAddress, &Character, sizeof(Character));
		if (!Success) {
			return FALSE;
		}

		if (Cch) {
			++(*Cch);
		}

		*Buffer++ = Character;
	} while (Character != '\0' && --MaxCch != 0);

	return TRUE;
}

BOOLEAN VaWrite(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	PCVOID		Buffer,
	IN	SIZE_T		Size)
{
	NTSTATUS Status;
	ULONG OldProtect;
	BOOLEAN ProtectSuccess;

	if (Size == 0 || Buffer == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//
	// In many situations the memory will be read-only so we will first set
	// access protections. This is required, for example, when writing to code
	// area.
	//
	Status = NtProtectVirtualMemory(
		ProcessHandle,
		(PPVOID) &VirtualAddress,
		&Size,
		PAGE_READWRITE,
		&OldProtect);

	ProtectSuccess = NT_SUCCESS(Status);

	Status = NtWriteVirtualMemory(
		ProcessHandle,
		(PVOID) VirtualAddress,
		Buffer,
		Size,
		NULL);

	//
	// We will restore the old page protections only if we succeeded to change them
	// earlier.
	//
	if (ProtectSuccess) {
		NtProtectVirtualMemory(
			ProcessHandle,
			(PPVOID) &VirtualAddress,
			&Size,
			OldProtect,
			&OldProtect);
	} else {
		// log info
	}

	//
	// Fail only if the actual write failed - note that the above NtProtectVirtualMemory
	// call does not set Status
	//
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
		return FALSE;
	}

	return TRUE;
}

BOOLEAN VaWrite1(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	BYTE		Data)
{
	return VaWrite(ProcessHandle, VirtualAddress, &Data, sizeof(Data));
}

BOOLEAN VaWrite2(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	WORD		Data)
{
	return VaWrite(ProcessHandle, VirtualAddress, &Data, sizeof(Data));
}

BOOLEAN VaWrite4(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	DWORD		Data)
{
	return VaWrite(ProcessHandle, VirtualAddress, &Data, sizeof(Data));
}

BOOLEAN VaWrite8(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	QWORD		Data)
{
	return VaWrite(ProcessHandle, VirtualAddress, &Data, sizeof(Data));
}