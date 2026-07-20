#define NEED_VERSION_DEFS
#include "buildcfg.h"
#include "kexsetup.h"
#include <KseGuid.h>
#include <taskschd.h>

HANDLE KexSetupTransactionHandle = NULL;
BOOLEAN KexSetupOkToCommitTransaction = FALSE;
ULONG InstalledSize;

VOID KexSetupWriteUninstallEntry(
	VOID)
{
	HKEY KeyHandle;
	WCHAR FormattedDate[9]; // YYYYMMDD\0
	WCHAR UninstallString[MAX_PATH + 11]; // +11 for " /UNINSTALL"

	try {
		KexSetupCreateKey(
			HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex",
			KEY_READ | KEY_WRITE,
			&KeyHandle);

		ASSERT (KeyHandle != NULL);
		ASSERT (KeyHandle != INVALID_HANDLE_VALUE);

		GetDateFormat(LOCALE_INVARIANT, 0, NULL, L"yyyyMMdd", FormattedDate, ARRAYSIZE(FormattedDate));
		StringCchPrintf(UninstallString, ARRAYSIZE(UninstallString), L"%s\\KexSetup.exe /UNINSTALL", KexDir);

		KexSetupRegWriteString(KeyHandle, L"DisplayName",		L"VxKex API Extensions for Windows® 7");
		KexSetupRegWriteString(KeyHandle, L"DisplayVersion",	_L(KEX_VERSION_STR));
		KexSetupRegWriteString(KeyHandle, L"Publisher",			L"vxiiduu");
		KexSetupRegWriteString(KeyHandle, L"InstallDate",		FormattedDate);
		KexSetupRegWriteString(KeyHandle, L"InstallLocation",	KexDir);
		KexSetupRegWriteString(KeyHandle, L"UninstallString",	UninstallString);
		KexSetupRegWriteString(KeyHandle, L"HelpLink",			_L(KEX_WEB_STR));
		KexSetupRegWriteI32   (KeyHandle, L"EstimatedSize",		InstalledSize / 1024);
		KexSetupRegWriteI32   (KeyHandle, L"NoRepair",			TRUE);
		KexSetupRegWriteI32   (KeyHandle, L"NoModify",			TRUE);
	} finally {
		RegCloseKey(KeyHandle);
	}
}

//
// This function installs the KexCfg elevation scheduled task.
//
// Note: Errors in this function aren't critical.
// If the scheduled task can't be added, it's not a big deal: there's a fallback
// method for what it's supposed to achieve within the KxCfgHlp library.
//
VOID KexSetupAddKexCfgScheduledTask(
	VOID)
{
	HRESULT Result;
	VARIANT VariantNull;
	WCHAR TaskXml[4096];

	ITaskService *TaskService;
	ITaskFolder *TaskFolder;
	IRegisteredTask *KexCfgTask;

	ZeroMemory(&VariantNull, sizeof(VariantNull));
	TaskService = NULL;
	TaskFolder = NULL;
	KexCfgTask = NULL;

	Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (FAILED(Result)) {
		ASSERT (SUCCEEDED(Result));
		return;
	}

	try {
		Result = CoCreateInstance(
			&CLSID_TaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			&IID_ITaskService,
			(PPVOID) &TaskService);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Connect to the Task Scheduler service.
		//

		Result = ITaskService_Connect(
			TaskService,
			VariantNull,
			VariantNull,
			VariantNull,
			VariantNull);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Get the root Task Scheduler folder interface so we can use it to
		// register a task.
		//

		Result = ITaskService_GetFolder(
			TaskService,
			L"\\",
			&TaskFolder);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Format the XML string that we'll use to register the task.
		//

		Result = StringCchPrintf(
			TaskXml,
			ARRAYSIZE(TaskXml),
			L"<?xml version='1.0' encoding='UTF-16'?>\r\n"
			L"<Task version='1.3' xmlns='http://schemas.microsoft.com/windows/2004/02/mit/task'>\r\n"
			L"  <RegistrationInfo>\r\n"
			L"    <Author>vxiiduu</Author>\r\n"
			L"    <Source>VxKex</Source>\r\n"
			L"    <Description>\r\n"
			L"This scheduled task is run on-demand by VxKex components and utilities (for example, "
			L"the shell extension). It allows VxKex to be enabled, disabled, or configured for a program "
			L"without requiring you to interact with a User Account Control prompt. "
			L"You may safely disable or remove this scheduled task if you want. However, keep in mind that "
			L"if you do this, and you have User Account Control enabled, you may get a consent prompt every "
			L"time you configure VxKex for a program."
			L"    </Description>\r\n"
			L"  </RegistrationInfo>\r\n"
			L"  <Triggers />\r\n"
			L"  <Principals>\r\n"
			L"    <Principal>\r\n"
			L"      <UserId>S-1-5-18</UserId>\r\n"
			L"      <RunLevel>HighestAvailable</RunLevel>\r\n"
			L"    </Principal>\r\n"
			L"  </Principals>\r\n"
			L"  <Settings>\r\n"
			L"    <MultipleInstancesPolicy>Parallel</MultipleInstancesPolicy>\r\n"
			L"    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\r\n"
			L"    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\r\n"
			L"    <AllowHardTerminate>true</AllowHardTerminate>\r\n"
			L"    <StartWhenAvailable>false</StartWhenAvailable>\r\n"
			L"    <RunOnlyIfNetworkAvailable>false</RunOnlyIfNetworkAvailable>\r\n"
			L"    <IdleSettings>\r\n"
			L"      <StopOnIdleEnd>false</StopOnIdleEnd>\r\n"
			L"      <RestartOnIdle>false</RestartOnIdle>\r\n"
			L"    </IdleSettings>\r\n"
			L"    <AllowStartOnDemand>true</AllowStartOnDemand>\r\n"
			L"    <Enabled>true</Enabled>\r\n"
			L"    <Hidden>false</Hidden>\r\n"
			L"    <RunOnlyIfIdle>false</RunOnlyIfIdle>\r\n"
			L"    <DisallowStartOnRemoteAppSession>false</DisallowStartOnRemoteAppSession>\r\n"
			L"    <UseUnifiedSchedulingEngine>false</UseUnifiedSchedulingEngine>\r\n"
			L"    <WakeToRun>false</WakeToRun>\r\n"
			L"    <ExecutionTimeLimit>PT1H</ExecutionTimeLimit>\r\n"
			L"    <Priority>7</Priority>\r\n"
			L"  </Settings>\r\n"
			L"  <Actions>\r\n"
			L"    <Exec>\r\n"
			L"      <Command>%s\\KexCfg.exe</Command>\r\n"
			L"      <Arguments>/SCHTASK $(Arg0)</Arguments>\r\n"
			L"      <WorkingDirectory>%s</WorkingDirectory>\r\n"
			L"    </Exec>\r\n"
			L"  </Actions>\r\n"
			L"</Task>",
			KexDir,
			KexDir);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Register the task.
		//

		Result = ITaskFolder_RegisterTask(
			TaskFolder,
			KXCFG_ELEVATION_SCHTASK_NAME,
			TaskXml,
			TASK_CREATE_OR_UPDATE,
			VariantNull,
			VariantNull,
			TASK_LOGON_GROUP,
			VariantNull,
			&KexCfgTask);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Set the security descriptor on the task so that every user on the system
		// can run the task. This step is necessary to ensure that non-administrator
		// users can also use VxKex.
		//
		// Later on, if there is demand, we can also add a setting somewhere (perhaps
		// through the KexCfg global settings interface) to disable this so that non
		// admins can't access the settings.
		//
		// Owner: Builtin Administrator
		// Group: Builtin Administrator
		// Local System has full access.
		// Administrators have full access.
		// Users can read and execute.
		//

		Result = IRegisteredTask_SetSecurityDescriptor(
			KexCfgTask,
			L"O:BAG:BAD:AI(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGX;;;BU)",
			0);

		ASSERT (SUCCEEDED(Result));

	} finally {
		SafeRelease(TaskService);
		SafeRelease(TaskFolder);
		SafeRelease(KexCfgTask);

		CoUninitialize();
	}
}

