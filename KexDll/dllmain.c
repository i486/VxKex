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

STATIC PCWSTR DllMainReasonToStringLookup[] = {
	L"DLL_PROCESS_DETACH",
	L"DLL_PROCESS_ATTACH",
	L"DLL_THREAD_ATTACH",
	L"DLL_THREAD_DETACH",
	L"DLL_PROCESS_VERIFIER",
	L"unknown"
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
	IN	PRTL_VERIFIER_PROVIDER_DESCRIPTOR	*Descriptor) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PVOID DllNotificationCookie;

	if (!KexData) {
		//
		// First off, initialize the KexData structure, since it contains
		// some basic data which we will need for logging, etc.
		//

		KexDataInitialize(&KexData);
		KexData->KexDllBase = DllBase;
	}
	
	if (Reason == DLL_PROCESS_VERIFIER) {
		//
		// Open log file.
		//

		KexOpenVxlLogForCurrentApplication(&KexData->LogHandle);

		//
		// Connect to KexSrv and tell him that we exist now.
		//

		KexSrvOpenChannel(&KexData->SrvChannel);
		KexSrvNotifyProcessStart(KexData->SrvChannel, &KexData->ImageBaseName);

		//
		// Install VxKex hard error handler.
		// This is responsible for displaying the custom error messages e.g.
		// when a DLL is not found.
		//

		KexHeInstallHandler();

		//
		// Initialize Advanced logging (displaying DbgPrint and DbgPrintEx
		// messages in the VxKex log).
		//
		
		KexInitializeAdvancedLogging();

		//
		// Try to get rid of as much Application verifier functionality as
		// possible.
		//

		KexDisableAVrf();

		//
		// Initialize DLL rewrite subsystem.
		//

		Status = KexInitializeDllRewrite();
		if (!NT_SUCCESS(Status)) {
			KexLogCriticalEvent(
				L"Failed to initialize DLL rewrite subsystem\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));

			// Abort initialization of VxKex.
			KexHeErrorBox(
				L"VxKex could not start because the DLL rewrite subsystem "
				L"could not be initialized. Try to reinstall VxKex, since "
				L"this problem may be caused by missing registry keys.\r\n"
				L"If the problem persists, please disable VxKex for this "
				L"program.");
		}

		//
		// Register our DLL load/unload callback.
		//

		Status = LdrRegisterDllNotification(
			0,
			KexDllNotificationCallback,
			NULL,
			&DllNotificationCookie);

		if (NT_SUCCESS(Status)) {
			KexLogInformationEvent(L"Successfully registered DLL notification callback.");
		} else {
			KexLogCriticalEvent(
				L"Failed to register DLL notification callback\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));

			KexHeErrorBox(
				L"VxKex could not start because the DLL notification callback "
				L"could not be installed. If the problem persists, please disable "
				L"VxKex for this program.");
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
				L"NTSTATUS error code: %s\r\n"
				L"Image base address: 0x%p\r\n",
				KexRtlNtStatusToString(Status),
				NtCurrentPeb()->ImageBaseAddress);

			KexHeErrorBox(
				L"VxKex could not start because the DLL imports of the main "
				L"process image could not be rewritten. If the problem persists, "
				L"please disable VxKex for this program.");
		}

		//
		// Initialize Propagation subsystem.
		//

		KexInitializePropagation();

		//
		// Perform version spoofing, if required.
		//

		KexApplyVersionSpoof();

		//
		// Register a useless descriptor with app verifier system.
		// We don't make use of any app verifier apis or specific functionality
		// at the moment, but if we don't do this the process will crash.
		//

		*Descriptor = &AVrfProviderDescriptor;
	} else if (Reason == DLL_PROCESS_DETACH) {
		VxlCloseLog(&KexData->LogHandle);
	}

	VxlWriteLog(
		KexData->LogHandle,
		KEX_COMPONENT,
		LogSeverityDebug,
		L"Test log entry");

	KexLogDebugEvent(
		L"DllMain called\r\n\r\n"
		L"DllBase = %p\r\n"
		L"Reason = %lu (%s)\r\n"
		L"Descriptor = 0x%p",
		DllBase,
		Reason,
		ARRAY_LOOKUP_BOUNDS_CHECKED(DllMainReasonToStringLookup, Reason),
		Descriptor);
	
	return TRUE;
} PROTECTED_FUNCTION_END_BOOLEAN