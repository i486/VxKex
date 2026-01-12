#include "buildcfg.h"
#include "kxmip.h"

KXMIAPI HRESULT WINAPI CreateAppContainerProfile(
	IN	PCWSTR				AppContainerName,
	IN	PCWSTR				DisplayName,
	IN	PCWSTR				Description,
	IN	PSID_AND_ATTRIBUTES	Capabilities,
	IN	ULONG				NumberOfCapabilities,
	OUT	PSID				*AppContainerSid)
{
	KexLogWarningEvent(
		L"CreateAppContainerProfile called: %s\r\n\r\n"
		L"DisplayName:          %s\r\n"
		L"NumberOfCapabilities: %lu",
		AppContainerName,
		DisplayName,
		Description,
		NumberOfCapabilities);

	KexDebugCheckpoint();

	return E_NOTIMPL;
}

KXMIAPI HRESULT WINAPI DeleteAppContainerProfile(
	IN	PCWSTR	AppContainerName)
{
	KexLogWarningEvent(
		L"DeleteAppContainerProfile called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return S_OK;
}

KXMIAPI HRESULT WINAPI DeriveAppContainerSidFromAppContainerName(
	IN	PCWSTR	AppContainerName,
	OUT	PSID	*AppContainerSid)
{
	KexLogWarningEvent(
		L"DeriveAppContainerSidFromAppContainerName called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return E_NOTIMPL;
}

KXMIAPI HRESULT WINAPI GetAppContainerFolderPath(
	IN	PCWSTR	AppContainerName,
	OUT	PPWSTR	FolderPath)
{
	KexLogWarningEvent(
		L"GetAppContainerFolderPath called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return E_NOTIMPL;
}

KXMIAPI HRESULT WINAPI GetAppContainerRegistryLocation(
	IN	REGSAM	DesiredAccess,
	OUT	PHKEY	AppContainerKey)
{
	KexLogWarningEvent(L"GetAppContainerRegistryLocation called");
	KexDebugCheckpoint();
	return E_NOTIMPL;
}