//
// This function deletes the KexCfg elevation scheduled task.
//
// As with KexSetupAddKexCfgScheduledTask, errors in this function
// aren't critical. Even though we call it a "scheduled" task, it only
// ever runs on-demand, so if it never gets deleted because we can't
// delete it... well, besides a few kb's of cruft left on the computer,
// there's no harm.
//
VOID KexSetupRemoveKexCfgScheduledTask(
	VOID)
{
	HRESULT Result;
	VARIANT VariantNull;

	ITaskService *TaskService;
	ITaskFolder *TaskFolder;

	ZeroMemory(&VariantNull, sizeof(VariantNull));
	TaskService = NULL;
	TaskFolder = NULL;

	Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	try {
		Result = CoCreateInstance(
			&CLSID_TaskScheduler,
			NULL,
			CLSCTX_INPROC_SERVER,
			&IID_ITaskService,
			(PPVOID) &TaskService);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Connect to Task Scheduler service.
		//

		Result = ITaskService_Connect(
			TaskService,
			VariantNull,
			VariantNull,
			VariantNull,
			VariantNull);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Get the ITaskFolder interface so we can use it to delete the task
		//

		Result = ITaskService_GetFolder(
			TaskService,
			L"\\",
			&TaskFolder);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
			return;
		}

		//
		// Delete the KexCfg scheduled task.
		//

		Result = ITaskFolder_DeleteTask(
			TaskFolder,
			KXCFG_ELEVATION_SCHTASK_NAME,
			0);

		if (FAILED(Result)) {
			ASSERT (SUCCEEDED(Result));
		}

	} finally {
		if (TaskService) {
			ITaskService_Release(TaskService);
			TaskService = NULL;
		}

		if (TaskFolder) {
			ITaskFolder_Release(TaskFolder);
			TaskFolder = NULL;
		}

		CoUninitialize();
	}
}

