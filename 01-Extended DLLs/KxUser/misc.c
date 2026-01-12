#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI GetProcessUIContextInformation(
	IN	HANDLE							ProcessHandle,
	OUT	PPROCESS_UICONTEXT_INFORMATION	UIContextInformation)
{
	if (!UIContextInformation) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	UIContextInformation->UIContext = PROCESS_UICONTEXT_DESKTOP;
	UIContextInformation->Flags		= PROCESS_UIF_NONE;

	return TRUE;
}

KXUSERAPI HWND WINAPI CreateWindowInBand(
	IN	DWORD			dwExStyle,
	IN	PCWSTR			lpClassName,
	IN	PCWSTR			lpWindowName,
	IN	DWORD			dwStyle,
	IN	INT				X,
	IN	INT				Y,
	IN	INT				nWidth,
	IN	INT				nHeight,
	IN	HWND			hWndParent,
	IN	HMENU			hMenu,
	IN	HINSTANCE		hInstance,
	IN	PVOID			lpParam,
	IN	ZBID			zbid)
{
	return CreateWindowExW(
		dwExStyle,
		lpClassName,
		lpWindowName,
		dwStyle,
		X,
		Y,
		nWidth,
		nHeight,
		hWndParent,
		hMenu,
		hInstance,
		lpParam);
}

KXUSERAPI BOOL WINAPI GetWindowBand(
	IN	HWND	Window,
	OUT	PZBID	Band)
{
	if (!IsWindow(Window)) {
		SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}

	if (!Band) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	if (Window == GetDesktopWindow()) {
		*Band = ZBID_DESKTOP;
	} else {
		*Band = ZBID_DEFAULT;
	}

	return TRUE;
}

KXUSERAPI BOOL WINAPI GetCurrentInputMessageSource(
	OUT	PINPUT_MESSAGE_SOURCE	MessageSource)
{
	if (!MessageSource) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	MessageSource->DeviceType = IMDT_UNAVAILABLE;
	MessageSource->OriginId = IMO_UNAVAILABLE;
	return TRUE;
}

KXUSERAPI BOOL WINAPI IsImmersiveProcess(
	IN	HANDLE	ProcessHandle)
{
	SetLastError(ERROR_SUCCESS);
	return FALSE;
}