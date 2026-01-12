#pragma once

#include <KexComm.h>
#include <powrprof.h>

#if !defined(KXMIAPI) && defined(KEX_ENV_WIN32)
#  define KXMIAPI
#  pragma comment(lib, "KxMi.lib")
#endif

typedef ULONG (CALLBACK *PDEVICE_NOTIFY_CALLBACK_ROUTINE) (
    IN	PVOID	Context OPTIONAL,
    IN	ULONG	Type,
    IN	PVOID	Setting);

typedef struct _DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS {
	PDEVICE_NOTIFY_CALLBACK_ROUTINE	Callback;
	PVOID							Context;
} TYPEDEF_TYPE_NAME(DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS);

//
// powrprof.c
//

KXMIAPI POWER_PLATFORM_ROLE WINAPI PowerDeterminePlatformRoleEx(
	IN	ULONG	Version);

KXMIAPI ULONG WINAPI PowerRegisterSuspendResumeNotification(
	IN	ULONG			Flags,
	IN	HANDLE			Recipient,
	OUT	PHPOWERNOTIFY	RegistrationHandle);

KXMIAPI ULONG WINAPI PowerUnregisterSuspendResumeNotification(
	IN OUT	HPOWERNOTIFY	RegistrationHandle);