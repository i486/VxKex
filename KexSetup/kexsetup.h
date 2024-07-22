///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexsetup.h
//
// Abstract:
//
//     Main header file for KexSetup
//
// Author:
//
//     vxiiduu (01-Feb-2024)
//
// Environment:
//
//     Win32, without any vxkex support components
//
// Revision History:
//
//     vxiiduu               01-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "buildcfg.h"
#include "resource.h"
#include <KexComm.h>

typedef enum {
	OperationModeInstall,
	OperationModeUninstall,
	OperationModeUpgrade
} TYPEDEF_TYPE_NAME(KEXSETUP_OPERATION_MODE);

//
// main.c
//

EXTERN BOOLEAN Is64BitOS;
EXTERN KEXSETUP_OPERATION_MODE OperationMode;
EXTERN BOOLEAN SilentUnattended;
EXTERN BOOLEAN PreserveConfig;
EXTERN WCHAR KexDir[MAX_PATH];
EXTERN ULONG ExistingVxKexVersion;
EXTERN ULONG InstallerVxKexVersion;

//
// cmdline.c
//

VOID ProcessCommandLineOptions(
	VOID);

//
// gui.c
//

EXTERN HWND MainWindow;
EXTERN ULONG CurrentScene;

VOID DisplayInstallerGUI(
	VOID);

INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

VOID SetScene(
	IN	ULONG	SceneNumber);

ULONG GetRequiredSpaceInBytes(
	VOID);

//
// perform.c
//

EXTERN HANDLE KexSetupTransactionHandle;

VOID KexSetupPerformActions(
	VOID);

//
// prereq.c
//

VOID KexSetupCheckForPrerequisites(
	VOID);

//
// util.c
//

VOID GetDefaultInstallationLocation(
	IN	PWSTR	InstallationPath);

BOOLEAN IsWow64(
	VOID);

VOID KexSetupCreateKey(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	SubKey,
	IN	REGSAM	DesiredAccess,
	OUT	PHKEY	KeyHandleOut);

VOID KexSetupDeleteKey(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	Subkey OPTIONAL);

VOID KexSetupRegWriteI32(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	IN	ULONG	Data);

VOID KexSetupRegWriteString(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	IN	PCWSTR	Data);

VOID KexSetupRegReadString(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	ValueName,
	OUT	PWSTR	Buffer,
	IN	ULONG	BufferCch);

VOID KexSetupSupersedeFile(
	IN	PCWSTR	SourceFile,
	IN	PCWSTR	TargetFile);

VOID KexSetupFormatPath(
	OUT	PWSTR	Buffer,
	IN	PCWSTR	Format,
	IN	...);

VOID KexSetupMoveFileSpecToDirectory(
	IN	PCWSTR	FileSpec,
	IN	PCWSTR	TargetDirectoryPath);

VOID KexSetupCreateDirectory(
	IN	PCWSTR	DirectoryPath);

BOOLEAN KexSetupDeleteFile(
	IN	PCWSTR	FilePath);

BOOLEAN KexSetupRemoveDirectoryRecursive(
	IN	PCWSTR	DirectoryPath);

BOOLEAN KexSetupDeleteFilesBySpec(
	IN	PCWSTR	FileSpec);

//
// version.c
//

ULONG KexSetupGetInstalledVersion(
	VOID);