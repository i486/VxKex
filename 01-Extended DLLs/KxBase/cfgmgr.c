#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI CONFIGRET WINAPI CM_Register_Notification(
	IN	PCM_NOTIFY_FILTER	Filter,
	IN	PVOID				Context OPTIONAL,
	IN	PCM_NOTIFY_CALLBACK	Callback,
	OUT	PHCMNOTIFICATION	NotifyContext)
{
	KexLogUnimplementedFunctionEvent();
	KexDebugCheckpoint();
	return CR_CALL_NOT_IMPLEMENTED;
}

KXBASEAPI CONFIGRET WINAPI CM_Unregister_Notification(
	IN	HCMNOTIFICATION		NotifyContext)
{
	KexLogUnimplementedFunctionEvent();
	KexDebugCheckpoint();
	return CR_CALL_NOT_IMPLEMENTED;
}