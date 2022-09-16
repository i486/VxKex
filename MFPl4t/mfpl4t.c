#include <KexDll.h>
#include <mfapi.h>

HRESULT DLLAPI MFCreateDXGIDeviceManager(
	OUT	UINT		*resetToken,
	OUT	IUnknown	**ppDeviceManager)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI MFCreateDXGISurfaceBuffer(
	IN	REFIID			riid,
	IN	IUnknown		*punkSurface,
	IN	UINT			uSubresourceIndex,
	IN	BOOL			fBottomUpWhenLinear,
	OUT	IMFMediaBuffer	**ppBuffer)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI MFLockDXGIDeviceManager(
	OUT	UINT			*pResetToken,
	OUT	IUnknown		**ppManager)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}

HRESULT DLLAPI MFUnlockDXGIDeviceManager(
	VOID)
{
	ODS_ENTRY();
	return E_NOTIMPL;
}