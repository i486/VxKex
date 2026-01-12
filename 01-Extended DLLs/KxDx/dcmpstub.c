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

//
// IDXGIFactoryMedia has something to do with DirectComposition so we can't properly
// support it.
//

HRESULT STDMETHODCALLTYPE IDXGIFactoryMedia_QueryInterface(
	IN	IUnknown	*This,
	IN	REFIID		RefIID,
	OUT	PPVOID		Object)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Object != NULL);

	*Object = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown)) {
		*Object = This;
	} else if (IsEqualIID(RefIID, &IID_IDXGIFactoryMedia)) {
		*Object = This;
	} else {
		ReportNoInterfaceError(RefIID, E_NOINTERFACE, L"QueryInterface failed");
		return E_NOINTERFACE;
	}

	return S_OK;
}

ULONG STDMETHODCALLTYPE IDXGIFactoryMedia_AddRef(
	IN	IUnknown	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE IDXGIFactoryMedia_Release(
	IN	IUnknown	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE IDXGIFactoryMedia_CreateSwapChainForCompositionSurfaceHandle(
	IN	IDXGIFactoryMedia			*This,
	IN	IUnknown					*Device,
	IN	HANDLE						Surface OPTIONAL,
	IN	CONST DXGI_SWAP_CHAIN_DESC1	*Desc,
	IN	IDXGIOutput					*RestrictToOutput OPTIONAL,
	OUT	IDXGISwapChain1				**SwapChain)
{
	KexLogWarningEvent(L"Unimplemented function CreateSwapChainForCompositionSurfaceHandle called");
	*SwapChain = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIFactoryMedia_CreateDecodeSwapChainForCompositionSurfaceHandle(
	IN	IDXGIFactoryMedia			*This,
	IN	IUnknown					*Device,
	IN	HANDLE						Surface OPTIONAL,
	IN	PVOID						Desc,
	IN	IDXGIResource				*YuvDecodeBuffers,
	IN	IDXGIOutput					*RestrictToOutput OPTIONAL,
	OUT	IUnknown					**SwapChain)
{
	KexLogWarningEvent(L"Unimplemented function CreateDecodeSwapChainForCompositionSurfaceHandle called");
	*SwapChain = NULL;
	return E_NOTIMPL;
}

IDXGIFactoryMediaVtbl DXGIFactoryMediaVtbl = {
	IDXGIFactoryMedia_QueryInterface,
	IDXGIFactoryMedia_AddRef,
	IDXGIFactoryMedia_Release,
	IDXGIFactoryMedia_CreateSwapChainForCompositionSurfaceHandle,
	IDXGIFactoryMedia_CreateDecodeSwapChainForCompositionSurfaceHandle
};

IDXGIFactoryMedia DXGIFactoryMedia = {
	&DXGIFactoryMediaVtbl
};

HRESULT WINAPI CreateIDXGIFactoryMedia(
	OUT	PPVOID	FactoryMedia)
{
	ASSERT (FactoryMedia != NULL);
	*FactoryMedia = &DXGIFactoryMedia;
	return S_OK;
}