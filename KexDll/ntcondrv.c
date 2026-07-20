///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntcondrv.c
//
// Abstract:
//
//     Extensions for system calls related to files and handles.
//
//     This is to support applications such as Zig which decide to pass console
//     handles directly to kernel calls.
//
// Author:
//
//     vxiiduu (05-Jul-2026)
//
// Revision History:
//
//     vxiiduu              05-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC PVOID ResolveKxBaseFunction(
	IN		PCSTR			Procedure)
{
	NTSTATUS Status;
	ANSI_STRING ProcedureName;
	STATIC PVOID DllHandle = NULL;
	PVOID ProcedureAddress;

	if (!DllHandle) {
		UNICODE_STRING DllName;

		RtlInitConstantUnicodeString(&DllName, L"KxBase.dll");

		Status = LdrLoadDll(
			NULL,
			NULL,
			&DllName,
			&DllHandle);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return NULL;
		}
	}

	RtlInitAnsiString(&ProcedureName, Procedure);

	Status = LdrGetProcedureAddress(
		DllHandle,
		&ProcedureName,
		0,
		(PPVOID) &ProcedureAddress);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	return ProcedureAddress;
}

STATIC BOOL KxBaseWriteConsoleA(
	IN		HANDLE			FileHandle,
	IN		PCVOID			Buffer,
	IN		ULONG			NumberOfBytesToWrite,
	OUT		PULONG			NumberOfBytesWritten OPTIONAL,
	IN OUT	LPOVERLAPPED	Overlapped OPTIONAL)
{
	STATIC BOOL (WINAPI *pKxBaseWriteConsoleA)(HANDLE, PCVOID, ULONG, PULONG, LPOVERLAPPED) = NULL;

	//
	// Get a pointer to WriteFile from kxbase if we don't already have it.
	//

	if (!pKxBaseWriteConsoleA) {
		pKxBaseWriteConsoleA = ResolveKxBaseFunction("WriteConsoleA");

		//
		// Still couldn't find the function. Fail.
		//

		if (!pKxBaseWriteConsoleA) {
			RtlSetLastWin32Error(ERROR_FUNCTION_NOT_CALLED);
			return FALSE;
		}
	}

	return pKxBaseWriteConsoleA(
		FileHandle,
		Buffer,
		NumberOfBytesToWrite,
		NumberOfBytesWritten,
		Overlapped);
}

STATIC BOOL KxBaseCloseConsoleHandle(
	IN		HANDLE			Handle)
{
	STATIC BOOL (WINAPI *pKxBaseCloseConsoleHandle)(HANDLE) = NULL;

	if (!pKxBaseCloseConsoleHandle) {
		pKxBaseCloseConsoleHandle = ResolveKxBaseFunction("CloseConsoleHandle");

		if (!pKxBaseCloseConsoleHandle) {
			RtlSetLastWin32Error(ERROR_FUNCTION_NOT_CALLED);
			return FALSE;
		}
	}

	return pKxBaseCloseConsoleHandle(Handle);
}

KEXAPI NTSTATUS NTAPI Ext_NtWriteFile(
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL)
{
	if ((KexData->Flags & KEXDATA_FLAG_CONDRV_EMULATION) && IsConsoleHandle(FileHandle)) {
		PTEB Teb;
		NTSTATUS OldLastStatus;
		ULONG OldLastError;

		//
		// Remember that it's completely valid for an application to pass a
		// handle to NtWriteFile that looks like a console handle (has handle tag
		// bits set), so this code needs to be resilient against such things and
		// bail out to the original function when something can't be handled.
		//

		if (Event || ApcRoutine || ApcContext || Key) {
			ASSERT (("Unexpected argument", FALSE));
			goto BailOut;
		}

		// deliberately ignore, it seems to be specified sometimes but we don't need
		// to care about it
		UNREFERENCED_PARAMETER (ByteOffset);

		if (!IoStatusBlock || !Buffer || !Length) {
			ASSERT (("Invalid argument", FALSE));
			goto BailOut;
		}

		try {
			try {
				BOOL Success;
				IO_STATUS_BLOCK IoStatusBlockTemp;

				KexRtlZeroMemory(&IoStatusBlockTemp, sizeof(IoStatusBlockTemp));

				// Save and restore last-status and last-error, since syscalls aren't
				// expected to modify those.
				Teb = NtCurrentTeb();
				OldLastStatus = Teb->LastStatusValue;
				OldLastError = Teb->LastErrorValue;

				Success = KxBaseWriteConsoleA(
					FileHandle,
					Buffer,
					Length,
					(PULONG) &IoStatusBlockTemp.Information,
					NULL);

				if (!Success) {
					NTSTATUS Status;
					ULONG LastError;

					//
					// possible causes:
					//   - app passes a real file handle that simply has tag bits set
					//   - we screwed up translating the parameters
					//   - app passed invalid parameters to us
					//

					LastError = RtlGetLastWin32Error();
					KexDebugCheckpoint();

					if (LastError == ERROR_INVALID_HANDLE) {
						// Maybe not a console handle.
						goto BailOut;
					}

					ASSERT (("Unexpected last-error code", FALSE));

					Status = Teb->LastStatusValue;
					IoStatusBlockTemp.Status = Status;
					*IoStatusBlock = IoStatusBlockTemp;

					return Status;
				}

				*IoStatusBlock = IoStatusBlockTemp;
			} finally {
				Teb->LastStatusValue = OldLastStatus;
				Teb->LastErrorValue = OldLastError;
			}
		} except (EXCEPTION_EXECUTE_HANDLER) {
			return GetExceptionCode();
		}

		return STATUS_SUCCESS;
	}

BailOut:
	return KexNtWriteFile(
		FileHandle,
		Event,
		ApcRoutine,
		ApcContext,
		IoStatusBlock,
		Buffer,
		Length,
		ByteOffset,
		Key);
}

KEXAPI NTSTATUS NTAPI Ext_NtClose(
	IN		HANDLE				Handle)
{
	if ((KexData->Flags & KEXDATA_FLAG_CONDRV_EMULATION) && IsConsoleHandle(Handle)) {
		NTSTATUS Status;
		BOOL Success;
		NTSTATUS OldLastStatus;
		ULONG OldLastError;
		PTEB Teb;

		Teb = NtCurrentTeb();
		OldLastStatus = Teb->LastStatusValue;
		OldLastError = Teb->LastErrorValue;

		Success = KxBaseCloseConsoleHandle(Handle);

		Status = Teb->LastStatusValue;
		Teb->LastStatusValue = OldLastStatus;
		Teb->LastErrorValue = OldLastError;

		if (!Success) {
			return Status;
		}

		return STATUS_SUCCESS;
	}

	return NtClose(Handle);
}