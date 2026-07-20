///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ashdetec.c
//
// Abstract:
//
//     Contains routines for detecting common application frameworks based on
//     the names of the DLLs they load.
//
// Author:
//
//     vxiiduu (19-May-2026)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              19-May-2026  Initial creation.
//     vxiiduu              25-May-2026  QSG environment variable hack
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

NTSTATUS AshSetIsQt6Process(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING Variable;
	UNICODE_STRING Value;

	Status = AshSelectDWriteImplementation(DWriteWindows10Implementation);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Users have reported that these variables can fix certain Qt6 programs
	// which make use of Qt Quick Scene Graph (QSG).
	// https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html
	// https://github.com/YuZhouRen86/VxKex-NEXT/issues/336
	// Affected apps: AmneziaVPN, GPT4All
	//

	RtlInitConstantUnicodeString(&Variable, L"QSG_RHI_BACKEND");
	RtlInitConstantUnicodeString(&Value, L"opengl");

	Status = RtlSetEnvironmentVariable(NULL, &Variable, &Value);
	ASSERT (NT_SUCCESS(Status));

	KexData->Flags |= KEXDATA_FLAG_QT6;
	return STATUS_SUCCESS;
}

NTSTATUS AshSetIsDotnetProcess(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING Variable;
	UNICODE_STRING Value;

	RtlInitConstantUnicodeString(&Variable, L"DOTNET_EnableWriteXorExecute");
	RtlInitConstantUnicodeString(&Value, L"0");

	Status = RtlSetEnvironmentVariable(NULL, &Variable, &Value);
	ASSERT (NT_SUCCESS(Status));

	KexData->Flags |= KEXDATA_FLAG_DOTNET;
	return STATUS_SUCCESS;
}

//
// Call this function to inform the App-Specific Hack subsystem of a new
// loaded DLL.
//

VOID AshDllLoadNotification(
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData)
{
	NTSTATUS Status;
	UNICODE_STRING BaseName;
	UNICODE_STRING TargetName;

	ASSERT (NotificationData != NULL);
	ASSERT (!KexData->IfeoParameters.DisableAppSpecific);

	//
	// Figure out the base name of the DLL.
	// We do this instead of using the BaseDllName member of the notification
	// data struct because the BaseDllName there may or may not include the
	// .DLL extension.
	//

	Status = KexRtlPathFindFileName(
		NotificationData->FullDllName,
		&BaseName);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return;
	}

	unless (KexData->Flags & KEXDATA_FLAG_CHROMIUM) {
		UNICODE_STRING LibCef;
		UNICODE_STRING Qt6WebEngineCore;

		//
		// APPSPECIFICHACK: Here, we detect CEF and qtwebengine because those are the
		// main Chromium- based frameworks which get loaded in external DLLs. Electron
		// can be detected through module exports of the main EXE, and actual Chrome/
		// Opera/Firefox browsers can be detected through the same method.
		//
		// TODO: Get rid of this somehow. It's extra crap that gets called for every
		// loaded DLL, although we have somewhat mitigated this penalty in the DLL
		// notification function by only calling this function when a non-Windows DLL
		// is loaded.
		//

		RtlInitConstantUnicodeString(&LibCef, L"libcef.dll");
		RtlInitConstantUnicodeString(&Qt6WebEngineCore, L"Qt6WebEngineCore.dll");

		if (RtlEqualUnicodeString(&BaseName, &LibCef, TRUE) ||
			RtlEqualUnicodeString(&BaseName, &Qt6WebEngineCore, TRUE)) {

			Status = AshSetIsChromiumProcess();
			ASSERT (NT_SUCCESS(Status));
			return;
		}
	}

	unless (KexData->Flags & KEXDATA_FLAG_QT6) {
		//
		// APPSPECIFICHACK: Newer versions of Qt6 require Windows 10 DWrite.
		// Otherwise, text is displayed as blank boxes.
		//

		RtlInitConstantUnicodeString(&TargetName, L"Qt6");

		if (RtlPrefixUnicodeString(&TargetName, &BaseName, TRUE)) {
			Status = AshSetIsQt6Process();
			ASSERT (NT_SUCCESS(Status));
			return;
		}
	}

	unless (KexData->Flags & KEXDATA_FLAG_DOTNET) {
		//
		// APPSPECIFICHACK: New versions of .NET (at least 7.0 and up) will
		// consume large amounts of kernel-mode memory due to them assuming Windows
		// 8.1 or higher. We can set DOTNET_EnableWriteXorExecute=0 in the environment
		// variables to work around this.
		//

		RtlInitConstantUnicodeString(&TargetName, L"coreclr.dll");

		if (RtlEqualUnicodeString(&BaseName, &TargetName, TRUE)) {
			Status = AshSetIsDotnetProcess();
			ASSERT (NT_SUCCESS(Status));
			return;
		}
	}

	RtlInitConstantUnicodeString(&TargetName, L"mimalloc.dll");

	if (RtlEqualUnicodeString(&BaseName, &TargetName, TRUE)) {
		UNICODE_STRING BCryptDllName;
		PVOID BCryptDllHandle;
		BOOL (WINAPI *BCryptDllMain)(HMODULE, ULONG, PVOID);

		//
		// APPSPECIFICHACK: Some versions of mimalloc have a bug where they try to call
		// into bcrypt.dll during DllMain which crashes because BCrypt is not yet
		// initialized. Apparently they get away with it on Win10. Anyway, the game
		// "Teardown" uses such a buggy version of mimalloc. The solution is to manually
		// call BCrypt.dll's initialization routine (this is safe to do because BCrypt
		// code is pretty well written and will set a flag to indicate initialization,
		// which means it's safe to call its DllMain multiple times).
		//

		RtlInitConstantUnicodeString(&BCryptDllName, L"bcrypt.dll");

		Status = LdrLoadDll(
			NULL,
			NULL,
			&BCryptDllName,
			&BCryptDllHandle);

		ASSERT (NT_SUCCESS(Status));

		if (NT_SUCCESS(Status)) {
			Status = KexLdrFindImageEntryPoint(
				BCryptDllHandle,
				(PPVOID) &BCryptDllMain);

			ASSERT (NT_SUCCESS(Status));

			if (NT_SUCCESS(Status)) {
				BOOL Success;

				KexLogInformationEvent(L"Applying app-specific hack for mimalloc");

				Success = BCryptDllMain(
					(HMODULE) BCryptDllHandle,
					DLL_PROCESS_ATTACH,
					NULL);

				ASSERT (Success);
			}
		}
	}
}