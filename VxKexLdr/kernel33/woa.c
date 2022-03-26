#include <Windows.h>
#include <KexDll.h>

#include "k32defs.h"

//
// Contains the implementation for WaitOnAddress and related functions
//

typedef struct _ACVAHASHTABLEADDRESSLISTENTRY {
	LIST_ENTRY			ListEntry; // MUST BE THE FIRST MEMBER OF THIS STRUCTURE
	VOLATILE LPVOID		lpAddr;
	CONDITION_VARIABLE	CVar;
	DWORD				dwWaiters;
} ACVAHASHTABLEADDRESSLISTENTRY, *PACVAHASHTABLEADDRESSLISTENTRY, *LPACVAHASHTABLEADDRESSLISTENTRY;

typedef struct _ACVAHASHTABLEENTRY {
	LIST_ENTRY			Addresses;	// list of ACVAHASHTABLEADDRESSLISTENTRY structures
	CRITICAL_SECTION	Lock;
} ACVAHASHTABLEENTRY, *PACVAHASHTABLEENTRY, *LPACVAHASHTABLEENTRY;

ACVAHASHTABLEENTRY WaitOnAddressHashTable[256];

VOID DllMain_InitWoa(
	VOID)
{
	INT i;

	for (i = 0; i < ARRAYSIZE(WaitOnAddressHashTable); ++i) {
		LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[i];
		InitializeCriticalSection(&lpHashTableEntry->Lock);
		InitializeListHead(&lpHashTableEntry->Addresses);
	}
}

STATIC INLINE INT HashAddress(
	IN	LPVOID	lpAddr)
{
	return (((ULONG_PTR) lpAddr) / 8) % ARRAYSIZE(WaitOnAddressHashTable);
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

	ODS_ENTRY(T("(%p, %p, %Iu, %I32u)"), lpAddr, lpCompare, cb, dwMilliseconds);

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

WINBASEAPI VOID WakeByAddressSingle(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;
	
	ODS_ENTRY(T("(%p)"), lpAddr);

	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		WakeConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}

WINBASEAPI VOID WakeByAddressAll(
	IN	LPVOID	lpAddr)
{
	LPACVAHASHTABLEENTRY lpHashTableEntry = &WaitOnAddressHashTable[HashAddress(lpAddr)];
	LPACVAHASHTABLEADDRESSLISTENTRY lpListEntry;

	ODS_ENTRY(T("(%p)"), lpAddr);
	
	EnterCriticalSection(&lpHashTableEntry->Lock);
	lpListEntry = FindACVAListEntryForAddress(lpHashTableEntry, lpAddr);

	if (lpListEntry) {
		WakeAllConditionVariable(&lpListEntry->CVar);
	}

	LeaveCriticalSection(&lpHashTableEntry->Lock);
}