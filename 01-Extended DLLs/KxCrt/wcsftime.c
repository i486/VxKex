#include "buildcfg.h"
#include <KexComm.h>

//
// The functions in this file mostly proxy over to the _W_ functions in
// ucrtbase.dll. The reason we need proxy functions and not just export
// forwarders is because msvcrt.dll and ucrtbase.dll use different private
// heaps, and using the wrong one will cause a crash.
//

typedef PWSTR (CDECL *P_W_GETDAYS)(VOID);
typedef PWSTR (CDECL *P_W_GETMONTHS)(VOID);
typedef PWSTR (CDECL *P_W_GETTNAMES)(VOID);
typedef VOID (CDECL *PFREE)(PVOID);

STATIC NTSTATUS GetUcrtProcAddress(
	IN	PCSTR		ProcedureName,
	OUT	PPVOID		ProcedureAddress)
{
	NTSTATUS Status;
	STATIC PVOID UcrtDllHandle = NULL;
	ANSI_STRING ProcedureNameAS;

	ASSUME (ProcedureName != NULL);
	ASSUME (ProcedureAddress != NULL);

	*ProcedureAddress = NULL;

	if (!UcrtDllHandle) {
		UNICODE_STRING UcrtDllName;

		RtlInitConstantUnicodeString(&UcrtDllName, L"ucrtbase.dll");

		Status = LdrLoadDll(
			NULL,
			NULL,
			&UcrtDllName,
			&UcrtDllHandle);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	ASSUME (UcrtDllHandle != NULL);

	Status = RtlInitAnsiStringEx(&ProcedureNameAS, ProcedureName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = LdrGetProcedureAddress(
		UcrtDllHandle,
		&ProcedureNameAS,
		0,
		ProcedureAddress);

	ASSERT (NT_SUCCESS(Status));

	return Status;
}

STATIC NTSTATUS TransferToMsvcrtHeap(
	IN OUT	PPVOID	Pointer,
	IN		SIZE_T	Size)
{
	NTSTATUS Status;
	PVOID NewPointer;
	STATIC PFREE UcrtFree = NULL;

	ASSUME (Pointer != NULL);
	ASSUME (*Pointer != NULL);
	ASSUME (Size != 0);

	if (!UcrtFree) {
		Status = GetUcrtProcAddress("free", (PPVOID) &UcrtFree);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	ASSUME (UcrtFree != NULL);

	NewPointer = malloc(Size);
	ASSERT (NewPointer != NULL);

	if (!NewPointer) {
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(NewPointer, *Pointer, Size);
	UcrtFree(*Pointer);
	*Pointer = NewPointer;

	return STATUS_SUCCESS;
}

PWSTR CDECL _W_Getdays(
	VOID)
{
	NTSTATUS Status;
	PWSTR UcrtGetdaysResult;
	STATIC P_W_GETDAYS UcrtWGetdays = NULL;

	if (!UcrtWGetdays) {
		Status = GetUcrtProcAddress("_W_Getdays", (PPVOID) &UcrtWGetdays);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return NULL;
		}
	}

	ASSUME (UcrtWGetdays != NULL);

	UcrtGetdaysResult = UcrtWGetdays();
	ASSERT (UcrtGetdaysResult != NULL);

	if (!UcrtGetdaysResult) {
		return NULL;
	}

	Status = TransferToMsvcrtHeap(
		(PPVOID) &UcrtGetdaysResult,
		(wcslen(UcrtGetdaysResult) + 1) * sizeof(WCHAR));

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	return UcrtGetdaysResult;
}

PWSTR CDECL _W_Getmonths(
	VOID)
{
	NTSTATUS Status;
	PWSTR UcrtGetmonthsResult;
	STATIC P_W_GETMONTHS UcrtWGetmonths = NULL;

	if (!UcrtWGetmonths) {
		Status = GetUcrtProcAddress("_W_Getmonths", (PPVOID) &UcrtWGetmonths);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return NULL;
		}
	}

	ASSUME (UcrtWGetmonths != NULL);

	UcrtGetmonthsResult = UcrtWGetmonths();
	ASSERT (UcrtGetmonthsResult != NULL);

	if (!UcrtGetmonthsResult) {
		return NULL;
	}

	Status = TransferToMsvcrtHeap(
		(PPVOID) &UcrtGetmonthsResult,
		(wcslen(UcrtGetmonthsResult) + 1) * sizeof(WCHAR));

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	return UcrtGetmonthsResult;
}

PWSTR CDECL _W_Gettnames(
	VOID)
{
	NTSTATUS Status;
	PWSTR UcrtGettnamesResult;
	STATIC P_W_GETTNAMES UcrtWGettnames = NULL;

	if (!UcrtWGettnames) {
		Status = GetUcrtProcAddress("_W_Gettnames", (PPVOID) &UcrtWGettnames);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return NULL;
		}
	}

	ASSUME (UcrtWGettnames != NULL);

	UcrtGettnamesResult = UcrtWGettnames();
	ASSERT (UcrtGettnamesResult != NULL);

	if (!UcrtGettnamesResult) {
		return NULL;
	}

	Status = TransferToMsvcrtHeap(
		(PPVOID) &UcrtGettnamesResult,
		(wcslen(UcrtGettnamesResult) + 1) * sizeof(WCHAR));

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	return UcrtGettnamesResult;
}

//
// A note on _Wcsftime: This function is implemented by calling _Wcsftime_l,
// which is present but non-exported in Win7 msvcrt. We can grab its location
// out of the functions wcsftime or _wcsftime_l, which are just stubs that
// call _Wcsftime_l.
//
// I will only bother doing that if there are actually apps that call it,
// though.
//