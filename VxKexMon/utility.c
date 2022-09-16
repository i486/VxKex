#include <KexComm.h>

NORETURN VOID IncorrectUsageError(
	VOID)
{
	ULONG LastError = GetLastError();
	InfoBoxF(L"This application is not intended to be used standalone. To run a "
			 L"program under VxKex, use 'VXKEXLDR.EXE'."
#ifdef _DEBUG
			 L" Error code: 0x%08lx: %s", LastError, GetLastErrorAsString()
#endif
			 );
	ExitProcess(LastError);
}

BOOLEAN ReadValidateCommandLine(
	IN	PCWSTR		String,
	OUT	PULONG		ProcessId,
	OUT	PBOOLEAN	KexDllDirIs64Bit)
{
	if (!ProcessId || !KexDllDirIs64Bit) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	*ProcessId = 0;
	*KexDllDirIs64Bit = FALSE;

	if (!String) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (swscanf_s(String, L"%8lx %hhu", ProcessId, KexDllDirIs64Bit) != 1) {
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	// Process IDs are always divisible by 4 on Windows 7 and earlier.
	// PID 0 is reserved for system idle process.
	// PID 4 is reserved NT Kernel & System process.
	if (*ProcessId & 3 || *ProcessId == 0 || *ProcessId == 4) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	return TRUE;
}