#include "buildcfg.h"
#include "kxdxp.h"
#include <KexW32ML.h>
#include <d2d1.h>

HRESULT (STDMETHODCALLTYPE *ID2D1Device_QueryInterface)(IUnknown *, REFIID, PPVOID);
HRESULT (STDMETHODCALLTYPE *ID2D1Factory1_QueryInterface)(IUnknown *, REFIID, PPVOID);

//
// ID2D1Device1
//

HRESULT STDMETHODCALLTYPE ID2D1Device1_QueryInterface(
	IN	ID2D1Device1	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Object)
{
	HRESULT Result;

	if (IsEqualIID(RefIID, &IID_ID2D1Device1)) {
		*Object = This;
		This->lpVtbl->Base.Base.Base.AddRef((IUnknown *) This);
		return S_OK;
	}

	Result = ID2D1Device_QueryInterface(
		(IUnknown *) This,
		RefIID,
		Object);

	if (FAILED(Result)) {
		ReportNoInterfaceError(RefIID, Result, L"QueryInterface failed");
	}

	return Result;
}

D2D1_RENDERING_PRIORITY STDMETHODCALLTYPE ID2D1Device1_GetRenderingPriority(
	IN	ID2D1Device1	*This)
{
	return D2D1_RENDERING_PRIORITY_NORMAL;
}

VOID STDMETHODCALLTYPE ID2D1Device1_SetRenderingPriority(
	IN	ID2D1Device1			*This,
	IN	D2D1_RENDERING_PRIORITY	RenderingPriority)
{
	return;
}

VOID WrapDirect2DDevice(
	IN OUT	ID2D1Device	*Device)
{
	STATIC ID2D1Device1Vtbl Vtbl;

	ASSERT (Device != NULL);

	Vtbl.Base = *Device->lpVtbl;
	ID2D1Device_QueryInterface = Vtbl.Base.Base.Base.QueryInterface;
	Vtbl.Base.Base.Base.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(IUnknown *, REFIID, PPVOID)) ID2D1Device1_QueryInterface;
	Vtbl.GetRenderingPriority = ID2D1Device1_GetRenderingPriority;
	Vtbl.SetRenderingPriority = ID2D1Device1_SetRenderingPriority;

	Device->lpVtbl = (ID2D1DeviceVtbl *) &Vtbl;
}

//
// ID2D1Factory2
//

HRESULT STDMETHODCALLTYPE ID2D1Factory2_QueryInterface(
	IN	ID2D1Factory2	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Object)
{
	HRESULT Result;

	if (IsEqualIID(RefIID, &IID_ID2D1Factory2)) {
		*Object = This;
		This->lpVtbl->Base.Base.Base.AddRef((IUnknown *) This);
		return S_OK;
	}

	Result = ID2D1Factory1_QueryInterface(
		(IUnknown *) This,
		RefIID,
		Object);

	if (FAILED(Result)) {
		ReportNoInterfaceError(RefIID, Result, L"QueryInterface failed");
	}

	return Result;
}

HRESULT STDMETHODCALLTYPE ID2D1Factory2_CreateDevice(
	ID2D1Factory2	*This,
	IDXGIDevice		*DXGIDevice,
	ID2D1Device1	**D2D1Device)
{
	HRESULT Result;

	ASSERT (This != NULL);
	ASSERT (DXGIDevice != NULL);
	ASSERT (D2D1Device != NULL);

	// Call the system ID2D1Factory1::CreateDevice.
	// This call will produce an ID2D1Device interface, which we
	// then need to wrap into an ID2D1Device1 interface.
	Result = This->lpVtbl->Base.CreateDevice(
		(ID2D1Factory1 *) This,
		DXGIDevice,
		(ID2D1Device **) D2D1Device);

	if (FAILED(Result)) {
		return Result;
	}

	WrapDirect2DDevice((ID2D1Device *) *D2D1Device);
	return Result;
}

VOID WrapDirect2DFactory(
	IN OUT	ID2D1Factory1	*Factory)
{
	STATIC ID2D1Factory2Vtbl Vtbl;

	ASSERT (Factory != NULL);

	Vtbl.Base = *Factory->lpVtbl;
	ID2D1Factory1_QueryInterface = Vtbl.Base.Base.Base.QueryInterface;
	Vtbl.Base.Base.Base.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(IUnknown *, REFIID, PPVOID)) ID2D1Factory2_QueryInterface;
	Vtbl.CreateDevice = ID2D1Factory2_CreateDevice;

	Factory->lpVtbl = (ID2D1Factory1Vtbl *) &Vtbl;
}