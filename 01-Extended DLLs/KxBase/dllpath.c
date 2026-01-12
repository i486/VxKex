#include "buildcfg.h"
#include "kxbasep.h"

VOID KxBaseAddKex3264ToBaseDefaultPath(
	VOID)
{
	HRESULT Result;
	NTSTATUS Status;
	PKERNELBASE_GLOBAL_DATA GlobalData;
	PWSTR NewBaseDefaultPathBuffer;
	ULONG NewBaseDefaultPathBufferCch;

	ASSERT (KexData != NULL);

	GlobalData = KernelBaseGetGlobalData();
	ASSERT (GlobalData != NULL);

	RtlAcquireSRWLockExclusive(GlobalData->BaseDefaultPathLock);

	try {
		NewBaseDefaultPathBufferCch =
			KexRtlUnicodeStringCch(&KexData->KexDir) +
			KexRtlUnicodeStringCch(GlobalData->BaseDefaultPath) +
			ARRAYSIZE(L"\\Kex00;"); // ARRAYSIZE includes the null terminator

		NewBaseDefaultPathBuffer = SafeAlloc(WCHAR, NewBaseDefaultPathBufferCch);
		ASSERT (NewBaseDefaultPathBuffer != NULL);

		if (NewBaseDefaultPathBuffer == NULL) {
			return;
		}

		Result = StringCchPrintf(
			NewBaseDefaultPathBuffer,
			NewBaseDefaultPathBufferCch,
			L"%wZ\\Kex%d;%wZ",
			&KexData->KexDir,
			KexRtlCurrentProcessBitness(),
			GlobalData->BaseDefaultPath);

		ASSERT (SUCCEEDED(Result));

		RtlFreeUnicodeString(GlobalData->BaseDefaultPath);

		Status = RtlInitUnicodeStringEx(
			GlobalData->BaseDefaultPath,
			NewBaseDefaultPathBuffer);

		ASSERT (NT_SUCCESS(Status));
	} finally {
		RtlReleaseSRWLockExclusive(GlobalData->BaseDefaultPathLock);
	}

	//
	// The search path is cached. This call will cause Kernel32 to invalidate
	// the search path cache.
	//

	SetDllDirectory(NULL);

	KexLogDebugEvent(
		L"BaseDefaultPath in KernelBaseGlobalData was successfully updated.\r\n\r\n"
		L"The new value is: \"%wZ\"",
		GlobalData->BaseDefaultPath);
}