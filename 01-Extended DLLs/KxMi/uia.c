#include "buildcfg.h"
#include "kxmip.h"

// Forget about including this header. It's completely BROKEN and won't
// compile without modifications.
//#include <UIAutomation.h>

KXMIAPI HRESULT WINAPI UiaRaiseNotificationEvent(
	IN		IUnknown					*Provider,
	IN		NotificationKind			NotifKind,
	IN		NotificationProcessing		NotifProcessing,
	IN		BSTR						DisplayString OPTIONAL,
	IN		BSTR						ActivityId)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

KXMIAPI HRESULT WINAPI UiaDisconnectProvider(
	IN		IUnknown					*Provider)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

//
// Some Unity games require this.
//
KXMIAPI HRESULT WINAPI UiaDisconnectAllProviders(
	VOID)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

//
// Apparently this fixes Rockstar launcher (although installer still does
// not work).
//
KXMIAPI HRESULT WINAPI UiaRaiseChangesEvent(
    IN	IUnknown	*Provider,
    IN	INT			EventIdCount,
    IN	PVOID		UiaChanges)
{
    KexLogUnimplementedFunctionEvent();
    return E_NOTIMPL;
}