#include "buildcfg.h"
#include "kxuserp.h"
#include <KxMi.h>

STATIC ULONG CALLBACK PowerSuspendResumeCallback(
	IN	PVOID	Context OPTIONAL,
    IN	ULONG	Type,
    IN	PVOID	Setting)
{
	return 0;
}

KXUSERAPI BOOL WINAPI RegisterSuspendResumeNotification(
	IN	HANDLE	Recipient,
	IN	ULONG	Flags)
{
	ULONG ErrorCode;
	HPOWERNOTIFY RegistrationHandle;
	DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS NotifySubscribeParameters;

	if (Flags == DEVICE_NOTIFY_WINDOW_HANDLE) {
		NotifySubscribeParameters.Callback = PowerSuspendResumeCallback;
		NotifySubscribeParameters.Context = (HWND) Recipient;
	} else {
		NotifySubscribeParameters = *(PDEVICE_NOTIFY_SUBSCRIBE_PARAMETERS) Recipient;
	}

	ErrorCode = PowerRegisterSuspendResumeNotification(
		Flags,
		&NotifySubscribeParameters,
		&RegistrationHandle);

	if (ErrorCode != ERROR_SUCCESS) {
		SetLastError(ErrorCode);
		return FALSE;
	}

	return TRUE;
}

KXUSERAPI BOOL WINAPI UnregisterSuspendResumeNotification(
	IN OUT	HPOWERNOTIFY	Handle)
{
	ULONG ErrorCode;

	ErrorCode = PowerUnregisterSuspendResumeNotification(Handle);

	if (ErrorCode != ERROR_SUCCESS) {
		SetLastError(ErrorCode);
		return FALSE;
	}

	return TRUE;
}