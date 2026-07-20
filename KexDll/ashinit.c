///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ashinit.c
//
// Abstract:
//
//     This file contains helper routines for app-specific hacks.
//
// Author:
//
//     vxiiduu (30-Apr-2026)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              16-Feb-2024  Initial creation.
//     vxiiduu              30-Apr-2026  Add Deno environment variable hack
//     vxiiduu              30-Apr-2026  Move early ASH initialization from
//                                       DllMain to AshInitialize
//     vxiiduu              23-Jun-2026  Added generic statically linked Qt6
//                                       detection.
//     vxiiduu              28-Jun-2026  Add Godot detection and app specific
//                                       hack to disable Vulkan.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC BOOLEAN AshDoesImageContainSection(
	IN	PVOID	ModuleBase,
	IN	PCSTR	Section)
{
	PIMAGE_NT_HEADERS NtHeaders;
	ANSI_STRING SectionName;
	PIMAGE_SECTION_HEADER SectionHeader;

	NtHeaders = RtlImageNtHeader(ModuleBase);
	ASSERT (NtHeaders != NULL);

	if (NtHeaders == NULL) {
		return FALSE;
	}

	RtlInitAnsiString(&SectionName, Section);

	SectionHeader = KexRtlSectionTableFromName(NtHeaders, &SectionName);

	return (SectionHeader != NULL) ? TRUE : FALSE;
}

//
// Statically linked Qt6 applications tend to have a .qtmimed PE section in their
// EXE file. We can check for this section to determine that an application has
// statically-linked Qt6.
//
STATIC BOOLEAN AshIsStaticallyLinkedQt6Image(
	IN	PVOID	ModuleBase)
{
	return AshDoesImageContainSection(ModuleBase, ".qtmimed");
}

//
// Godot games have a "pck" section.
//
STATIC BOOLEAN AshIsGodotImage(
	IN	PVOID	ModuleBase)
{
	return AshDoesImageContainSection(ModuleBase, "pck");
}

//
// Zig programs *seem* to always have a non-standard .buildid section.
//
STATIC BOOLEAN AshIsZigImage(
	IN	PVOID	ModuleBase)
{
	return AshDoesImageContainSection(ModuleBase, ".buildid");
}

//
// This is called from DllMain before any app's DLLs are loaded.
//
VOID AshInitialize(
	VOID)
{
	UNICODE_STRING VariableName;
	UNICODE_STRING VariableValue;

	if (KexData->IfeoParameters.DisableAppSpecific) {
		return;
	}

	if (!(KexData->Flags & KEXDATA_FLAG_QT6) &&
		AshIsStaticallyLinkedQt6Image(NtCurrentPeb()->ImageBaseAddress)) {

		//
		// APPSPECIFICHACK: Usually Qt6 applications get Win10 DWrite based on
		// detection of loaded Qt6 DLLs. Some applications have statically linked
		// Qt6 so we need to handle it specifically.
		//

		AshSetIsQt6Process();
	}

	if (AshIsGodotImage(NtCurrentPeb()->ImageBaseAddress)) {
		//
		// APPSPECIFICHACK: Godot games use Vulkan, which can cause crashes on some
		// Intel graphics drivers. We will disable Vulkan and force the game engine
		// to use OpenGL instead.
		//

		KexLogInformationEvent(L"App-Specific Hack applied for Godot");
		RtlInitConstantUnicodeString(&VariableName, L"VK_ICD_FILENAMES");
		RtlInitConstantUnicodeString(&VariableValue, L":null:");
		RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
	}

	if (AshIsZigImage(NtCurrentPeb()->ImageBaseAddress)) {
		KexLogInformationEvent(L"App-Specific Hack applied for Zig");
		KexData->Flags |= KEXDATA_FLAG_CONDRV_EMULATION;
	}

	if (AshExeBaseNameIs(L"node.exe")) {
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
	} else if (AshExeBaseNameIs(L"python.exe")) {
		//
		// APPSPECIFICHACK: Newer Python versions will, when run interactively, display
		// the warning message "warning: can't use pyrepl: Windows 10 TH2 or later required".
		// We can set PYTHON_BASIC_REPL=1 to remove the warning and make Python not use
		// ANSI escape sequences.
		//

		if (KexData->IfeoParameters.WinVerSpoof < WinVerSpoofWin10) {
			KexLogInformationEvent(L"App-Specific Hack applied for Python");
			RtlInitConstantUnicodeString(&VariableName, L"PYTHON_BASIC_REPL");
			RtlInitConstantUnicodeString(&VariableValue, L"1");
			RtlSetEnvironmentVariable(NULL, &VariableName, &VariableValue);
		}
	} else if (AshExeBaseNameIs(L"cavalry.exe")) {
		NTSTATUS Status;
		UNICODE_STRING RewriteEntry;

		//
		// APPSPECIFICHACK: Cavalry bundles its own icuuc.dll which is newer than the
		// version present in Windows 10 (which VxKex includes). Remove icuuc from the
		// DLL rewrite list to solve this problem.
		//

		KexLogInformationEvent(L"App-Specific Hack applied for Cavalry");
		RtlInitConstantUnicodeString(&RewriteEntry, L"icuuc");
		Status = KexRemoveDllRewriteEntry(&RewriteEntry);
		ASSERT (NT_SUCCESS(Status));

		Status = AshSetIsQt6Process();
		ASSERT (NT_SUCCESS(Status));

		// Do not use Win10 DWrite; I have heard reports it causes "weird" text.
		Status = AshSelectDWriteImplementation(DWriteNoImplementation);
		ASSERT (NT_SUCCESS(Status));
	} else if (KexRtlCurrentProcessBitness() == 64 &&
			   AshExeBaseNameIs(L"NieR Replicant ver.1.22474487139.exe")) {

		NTSTATUS Status;
		UNICODE_STRING RewriteEntry;

		//
		// APPSPECIFICHACK: NieR:Replicant tries to resolve MFCreateDXGIDeviceManager
		// from both MFPlat.dll and MshtmlMedia.dll. If it finds it in MFPlat, it uses
		// an alternate Win8+ code path, which causes a cutscene softlock.
		//
		// In order to fix this we will remove the MFPlat redirection from the DLL
		// rewrite list.
		//

		KexLogInformationEvent(L"App-Specific Hack applied for NieR Replicant");
		RtlInitConstantUnicodeString(&RewriteEntry, L"MFPlat");
		Status = KexRemoveDllRewriteEntry(&RewriteEntry);
		ASSERT (NT_SUCCESS(Status));
	}

	// APPSPECIFICHACK: Detect Chromium based on EXE exports.
	AshPerformChromiumDetectionFromModuleExports(NtCurrentPeb()->ImageBaseAddress);
}