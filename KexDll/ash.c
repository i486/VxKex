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
//     vxiiduu              03-Jan-2026  Do not consider files in %WinDir%\Temp
//                                       as Windows files.
//     vxiiduu              22-Feb-2026  Remove qt6 kerning hack, as it seems to
//                                       no longer be needed for qbittorrent.
//     vxiiduu              27-Apr-2026  Add Python environment variable hack
//     vxiiduu              30-Apr-2026  Move ASH initialization out of this file
//                                       to ashinit.c
//     vxiiduu              02-May-2026  Add AshModuleIsNonDllRewriteModule.
//     vxiiduu              19-May-2026  Move Qt6 stuff to ashdetec.c
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

STATIC NTSTATUS AshpGetFullAndBaseNameFromAddress(
	IN	PCVOID			AddressInsideModule,
	OUT	PUNICODE_STRING	FullDllName OPTIONAL,
	OUT	PUNICODE_STRING	BaseDllName OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllNameTemp;

	RtlInitEmptyUnicodeStringFromTeb(&FullDllNameTemp);

	if (!FullDllName && !BaseDllName) {
		// nothing to do
		return STATUS_SUCCESS;
	}

	//
	// Get full name.
	// This can fail if the address is outside any module, which is true for
	// JIT-generated code or obfuscated VM-protected code, for example.
	//

	Status = KexLdrGetDllFullNameFromAddress(
		AddressInsideModule,
		&FullDllNameTemp);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (FullDllName) {
		*FullDllName = FullDllNameTemp;
	}

	//
	// Get base name if necessary
	//

	if (BaseDllName) {
		Status = KexRtlPathFindFileName(&FullDllNameTemp, BaseDllName);
		ASSERT (NT_SUCCESS(Status));
	}

	return Status;
}

//
// This function is intended to be used like this:
//
//   if (AshModuleBaseNameIs(ReturnAddress(), L"kernel32.dll"))
//
// File extension (.dll, .exe etc.) is required.
//
KEXAPI BOOLEAN NTAPI AshModuleBaseNameIs(
	IN	PCVOID	AddressInsideModule,
	IN	PCWSTR	ModuleName)
{
	NTSTATUS Status;
	UNICODE_STRING BaseDllName;
	UNICODE_STRING ComparisonBaseName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		NULL,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	Status = RtlInitUnicodeStringEx(&ComparisonBaseName, ModuleName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return RtlEqualUnicodeString(&BaseDllName, &ComparisonBaseName, TRUE);
}

//
// As with AshModuleBaseNameIs, this is designed to be used with the
// ReturnAddress() macro as the argument.
//
KEXAPI BOOLEAN NTAPI AshModuleIsWindowsModule(
	IN	PCVOID	AddressInsideModule)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		&FullDllName,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return KexIsWindowsDll(&FullDllName, &BaseDllName);
}

//
// Similar to AshModuleIsWindowsModule but also returns TRUE for any DLL
// which is banned from having its imports rewritten, which is related to but
// not exactly the same set of DLLs as "Windows modules".
//
KEXAPI BOOLEAN NTAPI AshModuleIsDynamicRewriteExemptedModule(
	IN	PCVOID	AddressInsideModule)
{
	NTSTATUS Status;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;

	Status = AshpGetFullAndBaseNameFromAddress(
		AddressInsideModule,
		&FullDllName,
		&BaseDllName);

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	return !KexShouldRewriteDynamicImportsOfDll(&FullDllName, &BaseDllName);
}