#include <Windows.h>
#include <KexDll.h>

#include "synch.h"

// The size of this hash table can be freely tuned.
// Each ACVAHASHTABLEENTRY structure takes up 64 bytes on x64 and 32 bytes on x86.
//
// Number of entries in hash table | Memory used x64 / x86
//    1 (i.e. no hash table)       |   64B / 32B
//    2                            |  128B / 64B
//    4                            |  256B / 128B
//    8                            |  512B / 256B
//   16                            |   1KB / 512B
//   32                            |   2KB / 1KB
//   64                            |   4KB / 2KB
//  128                            |   8KB / 4KB
//  256                            |  16KB / 8KB

ACVAHASHTABLEENTRY WaitOnAddressHashTable[16];

VOID DllMain_InitWoa(
	VOID)
{
	INT i;

	ODS_ENTRY();

	for (i = 0; i < ARRAYSIZE(WaitOnAddressHashTable); ++i) {
		LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[i];
		InitializeCriticalSection(&lpHashTableEntry->Lock);
		InitializeListHead(&lpHashTableEntry->Addresses);
	}
}

STATIC INLINE INT HashAddress(
	IN	LPVOID	lpAddr)
{
	return (((ULONG_PTR) lpAddr) >> 4) % ARRAYSIZE(WaitOnAddressHashTable);
}

// Return TRUE if memory is the same. FALSE if memory is different.
#pragma warning(disable:4715) // not all control paths return a value
STATIC INLINE BOOL CompareVolatileMemory(
	IN	CONST VOLATILE LPVOID A1,
	IN	CONST LPVOID A2,
	IN	SIZE_T size)
{
	ASSUME(size == 1 || size == 2 || size == 4 || size == 8);

	switch (size) {
	case 1:		return (*(CONST LPBYTE)A1 == *(CONST LPBYTE)A2);
	case 2:		return (*(CONST LPWORD)A1 == *(CONST LPWORD)A2);
	case 4:		return (*(CONST LPDWORD)A1 == *(CONST LPDWORD)A2);
	case 8:		return (*(CONST LPQWORD)A1 == *(CONST LPQWORD)A2);
	}
}
#pragma warning(default:4715)

STATIC LPACVAHASHTABLEADDRESSLISTENTRY FindACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;

	ForEachListEntry(&lpHashTableEntry->Addresses, lpListEntry) {
		if (lpListEntry->lpAddr == lpAddr) {
			return lpListEntry;
		}
	}

	return NULL;
}

STATIC INLINE LPACVAHASHTABLEADDRESSLISTENTRY CreateACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	lpListEntry = (LPACVAHASHTABLEADDRESSLISTENTRY) HeapAlloc(GetProcessHeap(), 0, sizeof(ACVAHASHTABLEADDRESSLISTENTRY));

	if (!lpListEntry) {
		return NULL;
	}

	lpListEntry->lpAddr = lpAddr;
	lpListEntry->dwWaiters = 0;
	InitializeConditionVariable(&lpListEntry->CVar);
	InsertHeadList(&lpHashTableEntry->Addresses, (PLIST_ENTRY) lpListEntry);

	return lpListEntry;
}

STATIC INLINE LPACVAHASHTABLEADDRESSLISTENTRY FindOrCreateACVAListEntryForAddress(
	IN	LPACVAHASHTABLEENTRY	lpHashTableEntry,
	IN	LPVOID					lpAddr)
{
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (!lpListEntry) {
		lpListEntry = CreateACVAListEntryForAddress(lpHashTableEntry, lpAddr);
	}

	return lpListEntry;
}

STATIC INLINE VOID DeleteACVAListEntry(
	IN	LPACVAHASHTABLEADDRESSLISTENTRY	lpListEntry)
{
	RemoveEntryList((PLIST_ENTRY) lpListEntry);
	HeapFree(GetProcessHeap(), 0, lpListEntry);
}

WINBASEAPI BOOL WINAPI WaitOnAddress(
	IN	VOLATILE LPVOID	lpAddr,					// address to wait on
	IN	LPVOID			lpCompare,				// pointer to location of old value of lpAddr
	IN	SIZE_T			cb,						// number of bytes to compare
	IN	DWORD			dwMilliseconds OPTIONAL)// maximum number of milliseconds to wait
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	DWORD dwLastError;
	BOOL bSuccess;

	ODS_ENTRY(L"(%p, %p, %Iu, %I32u)", lpAddr, lpCompare, cb, dwMilliseconds);

	if (!lpAddr || !lpCompare) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	} else if (!(cb == 1 || cb == 2 || cb == 4 || cb == 8)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	EnterCriticalSection(&lpHashTableEntry->Lock);
	
	if (!CompareVolatileMemory(lpAddr, lpCompare, cb)) {
		LeaveCriticalSection(&lpHashTableEntry->Lock);
		SetLastError(ERROR_SUCCESS);
		return TRUE;
	}

	lpListEntry = FindOrCreateACVAListEntryForAddress(lpHashTableEntry, lpAddr);
	lpListEntry->dwWaiters++;
	bSuccess = SleepConditionVariableCS(&lpListEntry->CVar, &lpHashTableEntry->Lock, dwMilliseconds);
	dwLastError = GetLastError();

	if (--lpListEntry->dwWaiters == 0) {
		DeleteACVAListEntry(lpListEntry);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
	SetLastError(dwLastError);
	return bSuccess;
}

WINBASEAPI VOID WINAPI WakeByAddressSingle(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	
	ODS_ENTRY(L"(%p)", lpAddr);

	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		WakeConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}

WINBASEAPI VOID WINAPI WakeByAddressAll(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;

	ODS_ENTRY(L"(%p)", lpAddr);
	
	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		WakeAllConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}