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
//     vxiiduu              29-Nov-2025  Detect Deno and Node.js as Chromium processes
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This function is called when we reach a conclusion that the current process
// either is Chromium/Electron, or has loaded a Chromium-based framework.
//
NTSTATUS AshSetIsChromiumProcess(
	VOID)
{
	NTSTATUS Status;

	ASSUME (KexData != NULL);
	ASSUME (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM));

	KexData->Flags |= KEXDATA_FLAG_CHROMIUM;

	KexLogInformationEvent(L"Detected Chromium, applying compatibility fixes");

	//
	// Chromium uses dwrite factories introduced in win10.
	// Will AV if we don't use the win10 dwrite.
	//

	Status = AshSelectDWriteImplementation(DWriteWindows10Implementation);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Ensure the random number generator is initialized as soon as possible.
	// Once Chromium locks down the sandbox this call will fail.
	//

	Status = KexRtlInitializeKsec();
	ASSERT (NT_SUCCESS(Status));

	return Status;
}

NTSTATUS AshPerformChromiumDetectionFromModuleExports(
	IN	PCVOID	ModuleBase)
{
	NTSTATUS Status;
	ANSI_STRING GetHandleVerifier;
	ANSI_STRING IsSandboxedProcess;
	ANSI_STRING NapiGetVersion;
	PVOID ProcedureAddress;

	ASSUME (ModuleBase != NULL);

	RtlInitConstantAnsiString(&GetHandleVerifier, "GetHandleVerifier");
	RtlInitConstantAnsiString(&IsSandboxedProcess, "IsSandboxedProcess");
	RtlInitConstantAnsiString(&NapiGetVersion, "napi_get_version");

	//
	// napi_get_version is exported from the NodeJS and Deno JavaScript
	// runtimes, which are based on the Chromium V8 javascript engine.
	// They require the same compatibility fixes as Chromium, so we'll
	// treat them identically.
	//
	// Specifically, they require VirtualAlloc2 being hidden from them
	// in order to avoid an out of memory error.
	//

	Status = LdrGetProcedureAddress(
		ModuleBase,
		&NapiGetVersion,
		0,
		&ProcedureAddress);

	ASSERT (
		NT_SUCCESS(Status) ||
		Status == STATUS_PROCEDURE_NOT_FOUND ||
		Status == STATUS_ENTRYPOINT_NOT_FOUND);

	if (NT_SUCCESS(Status)) {
		goto IsChromiumProcess;
	}

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

IsChromiumProcess:
	Status = AshSetIsChromiumProcess();
	ASSERT (NT_SUCCESS(Status));

	return Status;
}