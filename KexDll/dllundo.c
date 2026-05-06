///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllundo.c
//
// Abstract:
//
//     Functions for dealing with undoing DLL rewriting.
//     The code in this file is not currently used because it was written as
//     part of an experiment related to Themida applications; however, it turned
//     out that it was not necessary to undo DLL rewriting.
//
//     Additional notes on Themida:
//       - Not redirecting the first GetModuleHandleA call to a Themida application
//         seems to allow it to run. My hypothesis is that it manually parses dll
//         exports a la Chromium.
//       - KakaoTalk works now.
//       - Unable to get LINE to work.
//
// Author:
//
//     vxiiduu (27-Apr-2026)
//
// Revision History:
//
//     vxiiduu              27-Apr-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS KexInitializeUndoList(
	OUT		PKEX_DLL_REWRITE_UNDO_LIST	UndoList)
{
	ASSERT (UndoList != NULL);

	UndoList->BytesUsed = 0;
	UndoList->BytesAllocated = 64;
	UndoList->UndoEntries = RtlAllocateHeap(RtlProcessHeap(), 0, UndoList->BytesAllocated);

	ASSERT (UndoList->UndoEntries != NULL);

	if (UndoList->UndoEntries == NULL) {
		RtlZeroMemory(UndoList, sizeof(*UndoList));
		return STATUS_NO_MEMORY;
	}

	return STATUS_SUCCESS;
}

VOID KexFreeUndoList(
	IN OUT	PKEX_DLL_REWRITE_UNDO_LIST	UndoList)
{
	ASSERT (UndoList != NULL);
	RtlFreeHeap(RtlProcessHeap(), 0, UndoList->UndoEntries);
	RtlZeroMemory(UndoList, sizeof(*UndoList));
}

NTSTATUS KexAddEntryUndoList(
	IN		PKEX_DLL_REWRITE_UNDO_LIST	UndoList,
	IN		PVOID						Location,
	IN		ULONG						NumberOfBytes,
	IN		PCVOID						UndoData)
{
	ULONG BytesNeeded;
	ULONG BytesFree;
	PKEX_DLL_REWRITE_UNDO_ENTRY NewEntry;

	ASSERT (UndoList != NULL);
	ASSERT (Location != NULL);
	ASSERT (NumberOfBytes != 0);
	ASSERT (UndoData != NULL);

	//
	// Figure out how much space the new undo entry will take up,
	// and figure out how much space remains in the undo list.
	//

	ASSERT (UndoList->BytesUsed <= UndoList->BytesAllocated);
	BytesNeeded = sizeof(KEX_DLL_REWRITE_UNDO_ENTRY) + NumberOfBytes;
	BytesFree = UndoList->BytesAllocated - UndoList->BytesUsed;

	//
	// If we need more space, reallocate.
	//

	if (BytesNeeded > BytesFree) {
		ULONG NewAllocationSize;
		PVOID NewAllocation;

		NewAllocationSize = max(UndoList->BytesAllocated * 2, UndoList->BytesUsed + BytesNeeded);

		NewAllocation = RtlReAllocateHeap(
			RtlProcessHeap(),
			0,
			UndoList->UndoEntries,
			NewAllocationSize);

		ASSERT (NewAllocation != NULL);

		if (NewAllocation == NULL) {
			return STATUS_NO_MEMORY;
		}

		UndoList->UndoEntries = NewAllocation;
		UndoList->BytesAllocated = NewAllocationSize;
	}

	//
	// Write new undo entry into the list and update the number of bytes used.
	//

	NewEntry = (PKEX_DLL_REWRITE_UNDO_ENTRY) RVA_TO_VA(UndoList->UndoEntries, UndoList->BytesUsed);
	NewEntry->Location = Location;
	NewEntry->NumberOfBytes = NumberOfBytes;
	RtlCopyMemory(NewEntry->UndoData, UndoData, NumberOfBytes);

	UndoList->BytesUsed += BytesNeeded;
	ASSERT (UndoList->BytesUsed <= UndoList->BytesAllocated);

	return STATUS_SUCCESS;
}

NTSTATUS KexPerformRollbackUndoList(
	IN		PCKEX_DLL_REWRITE_UNDO_LIST	UndoList)
{
	NTSTATUS Status;
	ULONG BytesProcessed;

	ASSERT (UndoList != NULL);

	BytesProcessed = 0;

	until (BytesProcessed >= UndoList->BytesUsed) {
		PCKEX_DLL_REWRITE_UNDO_ENTRY UndoEntry;
		PVOID BaseAddress;
		SIZE_T RegionSize;
		ULONG OldProtect;

		UndoEntry = (PCKEX_DLL_REWRITE_UNDO_ENTRY) RVA_TO_VA(UndoList->UndoEntries, BytesProcessed);

		//
		// Set memory protection to Read-Write
		//

		BaseAddress = UndoEntry->Location;
		RegionSize = UndoEntry->NumberOfBytes;

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			PAGE_READWRITE,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		//
		// Copy the undo data
		//

		KexRtlCopyMemory(
			UndoEntry->Location,
			UndoEntry->UndoData,
			UndoEntry->NumberOfBytes);

		//
		// Restore old memory protection
		//

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		//
		// Go to the next undo entry
		//

		BytesProcessed += sizeof(KEX_DLL_REWRITE_UNDO_ENTRY) + UndoEntry->NumberOfBytes;
	}

	return STATUS_SUCCESS;
}