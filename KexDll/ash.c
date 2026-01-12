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
//   if (AshModuleBaseNameIs(ReturnAddress(), L"kernel32"))
//
// Do not include a .dll extension on ModuleName (but if you do, it won't
// cause issues). If the ModuleName has an extension other than .dll (for
// example, .pyd) then you do need to include the extension.
//
KEXAPI BOOLEAN NTAPI AshModuleBaseNameIs(
	IN	PVOID	AddressInsideModule,
	IN	PCWSTR	ModuleName)
{
	NTSTATUS Status;
	UNICODE_STRING DllFullPath;
	UNICODE_STRING DllBaseName;
	UNICODE_STRING ComparisonBaseName;
	UNICODE_STRING DotDll;

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
	// Process the DLL name to remove the leading path elements and the
	// .dll extension.
	//

	Status = KexRtlPathFindFileName(&DllFullPath, &DllBaseName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	RtlInitConstantUnicodeString(&DotDll, L".dll");

	if (KexRtlUnicodeStringEndsWith(&DllBaseName, &DotDll, TRUE)) {
		// remove .dll extension from DLL name
		DllBaseName.Length -= DotDll.Length;
	}

	//
	// Process the comparison DLL name to remove the .dll extension.
	//

	Status = RtlInitUnicodeStringEx(&ComparisonBaseName, ModuleName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	if (KexRtlUnicodeStringEndsWith(&ComparisonBaseName, &DotDll, TRUE)) {
		// remove .dll extension, if present, from input DLL name to check against
		DllBaseName.Length -= DotDll.Length;
	}

	return RtlEqualUnicodeString(&DllBaseName, &ComparisonBaseName, TRUE);
}

VOID AshApplyQt6EnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;
	
	//
	// APPSPECIFICHACK: Some newer Qt6 apps require this in order to avoid trying
	// and failing to load Windows Runtime DLLs.
	//

	KexLogInformationEvent(L"App-Specific Hack applied for Qt6");
	RtlInitConstantUnicodeString(&VariableName, L"QT_QPA_PLATFORMTHEME");
	RtlInitConstantUnicodeString(&VariableValue, L"null");
	RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
}

VOID AshApplyQBittorrentEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	//
	// APPSPECIFICHACK: Applying the environment variable below will eliminate
	// the problem of bad kerning from qBittorrent. If more Qt apps are found
	// which have bad kerning, this may help fix those too.
	//

	if (AshExeBaseNameIs(L"qbittorrent.exe")) {
		KexLogInformationEvent(L"App-Specific Hack applied for qBittorrent");
		RtlInitConstantUnicodeString(&VariableName, L"QT_SCALE_FACTOR");
		RtlInitConstantUnicodeString(&VariableValue, L"1.000000001");
		RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
	}
}