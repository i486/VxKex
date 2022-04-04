#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>
#include <BaseDll.h>

// This function was written partially based on WinXP source, decompiled Win7 code,
// decompiled Win8 code and MSDN documentation. I haven't actually tested it, but
// hopefully it works. Lul. Anyway it's better than the low effort stub that was
// here before.
WINBASEAPI BOOL WINAPI GetOverlappedResultEx(
	IN	HANDLE					hFile,
	IN	VOLATILE LPOVERLAPPED	lpOverlapped,
	OUT	LPDWORD					lpNumberOfBytesTransferred,
	IN	DWORD					dwMilliseconds,
	IN	BOOL					bAlertable)
{
	ODS_ENTRY(L"(%p, %p, %p, %I32u, %d)", hFile, lpOverlapped, lpNumberOfBytesTransferred, dwMilliseconds, bAlertable);

	if (((DWORD) lpOverlapped->Internal) == (DWORD) STATUS_PENDING) {
		if (dwMilliseconds) {
			DWORD dwWaitReturn;

			dwWaitReturn = WaitForSingleObjectEx((lpOverlapped->hEvent != NULL) ? lpOverlapped->hEvent : hFile,
												 dwMilliseconds, bAlertable);

			if (dwWaitReturn != WAIT_OBJECT_0) {
				// Last Win32 error has been set by WaitForSingleObjectEx
				return FALSE;
			}
		} else {
			RtlSetLastWin32Error(ERROR_IO_INCOMPLETE);
			return FALSE;
		}
	}

	*lpNumberOfBytesTransferred = (DWORD) lpOverlapped->InternalHigh;

	if (NT_SUCCESS(lpOverlapped->Internal)) {
		return TRUE;
	} else {
		BaseSetLastNTError(lpOverlapped->Internal);
		return FALSE;
	}
}