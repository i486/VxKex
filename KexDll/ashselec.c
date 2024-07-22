///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ashselec.c
//
// Abstract:
//
//     This file contains routines which dynamically select between different
//     implementations of DLLs by changing the DLL rewrite entries.
//
// Author:
//
//     vxiiduu (16-Mar-2024)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              16-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC NTSTATUS AshpAddUpdateRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName OPTIONAL)
{
	NTSTATUS Status;

	ASSUME (VALID_UNICODE_STRING(DllName));
	ASSUME (RewrittenDllName == NULL || WELL_FORMED_UNICODE_STRING(RewrittenDllName));

	Status = KexRemoveDllRewriteEntry(DllName);
	ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

	if (!NT_SUCCESS(Status) && Status != STATUS_STRING_MAPPER_ENTRY_NOT_FOUND) {
		return Status;
	}

	if (RewrittenDllName && RewrittenDllName->Buffer != NULL) {
		Status = KexAddDllRewriteEntry(DllName, RewrittenDllName);
	} else {
		Status = STATUS_SUCCESS;
	}

	ASSERT (NT_SUCCESS(Status));
	return Status;
}

NTSTATUS AshSelectDWriteImplementation(
	IN	KEX_DWRITE_IMPLEMENTATION	Implementation)
{
	UNICODE_STRING DllName;
	UNICODE_STRING RewrittenDllName;

	RtlInitConstantUnicodeString(&DllName, L"DWrite");

	switch (Implementation) {
	case DWriteNoImplementation:
		RtlInitEmptyUnicodeString(&RewrittenDllName, NULL, 0);
		break;
	case DWriteWindows10Implementation:
		RtlInitConstantUnicodeString(&RewrittenDllName, L"dwrw10");
		break;
	default:
		NOT_REACHED;
	}

	return AshpAddUpdateRemoveDllRewriteEntry(&DllName, &RewrittenDllName);
}