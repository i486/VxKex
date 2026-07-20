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
//     vxiiduu              29-Nov-2025  Add NodeJS environment variable hack
//     vxiiduu              22-Feb-2026  Remove QBittorrent scaling/kerning hack
//                                       and add QBittorrent Win10 DWrite hack.
//     vxiiduu              30-Apr-2026  Move app-specific initialization to
//                                       AshInitialize in ashinit.c
//     vxiiduu              23-Jun-2026  Add MLS to KexDll message boxes.
//     vxiiduu              24-Jun-2026  Add support for new Explorer CPIWBYPA.
//     vxiiduu              27-Jun-2026  Add MacType APC hack.
//     vxiiduu              28-Jun-2026  Support KEX_DllRewriteEntries.
//     vxiiduu              06-Jul-2026  Added support for the alert thread by
//                                       thread ID syscalls (requires an init
//                                       function called per thread).
//     vxiiduu              09-Jul-2026  Failure to initialize DLL rewrite will
//                                       now be a fatal error (for non-propagated
//                                       processes) or cause an initialization
//                                       abort (for propagated processes).
//
///////////////////////////////////////////////////////////////////////////////

#define NEED_VERSION_DEFS
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
	IN		PVOID								DllBase,
	IN		ULONG								Reason,
	IN OUT	PRTL_VERIFIER_PROVIDER_DESCRIPTOR	*Descriptor)
{
	NTSTATUS Status;
	PTEB Teb;
	PPEB Peb;

	Teb = NtCurrentTeb();
	Peb = Teb->ProcessEnvironmentBlock;

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
		!(KexData->Flags & KEXDATA_FLAG_MSI_SERVICE) &&
		NtCurrentPeb()->SubSystemData == NULL) {

		//
		// This is MSIEXEC, but the MSI it is processing does not have VxKex,
		// enabled, we aren't running as the Windows Installer service, and
		// and we weren't simply propagated from another application.
		// Do nothing.
		//

		return TRUE;
	}

	if (Reason == DLL_PROCESS_VERIFIER) {
		PVOID DllNotificationCookie;

		ASSERT (KexData != NULL);

		//
		// Try to get rid of as much Application verifier functionality as
		// possible.
		//

		KexDisableAVrf();

		//
		// Queue an APC to patch the CreateProcessInternalW subsystem check and
		// also to work around MacType being incompatible with Application Verifier.
		//

		Status = NtQueueApcThread(
			NtCurrentThread(),
			KexPostInitializationApcRoutine,
			NULL,
			NULL,
			NULL);

		ASSERT (NT_SUCCESS(Status));

		//
		// If we're running in Explorer, we don't need to do anything else.
		//

		if (KexData->Flags & KEXDATA_FLAG_EXPLORER) {
			return TRUE;
		}

		//
		// Open log file.
		//

		Status = KexOpenVxlLogForCurrentApplication(&KexData->LogHandle);

		//
		// Hook hard errors so that we can log various kinds of loader failures.
		//

		Status = KexHkInstallBasicHook(NtRaiseHardError, Ext_NtRaiseHardError, NULL);
		ASSERT (NT_SUCCESS(Status));

		//
		// Log some basic information such as command-line parameters to
		// the log.
		//

		KexLogInformationEvent(
			L"Process created\r\n\r\n"
			L"The VxKex version is %hs (%s)\r\n"
			L"The program is %d-bit and the operating system is %d-bit\r\n"
			L"Full path to the EXE: %wZ\r\n"
			L"Command line:         %wZ\r\n",
			KEX_VERSION_STR,
			KexIsDebugBuild ? L"Debug" : L"Release",
			KexRtlCurrentProcessBitness(), KexRtlOperatingSystemBitness(),
			&Peb->ProcessParameters->ImagePathName,
			&Peb->ProcessParameters->CommandLine);

		//
		// Initialize Propagation subsystem.
		//

		Status = KexInitializePropagation();
		ASSERT (NT_SUCCESS(Status));

		//
		// Initialize DLL rewrite subsystem.
		//

		Status = KexInitializeDllRewrite();
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			if (!(KexData->Flags & KEXDATA_FLAG_PROPAGATED)) {
				//
				// Failed to initialize DLL rewrite on a non-propagated process. This
				// is a fatal error because without DLL rewrite being enabled, nothing
				// will really work.
				//

				KexHeErrorBox(
					_(L"VxKex has encountered an error because the DLL rewrite system could "
					  L"not initialize. A common cause for this error is a non-standard "
					  L"PATH environment variable. In order to troubleshoot, make sure logging "
					  L"is enabled in VxKex global settings."));

				NOT_REACHED;
			}

			// Bail out. Continuing will crash the process due to kernel32's "kxnt"
			// import rewrite.

			return TRUE;
		}

		//
		// Perform any app-specific hacks that need to be done before any further
		// process initialization occurs.
		// This must be done before rewriting the imports of the main EXE because
		// we might change the DLL rewrite settings based on what we detect here.
		//

		AshInitialize();

		//
		// If the user has specified any modifications to the DLL rewrite map, apply
		// them here, after ASH has made its changes but before rewriting the EXE
		// imports.
		//

		if (KexData->IfeoParameters.DllRewriteEntries[0] != '\0') {
			Status = KexApplyUserDllRewrite(KexData->IfeoParameters.DllRewriteEntries);
			ASSERT (NT_SUCCESS(Status) || Status == STATUS_INVALID_PARAMETER);

			if (Status == STATUS_INVALID_PARAMETER) {
				KexMessageBox(
					MB_ICONEXCLAMATION | MB_OK,
					_(L"Application Error (VxKex)"),
					_(L"The registry setting \"KEX_DllRewriteEntries\" has invalid syntax. "
					  L"Some or all of the rewrite entries may not have been applied."));
			}
		}

		//
		// After app-specific hacks are initialized, the IfeoParameters are
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
		// Perform version spoofing, if required.
		//

		KexApplyVersionSpoof();

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
		// Rewrite DLL Imports of our main application EXE.
		//

		Status = KexRewriteImageImportDirectory(
			Peb->ImageBaseAddress,
			&KexData->ImageBaseName,
			&Peb->ProcessParameters->ImagePathName);

		if (!NT_SUCCESS(Status) && Status != STATUS_IMAGE_NO_IMPORT_DIRECTORY) {
			KexLogCriticalEvent(
				L"Failed to rewrite DLL imports of the main process image.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)\r\n"
				L"Image base address: 0x%p\r\n",
				KexRtlNtStatusToString(Status), Status,
				Peb->ImageBaseAddress);

			KexHeErrorBox(_(
				L"VxKex could not start because the DLL imports of the main "
				L"process image could not be rewritten. If the problem persists, "
				L"please disable VxKex for this program."));

			NOT_REACHED;
		}
	} else if (Reason == DLL_PROCESS_ATTACH) {
		STATIC ULONG CallNumber = 0;

		//
		// For static imports, this is called once with Descriptor as a PCONTEXT.
		// For dynamic imports, this is called once with Descriptor == NULL.
		//
		// For verifier loads, this is called once with Descriptor as a
		// PPRTL_VERIFIER_PROVIDER_DESCRIPTOR, and then once *again* with Descriptor
		// == NULL.
		//

		++CallNumber;

		if (CallNumber == 1) {
			//
			// Disable DLL_THREAD_ATTACH calls if we're not running as a verifier
			// provider.
			//

			if (AVrfProviderDescriptor.VerifierImage == NULL) {
				Status = LdrDisableThreadCalloutsForDll(DllBase);
				ASSERT (NT_SUCCESS(Status));
			} else {
				// Call the ABTI component on the loader initialization thread.
				KexAlertByThreadIdThreadAttach();
			}
		}
	} else if (Reason == DLL_THREAD_ATTACH) {
		ASSERT (AVrfProviderDescriptor.VerifierImage != NULL);

		if (Teb->InitialThread) {
			//
			// Queue an APC in order to work around MacType being incompatible with
			// Application Verifier.
			//
			// We'll ignore failure in release builds, since this is only mandatory
			// for when MacType is enabled (which is a minority of systems).
			//

			Status = NtQueueApcThread(
				NtCurrentThread(),
				KexPostInitializationApcRoutine,
				NULL,
				NULL,
				NULL);

			ASSERT (NT_SUCCESS(Status));
		}

		// Call back to the NtAlertThreadByThreadId/NtWaitForAlertByThreadId
		// component since a small piece of initialization code needs to run
		// per-thread.
		KexAlertByThreadIdThreadAttach();
	} else if (Reason == DLL_PROCESS_DETACH) {
		// Close log, if it's open, so that all log entries are properly flushed.
		VxlCloseLog(&KexData->LogHandle);

		if (Descriptor == NULL) {
			// When Descriptor is NULL, we're being unloaded from the process and the
			// process will continue running without us. So we have to free extra
			// resources.

			SafeClose(KexData->BaseNamedObjects);
			SafeClose(KexData->UntrustedNamedObjects);
			SafeClose(KexData->GlobalKeyedEvent);

			// Safe to call even if never initialized.
			MlsCleanup();
		}
	}

	return TRUE;
}