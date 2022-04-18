#include <Windows.h>
#include <Shlwapi.h>
#include <KexComm.h>
#include <KexDll.h>

WINBASEAPI BOOL WINAPI PROXY_FUNCTION(VerifyVersionInfoW) (
	IN	LPOSVERSIONINFOEXW	lpVersionInfo,
	IN	DWORD				dwTypeMask,
	IN	DWORDLONG			dwlConditionMask)
{
	STATIC BOOL bAlwaysReturnTrue = -1;

	// APPSPECIFICHACK
	if (bAlwaysReturnTrue == -1) {
		bAlwaysReturnTrue = FALSE;
		bAlwaysReturnTrue |= IsExeBaseName(L"ONEDRIVESETUP.EXE");
	}

	if (bAlwaysReturnTrue) {
		ODS(L"bAlwaysReturnTrue == TRUE");
		return TRUE;
	} else {
		return VerifyVersionInfoW(lpVersionInfo, dwTypeMask, dwlConditionMask);
	}
}