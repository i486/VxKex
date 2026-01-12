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
		UNICODE_STRING SlashTemp;

		//
		// See if it starts with %SystemRoot%\Temp. In this case, we won't consider
		// it a Windows module.
		//
		// Some installers, such as the newest versions of the Microsoft C++ v14
		// Redistributable, copy themselves to %SystemRoot%\Temp and then run from
		// there and check the Windows version.
		//
		// We don't want such installers to be considered Windows executables.
		//

		KexRtlAdvanceUnicodeString(&DllFullPath, KexData->WinDir.Length);
		RtlInitConstantUnicodeString(&SlashTemp, L"\\Temp");

		if (RtlPrefixUnicodeString(&SlashTemp, &DllFullPath, TRUE)) {
			return FALSE;
		}

		//
		// It's a Windows executable (located in %SystemRoot% and not in the Temp
		// folder).
		//

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

VOID AshApplyNodeJSEnvironmentVariableHacks(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	ASSERT (AshExeBaseNameIs(L"node.exe"));

	//
	// APPSPECIFICHACK: Node.js requires this environment variable, otherwise it
	// will refuse to run, stating Windows 10 is required. Spoofing Windows version
	// to Windows 10 causes the application to crash; therefore, this environment
	// variable spoof is the best way to make it work.
	//

	KexLogInformationEvent(L"App-Specific Hack applied for Node.js");
	RtlInitConstantUnicodeString(&VariableName, L"NODE_SKIP_PLATFORM_CHECK");
	RtlInitConstantUnicodeString(&VariableValue, L"1");
	RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
}

NTSTATUS AshPerformQt6DetectionFromLoadedDll(
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData)
{
	NTSTATUS Status;
	UNICODE_STRING Qt6;
	UNICODE_STRING BaseName;

	ASSUME (!(KexData->Flags & KEXDATA_FLAG_QT6));

	Status = KexRtlPathFindFileName(NotificationData->FullDllName, &BaseName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	RtlInitConstantUnicodeString(&Qt6, L"Qt6");

	if (RtlPrefixUnicodeString(&Qt6, &BaseName, TRUE)) {
		//
		// Newer versions of Qt6 require the Windows 10 DWrite, otherwise text will be
		// displayed as a bunch of boxes.
		//
		Status = AshSelectDWriteImplementation(DWriteWindows10Implementation);
		ASSERT (NT_SUCCESS(Status));

		KexData->Flags |= KEXDATA_FLAG_QT6;
	}

	return Status;
}