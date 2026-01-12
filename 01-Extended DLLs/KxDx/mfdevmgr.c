#include "buildcfg.h"
#include "kxdxp.h"

KXDXAPI HRESULT WINAPI MFCreateDXGIDeviceManager(
	OUT	PUINT	ResetToken,
	OUT	PPVOID	DeviceManager)
{
	KexLogWarningEvent(L"Unimplemented function MFCreateDXGIDeviceManager called");

	*DeviceManager = NULL;
	return E_NOINTERFACE;
}