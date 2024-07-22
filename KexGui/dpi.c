#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>

STATIC BOOLEAN AlreadyInitialized = FALSE;
STATIC INT DpiX = USER_DEFAULT_SCREEN_DPI;
STATIC INT DpiY = USER_DEFAULT_SCREEN_DPI;

STATIC VOID DpiInitialize(
	VOID)
{
	HDC DeviceContext;

	DeviceContext = GetDC(NULL);
	DpiX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
	DpiY = GetDeviceCaps(DeviceContext, LOGPIXELSY);
	ReleaseDC(NULL, DeviceContext);

	AlreadyInitialized = TRUE;
}

KEXGDECLSPEC INT KEXGAPI DpiScaleX(
	IN	INT	PixelsX)
{
	if (!AlreadyInitialized) {
		DpiInitialize();
	}

	return MulDiv(PixelsX, DpiX, USER_DEFAULT_SCREEN_DPI);
}

KEXGDECLSPEC INT KEXGAPI DpiScaleY(
	IN	INT	PixelsY)
{
	if (!AlreadyInitialized) {
		DpiInitialize();
	}

	return MulDiv(PixelsY, DpiY, USER_DEFAULT_SCREEN_DPI);
}