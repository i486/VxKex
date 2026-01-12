#include "buildcfg.h"
#include "kxcrypp.h"

//
// This function creates random data and places it into the specified
// buffer.
//

KXCRYPAPI BOOL WINAPI ProcessPrng(
	OUT	PBYTE	Buffer,
	IN	SIZE_T	BufferCb)
{
	NTSTATUS Status;

	ASSERT (BufferCb <= ULONG_MAX);

	if (BufferCb > ULONG_MAX) {
		return FALSE;
	}

	Status = KexRtlGenerateRandomData(Buffer, (ULONG) BufferCb);
	ASSERT (NT_SUCCESS(Status));

	return NT_SUCCESS(Status);
}