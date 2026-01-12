#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI POWER_PLATFORM_ROLE WINAPI PowerDeterminePlatformRoleEx(
	IN	ULONG	Version)
{
	return PowerDeterminePlatformRole();
}

//
// Note: It is technically possible to make these work on Windows 7.
// However, it seems unlikely that applications actually use these in a
// useful manner, so they are stubbed for now.
//

KXBASEAPI ULONG WINAPI PowerRegisterSuspendResumeNotification(
	IN	ULONG			Flags,
	IN	HANDLE			Recipient,
	OUT	PHPOWERNOTIFY	RegistrationHandle)
{
	if (Flags != DEVICE_NOTIFY_CALLBACK) {
		return RtlNtStatusToDosError(STATUS_INVALID_PARAMETER);
	}

	return RtlNtStatusToDosError(STATUS_NOT_SUPPORTED);
}

KXBASEAPI ULONG WINAPI PowerUnregisterSuspendResumeNotification(
	IN OUT	HPOWERNOTIFY	RegistrationHandle)
{
	return RtlNtStatusToDosError(STATUS_NOT_SUPPORTED);
}