VOID KexSetupInstallFiles(
	VOID)
{
	WCHAR TargetPath[MAX_PATH];

	//
	// Install bitness agnostic data to KexDir.
	// This includes everything in Core\, as well as the current program
	// (kexsetup.exe).
	//

	KexSetupMoveFileSpecToDirectory(L".\\KexSetup.*", KexDir);
	KexSetupMoveFileSpecToDirectory(L".\\Core\\*", KexDir);

	//
	// Install KexDll to system32, and to syswow64 if 64bit OS.
	// The purpose of the wildcard KexDll.* is to include the pdb as well.
	//

	if (Is64BitOS) {
		GetSystemWindowsDirectory(TargetPath, ARRAYSIZE(TargetPath));
		PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"sysnative");
		KexSetupMoveFileSpecToDirectory(L".\\Core64\\KexDll.*", TargetPath);

		PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"KexDll.*.old_*");
		KexSetupDeleteFilesBySpec(TargetPath);

		if (KexIsReleaseBuild) {
			PathCchRemoveFileSpec(TargetPath, ARRAYSIZE(TargetPath));
			PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"KexDll.pdb");
			KexSetupDeleteFile(TargetPath);
		}
	}

	GetSystemWindowsDirectory(TargetPath, ARRAYSIZE(TargetPath));
	PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"system32"); // On 64-bit OS, this actually goes to syswow64
	KexSetupMoveFileSpecToDirectory(L".\\Core32\\KexDll.*", TargetPath);

	// Delete any lingering old files.
	PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"KexDll.*.old_*");
	KexSetupDeleteFilesBySpec(TargetPath);

	// If a release build, delete PDB now (it is no longer the correct one
	// anyway).
	if (KexIsReleaseBuild) {
		PathCchRemoveFileSpec(TargetPath, ARRAYSIZE(TargetPath));
		PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"KexDll.pdb");
		KexSetupDeleteFile(TargetPath);
	}

	//
	// Install native core files to KexDir, plus KexShl32 if 64bit OS
	//

	if (Is64BitOS) {
		KexSetupMoveFileSpecToDirectory(L".\\Core64\\*", KexDir);

		KexSetupFormatPath(TargetPath, L"%s\\KexShl32.dll", KexDir);
		KexSetupSupersedeFile(L".\\Core32\\KexShlEx.dll", TargetPath);

		if (KexIsDebugBuild) {
			// Move the pdbs as well
			KexSetupFormatPath(TargetPath, L"%s\\KexShl32.pdb", KexDir);
			KexSetupSupersedeFile(L".\\Core32\\KexShlEx.pdb", TargetPath);
		}
	} else {
		KexSetupMoveFileSpecToDirectory(L".\\Core32\\*", KexDir);
	}

	KexSetupFormatPath(TargetPath, L"%s\\*.old_*", KexDir);
	KexSetupDeleteFilesBySpec(TargetPath);

	if (KexIsReleaseBuild) {
		// Remove outdated PDB files
		KexSetupFormatPath(TargetPath, L"%s\\*.pdb", KexDir);
		KexSetupDeleteFilesBySpec(TargetPath);
	}

	//
	// Install extension DLLs to KexDir\Kex32 (and KexDir\Kex64 if 64bit OS)
	//

	// remove old PDBs
	KexSetupFormatPath(TargetPath, L"%s\\Kex32\\*.pdb", KexDir);
	KexSetupDeleteFilesBySpec(TargetPath);

	KexSetupFormatPath(TargetPath, L"%s\\Kex32", KexDir);
	KexSetupMoveFileSpecToDirectory(L".\\Kex32\\*", TargetPath);

	PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"*.old_*");
	KexSetupDeleteFilesBySpec(TargetPath);
	
	if (Is64BitOS) {
		// remove old PDBs
		KexSetupFormatPath(TargetPath, L"%s\\Kex64\\*.pdb", KexDir);
		KexSetupDeleteFilesBySpec(TargetPath);

		KexSetupFormatPath(TargetPath, L"%s\\Kex64", KexDir);
		KexSetupMoveFileSpecToDirectory(L".\\Kex64\\*", TargetPath);

		PathCchAppend(TargetPath, ARRAYSIZE(TargetPath), L"*.old_*");
		KexSetupDeleteFilesBySpec(TargetPath);
	}

	{
		PCWSTR UnwantedPrebuiltDlls[] = {
			L"dnsw8.dll",
			L"dcow8.dll",
			L"msvw10.dll",
			L"icuin.dll",
			L"icuuc.dll",
			L"mshtmlmedia.dll"
		};

		ULONG Index;

		//
		// Past releases of VxKex have included some prebuilt DLLs which are no longer
		// included and so we want to get rid of them now.
		//

		for (Index = 0; Index < ARRAYSIZE(UnwantedPrebuiltDlls); ++Index) {
			KexSetupFormatPath(TargetPath, L"%s\\Kex32\\%s", KexDir, UnwantedPrebuiltDlls[Index]);
			KexSetupDeleteFile(TargetPath);

			if (Is64BitOS) {
				KexSetupFormatPath(TargetPath, L"%s\\Kex64\\%s", KexDir, UnwantedPrebuiltDlls[Index]);
				KexSetupDeleteFile(TargetPath);
			}
		}
	}

	//
	// WolfSSL-based KxSChanl included a ROOT.crt (PEM formatted) certificate file.
	// The new native KxSChanl uses a ROOT.sst file. Delete the old .crt file.
	//

	KexSetupFormatPath(TargetPath, L"%s\\Certificates\\ROOT.crt", KexDir);
	KexSetupDeleteFile(TargetPath);

	//
	// Prior to 1.2.0.2020, CpiwBypa.dll and CpiwBp32.dll were installed into
	// KexDir. 1.2.0.2020 and later do not use these DLLs anymore so we will
	// get rid of them.
	//

	// Use a wildcard delete to get rid of PDBs as well (for debug builds).
	KexSetupFormatPath(TargetPath, L"%s\\CpiwB*", KexDir);
	KexSetupDeleteFilesBySpec(TargetPath);
}

