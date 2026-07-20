#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI IsValidDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	return FALSE;
}

KXUSERAPI BOOL WINAPI AreDpiAwarenessContextsEqual(
	IN	DPI_AWARENESS_CONTEXT	Value1,
	IN	DPI_AWARENESS_CONTEXT	Value2)
{
	return (IsValidDpiAwarenessContext(Value1) &&
			IsValidDpiAwarenessContext(Value2) &&
			((Value1 ^ Value2) & INT_MAX) == 0);
}

KXUSERAPI DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	Value)
{
	return DPI_AWARENESS_INVALID;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetDpiAwarenessContextForProcess(
	IN	HANDLE					ProcessHandle)
{
	if (ProcessHandle == NULL ||
		ProcessHandle == NtCurrentProcess() ||
		GetProcessId(ProcessHandle) == (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {

		if (IsProcessDPIAware()) {
			return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
		}
	}

	return DPI_AWARENESS_CONTEXT_UNAWARE;
}

KXUSERAPI BOOL WINAPI SetProcessDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	SetLastError(ERROR_ACCESS_DENIED);
	return FALSE;
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetThreadDpiAwarenessContext(
	VOID)
{
	return GetDpiAwarenessContextForProcess(NULL);
}

KXUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	DpiContext)
{
	return 0;
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

	if (WindowProcessId == (ULONG) NtCurrentTeb()->ClientId.UniqueProcess) {
		return GetDpiAwarenessContextForProcess(NULL);
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

KXUSERAPI UINT WINAPI GetDpiForSystem(
	VOID)
{
	STATIC ULONG SystemDpi = 0;

	if (!IsProcessDPIAware()) {
		return USER_DEFAULT_SCREEN_DPI;
	}

	if (SystemDpi == 0) {
		HDC DeviceContext;

		DeviceContext = GetDC(NULL);
		if (!DeviceContext) {
			return USER_DEFAULT_SCREEN_DPI;
		}

		SystemDpi = GetDeviceCaps(DeviceContext, LOGPIXELSX);
		ReleaseDC(NULL, DeviceContext);
	}

	return SystemDpi;
}

KXUSERAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR			Monitor,
	IN	MONITOR_DPI_TYPE	DpiType,
	OUT	PULONG				DpiX,
	OUT	PULONG				DpiY)
{
	if (DpiType >= MDT_MAXIMUM_DPI) {
		return E_INVALIDARG;
	}

	if (!DpiX || !DpiY) {
		return E_INVALIDARG;
	}

	//
	// APPSPECIFICHACK: Java applications using the "awt.dll" framework do not
	// scale properly on high DPI displays. I couldn't find out how to fix this
	// properly, so just pretend the screen is 96DPI. It's usable on 120DPI
	// monitors but unfortunately anything higher and text starts getting too small.
	//

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (AshModuleBaseNameIs(ReturnAddress(), L"awt.dll")) {
			*DpiX = USER_DEFAULT_SCREEN_DPI;
			*DpiY = USER_DEFAULT_SCREEN_DPI;
			return S_OK;
		}
	}

	*DpiX = GetDpiForSystem();
	*DpiY = *DpiX;

	return S_OK;
}

KXUSERAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR				Monitor,
	OUT	PDEVICE_SCALE_FACTOR	ScaleFactor)
{
	*ScaleFactor = (DEVICE_SCALE_FACTOR) (9600 / GetDpiForSystem());
	return S_OK;
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