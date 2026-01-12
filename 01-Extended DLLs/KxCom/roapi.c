#include "buildcfg.h"
#include "kxcomp.h"

KXCOMAPI HRESULT WINAPI RoGetAgileReference(
	IN	ULONG		Options,
	IN	REFIID		RefIID,
	IN	IUnknown	*pUnknown,
	OUT	IUnknown	**AgileReference)
{
	KexLogWarningEvent(L"Unimplemented stub function RoGetAgileReference was called");
	return E_NOTIMPL;
}