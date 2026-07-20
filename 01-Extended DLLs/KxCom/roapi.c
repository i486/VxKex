#include "buildcfg.h"
#include "kxcomp.h"

KXCOMAPI HRESULT WINAPI RoGetAgileReference(
	IN	ULONG		Options,
	IN	REFIID		RefIID,
	IN	IUnknown	*pUnknown,
	OUT	IUnknown	**AgileReference)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

KXCOMAPI HRESULT WINAPI RoGetParameterizedTypeInstanceIID(
	IN	ULONG		NameElementCount,
	IN	PPCWSTR		NameElements,
	IN	PVOID		MetadataLocator,
	OUT	LPGUID		Iid,
	OUT	PVOID		Extra OPTIONAL)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}