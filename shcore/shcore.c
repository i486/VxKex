#include <Windows.h>
#include <KexDll.h>

typedef enum _MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI,
	MDT_ANGULAR_DPI,
	MDT_RAW_DPI,
	MDT_DEFAULT
} MONITOR_DPI_TYPE;

typedef enum _SHELL_UI_COMPONENT {
	SHELL_UI_COMPONENT_TASKBARS,
	SHELL_UI_COMPONENT_NOTIFICATIONAREA,
	SHELL_UI_COMPONENT_DESKBAND
} SHELL_UI_COMPONENT;

typedef enum _PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef enum _DEVICE_SCALE_FACTOR {
	DEVICE_SCALE_FACTOR_INVALID,
	SCALE_100_PERCENT,
	SCALE_120_PERCENT,
	SCALE_125_PERCENT,
	SCALE_140_PERCENT,
	SCALE_150_PERCENT,
	SCALE_160_PERCENT,
	SCALE_175_PERCENT,
	SCALE_180_PERCENT,
	SCALE_200_PERCENT,
	SCALE_225_PERCENT,
	SCALE_250_PERCENT,
	SCALE_300_PERCENT,
	SCALE_350_PERCENT,
	SCALE_400_PERCENT,
	SCALE_450_PERCENT,
	SCALE_500_PERCENT
} DEVICE_SCALE_FACTOR;

DECLSPEC_IMPORT UINT WINAPI GetDpiForSystem(
	VOID);

DLLAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR hmonitor,
	IN	MONITOR_DPI_TYPE dpiType,
	OUT	LPUINT dpiX,
	OUT	LPUINT dpiY)
{
	HDC hdcScreen = GetDC(NULL);
	*dpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
	*dpiY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
	ReleaseDC(NULL, hdcScreen);
	return S_OK;
}

DLLAPI UINT WINAPI GetDpiForShellUIComponent(
	IN	SHELL_UI_COMPONENT	component)
{
	return GetDpiForSystem();
}

DLLAPI HRESULT WINAPI SetProcessDpiAwareness(
	IN	PROCESS_DPI_AWARENESS	value)
{
	if (value != PROCESS_DPI_UNAWARE) {
		SetProcessDPIAware();
	}

	return S_OK;
}

DLLAPI HRESULT WINAPI GetProcessDpiAwareness(
	IN	HANDLE					hprocess,
	OUT	PROCESS_DPI_AWARENESS	*value)
{
	// TODO: Do something proper for this. For example, create a remote thread and
	// call IsProcessDPIAware(). Or find out if user32.dll is treated special like
	// ntdll and kernel32 in that it is always loaded at the same address.
	*value = PROCESS_DPI_UNAWARE;
	return S_OK;
}

DLLAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR			hMon,
	OUT	DEVICE_SCALE_FACTOR	*pScale)
{
	// TODO: return a real value for this (can be read from somewhere in the registry
	// and divided by 96)
	*pScale = SCALE_100_PERCENT;
	return S_OK;
}