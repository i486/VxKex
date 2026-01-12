#include "buildcfg.h"
#include <KxCfgHlp.h>

KXCFGDECLSPEC BOOLEAN WINAPI KxCfgEnableVxKexForMsiexec(
	IN	BOOLEAN	Enable,
	IN	HANDLE	TransactionHandle OPTIONAL)
{
	WCHAR MsiexecPath[MAX_PATH];
	WCHAR MsiexecWow64Path[MAX_PATH];
	BOOLEAN Success;

	GetSystemDirectory(MsiexecPath, ARRAYSIZE(MsiexecPath));
	PathCchAppend(MsiexecPath, ARRAYSIZE(MsiexecPath), L"msiexec.exe");

	if (KexRtlOperatingSystemBitness() == 64) {
		// Also enable for the WOW64 msiexec
		GetSystemWow64Directory(MsiexecWow64Path, ARRAYSIZE(MsiexecWow64Path));
		PathCchAppend(MsiexecWow64Path, ARRAYSIZE(MsiexecWow64Path), L"msiexec.exe");
	}

	if (Enable) {
		KXCFG_PROGRAM_CONFIGURATION MsiexecConfiguration;

		RtlZeroMemory(&MsiexecConfiguration, sizeof(MsiexecConfiguration));
		MsiexecConfiguration.Enabled = TRUE;

		if (KexRtlOperatingSystemBitness() == 64) {
			Success = KxCfgSetConfiguration(
				MsiexecWow64Path,
				&MsiexecConfiguration,
				TransactionHandle);

			if (!Success) {
				return FALSE;
			}
		}

		Success = KxCfgSetConfiguration(MsiexecPath, &MsiexecConfiguration, TransactionHandle);

		return Success;
	} else {
		// WOW64
		if (KexRtlOperatingSystemBitness() == 64) {
			Success = KxCfgDeleteConfiguration(MsiexecWow64Path, TransactionHandle);

			if (!Success) {
				return FALSE;
			}
		}

		Success = KxCfgDeleteConfiguration(MsiexecPath, TransactionHandle);

		return Success;
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

	// Both 32-bit and 64-bit msiexec must be vxkex'd for us to say that VxKex
	// is enabled for MSIs.
	if (KexRtlOperatingSystemBitness() == 64) {
		GetSystemWow64Directory(MsiexecPath, ARRAYSIZE(MsiexecPath));
		PathCchAppend(MsiexecPath, ARRAYSIZE(MsiexecPath), L"msiexec.exe");

		Success = KxCfgGetConfiguration(MsiexecPath, &MsiexecConfiguration);

		if (!Success || !MsiexecConfiguration.Enabled) {
			return FALSE;
		}
	}

	return TRUE;
}