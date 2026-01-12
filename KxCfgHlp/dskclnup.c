#include "buildcfg.h"
#include <KexComm.h>
#include <KxCfgHlp.h>

#define DDEVCF_DOSUBDIRS			0x00000001 	// recursively search/remove 
#define DDEVCF_REMOVEAFTERCLEAN		0x00000002 	// remove from the registry after run once
#define DDEVCF_REMOVEREADONLY		0x00000004  // remove file even if it is read-only
#define DDEVCF_REMOVESYSTEM			0x00000008  // remove file even if it is system
#define DDEVCF_REMOVEHIDDEN			0x00000010  // remove file even if it is hidden
#define DDEVCF_DONTSHOWIFZERO		0x00000020  // don't show this cleaner if it has nothing to clean
#define DDEVCF_REMOVEDIRS			0x00000040  // Match filelist against directories and remove everything under them.
#define DDEVCF_RUNIFOUTOFDISKSPACE	0x00000080  // Only run if machine is out of disk space.
#define DDEVCF_REMOVEPARENTDIR		0x00000100  // remove the parent directory once done.
#define DDEVCF_PRIVATE_LASTACCESS	0x10000000  // use LastAccessTime

//
// This file contains routines which install or uninstall the Disk Cleanup
// handler for the .vxl files in the log directory.
//
// The KexDir and LogDir parameters are intended only for use from the
// installer. Specify NULL if not running from the installer.
//

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgInstallDiskCleanupHandler(
	IN	PCWSTR	KexDir OPTIONAL,
	IN	PCWSTR	LogDir OPTIONAL,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	HKEY KeyHandle;
	WCHAR IconPath[MAX_PATH];
	WCHAR LogDirBuffer[MAX_PATH];

	//
	// HKLM\Software\Microsoft\Windows\CurrentVersion\Explorer\
	// VolumeCaches (key)
	//   VxKex Log Files (*) (key)
	//     (Default)				= REG_SZ "{C0E13E61-0CC6-11d1-BBB6-0060978B2AE6}"
	//     Description				= REG_SZ "VxKex may create log files each time you launch an application, "
	//										 "which consume disk space. Log files older than 3 days can safely "
	//										 "be deleted."
	//     Display					= REG_SZ "VxKex Log Files"
	//     FileList					= REG_SZ "*.vxl"
	//     Flags					= REG_DWORD (DDEVCF_DONTSHOWIFZERO)
	//     Folder					= REG_SZ "<LogDir>"
	//     IconPath					= REG_SZ "<KexDir>\VxlView.exe,1"
	//     LastAccess				= REG_DWORD 0x00000003
	//

	KeyHandle = KxCfgpCreateKey(
		HKEY_LOCAL_MACHINE,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"
		L"VolumeCaches\\VxKex Log Files",
		KEY_READ | KEY_WRITE,
		TransactionHandle);

	ASSERT (KeyHandle != NULL);

	if (!KeyHandle) {
		return FALSE;
	}

	if (KexDir) {
		StringCchPrintf(IconPath, ARRAYSIZE(IconPath), L"%s\\VxlView.exe,1", KexDir);
	} else {
		KxCfgGetKexDir(IconPath, ARRAYSIZE(IconPath));
		PathCchAppend(IconPath, ARRAYSIZE(IconPath), L"VxlView.exe,1");
	}

	if (!LogDir) {
		KxCfgQueryLoggingSettings(NULL, LogDirBuffer, ARRAYSIZE(LogDirBuffer));
		LogDir = LogDirBuffer;
	}

	RegWriteString(KeyHandle, NULL, NULL, L"{C0E13E61-0CC6-11d1-BBB6-0060978B2AE6}");
	RegWriteString(KeyHandle, NULL, L"Display", L"VxKex Log Files");
	RegWriteString(KeyHandle, NULL, L"Description",
		L"VxKex may create log files each time you launch an application, "
		L"which consumes disk space. Log files older than 3 days can safely "
		L"be deleted.");
	RegWriteString(KeyHandle, NULL, L"Folder", LogDir);
	RegWriteString(KeyHandle, NULL, L"FileList", L"*.vxl");
	RegWriteString(KeyHandle, NULL, L"IconPath", IconPath);
	RegWriteI32(KeyHandle, NULL, L"LastAccess", 3);
	RegWriteI32(KeyHandle, NULL, L"Flags", DDEVCF_DONTSHOWIFZERO);

	SafeClose(KeyHandle);
	return TRUE;
}

KXCFGDECLSPEC BOOLEAN KXCFGAPI KxCfgRemoveDiskCleanupHandler(
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	ULONG ErrorCode;

	ErrorCode = KxCfgpDeleteKey(
		NULL,
		L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\"
		L"Explorer\\VolumeCaches\\VxKex Log Files",
		TransactionHandle);

	ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

	if (ErrorCode != ERROR_SUCCESS && ErrorCode != ERROR_FILE_NOT_FOUND) {
		SetLastError(ErrorCode);
		return FALSE;
	}

	return TRUE;
}