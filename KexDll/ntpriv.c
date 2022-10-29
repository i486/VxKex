///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ntpriv.c
//
// Abstract:
//
//     Re-implementations of some small non-exported NTDLL functions.
//     Mostly based on decompilation of Win7 NTDLL.
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// Notes:
//   - The real LdrpAllocateDataTableEntry allocates from LdrpHeap (a private
//     heap which is inaccessible to us). Therefore, we can't let NTDLL free
//     the entry.
//
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