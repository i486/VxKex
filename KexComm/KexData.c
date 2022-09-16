#include <KexComm.h>
#include <KexData.h>

BOOL KexOpenIfeoKeyForExe(
	IN	PCWSTR	ExeFullPath OPTIONAL,
	IN	REGSAM	DesiredSam,
	OUT	PHKEY	Key)
{
	WCHAR KexIfeoKeyName[54 + MAX_PATH] = L"SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\";
	LSTATUS Status;

	if (ExeFullPath) {
		if (FAILED(StringCchCat(KexIfeoKeyName, ARRAYSIZE(KexIfeoKeyName), ExeFullPath))) {
			return FALSE;
		}
	}

	Status = RegOpenKeyEx(HKEY_CURRENT_USER, KexIfeoKeyName, 0, DesiredSam, Key);

	if (Status != ERROR_SUCCESS) {
		SetLastError(Status);
		return FALSE;
	}

	return TRUE;
}

BOOL KexReadIfeoParameters(
	IN	PCWSTR					ExeFullPath,
	OUT	PKEX_IFEO_PARAMETERS	KexIfeoParameters)
{
	HRESULT Result;
	HKEY Key;

	if (!ExeFullPath || !KexIfeoParameters) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	ZeroMemory(KexIfeoParameters, sizeof(*KexIfeoParameters));

	if (FAILED((Result = StringCchCopy(KexIfeoParameters->WinVerSpoof, ARRAYSIZE(KexIfeoParameters->WinVerSpoof), L"NONE")))) {
		SetLastError(WIN32_FROM_HRESULT(Result));
		return FALSE;
	}

	if (!KexOpenIfeoKeyForExe(ExeFullPath, KEY_QUERY_VALUE, &Key)) {
		return FALSE;
	}

	CHECKED(RegReadSz(Key, NULL, L"WinVerSpoof", KexIfeoParameters->WinVerSpoof, ARRAYSIZE(KexIfeoParameters->WinVerSpoof)));
	CHECKED(RegReadBoolean(Key, NULL, L"EnableVxKex",			&KexIfeoParameters->EnableVxKex));
	CHECKED(RegReadBoolean(Key, NULL, L"AlwaysShowDebug",		&KexIfeoParameters->AlwaysShowDebug));
	CHECKED(RegReadBoolean(Key, NULL, L"DisableForChild",		&KexIfeoParameters->DisableForChild));
	CHECKED(RegReadBoolean(Key, NULL, L"DisableAppSpecific",	&KexIfeoParameters->DisableAppSpecific));
	CHECKED(RegReadBoolean(Key, NULL, L"WaitForChild",			&KexIfeoParameters->WaitForChild));
	CHECKED(RegReadBoolean(Key, NULL, L"DebuggerSpoof",			&KexIfeoParameters->DebuggerSpoof));

	return TRUE;

Error:
	return FALSE;
}