#include "buildcfg.h"
#include "kxcrypp.h"

//
// This is just a convenience function which wraps other functions in bcrypt.dll.
//

KXCRYPAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PBYTE				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PBYTE				Input,
	IN	ULONG				InputCb,
	OUT	PBYTE				Output,
	IN	ULONG				OutputCb)
{
	NTSTATUS Status;
	BCRYPT_HASH_HANDLE HashHandle;

	HashHandle = NULL;

	Status = BCryptCreateHash(Algorithm, &HashHandle, 0, 0, Secret, SecretCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptHashData(HashHandle, Input, InputCb, 0);
	if (!NT_SUCCESS(Status)) {
		goto CleanupExit;
	}

	Status = BCryptFinishHash(HashHandle, Output, OutputCb, 0);

CleanupExit:
	if (HashHandle) {
		BCryptDestroyHash(HashHandle);
	}

	return Status;
}