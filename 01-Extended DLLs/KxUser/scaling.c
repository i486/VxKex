#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI AreDpiAwarenessContextsEqual(
	IN	DPI_AWARENESS_CONTEXT	Value1,
	IN	DPI_AWARENESS_CONTEXT	Value2)
{
	return (Value1 == Value2);
}

KXUSERAPI BOOL WINAPI IsValidDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	switch (Value) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		return TRUE;
	default:
		return FALSE;
	}
}

KXUSERAPI DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	switch (Value) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
	case DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED:
		return DPI_AWARENESS_UNAWARE;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
		return DPI_AWARENESS_SYSTEM_AWARE;
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		return DPI_AWARENESS_PER_MONITOR_AWARE;
	default:
		return DPI_AWARENESS_INVALID;
	}
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(
	VOID)
{
	if (IsProcessDPIAware()) {
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	} else {
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	BOOLEAN OldDpiAwareness;

	OldDpiAwareness = IsProcessDPIAware();

	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
		SetProcessDPIAware();
		break;
	default:
		return 0;
	}

	if (OldDpiAwareness) {
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	} else {
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	switch (DpiContext) {
	case DPI_AWARENESS_CONTEXT_UNAWARE:
		NOTHING;
		break;
	case DPI_AWARENESS_CONTEXT_SYSTEM_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE:
	case DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2:
		SetProcessDPIAware();
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext(
	IN	HWND	Window)
{
	ULONG WindowThreadId;
	ULONG WindowProcessId;

	WindowThreadId = GetWindowThreadProcessId(Window, &WindowProcessId);
	if (!WindowThreadId) {
		return 0;
	}

	if (WindowProcessId = (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {
		return GetThreadDpiAwarenessContext();
	}

	return DPI_AWARENESS_CONTEXT_UNAWARE;
}

KXUSERAPI BOOL WINAPI GetProcessDpiAwarenessInternal(
	IN	HANDLE					ProcessHandle,
	OUT	PROCESS_DPI_AWARENESS	*DpiAwareness)
{
	if (ProcessHandle == NULL ||
		ProcessHandle == NtCurrentProcess() ||
		GetProcessId(ProcessHandle) == (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {

		*DpiAwareness = IsProcessDPIAware() ? PROCESS_SYSTEM_DPI_AWARE : PROCESS_DPI_UNAWARE;
	} else {
		*DpiAwareness = PROCESS_DPI_UNAWARE;
	}

	return TRUE;
}

KXUSERAPI HRESULT WINAPI GetProcessDpiAwareness(
	IN	HANDLE					ProcessHandle,
	OUT	PROCESS_DPI_AWARENESS	*DpiAwareness)
{
	BOOLEAN Success;

	Success = GetProcessDpiAwarenessInternal(ProcessHandle, DpiAwareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessInternal(
	IN	PROCESS_DPI_AWARENESS	DpiAwareness)
{
	if (DpiAwareness >= PROCESS_MAX_DPI_AWARENESS) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (DpiAwareness != PROCESS_DPI_UNAWARE) {
		// On Windows 7, SetProcessDPIAware() always returns TRUE
		// no matter what, so there is no point in checking its
		// return value.
		SetProcessDPIAware();
	}

	return TRUE;
}

KXUSERAPI HRESULT WINAPI SetProcessDpiAwareness(
	IN	PROCESS_DPI_AWARENESS	Awareness)
{
	BOOLEAN Success;
	
	Success = SetProcessDpiAwarenessInternal(Awareness);

	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXUSERAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR			Monitor,
	IN	MONITOR_DPI_TYPE	DpiType,
	OUT	PULONG				DpiX,
	OUT	PULONG				DpiY)
{
	HDC DeviceContext;

	if (DpiType >= MDT_MAXIMUM_DPI) {
		return E_INVALIDARG;
	}

	if (!DpiX || !DpiY) {
		return E_INVALIDARG;
	}

	if (!IsProcessDPIAware()) {
		*DpiX = USER_DEFAULT_SCREEN_DPI;
		*DpiY = USER_DEFAULT_SCREEN_DPI;
		return S_OK;
	}

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		*DpiX = USER_DEFAULT_SCREEN_DPI;
		*DpiY = USER_DEFAULT_SCREEN_DPI;
		return S_OK;
	}

	*DpiX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
	*DpiY = GetDeviceCaps(DeviceContext, LOGPIXELSY);

	if (DpiType == MDT_EFFECTIVE_DPI) {
		DEVICE_SCALE_FACTOR ScaleFactor;

		// We have to multiply the DPI values by the scaling factor.
		GetScaleFactorForMonitor(Monitor, &ScaleFactor);

		*DpiX *= ScaleFactor;
		*DpiY *= ScaleFactor;
		*DpiX /= 100;
		*DpiY /= 100;
	}

	ReleaseDC(NULL, DeviceContext);
	return S_OK;
}

KXUSERAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR				Monitor,
	OUT	PDEVICE_SCALE_FACTOR	ScaleFactor)
{
	HDC DeviceContext;
	ULONG LogPixelsX;

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		*ScaleFactor = SCALE_100_PERCENT;
		return S_OK;
	}

	LogPixelsX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
	ReleaseDC(NULL, DeviceContext);

	*ScaleFactor = (DEVICE_SCALE_FACTOR) (9600 / LogPixelsX);
	return S_OK;
}

KXUSERAPI UINT WINAPI GetDpiForSystem(
	VOID)
{
	HDC DeviceContext;
	ULONG LogPixelsX;

	if (!IsProcessDPIAware()) {
		return 96;
	}

	DeviceContext = GetDC(NULL);
	if (!DeviceContext) {
		return 96;
	}

	LogPixelsX = GetDeviceCaps(DeviceContext, LOGPIXELSX);
	ReleaseDC(NULL, DeviceContext);

	return LogPixelsX;
}

KXUSERAPI UINT WINAPI GetDpiForWindow(
	IN	HWND	Window)
{
	if (!IsWindow(Window)) {
		return 0;
	}

	return GetDpiForSystem();
}

KXUSERAPI BOOL WINAPI AdjustWindowRectExForDpi(
	IN OUT	LPRECT	Rect,
	IN		ULONG	WindowStyle,
	IN		BOOL	HasMenu,
	IN		ULONG	WindowExStyle,
	IN		ULONG	Dpi)
{
	// I'm not sure how to implement this function properly.
	// If it turns out to be important, I'll have to do some testing
	// on a Win10 VM.

	return AdjustWindowRectEx(
		Rect,
		WindowStyle,
		HasMenu,
		WindowExStyle);
}

KXUSERAPI UINT WINAPI GetDpiForShellUIComponent(
	IN	SHELL_UI_COMPONENT	component)
{
	return GetDpiForSystem();
};

KXUSERAPI BOOL WINAPI LogicalToPhysicalPointForPerMonitorDPI(
	IN		HWND	Window,
	IN OUT	PPOINT	Point)
{
	return LogicalToPhysicalPoint(Window, Point);
}

KXUSERAPI BOOL WINAPI PhysicalToLogicalPointForPerMonitorDPI(
	IN		HWND	Window,
	IN OUT	PPOINT	Point)
{
	return PhysicalToLogicalPoint(Window, Point);
}

KXUSERAPI BOOL WINAPI EnableNonClientDpiScaling(
	IN	HWND	Window)
{
	return TRUE;
}