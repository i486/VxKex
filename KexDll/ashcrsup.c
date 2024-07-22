///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ashcrsup.c
//
// Abstract:
//
//     This file contains routines which detect Chromium-based browsers and
//     frameworks.
//
// Author:
//
//     vxiiduu (16-Mar-2024)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              16-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This function is called when we reach a conclusion that the current process
// either is Chromium/Electron, or has loaded a Chromium-based framework.
//
STATIC NTSTATUS AshpSetIsChromiumProcess(
	VOID)
{
	NTSTATUS Status;

	ASSUME (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM));

	KexLogInformationEvent(L"Detected Chromium, applying compatibility fixes");

	//
	// Chromium uses dwrite factories introduced in win10.
	// Will AV if we don't use the win10 dwrite.
	//

	Status = AshSelectDWriteImplementation(DWriteWindows10Implementation);
	ASSERT (NT_SUCCESS(Status));

	KexData->Flags |= KEXDATA_FLAG_CHROMIUM;

	return Status;
}

//
// This function is called from within the DLL notification routine (dllnotif.c)
// and we're supposed to infer from the loaded DLL whether it's some kind of
// Chromium DLL.
//
// Here, we detect CEF and qtwebengine because those are the main Chromium-
// based frameworks which get loaded in external DLLs. Electron can be detected
// through module exports of the main EXE, and actual Chrome/Opera/Firefox
// browsers can be detected through the same method.
//
// TODO: Get rid of this somehow. It's extra crap that gets called for every
// loaded DLL, although we have somewhat mitigated this penalty in the DLL
// notification function by only calling this function when a non-Windows DLL
// is loaded.
//
NTSTATUS AshPerformChromiumDetectionFromLoadedDll(
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData)
{
	NTSTATUS Status;
	UNICODE_STRING LibCef;
	UNICODE_STRING Qt6WebEngineCore;
	UNICODE_STRING BaseName;

	ASSUME (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM));

	Status = KexRtlPathFindFileName(NotificationData->FullDllName, &BaseName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	RtlInitConstantUnicodeString(&LibCef, L"libcef.dll");
	RtlInitConstantUnicodeString(&Qt6WebEngineCore, L"Qt6WebEngineCore.dll");

	if (RtlEqualUnicodeString(&BaseName, &LibCef, TRUE) ||
		RtlEqualUnicodeString(&BaseName, &Qt6WebEngineCore, TRUE)) {

		Status = AshpSetIsChromiumProcess();
		ASSERT (NT_SUCCESS(Status));
	}

	return Status;
}

NTSTATUS AshPerformChromiumDetectionFromModuleExports(
	IN	PVOID	ModuleBase)
{
	NTSTATUS Status;
	ANSI_STRING GetHandleVerifier;
	ANSI_STRING IsSandboxedProcess;
	PVOID ProcedureAddress;

	ASSUME (ModuleBase != NULL);

	RtlInitConstantAnsiString(&GetHandleVerifier, "GetHandleVerifier");
	RtlInitConstantAnsiString(&IsSandboxedProcess, "IsSandboxedProcess");

	//
	// These two function names are exported from Chrome and Electron
	// EXEs. Also from firefox too because firefox uses the same sandbox
	// as chrome.
	//

	Status = LdrGetProcedureAddress(
		ModuleBase,
		&GetHandleVerifier,
		0,
		&ProcedureAddress);

	ASSERT (
		NT_SUCCESS(Status) ||
		Status == STATUS_PROCEDURE_NOT_FOUND ||
		Status == STATUS_ENTRYPOINT_NOT_FOUND);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = LdrGetProcedureAddress(
		ModuleBase,
		&IsSandboxedProcess,
		0,
		&ProcedureAddress);

	ASSERT (
		NT_SUCCESS(Status) ||
		Status == STATUS_PROCEDURE_NOT_FOUND ||
		Status == STATUS_ENTRYPOINT_NOT_FOUND);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Both functions found. This is a chromium process.
	//

	Status = AshpSetIsChromiumProcess();
	ASSERT (NT_SUCCESS(Status));

	return Status;
}