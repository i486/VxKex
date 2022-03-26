#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>

typedef enum _AUTOALLOCTYPE {
	AutoInvalidAlloc,
	AutoStackAlloc,
	AutoHeapAlloc
} AUTOALLOCTYPE, *PAUTOALLOCTYPE, *LPAUTOALLOCTYPE;

LPVOID __AutoHeapAllocHelper(
	IN	SIZE_T	cb)
{
	LPBYTE lpb = (LPBYTE) HeapAlloc(NtCurrentPeb()->ProcessHeap, 0, cb);

	if (!lpb) {
		return NULL;
	}

	*lpb = AutoHeapAlloc;
	return (LPVOID) (lpb + 1);
}

LPVOID __AutoStackAllocHelper(
	IN	LPVOID	lpv)
{
	LPBYTE lpb = (LPBYTE) lpv;
	*lpb = AutoStackAlloc;
	return (LPVOID) (lpb + 1);
}

VOID AutoFree(
	IN	LPVOID	lpv)
{
	LPBYTE lpb = ((LPBYTE) lpv) - 1;

	if (lpv != NULL && *lpb == AutoHeapAlloc) {
		HeapFree(NtCurrentPeb()->ProcessHeap, 0, lpb);
	}
}