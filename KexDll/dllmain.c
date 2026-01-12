///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for KexDll.
//
//     KexDll is loaded at process initialization of every kex process and
//     is what makes it a kex process (by rewriting dlls, etc.).
//
// Author:
//
//     vxiiduu (17-Oct-2022)
//
// Environment:
//
//     Native mode.
//     This DLL is loaded before kernel32, and it can only import from NTDLL.
//
// Revision History:
//
//     vxiiduu              17-Oct-2022  Initial creation.
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//     vxiiduu              23-Feb-2024  Remove support for advanced logging.
//     vxiiduu              23-Feb-2024  Remove unneeded debug logging
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

INT WINAPI MessageBoxAHookProc(HWND, PCSTR, PCSTR, UINT);

STATIC RTL_VERIFIER_DLL_DESCRIPTOR AVrfDllDescriptor[] = {
	{NULL, 0, NULL, NULL}
};

STATIC RTL_VERIFIER_PROVIDER_DESCRIPTOR AVrfProviderDescriptor = {
	sizeof(RTL_VERIFIER_PROVIDER_DESCRIPTOR),		// Length
	AVrfDllDescriptor,								// ProviderDlls
	NULL,											// ProviderDllLoadCallback
	NULL,											// ProviderDllUnloadCallback

	NULL,											// VerifierImage
	0,												// VerifierFlags
	0,												// VerifierDebug
	NULL,											// RtlpGetStackTraceAddress
	NULL,											// RtlpDebugPageHeapCreate
	NULL,											// RtlpDebugPageHeapDestroy

	NULL											// ProviderNtdllHeapFreeCallback
};

