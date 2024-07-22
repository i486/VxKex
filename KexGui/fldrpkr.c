#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <ShObjIdl.h>

KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI PickFolder(
	IN		HWND	OwnerWindow OPTIONAL,
	IN		PCWSTR	DefaultValue OPTIONAL,
	IN		ULONG	AdditionalFlags OPTIONAL, // FOS_*
	IN		PWSTR	DirectoryPath,
	IN		ULONG	DirectoryPathCch)
{
	HRESULT Result;
	IFileDialog *FileDialog;
	IShellItem *ShellItem;
	PWSTR ShellName;
	ULONG Flags;

	ASSERT (DirectoryPath != NULL);
	ASSERT (DirectoryPathCch != 0);

	FileDialog = NULL;
	ShellItem = NULL;
	
	Result = CoCreateInstance(
		&CLSID_FileOpenDialog,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IFileOpenDialog,
		(PPVOID) &FileDialog);

	if (!DefaultValue) {
		DefaultValue = L"";
	}

	if (FAILED(Result)) {
		StringCchCopy(DirectoryPath, DirectoryPathCch, DefaultValue);
		return FALSE;
	}

	IFileDialog_GetOptions(FileDialog, &Flags);
	IFileDialog_SetOptions(FileDialog, Flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | AdditionalFlags);
	IFileDialog_Show(FileDialog, OwnerWindow);
	IFileDialog_GetResult(FileDialog, &ShellItem);
	IFileDialog_Release(FileDialog);

	if (!ShellItem) {
		StringCchCopy(DirectoryPath, DirectoryPathCch, DefaultValue);
		return FALSE;
	}

	IShellItem_GetDisplayName(ShellItem, SIGDN_FILESYSPATH, &ShellName);
	StringCchCopy(DirectoryPath, DirectoryPathCch, ShellName);
	CoTaskMemFree(ShellName);
	IShellItem_Release(ShellItem);

	return TRUE;
}