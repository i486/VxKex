#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <ShellAPI.h>
#include <ShlObj.h>

KEXGDECLSPEC BOOLEAN KEXGAPI ShowPropertiesDialog(
	IN	PCWSTR	FilePath,
	IN	INT		ShowControl)
{
	SHELLEXECUTEINFO ShellExecuteInfo;

	RtlZeroMemory(&ShellExecuteInfo, sizeof(ShellExecuteInfo));
	ShellExecuteInfo.cbSize	= sizeof(ShellExecuteInfo);
	ShellExecuteInfo.fMask	= SEE_MASK_INVOKEIDLIST | SEE_MASK_NOASYNC | SEE_MASK_UNICODE;
	ShellExecuteInfo.hwnd	= KexgApplicationMainWindow;
	ShellExecuteInfo.lpVerb	= L"properties";
	ShellExecuteInfo.lpFile	= FilePath;
	ShellExecuteInfo.nShow	= ShowControl;

	return ShellExecuteEx(&ShellExecuteInfo);
}

KEXGDECLSPEC HRESULT KEXGAPI OpenFileLocation(
	IN	PCWSTR	FilePath,
	IN	ULONG	Flags)
{
	HRESULT Result;
	PIDLIST_ABSOLUTE IDList;
	ULONG SfgaoFlags;

	Result = SHParseDisplayName(FilePath, NULL, &IDList, 0, &SfgaoFlags);
	if (FAILED(Result)) {
		return Result;
	}

	CoInitialize(NULL);

	Result = SHOpenFolderAndSelectItems(IDList, 0, NULL, Flags);

	CoUninitialize();

	return Result;
}