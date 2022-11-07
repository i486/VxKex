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
//     vxiiduu              06-Nov-2022  Add IFEO parameter reading.
//     vxiiduu              07-Nov-2022  Remove spurious range check.
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
	0,															// Flags
	KEX_VERSION_DW,												// InstalledVersion
	
	{															// IfeoParameters
		FALSE,													// DisableForChild
		FALSE,													// DisableAppSpecific
		WinVerSpoofNone,										// WinVerSpoof
		FALSE,													// StrongVersionSpoof
	},

	// make sure the trailing spaces are preserved such that the length of the buffer
	// is 260 wchars
	RTL_CONSTANT_STRING(L"C:\\Windows                                                                                                                                                                                                                                                         "),
	RTL_CONSTANT_STRING(L"C:\\Program Files\\VxKex                                                                                                                                                                                                                                            "),
	RTL_CONSTANT_STRING(L"UNKNOWN.EXE                                                                                                                                                                                                                                                         "),
	
	NULL,														// SrvChannel
	NULL,														// KexDllBase
	NULL														// SystemDllBase
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
	VOID) PROTECTED_FUNCTION
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

	RtlInitConstantUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\VXsoft\\VxKex");
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
} PROTECTED_FUNCTION_END

STATIC NTSTATUS KexpInitializeIfeoParameters(
	OUT	PKEX_IFEO_PARAMETERS	IfeoParameters) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	HANDLE IfeoKeyHandle;
	PPEB Peb;

	Peb = NtCurrentPeb();

	//
	// Remember that if this is a propagated VxKex, the hook on NtOpenKey
	// will still be active, and LdrOpenImageFileOptionsKey will read the
	// propagation virtual key. This is intended behavior.
	//

	Status = LdrOpenImageFileOptionsKey(
		&NtCurrentPeb()->ProcessParameters->ImagePathName,
		FALSE,
		&IfeoKeyHandle);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	LdrQueryImageFileKeyOption(
		IfeoKeyHandle, 
		L"KEX_DisableForChild", 
		REG_DWORD, 
		&IfeoParameters->DisableForChild,
		sizeof(IfeoParameters->DisableForChild),
		NULL);
	
	LdrQueryImageFileKeyOption(
		IfeoKeyHandle, 
		L"KEX_DisableAppSpecific", 
		REG_DWORD, 
		&IfeoParameters->DisableAppSpecific,
		sizeof(IfeoParameters->DisableAppSpecific),
		NULL);
	
	LdrQueryImageFileKeyOption(
		IfeoKeyHandle, 
		L"KEX_WinVerSpoof", 
		REG_DWORD, 
		&IfeoParameters->WinVerSpoof,
		sizeof(IfeoParameters->WinVerSpoof),
		NULL);

	LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_StrongVersionSpoof",
		REG_DWORD,
		&IfeoParameters->StrongVersionSpoof,
		sizeof(IfeoParameters->StrongVersionSpoof),
		NULL);

	NtClose(IfeoKeyHandle);
	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

KEXAPI NTSTATUS NTAPI KexDataInitialize(
	OUT	PPKEX_PROCESS_DATA	KexDataOut OPTIONAL) PROTECTED_FUNCTION
{
	PPEB Peb;

	Peb = NtCurrentPeb();

	if (KexData) {
		// Already initialized - fail
		return STATUS_ACCESS_DENIED;
	}

	//
	// If we were propagated from another process, Peb->SubSystemData will be
	// 0xB02BA295 (magic number set by NtOpenKeyEx hook).
	//

	if (Peb->SubSystemData == (PVOID) 0xB02BA295L) {
		_KexData.Flags |= KEXDATA_FLAG_PROPAGATED;
		Peb->SubSystemData = NULL;
	}

	//
	// Grab windir from SharedUserData. It's faster and more reliable
	// than trying to query environment variables or registry.
	//

	RtlInitUnicodeString(&_KexData.WinDir, SharedUserData->NtSystemRoot);

	KexRtlGetProcessImageBaseName(&_KexData.ImageBaseName);
	KexpInitializeIfeoParameters(&_KexData.IfeoParameters);
	KexpInitializeGlobalConfig();

	//
	// Get native NTDLL base address.
	//
	
	_KexData.SystemDllBase = KexLdrGetNativeSystemDllBase();

	//
	// All done
	//

	KexData = &_KexData;

	if (KexDataOut) {
		*KexDataOut = KexData;
	}

	return STATUS_SUCCESS;
} PROTECTED_FUNCTION_END

#undef GENERATE_QKMV_TABLE_ENTRY
#undef GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING