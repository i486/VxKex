#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI Ext_SetTokenInformation(
	IN	HANDLE					TokenHandle,
	IN	TOKEN_INFORMATION_CLASS	TokenInformationClass,
	IN	PVOID					TokenInformation,
	IN	ULONG					TokenInformationLength)
{
	if (TokenInformationClass == TokenIntegrityLevel) {
		return TRUE;
	}

	return SetTokenInformation(
		TokenHandle,
		TokenInformationClass,
		TokenInformation,
		TokenInformationLength);
}