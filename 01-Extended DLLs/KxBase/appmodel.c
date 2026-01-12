#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI LONG WINAPI GetCurrentPackageFullName(
	IN OUT	PULONG	PackageFullNameLength,
	OUT		PWSTR	PackageFullName OPTIONAL)
{
	if (!PackageFullNameLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFullNameLength != 0 && !PackageFullName) {
		return ERROR_INVALID_PARAMETER;
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetCurrentPackageId(
	IN OUT	PULONG	BufferLength,
	OUT		PBYTE	Buffer OPTIONAL)
{
	if (!BufferLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*BufferLength != 0 && !Buffer) {
		return ERROR_INVALID_PARAMETER;
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackageFamilyName(
	IN		HANDLE	ProcessHandle,
	IN OUT	PULONG	NameLength,
	OUT		PWSTR	PackageFamilyName OPTIONAL)
{
	if (!ProcessHandle || !NameLength || *NameLength && !PackageFamilyName) {
		return ERROR_INVALID_PARAMETER;
	}

	*NameLength = 0;

	if (PackageFamilyName) {
		PackageFamilyName[0] = '\0';
	}

	return APPMODEL_ERROR_NO_PACKAGE;
}

KXBASEAPI LONG WINAPI GetPackagesByPackageFamily(
	IN		PCWSTR	PackageFamilyName,
	IN OUT	PULONG	Count,
	OUT		PPWSTR	PackageFullNames OPTIONAL,
	IN OUT	PULONG	BufferLength,
	OUT		PPWSTR	Buffer OPTIONAL)
{
	if (!Count || !BufferLength) {
		return ERROR_INVALID_PARAMETER;
	}

	*Count = 0;
	*BufferLength = 0;

	return ERROR_SUCCESS;
}

KXBASEAPI LONG WINAPI AppPolicyGetProcessTerminationMethod(
	IN	HANDLE	ProcessToken,
	OUT	PULONG	Policy)
{
	if (!ProcessToken || !Policy) {
		return RtlNtStatusToDosError(STATUS_INVALID_PARAMETER);
	}

	*Policy = 0;
	return ERROR_SUCCESS;
}

KXBASEAPI HRESULT WINAPI CreateAppContainerProfile(
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

KXBASEAPI HRESULT WINAPI DeleteAppContainerProfile(
	IN	PCWSTR	AppContainerName)
{
	KexLogWarningEvent(
		L"DeleteAppContainerProfile called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return S_OK;
}

KXBASEAPI HRESULT WINAPI DeriveAppContainerSidFromAppContainerName(
	IN	PCWSTR	AppContainerName,
	OUT	PSID	*AppContainerSid)
{
	KexLogWarningEvent(
		L"DeriveAppContainerSidFromAppContainerName called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return E_NOTIMPL;
}

KXBASEAPI HRESULT WINAPI GetAppContainerFolderPath(
	IN	PCWSTR	AppContainerName,
	OUT	PPWSTR	FolderPath)
{
	KexLogWarningEvent(
		L"GetAppContainerFolderPath called: %s",
		AppContainerName);

	KexDebugCheckpoint();

	return E_NOTIMPL;
}

KXBASEAPI HRESULT WINAPI GetAppContainerRegistryLocation(
	IN	REGSAM	DesiredAccess,
	OUT	PHKEY	AppContainerKey)
{
	KexLogWarningEvent(L"GetAppContainerRegistryLocation called");
	KexDebugCheckpoint();
	return E_NOTIMPL;
}

KXBASEAPI HRESULT WINAPI AppXGetPackageSid(
	IN	PCWSTR	PackageMoniker,
	OUT	PSID	*PackageSid)
{
	KexLogWarningEvent(L"AppXGetPackageSid called: %s", PackageMoniker);
	KexDebugCheckpoint();

	if (!PackageMoniker || !PackageSid) {
		return E_INVALIDARG;
	}

	return E_NOTIMPL;
}

KXBASEAPI VOID WINAPI AppXFreeMemory(
	IN	PVOID	Pointer)
{
	RtlFreeHeap(RtlProcessHeap(), 0, Pointer);
}

KXBASEAPI ULONG WINAPI PackageFamilyNameFromFullName(
	IN		PCWSTR	PackageFullName,
	IN OUT	PULONG	PackageFamilyNameLength,
	OUT		PWSTR	PackageFamilyName)
{
	if (!PackageFullName || !PackageFamilyNameLength) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFamilyNameLength != 0 && !PackageFamilyName) {
		return ERROR_INVALID_PARAMETER;
	}

	if (*PackageFamilyNameLength != 0) {
		PackageFamilyName[0] = '\0';
	}

	*PackageFamilyNameLength = 0;
	return ERROR_SUCCESS;
}

KXBASEAPI LONG WINAPI GetApplicationUserModelId(
	IN		HANDLE	ProcessHandle,
	IN OUT	PULONG	ApplicationUserModelIdLength,
	OUT		PWSTR	ApplicationUserModelId)
{
	return APPMODEL_ERROR_NO_APPLICATION;
}

KXBASEAPI LONG WINAPI GetCurrentApplicationUserModelId(
	IN OUT	PULONG	Cch,
	OUT		PWSTR	Buffer)
{
	return APPMODEL_ERROR_NO_APPLICATION;
}