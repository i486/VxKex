#include "buildcfg.h"
#include "kxmip.h"
#include <WtsApi32.h>

STATIC VOID WINAPI PostProcessWTSSessionInfoExAorW(
	IN	PVOID			SessionInfoExAorW)
{
	PWTSINFOEXW WTSInfo;
	LONG SessionFlags;

	//
	// Fixes broken desktop notifications in Chromium browsers due to a discrepancy
	// in how Win7 and Win8+ report the locked status of the screen.
	//

	if (!(KexData->Flags & KEXDATA_FLAG_CHROMIUM) &&
		KexData->IfeoParameters.WinVerSpoof < WinVerSpoofWin8) {

		// We only need to "fix" the WTS info if this is a Chromium process (since
		// new versions of Chrome assume the Win8 and later behavior), or if we are
		// spoofing to a Windows version greater than 7.
		return;
	}

	//
	// This function will pretend that the ANSI and Unicode variants of the
	// structure are equivalent. They are not, but the offset of the SessionFlags
	// member is the same for both.
	//

	WTSInfo = (PWTSINFOEXW) SessionInfoExAorW;
	SessionFlags = WTSInfo->Data.WTSInfoExLevel1.SessionFlags;

	//
	// Reverse the meaning of the locked and unlocked session state flags.
	// Microsoft reversed the meaning of these flags in Win8 and later.
	//
	// https://learn.microsoft.com/en-us/windows/win32/api/wtsapi32/ns-wtsapi32-wtsinfoex_level1_w
	//
	if (SessionFlags != WTS_SESSIONSTATE_UNKNOWN) {
		if (SessionFlags & WTS_SESSIONSTATE_UNLOCK) {
			SessionFlags &= ~WTS_SESSIONSTATE_UNLOCK;
		} else {
			SessionFlags |= WTS_SESSIONSTATE_UNLOCK;
		}
	}

	WTSInfo->Data.WTSInfoExLevel1.SessionFlags = SessionFlags;
}

KXMIAPI BOOL WINAPI Ext_WTSQuerySessionInformationA(
	IN	HANDLE			Server,
	IN	ULONG			SessionId,
	IN	WTS_INFO_CLASS	InformationClass,
	OUT	PPSTR			Buffer,
	OUT	PULONG			BytesReturned)
{
	BOOL Success;

	Success = WTSQuerySessionInformationA(
		Server,
		SessionId,
		InformationClass,
		Buffer,
		BytesReturned);

	if (InformationClass == WTSSessionInfoEx && Success) {
		PostProcessWTSSessionInfoExAorW(*Buffer);
	}

	return Success;
}

KXMIAPI BOOL WINAPI Ext_WTSQuerySessionInformationW(
	IN	HANDLE			Server,
	IN	ULONG			SessionId,
	IN	WTS_INFO_CLASS	InformationClass,
	OUT	PPWSTR			Buffer,
	OUT	PULONG			BytesReturned)
{
	BOOL Success;

	Success = WTSQuerySessionInformationW(
		Server,
		SessionId,
		InformationClass,
		Buffer,
		BytesReturned);

	if (InformationClass == WTSSessionInfoEx && Success) {
		PostProcessWTSSessionInfoExAorW(*Buffer);
	}

	return Success;
}