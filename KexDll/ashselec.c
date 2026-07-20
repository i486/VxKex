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

	return KexAddUpdateRemoveDllRewriteEntry(&DllName, &RewrittenDllName);
}