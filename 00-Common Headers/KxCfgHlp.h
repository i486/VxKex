///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KxCfgHlp.h
//
// Abstract:
//
//     Main header file for the VxKex Configuration Helper library.
//
// Author:
//
//     vxiiduu (02-Feb-2024)
//
// Environment:
//
//     Win32 mode.
//
// Revision History:
//
//     vxiiduu              02-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexComm.h>
#include <KexDll.h>
#include <KexW32ML.h>

#ifdef KEX_ENV_NATIVE
#  error This header file cannot be used in a native mode project.
#endif

#ifndef KXCFGDECLSPEC
#  define KXCFGDECLSPEC
#  pragma comment(lib, "KxCfgHlp.lib")
#endif

#define KXCFGAPI WINAPI

#define KXCFG_ELEVATION_SCHTASK_NAME L"VxKex Configuration Elevation Task"

//
// Type definitions
//

// Increment KXCFG_PRESERVED_CONFIGURATION_VERSION whenever breaking changes
// are made to the KXCFG_PROGRAM_CONFIGURATION structure or its sub-structures,
// such as KEX_IFEO_PARAMETERS.
typedef struct {
	BOOLEAN				Enabled;
	KEX_IFEO_PARAMETERS	IfeoParameters;
} TYPEDEF_TYPE_NAME(KXCFG_PROGRAM_CONFIGURATION);

//
// ExeFullPathOrBaseName can be either a full path (C:\folder\program.exe)
// or a basename (program.exe). The callee can determine this easily by
// checking IsLegacyConfiguration (TRUE means it's a base name only).
//
// This function should return TRUE to continue enumeration or FALSE to stop
// enumerating.
//
typedef BOOLEAN (CALLBACK *PKXCFG_ENUMERATE_CONFIGURATION_CALLBACK) (
	IN	PCWSTR							ExeFullPathOrBaseName,
	IN	BOOLEAN							IsLegacyConfiguration,
	IN	PVOID							ExtraParameter);

//
// Public functions
//

KXCFGDECLSPEC HKEY KXCFGAPI KxCfgOpenVxKexRegistryKey(
	IN	BOOLEAN		PerUserKey,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL);

KXCFGDECLSPEC HKEY KXCFGAPI KxCfgOpenLegacyVxKexRegistryKey(
	IN	BOOLEAN		PerUserKey,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgGetConfiguration(
	IN	PCWSTR							ExeFullPath,
	OUT	PKXCFG_PROGRAM_CONFIGURATION	Configuration OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnumerateConfiguration(
	IN	PKXCFG_ENUMERATE_CONFIGURATION_CALLBACK	ConfigurationCallback,
	IN	PVOID									CallbackExtraParameter);

KXCFGDECLSPEC BOOLEAN KxCfgSetConfiguration(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration,
	IN	HANDLE							TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KxCfgDeleteConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgDeleteLegacyConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgGetKexDir(
	OUT	PWSTR	Buffer,
	IN	ULONG	BufferCch);

KXCFGDECLSPEC BOOLEAN WINAPI KxCfgEnableVxKexForMsiexec(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN WINAPI KxCfgQueryVxKexEnabledForMsiexec(
	VOID);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnableExplorerCpiwBypass(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryExplorerCpiwBypass(
	VOID);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryLegacyExplorerCpiwBypass(
	VOID);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnableLegacyExplorerCpiwBypass(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryShellContextMenuEntries(
	OUT	PBOOLEAN	ExtendedMenu OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgConfigureShellContextMenuEntries(
	IN	BOOLEAN	Enable,
	IN	BOOLEAN	ExtendedMenu,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryLoggingSettings(
	IN	BOOLEAN		PerUserSettings,
	OUT	PBOOLEAN	IsEnabled OPTIONAL,
	OUT	PWSTR		LogDir OPTIONAL,
	IN	ULONG		LogDirCch,
	IN	HANDLE		TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgConfigureLoggingSettings(
	IN	BOOLEAN		PerUserSettings,
	IN	BOOLEAN		Enabled,
	IN	PCWSTR		LogDir OPTIONAL,
	IN	HANDLE		TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgInstallDiskCleanupHandler(
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgRemoveDiskCleanupHandler(
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgQueryLegacyKxSChanlSsp(
	VOID);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgEnableLegacyKxSChanlSsp(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgPreserveConfiguration(
	IN	PCWSTR	ExeFullPath,
	IN	HANDLE	TransactionHandle OPTIONAL);

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgRestorePreservedConfiguration(
	IN	HANDLE	TransactionHandle OPTIONAL);

#ifdef KXCFGDECLSPEC
//
// Private functions
//

BOOLEAN KxCfgpRemoveKexDllFromVerifierDlls(
	IN	PWSTR	VerifierDlls);

BOOLEAN KxCfgpCreateIfeoKeyForProgram(
	IN	PCWSTR	ExeFullPath,
	OUT	PHKEY	KeyHandle,
	IN	HANDLE	TransactionHandle OPTIONAL);

BOOLEAN KxCfgpElevationRequired(
	VOID);

BOOLEAN KxCfgpElevatedSetConfiguration(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration);

BOOLEAN KxCfgpElevatedDeleteConfiguration(
	IN	PCWSTR							ExeFullPath);

BOOLEAN KxCfgpElevatedSetConfigurationTaskScheduler(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration);

BOOLEAN KxCfgpElevatedSetConfigurationShellExecute(
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration);

BOOLEAN KxCfgpAssembleKexCfgCommandLine(
	OUT	PWSTR							Buffer,
	IN	ULONG							BufferCch,
	IN	PCWSTR							ExeFullPath,
	IN	PKXCFG_PROGRAM_CONFIGURATION	Configuration);

HKEY KxCfgpCreateKey(
	IN	HKEY		RootDirectory,
	IN	PCWSTR		KeyPath,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL);

HKEY KxCfgpOpenKey(
	IN	HKEY		RootDirectory,
	IN	PCWSTR		KeyPath,
	IN	ACCESS_MASK	DesiredAccess,
	IN	HANDLE		TransactionHandle OPTIONAL);

ULONG KxCfgpDeleteKey(
	IN	HKEY	KeyHandle,
	IN	PCWSTR	KeyPath OPTIONAL,
	IN	HANDLE	TransactionHandle OPTIONAL);

#endif