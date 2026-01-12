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
	return E_NOTIMPL;
}