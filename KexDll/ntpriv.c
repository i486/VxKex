///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntpriv.c
//
// Abstract:
//
//     Re-implementations of some small non-exported functions.
//     Mostly based on decompilation of Win7.
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//     vxiiduu              06-Nov-2022  Add LdrpFindLoadedDllByHandle
//                                       Remove incorrect comment (LdrpHeap is
//                                       actually the same as the process heap)
//     vxiiduu              08-Mar-2024  Add BaseGetNamedObjectDirectory.
//     vxiiduu              11-Mar-2024  Move BaseGetNamedObjectDirectory to
//                                       KxBase.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"
#include <KxBase.h>

PLDR_DATA_TABLE_ENTRY NTAPI LdrpAllocateDataTableEntry(
	IN	PVOID	DllBase)
{
	PLDR_DATA_TABLE_ENTRY Entry;
	PIMAGE_NT_HEADERS NtHeaders;

	NtHeaders = RtlImageNtHeader(DllBase);
	if (!NtHeaders) {
		return NULL;
	}

	Entry = SafeAlloc(LDR_DATA_TABLE_ENTRY, 1);
	if (!Entry) {
		return NULL;
	}

	RtlZeroMemory(Entry, sizeof(*Entry));
			
	Entry->DllBase = DllBase;
	Entry->SizeOfImage = NtHeaders->OptionalHeader.SizeOfImage;
	Entry->TimeDateStamp = NtHeaders->FileHeader.TimeDateStamp;
	Entry->PatchInformation = NULL;

	InitializeListHead(&Entry->ForwarderLinks);
	InitializeListHead(&Entry->ServiceTagLinks);
	InitializeListHead(&Entry->StaticLinks);

	return Entry;
}

BOOLEAN NTAPI LdrpFindLoadedDllByHandle(
	IN	PVOID					DllHandle,
	OUT	PPLDR_DATA_TABLE_ENTRY	DataTableEntry)
{
	PLDR_DATA_TABLE_ENTRY Entry;
	PPEB_LDR_DATA PebLdr;

	PebLdr = NtCurrentPeb()->Ldr;
	Entry = (PLDR_DATA_TABLE_ENTRY) PebLdr->InLoadOrderModuleList.Flink;

	if (IsListEmpty(&PebLdr->InLoadOrderModuleList)) {
		return FALSE;
	}

	while (Entry->DllBase != DllHandle || !Entry->InMemoryOrderLinks.Flink) {
		Entry = (PLDR_DATA_TABLE_ENTRY) Entry->InLoadOrderLinks.Flink;

		if ((PLIST_ENTRY) Entry == &PebLdr->InLoadOrderModuleList) {
			return FALSE;
		}
	}

	*DataTableEntry = Entry;
	return TRUE;
}