//
// DllMain is called 3 times during process initialization.
//
//  1. DLL_PROCESS_VERIFIER, with a valid pointer as the Descriptor parameter
//     which we have to fill out
//
//  2. DLL_PROCESS_ATTACH, with a valid pointer as the Descriptor parameter
//     (this time, the structure is filled out by the system)
//
//  3. DLL_PROCESS_ATTACH, with NULL as the Descriptor parameter. This is the
//     "normal" DLL_PROCESS_ATTACH call, the previous call being made by the
//     application verifier machinery inside NTDLL.
//
BOOL WINAPI DllMain(
	IN	PVOID								DllBase,
	IN	ULONG								Reason,
	IN	PRTL_VERIFIER_PROVIDER_DESCRIPTOR	*Descriptor)
{
	NTSTATUS Status;
	PVOID DllNotificationCookie;

	if (Reason == DLL_PROCESS_VERIFIER) {
		//
		// Register a useless descriptor with app verifier system.
		// We don't make use of any app verifier apis or specific functionality
		// at the moment, but if we don't do this the process will crash.
		//

		*Descriptor = &AVrfProviderDescriptor;
	}

	if (!KexData) {
		//
		// Initialize the KexData structure, since it contains some basic data
		// which we will need for logging, etc.
		//

		KexDataInitialize(&KexData);
		KexData->KexDllBase = DllBase;
	}

	if ((KexData->Flags & KEXDATA_FLAG_MSIEXEC) &&
		!(KexData->Flags & KEXDATA_FLAG_ENABLED_FOR_MSI) &&
		NtCurrentPeb()->SubSystemData == NULL) {

		//
		// This is MSIEXEC, but the MSI it is processing does not have VxKex enabled,
		// and we weren't simply propagated from another application.
		// Do nothing.
		//

		return TRUE;
	}

	if (Reason == DLL_PROCESS_VERIFIER) {
		PPEB Peb;

		ASSERT (KexData != NULL);

		Peb = NtCurrentPeb();

		//
		// Open log file.
		//

		KexOpenVxlLogForCurrentApplication(&KexData->LogHandle);

		//
		// Log some basic information such as command-line parameters to
		// the log.
		//

		KexLogInformationEvent(
			L"Process created\r\n\r\n"
			L"Full path to the EXE: %wZ\r\n"
			L"Command line:         %wZ\r\n",
			&Peb->ProcessParameters->ImagePathName,
			&Peb->ProcessParameters->CommandLine);

		//
		// Try to get rid of as much Application verifier functionality as
		// possible.
		//

		KexDisableAVrf();

		//
		// Initialize Propagation subsystem.
		//

		KexInitializePropagation();

		//
		// After the propagation system is initialized, the IfeoParameters are
		// finalized, so print them out to the log.
		//

		KexLogInformationEvent(
			L"IfeoParameters values are finalized.\r\n\r\n"
			L"DisableForChild:      %d\r\n"
			L"DisableAppSpecific:   %d\r\n"
			L"WinVerSpoof:          %d\r\n"
			L"StrongVersionSpoof:   0x%08lx",
			KexData->IfeoParameters.DisableForChild,
			KexData->IfeoParameters.DisableAppSpecific,
			KexData->IfeoParameters.WinVerSpoof,
			KexData->IfeoParameters.StrongVersionSpoof);

		//
		// Hook any other NT system calls we are going to hook.
		//

		KexHkInstallBasicHook(NtQueryInformationThread, Ext_NtQueryInformationThread, NULL);
		KexHkInstallBasicHook(NtSetInformationThread, Ext_NtSetInformationThread, NULL);
		KexHkInstallBasicHook(NtQueryInformationProcess, Ext_NtQueryInformationProcess, NULL);
		KexHkInstallBasicHook(NtNotifyChangeKey, Ext_NtNotifyChangeKey, NULL);
		KexHkInstallBasicHook(NtNotifyChangeMultipleKeys, Ext_NtNotifyChangeMultipleKeys, NULL);
		KexHkInstallBasicHook(NtCreateSection, Ext_NtCreateSection, NULL);
		KexHkInstallBasicHook(NtRaiseHardError, Ext_NtRaiseHardError, NULL);
		KexHkInstallBasicHook(NtAssignProcessToJobObject, Ext_NtAssignProcessToJobObject, NULL);

		//
		// Perform version spoofing, if required.
		//

		KexApplyVersionSpoof();

		//
		// Initialize DLL rewrite subsystem.
		//

		Status = KexInitializeDllRewrite();
		ASSERT (NT_SUCCESS(Status));

		//
		// Register our DLL load/unload callback.
		//

		Status = LdrRegisterDllNotification(
			0,
			KexDllNotificationCallback,
			NULL,
			&DllNotificationCookie);

		ASSERT (NT_SUCCESS(Status));

		//
		// Perform any app-specific hacks that need to be done before any further
		// process initialization occurs.
		// This must be done before rewriting the imports of the main EXE because
		// we might change the DLL rewrite settings based on what we detect here.
		//

		unless (KexData->IfeoParameters.DisableAppSpecific) {
			// APPSPECIFICHACK: Environment variable hack for QBittorrent to fix
			// bad kerning.
			if (AshExeBaseNameIs(L"qbittorrent.exe")) {
				AshApplyQBittorrentEnvironmentVariableHacks();
			}

			// APPSPECIFICHACK: Detect Chromium based on EXE exports.
			AshPerformChromiumDetectionFromModuleExports(Peb->ImageBaseAddress);
		}

		//
		// Rewrite DLL Imports of our main application EXE.
		//

		Status = KexRewriteImageImportDirectory(
			NtCurrentPeb()->ImageBaseAddress,
			&KexData->ImageBaseName,
			&NtCurrentPeb()->ProcessParameters->ImagePathName);

		if (!NT_SUCCESS(Status) && Status != STATUS_IMAGE_NO_IMPORT_DIRECTORY) {
			KexLogCriticalEvent(
				L"Failed to rewrite DLL imports of the main process image.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)\r\n"
				L"Image base address: 0x%p\r\n",
				KexRtlNtStatusToString(Status), Status,
				NtCurrentPeb()->ImageBaseAddress);

			KexHeErrorBox(
				L"VxKex could not start because the DLL imports of the main "
				L"process image could not be rewritten. If the problem persists, "
				L"please disable VxKex for this program.");

			NOT_REACHED;
		}

	} else if (Reason == DLL_PROCESS_ATTACH && Descriptor == NULL) {
		Status = LdrDisableThreadCalloutsForDll(DllBase);
		ASSERT (NT_SUCCESS(Status));
	} else if (Reason == DLL_PROCESS_DETACH) {
		VxlCloseLog(&KexData->LogHandle);
	}

	return TRUE;
}