//
// This function is called (indirectly) by KexSetupUninstall to delete
// or disable VxKex for every program the user configured it for.
//
// It's important that we do this, because it could cause those programs
// to stop working altogether until the user does some manual work to
// fix the problem if we fail to remove the configuration.
//
BOOLEAN CALLBACK KexSetupConfigurationEnumerationCallback(
	IN	PCWSTR							ExeFullPathOrBaseName,
	IN	BOOLEAN							IsLegacyConfiguration,
	IN	PVOID							ExtraParameter)
{
	BOOLEAN Success;

	// always delete pre-rewrite configuration.
	if (IsLegacyConfiguration) {
		Success = KxCfgDeleteLegacyConfiguration(
			ExeFullPathOrBaseName,
			KexSetupTransactionHandle);

		if (!Success) {
			ErrorBoxF(
				_(L"Setup was unable to delete VxKex legacy configuration for \"%s\". %s"),
				ExeFullPathOrBaseName, GetLastErrorAsString());

			RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
		}

		return TRUE;
	}

	ASSERT (!PathIsRelative(ExeFullPathOrBaseName));

	if (PreserveConfig) {
		//
		// Save VxKex configuration for this program to the registry if we're supposed
		// to preserve VxKex settings.
		// This will be restored the next time VxKex is installed.
		//

		Success = KxCfgPreserveConfiguration(
			ExeFullPathOrBaseName,
			KexSetupTransactionHandle);

		ASSERT (Success);

		if (!Success) {
			// continue enumeration
			return TRUE;
		}
	}

	Success = KxCfgDeleteConfiguration(
		ExeFullPathOrBaseName,
		KexSetupTransactionHandle);
		
	if (!Success) {
		ErrorBoxF(
			_(L"Setup was unable to delete VxKex configuration for \"%s\". %s"),
			ExeFullPathOrBaseName, GetLastErrorAsString());

		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	return TRUE;
}

VOID KexSetupUninstall(
	VOID)
{
	BOOLEAN Success;
	WCHAR PathBuffer[MAX_PATH];

	//
	// If the installer is running from KexDir, we will move it out of KexDir to a
	// temporary location. Otherwise, we won't be able to fully remove KexDir.
	//
	
	GetModuleFileName(NULL, PathBuffer, ARRAYSIZE(PathBuffer));

	if (PathIsPrefix(KexDir, PathBuffer)) {
		WCHAR NewKexSetupLocation[MAX_PATH];
		WCHAR RandomIdentifier[24];

		GetTempPath(ARRAYSIZE(PathBuffer), NewKexSetupLocation);

		StringCchPrintf(
			RandomIdentifier,
			ARRAYSIZE(RandomIdentifier),
			L"KexSetup-%08x",
			GetTickCount());
			
		PathCchAppend(NewKexSetupLocation, ARRAYSIZE(NewKexSetupLocation), RandomIdentifier);
		KexSetupCreateDirectory(NewKexSetupLocation);
		PathCchAppend(NewKexSetupLocation, ARRAYSIZE(NewKexSetupLocation), L"KexSetup.exe");
		KexSetupSupersedeFile(PathBuffer, NewKexSetupLocation);

		//
		// Schedule the temporary file and directory to be deleted later.
		//

		MoveFileTransacted(
			NewKexSetupLocation,
			NULL,
			NULL,
			NULL,
			MOVEFILE_DELAY_UNTIL_REBOOT,
			KexSetupTransactionHandle);

		PathCchRemoveFileSpec(NewKexSetupLocation, ARRAYSIZE(NewKexSetupLocation));

		MoveFileTransacted(
			NewKexSetupLocation,
			NULL,
			NULL,
			NULL,
			MOVEFILE_DELAY_UNTIL_REBOOT,
			KexSetupTransactionHandle);
	}

	//
	// Delete the rest of KexDir and remove KexDll from system32 and syswow64.
	//

	KexSetupRemoveDirectoryRecursive(KexDir);

	GetSystemWindowsDirectory(PathBuffer, ARRAYSIZE(PathBuffer));
	PathCchAppend(PathBuffer, ARRAYSIZE(PathBuffer), L"system32\\KexDll.*");
	KexSetupDeleteFilesBySpec(PathBuffer);

	if (Is64BitOS) {
		GetSystemWindowsDirectory(PathBuffer, ARRAYSIZE(PathBuffer));
		PathCchAppend(PathBuffer, ARRAYSIZE(PathBuffer), L"sysnative\\KexDll.*");
		KexSetupDeleteFilesBySpec(PathBuffer);
	}

	//
	// Delete system and user LogDir.
	//

	if (ExistingVxKexVersion >= 0x80000000) {
		HKEY VxKexKeyHandle;

		try {
			VxKexKeyHandle = KxCfgOpenVxKexRegistryKey(
				FALSE,
				KEY_READ,
				KexSetupTransactionHandle);

			if (VxKexKeyHandle == NULL) {
				leave;
			}

			// We do all this rather than using KexSetupRemoveDirectoryRecursive
			// because the LogDir is a user-controlled location. Imagine how bad
			// it would be if the user set LogDir to his desktop and we just go
			// and delete his entire desktop.
			KexSetupRegReadString(VxKexKeyHandle, L"LogDir", PathBuffer, ARRAYSIZE(PathBuffer));
			PathCchAppend(PathBuffer, ARRAYSIZE(PathBuffer), L"*.vxl");
			KexSetupDeleteFilesBySpec(PathBuffer);
			PathCchRemoveFileSpec(PathBuffer, ARRAYSIZE(PathBuffer));
			RemoveDirectoryTransacted(PathBuffer, KexSetupTransactionHandle);
			SafeClose(VxKexKeyHandle);

			VxKexKeyHandle = KxCfgOpenVxKexRegistryKey(
				TRUE,
				KEY_READ,
				KexSetupTransactionHandle);

			if (VxKexKeyHandle == NULL) {
				leave;
			}

			KexSetupRegReadString(VxKexKeyHandle, L"LogDir", PathBuffer, ARRAYSIZE(PathBuffer));
			PathCchAppend(PathBuffer, ARRAYSIZE(PathBuffer), L"*.vxl");
			KexSetupDeleteFilesBySpec(PathBuffer);
			PathCchRemoveFileSpec(PathBuffer, ARRAYSIZE(PathBuffer));
			RemoveDirectoryTransacted(PathBuffer, KexSetupTransactionHandle);
		} except (GetExceptionCode() == STATUS_KEXSETUP_FAILURE) {
			// ignore error - not critical
		}

		SafeClose(VxKexKeyHandle);
	}

	//
	// Delete propagation virtual key.
	//

	if (ExistingVxKexVersion >= 0x80000000) {
		KexSetupDeleteKey(
			HKEY_LOCAL_MACHINE,
			L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
			L"Image File Execution Options\\{VxKexPropagationVirtualKey}");
	}

	if (PreserveConfig) {
		HKEY KeyHandle;
		LSTATUS ErrorCode;

		// should always be the case since PreserveConfig only happens when user is
		// explicitly uninstalling (not when upgrading from legacy vxkex)
		ASSERT (ExistingVxKexVersion >= 0x80000000);

		//
		// Delete InstalledVersion in the VxKex HKLM key, so that future invocations
		// of the installer know that we're uninstalled.
		// Upon any error, just fall back to the code path that does not preserve
		// configuration.
		//

		KeyHandle = KxCfgOpenVxKexRegistryKey(
			FALSE,
			KEY_READ | KEY_WRITE,
			KexSetupTransactionHandle);

		ASSERT (KeyHandle != NULL);

		if (KeyHandle == NULL) {
			goto PreserveConfigFailure;
		}

		ErrorCode = RegDeleteValue(KeyHandle, L"InstalledVersion");
		ASSERT (ErrorCode == ERROR_SUCCESS);

		SafeClose(KeyHandle);

		if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
			goto PreserveConfigFailure;
		}
	} else {
PreserveConfigFailure:
		PreserveConfig = FALSE;

		//
		// Delete VxKex HKLM and HKCU registry keys.
		//

		if (ExistingVxKexVersion < 0x80000000) {
			KexSetupDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\VXsoft\\VxKexLdr");
		} else {
			KexSetupDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\VXsoft\\VxKex");
		}

		if (ExistingVxKexVersion < 0x80000000) {
			KexSetupDeleteKey(HKEY_CURRENT_USER, L"Software\\VXsoft\\VxKexLdr");
		} else {
			KexSetupDeleteKey(HKEY_CURRENT_USER, L"Software\\VXsoft\\VxKex");
		}
	}

	//
	// Delete all VxKex program configuration from IFEO.
	// If PreserveConfig is TRUE, all VxKex configuration is preserved.
	//

	KxCfgEnumerateConfiguration(
		KexSetupConfigurationEnumerationCallback,
		NULL);
	
	//
	// Unregister shell extension and .vxl file extension handler.
	//

	try {
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L".vxl");
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"vxlfile");
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"CLSID\\" CLSID_STRING_KEXSHLEX);

		if (Is64BitOS) {
			KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"Wow6432Node\\CLSID\\" CLSID_STRING_KEXSHLEX);
		}

		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page");
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"lnkfile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page");
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"Msi.Package\\shellex\\PropertySheetHandlers\\KexShlEx Property Page");
	} except (GetExceptionCode() == STATUS_KEXSETUP_FAILURE) {
		// Ignore the error. Failure to delete these values isn't critical.
		ASSERT ((FALSE, "Failed to unregister shell extension and .vxl file handler"));
	}

	//
	// Remove .vxl disk cleanup handler.
	//

	Success = KxCfgRemoveDiskCleanupHandler(KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// Remove VxKex context menu entry
	//

	Success = KxCfgConfigureShellContextMenuEntries(
		FALSE,
		FALSE,
		KexSetupTransactionHandle);

	ASSERT (Success);

	//
	// Delete uninstall entry.
	//

	KexSetupDeleteKey(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\VxKex");

	//
	// Delete the KexCfg elevation scheduled task.
	// We do this last because it's not transacted (it can't be).
	//

	KexSetupRemoveKexCfgScheduledTask();
}

VOID KexSetupInstall(
	VOID)
{
	BOOLEAN Success;
	ULONG ErrorCode;
	HKEY KeyHandle;
	WCHAR TargetPath[MAX_PATH];
	WCHAR SystemLogDir[MAX_PATH];
	WCHAR UserLogDir[MAX_PATH];

	//
	// Get the installed size (required when adding the uninstall entry)
	// right now before we start moving files out of the installer dir.
	//

	InstalledSize = GetRequiredSpaceInBytes();

	//
	// Create KexDir, and KexDir\Kex32 (and KexDir\Kex64 if 64bit OS).
	//
	
	KexSetupCreateDirectory(KexDir);

	KexSetupFormatPath(TargetPath, L"%s\\Kex32", KexDir);
	KexSetupCreateDirectory(TargetPath);
		
	if (Is64BitOS) {
		KexSetupFormatPath(TargetPath, L"%s\\Kex64", KexDir);
		KexSetupCreateDirectory(TargetPath);
	}

	//
	// Create and populate the VxKex registry key.
	//
	
	KexSetupCreateKey(
		HKEY_LOCAL_MACHINE,
		L"Software\\VXsoft\\VxKex",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		ErrorCode = ExpandEnvironmentStrings(
			L"%PROGRAMDATA%\\VxKex\\Logs",
			SystemLogDir,
			ARRAYSIZE(SystemLogDir));

		ASSERT (ErrorCode != 0);
		ASSERT (ErrorCode <= ARRAYSIZE(SystemLogDir));

		KexSetupRegWriteI32(KeyHandle, L"InstalledVersion", InstallerVxKexVersion);
		KexSetupRegWriteString(KeyHandle, L"KexDir", KexDir);
		KexSetupRegWriteString(KeyHandle, L"LogDir", SystemLogDir);
	} finally {
		SafeClose(KeyHandle);
	}

	//
	// Create and populate VxKex HKCU registry key.
	//

	KexSetupCreateKey(
		HKEY_CURRENT_USER,
		L"Software\\VXsoft\\VxKex",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		//
		// Since the HKCU LogDir value is non-critical, we won't bother informing the
		// user of errors in ExpandEnvironmentStrings.
		//

		ErrorCode = ExpandEnvironmentStrings(
			L"%LOCALAPPDATA%\\VxKex\\Logs",
			UserLogDir,
			ARRAYSIZE(UserLogDir));
		
		ASSERT (ErrorCode != 0);
		ASSERT (ErrorCode <= ARRAYSIZE(UserLogDir));

		unless (ErrorCode == 0 || ErrorCode > ARRAYSIZE(UserLogDir)) {
			KexSetupRegWriteString(
				KeyHandle,
				L"LogDir",
				UserLogDir);
		}
	} finally {
		SafeClose(KeyHandle);
	}

	//
	// Create and populate Propagation Virtual Key
	//

	KexSetupCreateKey(
		HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows NT\\CurrentVersion\\"
		L"Image File Execution Options\\{VxKexPropagationVirtualKey}",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteI32(KeyHandle, L"GlobalFlag", FLG_APPLICATION_VERIFIER);
		KexSetupRegWriteI32(KeyHandle, L"VerifierFlags", 0x80000000);
		KexSetupRegWriteString(KeyHandle, L"VerifierDlls", L"KexDll.dll");
	} finally {
		SafeClose(KeyHandle);
	}

	//
	// Enable VxKex for MSIEXEC.
	// This step is not critical, so we do not fail if it doesn't succeed.
	//

	Success = KxCfgEnableVxKexForMsiexec(TRUE, KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// Call subroutine to install VxKex files.
	//

	KexSetupInstallFiles();

	//
	// Add uninstall entry.
	//

	KexSetupWriteUninstallEntry();

	//
	// Register the .vxl file extension for VxlView.exe.
	//
	// HKEY_CLASSES_ROOT
	//   .vxl
	//     (Default)			= REG_SZ "vxlfile"
	//   vxlfile
	//     shell
	//       open
	//         command
	//           (Default)		= REG_SZ ""<KexDir>\VxlView.exe" "%1""
	//     DefaultIcon
	//       (Default)			= REG_SZ "<KexDir>\VxlView.exe,1"
	//

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L".vxl",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteString(
			KeyHandle,
			NULL,
			L"vxlfile");
	} finally {
		SafeClose(KeyHandle);
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"vxlfile",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteString(
			KeyHandle,
			NULL,
			L"VxLog File");
	} finally {
		SafeClose(KeyHandle);
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"vxlfile\\shell\\open\\command",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupFormatPath(TargetPath, L"\"%s\\VxlView.exe\" \"%%1\"", KexDir);

		KexSetupRegWriteString(
			KeyHandle,
			NULL,
			TargetPath);
	} finally {
		SafeClose(KeyHandle);
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"vxlfile\\DefaultIcon",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		// Note: Don't add extra quotes to this. It will make it stop working.
		KexSetupFormatPath(TargetPath, L"%s\\VxlView.exe,1", KexDir);

		KexSetupRegWriteString(
			KeyHandle,
			NULL,
			TargetPath);
	} finally {
		SafeClose(KeyHandle);
	}

	//
	// Register disk cleanup handler for .vxl files
	//

	Success = KxCfgInstallDiskCleanupHandler(KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// Register the shell extension, KexShlEx (and KexShl32 if 64bit OS).
	//
	// HKEY_CLASSES_ROOT
	//   CLSID
	//     {9AACA888-A5F5-4C01-852E-8A2005C1D45F} (*)
	//       InProcServer32
	//         (Default)		= REG_SZ "<KexDir>\KexShlEx.dll"
	//         ThreadingModel	= REG_SZ "Apartment"
	//   exefile
	//     shellex
	//       PropertySheetHandlers
	//         KexShlEx Property Page (*)
	//           (Default)	= REG_SZ "{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
	//   lnkfile
	//     shellex
	//       PropertySheetHandlers
	//         KexShlEx Property Page (*)
	//           (Default)	= REG_SZ "{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
	//   Msi.Package
	//     shellex
	//       PropertySheetHandlers
	//         KexShlEx Property Page (*)
	//           (Default)	= REG_SZ "{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
	//

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupFormatPath(TargetPath, L"%s\\KexShlEx.dll", KexDir);
		KexSetupRegWriteString(KeyHandle, NULL, TargetPath);
		KexSetupRegWriteString(KeyHandle, L"ThreadingModel", L"Apartment");
	} finally {
		SafeClose(KeyHandle);
	}

	if (Is64BitOS) {
		// Register KexShl32.dll as a 32 bit shell extension.

		KexSetupCreateKey(
			HKEY_CLASSES_ROOT,
			L"Wow6432Node\\CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32",
			KEY_READ | KEY_WRITE,
			&KeyHandle);

		try {
			KexSetupFormatPath(TargetPath, L"%s\\KexShl32.dll", KexDir);
			KexSetupRegWriteString(KeyHandle, NULL, TargetPath);
			KexSetupRegWriteString(KeyHandle, L"ThreadingModel", L"Apartment");
		} finally {
			SafeClose(KeyHandle);
		}
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteString(KeyHandle, NULL, CLSID_STRING_KEXSHLEX);
	} finally {
		SafeClose(KeyHandle);
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"lnkfile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteString(KeyHandle, NULL, CLSID_STRING_KEXSHLEX);
	} finally {
		SafeClose(KeyHandle);
	}

	KexSetupCreateKey(
		HKEY_CLASSES_ROOT,
		L"Msi.Package\\shellex\\PropertySheetHandlers\\KexShlEx Property Page",
		KEY_READ | KEY_WRITE,
		&KeyHandle);

	try {
		KexSetupRegWriteString(KeyHandle, NULL, CLSID_STRING_KEXSHLEX);
	} finally {
		SafeClose(KeyHandle);
	}

	//
	// Enable Explorer CPIW bypass by default on new installs.
	//

	Success = KxCfgEnableExplorerCpiwBypass(
		TRUE,
		KexSetupTransactionHandle);

	ASSERT (Success);

	//
	// Restore any preserved configuration that may exist.
	// Preserved configuration is created when the user previously uninstalled
	// with the "Keep my compatibility settings" checkbox checked.
	//

	Success = KxCfgRestorePreservedConfiguration(KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// Add scheduled task for UAC-free configuration.
	// We do this last because it can't be transacted.
	// (well, it can, but not with the official task scheduler API).
	//

	KexSetupAddKexCfgScheduledTask();
}

VOID KexSetupUpgrade(
	VOID)
{
	BOOLEAN Success;
	HKEY KeyHandle;

	//
	// If pre-rewrite version is present, we will do a full uninstall and reinstall.
	//

	if (ExistingVxKexVersion < 0x80000000) {
		KexSetupUninstall();
		KexSetupInstall();
		return;
	}

	//
	// Update the InstalledVersion in Vxkex HKLM key.
	// Delete the DllRewrite key which was present in older versions
	// but is no longer used.
	// Update logging settings to 1.1.6.x and above registry values.
	//

	KeyHandle = KxCfgOpenVxKexRegistryKey(
		FALSE,
		KEY_READ | KEY_WRITE,
		KexSetupTransactionHandle);

	if (!KeyHandle) {
		ErrorBoxF(L"Setup was unable to open the VxKex HKLM registry key. %s", GetLastErrorAsString());
		RtlRaiseStatus(STATUS_KEXSETUP_FAILURE);
	}

	try {
		KexSetupRegWriteI32(KeyHandle, L"InstalledVersion", InstallerVxKexVersion);
		KexSetupDeleteKey(KeyHandle, L"DllRewrite");
		KexSetupChangeDisableLoggingToEnableLogging(KeyHandle);
	} finally {
		SafeClose(KeyHandle);
	}

	KeyHandle = KxCfgOpenVxKexRegistryKey(
		TRUE,
		KEY_READ | KEY_WRITE,
		KexSetupTransactionHandle);

	// Failure to open the HKCU key is not fatal
	if (KeyHandle) {
		KexSetupChangeDisableLoggingToEnableLogging(KeyHandle);
		SafeClose(KeyHandle);
	}

	//
	// In version 1.1.6.x we changed the disk cleanup handler to remove old
	// log files after 1 day (instead of 3 days) and we also enabled it to clean
	// up files in the system log directory (by default %ProgramData%\VxKex\Logs).
	// This happened because with the enhanced MSI support added in 1.1.4.x, some
	// logs from msiexec.exe service can go to that directory, and we don't want
	// them to accumulate infinitely.
	//
	// We will refresh the disk cleanup handler in order to make sure that change
	// gets applied to upgrade installations.
	//

	Success = KxCfgInstallDiskCleanupHandler(KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// In version 1.1.6.1896 we added a Security Support Provider which enabled
	// support for TLS 1.3. This was registered through the registry value
	// HKLM\SYSTEM\CurrentControlSet\Control\SecurityProviders\SecurityProviders.
	//
	// This was suboptimal since it caused KxSChanl to be loaded into random
	// applications, even when VxKex was not enabled.
	//
	// For version 1.2.0.1994 we changed KxSChanl to no longer be globally
	// registered. Even though 1.1.6.x was never officially released, various
	// beta/pre-release versions were distributed, which means we now have to
	// delete the legacy SSP registration.
	//

	Success = KxCfgEnableLegacyKxSChanlSsp(FALSE, KexSetupTransactionHandle);
	ASSERT (Success);

	//
	// In version 1.2.0.2020 we changed the BHO-based CPIW bypass to an IFEO-based
	// version for reliability. If the user had previously had the CPIW bypass
	// enabled, we will upgrade it to the new version.
	//

	if (KxCfgQueryLegacyExplorerCpiwBypass()) {
		Success = KxCfgEnableLegacyExplorerCpiwBypass(FALSE, KexSetupTransactionHandle);
		ASSERT (Success);

		Success = KxCfgEnableExplorerCpiwBypass(TRUE, KexSetupTransactionHandle);
		ASSERT (Success);
	}

	try {
		// Unregister and get rid of the BHO.
		KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"CLSID\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}");

		if (Is64BitOS) {
			KexSetupDeleteKey(HKEY_CLASSES_ROOT, L"Wow6432Node\\CLSID\\{7EF224FC-1840-433C-9BCB-2951DE71DDBD}");
		}
	} except (GetExceptionCode() == STATUS_KEXSETUP_FAILURE) {
		ASSERT ((FALSE, "Failed to unregister CpiwBypa browser helper object"));
	}

	//
	// Call subroutine to install updated VxKex files.
	//

	KexSetupInstallFiles();

	//
	// Update the uninstall entry.
	//

	KexSetupWriteUninstallEntry();
}

//
// Call this function when all data has been gathered from the user and we
// are ready to do the install/upgrade/uninstall.
//
VOID KexSetupPerformActions(
	VOID)
{
	NTSTATUS Status;
	BOOLEAN Success;

	// this will happen if we are passed /HWND from being started elevated
	// by a potentially non-elevated parent process
	if (IsWindow(MainWindow)) {
		HANDLE ParentProcess;
		HANDLE ElevatedProcess;
		ULONG ParentProcessId;

		//
		// Give the parent process a handle to ourself so that he knows when
		// we have exited (and can query the exit code).
		//
		// The OpenProcess call that we use to get a handle to our parent process
		// will fail when a standard (non-administrator) account runs the
		// installer and uses an admin password to start the installation.
		//
		// In order to work around this problem, we will try and enable the
		// SeDebugPrivilege privilege. I'm not sure whether this is the best
		// solution, but it works.
		//

		{
			HANDLE TokenHandle;
			TOKEN_PRIVILEGES Privileges;
			LUID Luid;

			Success = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Luid);
			ASSERT (Success);

			if (!Success) {
				goto FailDebugPrivilege;
			}

			Success = OpenProcessToken(
				GetCurrentProcess(),
				TOKEN_ADJUST_PRIVILEGES,
				&TokenHandle);

			ASSERT (Success);

			if (!Success) {	
				goto FailDebugPrivilege;
			}

			Privileges.PrivilegeCount = 1;
			Privileges.Privileges[0].Luid = Luid;
			Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			Success = AdjustTokenPrivileges(
				TokenHandle,
				FALSE,
				&Privileges,
				sizeof(Privileges),
				NULL,
				NULL);

			ASSERT (Success);

			CloseHandle(TokenHandle);
		}

FailDebugPrivilege:

		GetWindowThreadProcessId(MainWindow, &ParentProcessId);
		
		ParentProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, ParentProcessId);
		ASSERT (ParentProcess != NULL);

		DuplicateHandle(
			GetCurrentProcess(),
			GetCurrentProcess(),
			ParentProcess,
			&ElevatedProcess,
			SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION,
			FALSE,
			0);
		
		CloseHandle(ParentProcess);

		ASSERT (ElevatedProcess != NULL);

		SendMessage(
			MainWindow,
			KSM_NOTIFY_ELEVATED_PROCESS_START,
			0,
			(LPARAM) ElevatedProcess);
	}

	//
	// If VxKex is already installed, we will get the existing KexDir.
	//

	if (ExistingVxKexVersion) {
		Success = KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));

		if (!Success) {
			ErrorBoxF(_(
				L"Setup was unable to determine the location "
				L"of the existing VxKex installation. %s"),
				GetLastErrorAsString());

			ExitProcess(STATUS_KEXSETUP_FAILURE);
		}
	}

	PathCchRemoveBackslash(KexDir, ARRAYSIZE(KexDir));

	//
	// Create a transaction for whatever action we are performing to ensure
	// it is atomic.
	//

	KexSetupTransactionHandle = CreateSimpleTransaction(L"KexSetup Transaction");

	if (KexSetupTransactionHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(
			_(L"Setup was unable to create a transaction for this operation. %s"),
			GetLastErrorAsString());

		ExitProcess(STATUS_KEXSETUP_FAILURE);
	}

	//
	// Call a subroutine to perform the requested action.
	//

	try {
		switch (OperationMode) {
		case OperationModeInstall:
			KexSetupInstall();
			break;
		case OperationModeUpgrade:
			KexSetupUpgrade();
			break;
		case OperationModeUninstall:
			KexSetupUninstall();
			break;
		default:
			NOT_REACHED;
		}

		KexSetupOkToCommitTransaction = TRUE;
	} except (GetExceptionCode() == STATUS_KEXSETUP_FAILURE) {
		KexSetupOkToCommitTransaction = FALSE;
	}

	if (KexSetupOkToCommitTransaction) {
		Status = NtCommitTransaction(KexSetupTransactionHandle, TRUE);
		Success = NT_SUCCESS(Status);

		if (!Success) {
			ErrorBoxF(L"Setup failed to commit the transaction. %s", NtStatusAsString(Status));
		}
	} else {
		Success = FALSE;
		Status = NtRollbackTransaction(KexSetupTransactionHandle, TRUE);
	}

	ASSERT (NT_SUCCESS(Status));
	SafeClose(KexSetupTransactionHandle);

	if (!Success) {
		ExitProcess(STATUS_KEXSETUP_FAILURE);
	}
}