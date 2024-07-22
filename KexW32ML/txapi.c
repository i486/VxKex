#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC HANDLE KW32MLAPI CreateSimpleTransaction(
	IN	PCWSTR	Description OPTIONAL)
{
	NTSTATUS Status;
	HANDLE TransactionHandle;
	UNICODE_STRING DescriptionUS;

	if (Description) {
		Status = RtlInitUnicodeStringEx(&DescriptionUS, Description);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			SetLastError(RtlNtStatusToDosError(Status));
			return NULL;
		}
	}

	Status = NtCreateTransaction(
		&TransactionHandle,
		TRANSACTION_ALL_ACCESS,
		NULL,
		NULL,
		NULL,
		0,
		0,
		0,
		NULL,
		Description ? &DescriptionUS : NULL);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		SetLastError(RtlNtStatusToDosError(Status));
		return NULL;
	}

	ASSERT (TransactionHandle != NULL);
	return TransactionHandle;
}