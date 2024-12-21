#include "buildcfg.h"
#include "kxdxp.h"

KXDXAPI HRESULT WINAPI DCompositionCreateDevice(
	IN	IDXGIDevice	*DXGIDevice OPTIONAL,
	IN	REFIID		RefIID,
	OUT	PPVOID		DCompositionDevice)
{
	ASSERT (DCompositionDevice != NULL);

	KexLogWarningEvent(L"Unimplemented function DCompositionCreateDevice called");
	*DCompositionDevice = NULL;
	return E_NOINTERFACE;
}

KXDXAPI HRESULT WINAPI DCompositionCreateDevice2(
	IN	IUnknown	*RenderingDevice OPTIONAL,
	IN	REFIID		RefIID,
	OUT	PPVOID		DCompositionDevice)
{
	ASSERT (DCompositionDevice != NULL);

	KexLogWarningEvent(L"Unimplemented function DCompositionCreateDevice2 called");
	*DCompositionDevice = NULL;
	return E_NOINTERFACE;
}

KXDXAPI HRESULT WINAPI DCompositionCreateDevice3(
	IN	IUnknown	*RenderingDevice OPTIONAL,
	IN	REFIID		RefIID,
	OUT	PPVOID		DCompositionDevice)
{
	ASSERT (DCompositionDevice != NULL);

	KexLogWarningEvent(L"Unimplemented function DCompositionCreateDevice3 called");
	*DCompositionDevice = NULL;
	return E_NOINTERFACE;
}