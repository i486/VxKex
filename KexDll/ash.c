///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ash.c
//
// Abstract:
//
//     This file contains helper routines for app-specific hacks.
//
// Author:
//
//     vxiiduu (16-Feb-2024)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              16-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// ExeName must include the .exe extension.
//
KEXAPI BOOLEAN NTAPI AshExeBaseNameIs(
	IN	PCWSTR	ExeName)
{
	NTSTATUS Status;
	UNICODE_STRING ExeNameUS;

	ASSERT (KexData != NULL);

	Status = RtlInitUnicodeStringEx(&ExeNameUS, ExeName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return RtlEqualUnicodeString(&KexData->ImageBaseName, &ExeNameUS, TRUE);
}

//
// This function is intended to be used like this:
//
//   if (AshModuleBaseNameIs(ReturnAddress(), L"kernel32.dll"))
//
// File extension (.dll, .exe etc.) is required.
//
KEXAPI BOOLEAN NTAPI AshModuleBaseNameIs(
	IN	PVOID	AddressInsideModule,
	IN	PCWSTR	ModuleName)
{
	NTSTATUS Status;
	UNICODE_STRING DllFullPath;
	UNICODE_STRING DllBaseName;
	UNICODE_STRING ComparisonBaseName;

	RtlInitEmptyUnicodeStringFromTeb(&DllFullPath);

	//
	// Get the name of the DLL in which the specified address resides.
	//

	Status = KexLdrGetDllFullNameFromAddress(
		AddressInsideModule,
		&DllFullPath);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	//
	// Convert full path into base name.
	//

	Status = KexRtlPathFindFileName(&DllFullPath, &DllBaseName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	Status = RtlInitUnicodeStringEx(&ComparisonBaseName, ModuleName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return RtlEqualUnicodeString(&DllBaseName, &ComparisonBaseName, TRUE);
}

//
// As with AshModuleBaseNameIs, this is designed to be used with the
// ReturnAddress() macro as the argument.
//
KEXAPI BOOLEAN NTAPI AshModuleIsWindowsModule(
	IN	PVOID	AddressInsideModule)
{
	NTSTATUS Status;
	UNICODE_STRING DllFullPath;

	RtlInitEmptyUnicodeStringFromTeb(&DllFullPath);

	//
	// Get the name of the DLL in which the specified address resides.
	//

	Status = KexLdrGetDllFullNameFromAddress(
		AddressInsideModule,
		&DllFullPath);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	//
	// See if it starts with %SystemRoot%.
	//

	if (RtlPrefixUnicodeString(&KexData->WinDir, &DllFullPath, TRUE)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

VOID AshApplyQBittorrentEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	ASSERT (AshExeBaseNameIs(L"qbittorrent.exe"));

	//
	// APPSPECIFICHACK: Applying the environment variable below will eliminate
	// the problem of bad kerning from qBittorrent. If more Qt apps are found
	// which have bad kerning, this may help fix those too.
	//

	KexLogInformationEvent(L"App-Specific Hack applied for qBittorrent");
	RtlInitConstantUnicodeString(&VariableName, L"QT_SCALE_FACTOR");
	RtlInitConstantUnicodeString(&VariableValue, L"1.0000001");
	RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
}