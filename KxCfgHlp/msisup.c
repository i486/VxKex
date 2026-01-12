#include "buildcfg.h"
#include <KxCfgHlp.h>

KXCFGDECLSPEC BOOLEAN WINAPI KxCfgEnableVxKexForMsiexec(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	WCHAR MsiexecPath[MAX_PATH];

	GetSystemDirectory(MsiexecPath, ARRAYSIZE(MsiexecPath));
	PathCchAppend(MsiexecPath, ARRAYSIZE(MsiexecPath), L"msiexec.exe");

	if (Enable) {
		KXCFG_PROGRAM_CONFIGURATION MsiexecConfiguration;

		RtlZeroMemory(&MsiexecConfiguration, sizeof(MsiexecConfiguration));
		MsiexecConfiguration.Enabled = TRUE;

		return KxCfgSetConfiguration(MsiexecPath, &MsiexecConfiguration, TransactionHandle);
	} else {
		return KxCfgDeleteConfiguration(MsiexecPath, TransactionHandle);
	}
}

KXCFGDECLSPEC BOOLEAN WINAPI KxCfgQueryVxKexEnabledForMsiexec(
	VOID)
{
	BOOLEAN Success;
	WCHAR MsiexecPath[MAX_PATH];
	KXCFG_PROGRAM_CONFIGURATION MsiexecConfiguration;

	GetSystemDirectory(MsiexecPath, ARRAYSIZE(MsiexecPath));
	PathCchAppend(MsiexecPath, ARRAYSIZE(MsiexecPath), L"msiexec.exe");

	Success = KxCfgGetConfiguration(MsiexecPath, &MsiexecConfiguration);

	if (!Success || !MsiexecConfiguration.Enabled) {
		return FALSE;
	}

	return TRUE;
}