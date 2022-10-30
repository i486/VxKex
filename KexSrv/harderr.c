///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     harderr.c
//
// Abstract:
//
//     Displays a better looking and more informative Hard Error dialog than
//     the stock one when a kex process crashes.
//
// Author:
//
//     vxiiduu (24-Oct-2022)
//
// Revision History:
//
//     vxiiduu               24-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexsrvp.h"
#include <KexDll.h>
#include <ShellAPI.h>

// Can't start at 1 or it will conflict with other system defined buttons
// e.g. the window close button
#define TDBTN_REPORT 10
#define TDBTN_VIEWLOG 11
#define TDBTN_EXIT 12

STATIC HRESULT CALLBACK KexSrvpHardErrorTaskDialogCallback(
	IN	HWND		TaskDialogWindow,
	IN	UINT		Notification,
	IN	WPARAM		WParam,
	IN	LPARAM		LParam,
	IN	LONG_PTR	Context)
{
	PKEXSRV_PER_CLIENT_INFO ClientInfo;

	ClientInfo = (PKEXSRV_PER_CLIENT_INFO) Context;

	if (Notification == TDN_CREATED) {
		//
		// Sometimes, the task dialog can display behind other
		// windows. We want the user to know about the failure.
		//
		// If the user wants to temporarily get rid of the task
		// dialog, he can press the Minimize button.
		//

		SetWindowPos(TaskDialogWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	} else if (Notification == TDN_BUTTON_CLICKED) {
		//
		// We don't want to still be on top of other windows once
		// the user decides what to do.
		//
		SetWindowPos(TaskDialogWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		//
		// Returning S_FALSE when a button is clicked will not close
		// the task dialog. But returning S_OK will close it.
		//
		switch (WParam) {
		case TDBTN_REPORT:
			ShellExecute(NULL, NULL, _L(KEX_BUGREPORT_STR), NULL, NULL, SW_SHOWNORMAL);
			return S_FALSE;
		case TDBTN_VIEWLOG:
			ShellExecute(NULL, NULL, ClientInfo->LogFilePath, NULL, NULL, SW_SHOWNORMAL);
			return S_FALSE;
		default:
			return S_OK;
		}
	}

	return S_OK;
}

VOID KexSrvpHardError(
	IN	PKEXSRV_PER_CLIENT_INFO	ClientInfo,
	IN	NTSTATUS				Status,
	IN	ULONG					UlongParameter,
	IN	PCWSTR					StringParameter1,
	IN	PCWSTR					StringParameter2)
{
	WCHAR MainTitleText[64];
	WCHAR MainBodyText[384];
	WCHAR ExpandText[256];
	HRESULT Result;
	TASKDIALOGCONFIG TaskDialogConfig;
	TASKDIALOG_BUTTON TaskDialogButtons[] = {
		{ TDBTN_REPORT, L"Report bug" },
		{ TDBTN_VIEWLOG, L"View log" },
		{ TDBTN_EXIT, L"Exit" }
	};

	RtlZeroMemory(&TaskDialogConfig, sizeof(TASKDIALOGCONFIG));

	StringCchPrintf(
		MainTitleText, ARRAYSIZE(MainTitleText),
		L"%s failed to start",
		ClientInfo->ApplicationName);

	//
	// The possible NTSTATUS values at the top level are:
	//
	//   STATUS_INVALID_IMAGE_FORMAT (from LdrpFindOrMapDll)
	//   STATUS_IMAGE_MACHINE_TYPE_MISMATCH (from LdrpFindOrMapDll)
	//   STATUS_APP_INIT_FAILURE (from LdrpInitializationFailure)
	//   STATUS_DLL_NOT_FOUND (from LdrpLoadImportModule)
	//   STATUS_ENTRYPOINT_NOT_FOUND (from LdrpSnapThunk)
	//   STATUS_ORDINAL_NOT_FOUND (from LdrpSnapThunk)
	//
	// Note that when the primary NTSTATUS value (in the Status param
	// of this function) is STATUS_APP_INIT_FAILURE, the secondary
	// NTSTATUS (UlongParameter) can be anything. We just handle the
	// most common ones and then display the system-provided error
	// string if it's not one we handle.
	//
	// VxKex also defines some custom NTSTATUS values:
	//
	//   STATUS_KEXDLL_INITIALIZATION_FAILURE
	//
	switch (Status) {
	case STATUS_DLL_NOT_FOUND:
		TaskDialogConfig.pszWindowTitle = L"DLL Not Found";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"The required %s could not be found.",
			StringParameter1);
		break;
	case STATUS_ORDINAL_NOT_FOUND:
		TaskDialogConfig.pszWindowTitle = L"Missing Function In DLL";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"The required function ordinal #%lu could not be found in %s.",
			UlongParameter,
			StringParameter1);
		break;
	case STATUS_ENTRYPOINT_NOT_FOUND:
		TaskDialogConfig.pszWindowTitle = L"Missing Function In DLL";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"The required function %s was not found in %s.",
			StringParameter1,
			StringParameter2);
		break;
	case STATUS_INVALID_IMAGE_FORMAT:
		TaskDialogConfig.pszWindowTitle = L"Invalid or Corrupt Executable or DLL";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"%s appears to be invalid or corrupt.",
			StringParameter1);
		break;
	case STATUS_IMAGE_MACHINE_TYPE_MISMATCH:
		TaskDialogConfig.pszWindowTitle = L"Processor Architecture Mismatch";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"%s is designed for a different processor architecture.\r\n\r\n"
			L"This error is usually caused by attempting to load a 32-bit DLL "
			L"in a 64-bit process, or vice versa.",
			StringParameter1);
		break;
	case STATUS_APP_INIT_FAILURE:
		switch (UlongParameter) {
		case STATUS_ACCESS_VIOLATION:
			TaskDialogConfig.pszWindowTitle = L"Access Violation";
			TaskDialogConfig.pszContent = L"The application or a loaded DLL has attempted to access "
										  L"an invalid memory address. Unfortunately, the code module "
										  L"which has caused this failure is unknown.";
			break;
		case STATUS_DLL_INIT_FAILED:
			TaskDialogConfig.pszWindowTitle = L"DLL Initialization Failure";
			TaskDialogConfig.pszContent = L"One of the DLLs statically linked by the application "
										  L"has failed within its entry point. Unfortunately, the DLL "
										  L"which has caused this failure is unknown.";
			break;
		default:
			TaskDialogConfig.pszWindowTitle = L"Miscellaneous Error";
			TaskDialogConfig.pszContent = NtStatusAsString((NTSTATUS) UlongParameter);
			break;
		}

		break;
	case STATUS_KEXDLL_INITIALIZATION_FAILURE:
		TaskDialogConfig.pszWindowTitle = L"VxKex Initialization Failure";
		TaskDialogConfig.pszContent = StringParameter1;
		break;
	default:
		TaskDialogConfig.pszWindowTitle = L"Unknown Error";
		StringCchPrintf(
			MainBodyText, ARRAYSIZE(MainBodyText),
			L"An unknown error was encountered during process startup.\r\n"
			L"NTSTATUS code: 0x%08lx\r\n",
			L"%s",
			Status,
			NtStatusAsString((NTSTATUS) UlongParameter));
		break;
	}

	if (!TaskDialogConfig.pszContent) {
		TaskDialogConfig.pszContent = MainBodyText;
	}

	StringCchPrintf(
		ExpandText, ARRAYSIZE(ExpandText),
		L"NTSTATUS: 0x%08lx\r\n"
		L"UlongParameter: 0x%08lx\r\n"
		L"StringParameter1: %s\r\n"
		L"StringParameter2: %s",
		Status,
		UlongParameter,
		StringParameter1,
		StringParameter2);

	TaskDialogConfig.cbSize					= sizeof(TASKDIALOGCONFIG);
	TaskDialogConfig.dwFlags				= TDF_CAN_BE_MINIMIZED;
	TaskDialogConfig.pszWindowTitle			= L"Application Error (VxKex)";
	TaskDialogConfig.pszMainIcon			= TD_ERROR_ICON;
	TaskDialogConfig.pszMainInstruction		= MainTitleText;
	TaskDialogConfig.pszExpandedInformation	= ExpandText;
	TaskDialogConfig.cButtons				= ARRAYSIZE(TaskDialogButtons);
	TaskDialogConfig.pButtons				= TaskDialogButtons;
	TaskDialogConfig.nDefaultButton			= TDBTN_EXIT;
	TaskDialogConfig.pfCallback				= KexSrvpHardErrorTaskDialogCallback;
	TaskDialogConfig.lpCallbackData			= (LONG_PTR) ClientInfo;

	Result = TaskDialogIndirect(
		&TaskDialogConfig,
		NULL,
		NULL,
		NULL);

	ASSERT (SUCCEEDED(Result));
	if (FAILED(Result)) {
		return;
	}
}