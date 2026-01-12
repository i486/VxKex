///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllnotif.c
//
// Abstract:
//
//     Contains a routine which is called after a DLL is mapped into the
//     process, but before its imports are resolved. This is the opportune
//     moment to rewrite DLL imports.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//     vxiiduu              23-Feb-2024  Change wording from "loaded" to "mapped"
//                                       in order to better reflect reality.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This function is called within a try/except block inside NTDLL. So if we
// fuck up here, nothing super bad is going to happen, although of course it
// should still be avoided.
//
// NotificationData->Flags contains values starting with LDR_DLL_LOADED_FLAG_
// and it can (in Windows XP) only be 0 or 1. A value of 1 means that the DLL
// has been relocated. In Windows 7, NotificationData->Flags is always zero 
// and there is no information that can be gathered through this flag.
//
// This function is not called to notify for DLL unloads if the process is
// exiting. This is because, as an optimization, the LdrUnloadDll (XP) or 
// LdrpUnloadDll (Win7) routine does not actually unmap any DLLs when the
// process is exiting.
//
VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context OPTIONAL)
{
	NTSTATUS Status;
	STATIC CONST PCWSTR ReasonToStringLookup[] = {
		L"(unknown)",
		L"mapped",
		L"unmapped",
		L"(unknown)"
	};

	KexLogDetailEvent(
		L"The DLL %wZ was %s\r\n\r\n"
		L"Full path to the DLL: \"%wZ\"\r\n"
		L"Loaded at 0x%p (size: %I32u bytes)",
		NotificationData->BaseDllName,
		ARRAY_LOOKUP_BOUNDS_CHECKED(ReasonToStringLookup, Reason),
		NotificationData->FullDllName,
		NotificationData->DllBase,
		NotificationData->SizeOfImage);

	if (Reason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		BOOLEAN ShouldRewriteImports;

		ShouldRewriteImports = KexShouldRewriteImportsOfDll(
			NotificationData->BaseDllName,
			NotificationData->FullDllName);

		if (ShouldRewriteImports) {
			Status = KexRewriteImageImportDirectory(
				NotificationData->DllBase,
				NotificationData->BaseDllName,
				NotificationData->FullDllName);
		}

		// APPSPECIFICHACK: If Qt6 dlls gets loaded, apply environment variable ASH.
		// We cannot apply this ASH unconditionally, because if a Qt5 application
		// sees it, it will disable theming and look ugly and use a shitty file picker.
		unless (KexData->IfeoParameters.DisableAppSpecific) {
			STATIC BOOLEAN AlreadyApplied = FALSE;
			UNICODE_STRING Qt6;

			unless (AlreadyApplied) {
				RtlInitConstantUnicodeString(&Qt6, L"Qt6Gui.dll");

				if (NotificationData->BaseDllName) {
					if (KexRtlUnicodeStringEndsWith(NotificationData->FullDllName, &Qt6, TRUE)) {
						AshApplyQt6EnvironmentVariableHacks();
						AlreadyApplied = TRUE;
					}
				}
			}
		}
	}
}