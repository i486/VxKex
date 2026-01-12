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
//     vxiiduu              23-Feb-2024  Add setting to disable logging.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

// The default data in here is used in case something fails in
// KexDataInitialize.

STATIC WCHAR WinDir_Buffer[MAX_PATH];
STATIC WCHAR KexDir_Buffer[MAX_PATH];
STATIC WCHAR ImageBaseName_Buffer[MAX_PATH];

// A note on this structure - KexDll must be compiled with /GF- (disable
// string pooling) or otherwise an access violation will occur. We don't want
// the string literals going into read-only memory.
KEX_PROCESS_DATA _KexData = {
	0,															// Flags
	
	{															// IfeoParameters
		FALSE,													// DisableForChild
		FALSE,													// DisableAppSpecific
		WinVerSpoofNone,										// WinVerSpoof
		FALSE,													// StrongVersionSpoof
		FALSE,													// BreakOnHardError
		FALSE,													// DisableDllDirectory
		D3D12AutomaticImplementation,							// D3D12Implementation
		DnsapiAutomaticImplementation,							// DnsapiImplementation
	},

	// make sure the trailing spaces are preserved such that the length of the buffer
	// is 260 wchars
	RTL_CONSTANT_STRING(L"C:\\Windows                                                                                                                                                                                                                                                         "),
	RTL_CONSTANT_STRING(L"C:\\Program Files\\VxKex                                                                                                                                                                                                                                            "),
	RTL_CONSTANT_STRING(L"C:\\ProgramData\\VxKex\\Logs                                                                                                                                                                                                                                        "),
	RTL_CONSTANT_STRING(L"UNKNOWN.EXE                                                                                                                                                                                                                                                         "),
	
	NULL,														// LogHandle
	NULL,														// KexDllBase
	NULL,														// SystemDllBase
	NULL,														// NativeSystemDllBase
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
	ULONG DisableLogging;

	ULONG QueryTableNumberOfElements;
	KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY QueryTable[] = {
		{RTL_CONSTANT_STRING(L"DisableLogging"), 0, 8, &DisableLogging, REG_RESTRICT_DWORD, 0},
		GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING	(KexDir),
		GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING	(LogDir)
	};

	DisableLogging = 0;

	//
	// Open the vxkex HKLM key.
	//

	RtlInitConstantUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\VXsoft\\VxKex");
	InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenKey(
		&KeyHandle,
		KEY_READ | KEY_WOW64_64KEY,
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

	if (DisableLogging) {
		_KexData.Flags |= KEXDATA_FLAG_DISABLE_LOGGING;
	}

	//
	// Fixup lengths of UNICODE_STRINGs read from the registry.
	//

	KexRtlUpdateNullTerminatedUnicodeStringLength(&_KexData.KexDir);
	KexRtlUpdateNullTerminatedUnicodeStringLength(&_KexData.LogDir);

	SafeClose(KeyHandle);
	return Status;
}

//
// Local configuration is read from HKCU and overrides global configuration.
//

STATIC NTSTATUS KexpInitializeLocalConfig(
	VOID)
{
	NTSTATUS Status;
	HANDLE CurrentUserKeyHandle;
	HANDLE KeyHandle;
	UNICODE_STRING KeyName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG DisableLogging;

	ULONG QueryTableNumberOfElements;
	KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY QueryTable[] = {
		{RTL_CONSTANT_STRING(L"DisableLogging"), 0, 8, &DisableLogging, REG_RESTRICT_DWORD, 0},
		GENERATE_QKMV_TABLE_ENTRY_UNICODE_STRING	(LogDir)
	};

	DisableLogging = _KexData.Flags & KEXDATA_FLAG_DISABLE_LOGGING;

	Status = RtlOpenCurrentUser(KEY_ENUMERATE_SUB_KEYS, &CurrentUserKeyHandle);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	RtlInitConstantUnicodeString(&KeyName, L"Software\\VXsoft\\VxKex");

	InitializeObjectAttributes(
		&ObjectAttributes, 
		&KeyName, 
		OBJ_CASE_INSENSITIVE, 
		CurrentUserKeyHandle, 
		NULL);

	Status = NtOpenKey(
		&KeyHandle,
		KEY_READ | KEY_WOW64_64KEY,
		&ObjectAttributes);

	SafeClose(CurrentUserKeyHandle);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	QueryTableNumberOfElements = ARRAYSIZE(QueryTable);
	KexRtlQueryKeyMultipleValueData(
		KeyHandle,
		QueryTable,
		&QueryTableNumberOfElements,
		0);

	if (DisableLogging) {
		_KexData.Flags |= KEXDATA_FLAG_DISABLE_LOGGING;
	} else {
		_KexData.Flags &= ~KEXDATA_FLAG_DISABLE_LOGGING;
	}

	KexRtlUpdateNullTerminatedUnicodeStringLength(&_KexData.LogDir);

	SafeClose(KeyHandle);
	return Status;
}

STATIC NTSTATUS KexpInitializeIfeoParameters(
	OUT	PKEX_PROCESS_DATA	Data)
{
	NTSTATUS Status;
	ULONG NoKexOptionsInRegistry;
	HANDLE IfeoKeyHandle;
	UNICODE_STRING MsiexecFullPath;
	PKEX_IFEO_PARAMETERS IfeoParameters;
	PPEB Peb;

	Peb = NtCurrentPeb();
	IfeoParameters = &Data->IfeoParameters;

	NoKexOptionsInRegistry = 0xffffffff;

	RtlInitEmptyUnicodeStringFromTeb(&MsiexecFullPath);
	RtlAppendUnicodeStringToString(&MsiexecFullPath, &Data->WinDir);
	RtlAppendUnicodeToString(&MsiexecFullPath, L"\\system32\\msiexec.exe");

	if (RtlEqualUnicodeString(&Peb->ProcessParameters->ImagePathName, &MsiexecFullPath, TRUE)) {
		UNICODE_STRING CommandLine;
		UNICODE_STRING MsiFullPath;
		UNICODE_STRING DotMsi;
		PWCHAR DotMsiLocation;

		Data->Flags |= KEXDATA_FLAG_MSIEXEC;
		Status = STATUS_OBJECT_NAME_NOT_FOUND;

		//
		// We are currently loaded into %SystemRoot%\system32\msiexec.exe.
		// Open the IFEO key for the .msi file, not the one for msiexec.exe itself.
		// This is going to require us to parse command line arguments.
		//

		CommandLine = Peb->ProcessParameters->CommandLine;
		RtlInitConstantUnicodeString(&DotMsi, L".MSI\"");

		DotMsiLocation = KexRtlFindUnicodeSubstring(&CommandLine, &DotMsi, TRUE);
		
		if (DotMsiLocation) {
			MsiFullPath.Length = DotMsi.Length;
			MsiFullPath.MaximumLength = DotMsi.Length;
			MsiFullPath.Buffer = DotMsiLocation;

			// we've found the .msi extension and the close quote, now retreat the
			// string until we find the start quote.
			until (MsiFullPath.Buffer[0] == '"' || MsiFullPath.Buffer == CommandLine.Buffer) {
				KexRtlRetreatUnicodeString(&MsiFullPath, sizeof(WCHAR));
			}

			if (MsiFullPath.Buffer[0] == '"') {
				KexRtlAdvanceUnicodeString(&MsiFullPath, sizeof(WCHAR));
				MsiFullPath.Length -= sizeof(WCHAR);

				Status = LdrOpenImageFileOptionsKey(
					&MsiFullPath,
					FALSE,
					&IfeoKeyHandle);

				if (NT_SUCCESS(Status)) {
					Data->Flags |= KEXDATA_FLAG_ENABLED_FOR_MSI;
				}
			}
		}
	} else {
		//
		// Open the IFEO key for the current executable.
		//

		Status = LdrOpenImageFileOptionsKey(
			&Peb->ProcessParameters->ImagePathName,
			FALSE,
			&IfeoKeyHandle);
	}

	if (!NT_SUCCESS(Status)) {
		goto Exit;
	}

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_DisableForChild",
		REG_DWORD,
		&IfeoParameters->DisableForChild,
		sizeof(IfeoParameters->DisableForChild),
		NULL);
	
	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_DisableAppSpecific",
		REG_DWORD,
		&IfeoParameters->DisableAppSpecific,
		sizeof(IfeoParameters->DisableAppSpecific),
		NULL);
	
	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_WinVerSpoof",
		REG_DWORD,
		&IfeoParameters->WinVerSpoof,
		sizeof(IfeoParameters->WinVerSpoof),
		NULL);

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_StrongVersionSpoof",
		REG_DWORD,
		&IfeoParameters->StrongVersionSpoof,
		sizeof(IfeoParameters->StrongVersionSpoof),
		NULL);

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_BreakOnHardError",
		REG_DWORD,
		&IfeoParameters->BreakOnHardError,
		sizeof(IfeoParameters->BreakOnHardError),
		NULL);

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_DisableDllDirectory",
		REG_DWORD,
		&IfeoParameters->DisableDllDirectory,
		sizeof(IfeoParameters->DisableDllDirectory),
		NULL);

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_D3D12Implementation",
		REG_DWORD,
		&IfeoParameters->D3D12Implementation,
		sizeof(IfeoParameters->D3D12Implementation),
		NULL);

	NoKexOptionsInRegistry &= LdrQueryImageFileKeyOption(
		IfeoKeyHandle,
		L"KEX_DnsapiImplementation",
		REG_DWORD,
		&IfeoParameters->DnsapiImplementation,
		sizeof(IfeoParameters->DnsapiImplementation),
		NULL);

	SafeClose(IfeoKeyHandle);

