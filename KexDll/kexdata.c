///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexdata.c
//
// Abstract:
//
//     Contains a definition and an initialization routine for the
//     KEX_PROCESS_DATA structure.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

// The default data in here is used in case something fails in
// KexDataInitialize.

STATIC WCHAR WinDir_Buffer[MAX_PATH];
STATIC WCHAR KexDir_Buffer[MAX_PATH];
STATIC WCHAR ImageBaseName_Buffer[MAX_PATH];

KEX_PROCESS_DATA _KexData = {
	KEX_VERSION_DW,												// InstalledVersion
	
	{															// IfeoParameters
		FALSE,													// DisableForChild
		FALSE,													// DisableAppSpecific
		WinVerSpoofNone											// WinVerSpoof
	},

	// make sure the trailing spaces are preserved such that the length of the buffer
	// is 260 wchars
	RTL_CONSTANT_STRING(L"C:\\Windows                                                                                                                                                                                                                                                         "),
	RTL_CONSTANT_STRING(L"C:\\Program Files\\VxKex                                                                                                                                                                                                                                            "),
	RTL_CONSTANT_STRING(L"UNKNOWN.EXE                                                                                                                                                                                                                                                         "),
	
	NULL														// SrvChannel
};

PKEX_PROCESS_DATA KexData = NULL;

#define GENERATE_QKMV_TABLE_ENTRY(Name, Restrict) \
	{ \
		RTL_CONSTANT_STRING(L#Name), \
		0, \
		sizeof(_KexData.Name), \
		&_KexData.Name, \
		Restrict, \
		0 \
	}

#define GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING(Name) \
	{ \
		RTL_CONSTANT_STRING(L#Name), \
		0, \
		_KexData.Name.MaximumLength, \
		_KexData.Name.Buffer, \
		REG_RESTRICT_SZ | REG_RESTRICT_EXPAND_SZ, \
		0 \
	}

STATIC NTSTATUS KexpInitializeGlobalConfig(
	VOID)
{
	NTSTATUS Status;
	HANDLE KeyHandle;
	UNICODE_STRING KeyName;
	OBJECT_ATTRIBUTES ObjectAttributes;

	ULONG QueryTableNumberOfElements;
	KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY QueryTable[] = {
		GENERATE_QKMV_TABLE_ENTRY					(InstalledVersion, REG_RESTRICT_DWORD),
		GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING	(KexDir)
	};

	//
	// Open the vxkex HKLM key.
	//

	RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\VXsoft\\VxKex");
	InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenKey(
		&KeyHandle,
		KEY_READ,
		&ObjectAttributes);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Query global configuration values.
	//

	QueryTableNumberOfElements = ARRAYSIZE(QueryTable);
	KexRtlQueryKeyMultipleValueData(
		KeyHandle,
		QueryTable,
		&QueryTableNumberOfElements,
		0);

	//
	// Fixup lengths of UNICODE_STRINGs read from the registry.
	//

	KexRtlUpdateNullTerminatedUnicodeStringLength(&_KexData.KexDir);

	NtClose(KeyHandle);
	return Status;
}

STATIC NTSTATUS KexpInitializeIfeoParameters(
	VOID)
{
	//
	// TODO
	//

	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI KexDataInitialize(
	OUT	PPKEX_PROCESS_DATA	KexDataOut OPTIONAL)
{
	if (KexData) {
		// Already initialized - fail
		return STATUS_ACCESS_DENIED;
	}

	//
	// Grab windir from SharedUserData. It's faster and more reliable
	// than trying to query environment variables or registry.
	//

	RtlInitUnicodeString(&_KexData.WinDir, SharedUserData->NtSystemRoot);

	KexRtlGetProcessImageBaseName(&_KexData.ImageBaseName);
	KexpInitializeIfeoParameters();
	KexpInitializeGlobalConfig();

	//
	// All done
	//

	KexData = &_KexData;

	if (KexDataOut) {
		*KexDataOut = KexData;
	}

	return STATUS_SUCCESS;
}

#undef GENERATE_QKMV_TABLE_ENTRY
#undef GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING