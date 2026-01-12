///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     elevate.c
//
// Abstract:
//
//     Functions related to elevation of privileges necessary to set VxKex
//     settings as a non-elevated user.
//
//     We want users to be able to change VxKex configuration without accepting
//     a UAC prompt every time. However, normal users and non-elevated admins
//     cannot write to the IFEO key, which we need to do.
//
//     The solution we have chosen is to set up a scheduled task which runs
//     as the local SYSTEM account. This scheduled task starts a helper
//     process, KexCfg.exe, with command-line arguments which indicate which
//     program to change VxKex configuration for and the configuration itself.
//
// Author:
//
//     vxiiduu (03-Feb-2024)
//
// Environment:
//
//     Win32 mode. This code must be able to run under the LOCAL SYSTEM
//     account.
//
// Revision History:
//
//     vxiiduu              03-Feb-2024  Initial creation.
//     vxiiduu              22-Feb-2024  Use SafeRelease instead of if statement.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>
#include <taskschd.h>

//
// This function checks whether we need to elevate privileges in order to
// write to IFEO. The way we do this is simply by trying to open the key,
// and if we get STATUS_ACCESS_DENIED, then we can conclude we need to
// elevate. Simple.
//
BOOLEAN KxCfgpElevationRequired(
	VOID)
{
	NTSTATUS Status;
	HANDLE KeyHandle;
	UNICODE_STRING KeyName;
	OBJECT_ATTRIBUTES ObjectAttributes;

	RtlInitConstantUnicodeString(
		&KeyName,
		L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options");

	InitializeObjectAttributes(&ObjectAttributes, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenKey(
		&KeyHandle,
		KEY_READ | KEY_WRITE,
		&ObjectAttributes);

	if (Status == STATUS_ACCESS_DENIED) {
		return TRUE;
	} else {
		NtClose(KeyHandle);
		return FALSE;
	}
}

//
// This function is called when we are a NON elevated process which wants
// to set VxKex configuration for a program.
//
// In this function we will try to run the "VxKex Configuration Elevation Task"
// scheduled task. If that fails (for example: Task scheduler service not
// running, or the user deleted the scheduled task for some reason), we'll
// fall back to using ShellExecute and the UAC dialog might appear.
//
// The return value of this function is not a hard guarantee. If it returns
// FALSE, it definitely means the configuration was not applied. But if it
// returns TRUE that just means all the API calls succeeded. There is no
// verification to check whether the configuration was ACTUALLY applied in
// the registry.
//
// The KexCfg.exe helper program uses transactions to apply configuration,
// so we do have a guarantee that configuration won't be partially applied.
//
BOOLEAN KxCfgpElevatedSetConfiguration(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration)
{
	BOOLEAN Success;

	ASSERT (ExeFullPath != NULL);
	ASSERT (Configuration != NULL);
	ASSERT (!PathIsRelative(ExeFullPath));

	// make sure this function doesn't get called by accident
	ASSERT (KxCfgpElevationRequired() == TRUE);

	Success = KxCfgpElevatedSetConfigurationTaskScheduler(ExeFullPath, Configuration);

	if (!Success) {
		// fallback
		Success = KxCfgpElevatedSetConfigurationShellExecute(ExeFullPath, Configuration);
	}

	return Success;
}

BOOLEAN KxCfgpElevatedDeleteConfiguration(
	IN	PCWSTR							ExeFullPath)
{
	KXCFG_PROGRAM_CONFIGURATION NullConfiguration;
	RtlZeroMemory(&NullConfiguration, sizeof(NullConfiguration));
	return KxCfgpElevatedSetConfiguration(ExeFullPath, &NullConfiguration);
}

BOOLEAN KxCfgpElevatedSetConfigurationTaskScheduler(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration)
{
	BOOLEAN Success;
	HRESULT Result;
	VARIANT VariantNull;
	VARIANT VariantArgs;
	LPSAFEARRAY ArgsArray;
	SAFEARRAYBOUND ArgsArrayBounds;
	WCHAR Args[512];
	LONG Zero;

	ITaskService *TaskService;
	ITaskFolder *TaskFolder;
	IRegisteredTask *KexCfgTask;

	ASSERT (ExeFullPath != NULL);
	ASSERT (Configuration != NULL);
	ASSERT (!PathIsRelative(ExeFullPath));

	ZeroMemory(&VariantNull, sizeof(VariantNull));
	ArgsArray = NULL;
	Zero = 0;
	TaskService = NULL;
	TaskFolder = NULL;
	KexCfgTask = NULL;

	Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(Result)) {
		return FALSE;
	}

	try {
		Result = CoCreateInstance(
			&CLSID_TaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			&IID_ITaskService,
			(PPVOID) &TaskService);

		if (FAILED(Result)) {
			return FALSE;
		}

		ASSERT (TaskService != NULL);

		//
		// Connect to the Task Scheduler service on the local machine.
		//

		Result = ITaskService_Connect(
			TaskService,
			VariantNull,
			VariantNull,
			VariantNull,
			VariantNull);

		if (FAILED(Result)) {
			return FALSE;
		}

		//
		// Get an ITaskFolder interface for the root Task Scheduler folder.
		// (i.e. %SystemRoot%\system32\Tasks)
		//

		Result = ITaskService_GetFolder(
			TaskService,
			L"\\",
			&TaskFolder);

		if (FAILED(Result)) {
			return FALSE;
		}

		ASSERT (TaskFolder != NULL);

		//
		// Get an IRegisteredTask interface for the KexCfg elevation task.
		//

		Result = ITaskFolder_GetTask(
			TaskFolder,
			KXCFG_ELEVATION_SCHTASK_NAME,
			&KexCfgTask);

		if (FAILED(Result)) {
			return FALSE;
		}

		//
		// Assemble the command line to pass to KexCfg.
		// Note that /SCHTASK is automatically passed before these args because
		// it was included in the task definition XML.
		//

		Success = KxCfgpAssembleKexCfgCommandLine(
			Args,
			ARRAYSIZE(Args),
			ExeFullPath,
			Configuration);

		if (!Success) {
			ASSERT (Success);
			return FALSE;
		}
		
		//
		// Prepare a SAFEARRAY variant to pass to IRegisteredTask_Run().
		// I found through a lot of experimentation that this is required to
		// make the variable arguments feature of Task Scheduler actually work,
		// even though the documentation implies that you can just pass a BSTR
		// variant (and this in fact does not create an error code at all).
		//
		// This entire SAFEARRAY thing, like all the rest of COM "technology",
		// is an overcomplicated, obtuse piece of shit.
		//

		ArgsArrayBounds.cElements	= 1;
		ArgsArrayBounds.lLbound		= 0;

		ArgsArray = SafeArrayCreate(VT_BSTR, 1, &ArgsArrayBounds);
		if (!ArgsArray) {
			return FALSE;
		}

		// The string allocated by SysAllocString is automatically freed by the
		// SafeArrayDestroy function, which is called in the SEH finally block.
		Result = SafeArrayPutElement(
			ArgsArray,
			&Zero,
			SysAllocString(Args));

		if (FAILED(Result)) {
			return FALSE;
		}

		VariantArgs.vt		= VT_ARRAY | VT_BSTR;
		VariantArgs.parray	= ArgsArray;

		//
		// Call the IRegisteredTask_Run method to finally call our task.
		//

		Result = IRegisteredTask_Run(
			KexCfgTask,
			VariantArgs,
			NULL);

		if (FAILED(Result)) {
			return FALSE;
		}

	} finally {
		SafeRelease(TaskService);
		SafeRelease(TaskFolder);
		SafeRelease(KexCfgTask);

		if (ArgsArray) {
			SafeArrayDestroy(ArgsArray);
			ArgsArray = NULL;
		}

		CoUninitialize();
	}

	return TRUE;
}

BOOLEAN KxCfgpElevatedSetConfigurationShellExecute(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration)
{
	BOOLEAN Success;
	WCHAR KexCfgFullPath[MAX_PATH];
	WCHAR Args[512];
	HINSTANCE ShellExecuteError;

	//
	// get full path to KexCfg.exe
	//

	Success = KxCfgGetKexDir(
		KexCfgFullPath,
		ARRAYSIZE(KexCfgFullPath));

	if (!Success) {
		return FALSE;
	}

	PathCchAppend(KexCfgFullPath, ARRAYSIZE(KexCfgFullPath), L"KexCfg.exe");

	//
	// assemble command line for KexCfg
	//

	Success = KxCfgpAssembleKexCfgCommandLine(
		Args,
		ARRAYSIZE(Args),
		ExeFullPath,
		Configuration);

	if (!Success) {
		return FALSE;
	}
	
	//
	// call KexCfg elevated using ShellExecute
	//

	ShellExecuteError = ShellExecute(
		NULL,
		L"runas",
		KexCfgFullPath,
		Args,
		NULL,
		SW_SHOWNORMAL);

	if ((ULONG) ShellExecuteError <= 32) {
		return FALSE;
	}

	return TRUE;
}

BOOLEAN KxCfgpAssembleKexCfgCommandLine(
	OUT	PWSTR							Buffer,
	IN	ULONG							BufferCch,
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration)
{
	HRESULT Result;

	ASSERT (Buffer != NULL);
	ASSERT (BufferCch != 0);
	ASSERT (ExeFullPath != NULL);
	ASSERT (!PathIsRelative(ExeFullPath));
	ASSERT (Configuration != NULL);

	Result = StringCchPrintf(
		Buffer,
		BufferCch,
		L"/EXE:\"%s\" /ENABLE:%lu /DISABLEFORCHILD:%lu "
		L"/DISABLEAPPSPECIFIC:%lu /WINVERSPOOF:%lu /STRONGSPOOF:%08x",
		ExeFullPath,
		Configuration->Enabled,
		Configuration->DisableForChild,
		Configuration->DisableAppSpecificHacks,
		Configuration->WinVerSpoof,
		Configuration->StrongSpoofOptions);

	ASSERT (SUCCEEDED(Result));

	return SUCCEEDED(Result);
}