Exit:
	unless (NoKexOptionsInRegistry) {
		// Indicate that this process has VxKex options present in the registry.
		Data->Flags |= KEXDATA_FLAG_IFEO_OPTIONS_PRESENT;
	}

	return Status;
}

KEXAPI NTSTATUS NTAPI KexDataInitialize(
	OUT	PPKEX_PROCESS_DATA	KexDataOut OPTIONAL)
{
	if (KexData) {
		if (KexDataOut) {
			*KexDataOut = KexData;
		}
		
		return STATUS_ALREADY_INITIALIZED;
	}

	//
	// Grab windir from SharedUserData. It's faster and more reliable
	// than trying to query environment variables or registry.
	//

	RtlInitUnicodeString(&_KexData.WinDir, SharedUserData->NtSystemRoot);

	KexRtlGetProcessImageBaseName(&_KexData.ImageBaseName);
	KexpInitializeIfeoParameters(&_KexData);
	KexpInitializeGlobalConfig();
	KexpInitializeLocalConfig();

	//
	// Get NTDLL base address.
	//
	
	_KexData.SystemDllBase = KexLdrGetSystemDllBase();
	_KexData.NativeSystemDllBase = KexLdrGetNativeSystemDllBase();

	ASSERT (_KexData.SystemDllBase != NULL);
	ASSERT (_KexData.NativeSystemDllBase != NULL);

	if (KexRtlCurrentProcessBitness() != KexRtlOperatingSystemBitness()) {
		ASSERT (_KexData.SystemDllBase != _KexData.NativeSystemDllBase);
	}

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