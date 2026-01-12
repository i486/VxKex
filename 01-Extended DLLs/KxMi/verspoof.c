#include "buildcfg.h"
#include "kxmip.h"

//
// Chromium checks the OS version by calling GetFileVersionInfo on kernel32.
// For versions of Chromium above 118, this spoof is necessary to get it to run,
// and on prior versions, this spoof removes the annoying Windows 7 deprecation
// notice.
//

KXMIAPI BOOL WINAPI Ext_GetFileVersionInfoW(
	IN	PCWSTR	FileName,
	IN	ULONG	Unused,
	IN	ULONG	BufferCb,
	OUT	PVOID	VersionInfo)
{
	BOOLEAN Success;

	Success = GetFileVersionInfoW(FileName, Unused, BufferCb, VersionInfo);

	if (!Success) {
		return FALSE;
	}

	//
	// APPSPECIFICHACK: Spoof kernel32.dll version for Chromium.
	//
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (KexData->Flags & KEXDATA_FLAG_CHROMIUM) {
			ASSERT (FileName != NULL);

			if (StringEqual(FileName, L"kernel32.dll") || StringEqual(FileName, L"kernelbase.dll")) {
				PVERHEAD VerHead;

				ASSERT (VersionInfo != NULL);
				ASSERT (BufferCb >= sizeof(VERHEAD));

				if (BufferCb < sizeof(VERHEAD)) {
					// bail out and don't do anything
					return Success;
				}

				VerHead = (PVERHEAD) VersionInfo;

				KexLogDebugEvent(
					L"Spoofing Kernel32/Kernelbase file version for Chromium\r\n\r\n"
					L"Original dwFileVersionMS = 0x%08lx\r\n"
					L"Original dwFileVersionLS = 0x%08lx\r\n",
					VerHead->vsf.dwFileVersionMS,
					VerHead->vsf.dwFileVersionLS);

				// 10.0.10240.0 (Windows 10 1504, aka "RTM")
				VerHead->vsf.dwFileVersionMS = 0x000A0000;
				VerHead->vsf.dwFileVersionLS = 0x28000000;
			}
		}
	}

	return Success;
}