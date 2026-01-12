#include "buildcfg.h"
#include "kxuserp.h"

KXUSERAPI BOOL WINAPI GetPointerDevices(
	IN OUT	UINT32				*DeviceCount,
	OUT		POINTER_DEVICE_INFO	*PointerDevices)
{
	*DeviceCount = 0;
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerType(
	IN	DWORD				PointerId,
	OUT	POINTER_INPUT_TYPE	*PointerType)
{
	*PointerType = PT_MOUSE;
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerInfo(
	IN	DWORD			PointerId,
	OUT	POINTER_INFO	*PointerInfo)
{
	PointerInfo->pointerType = PT_MOUSE;
	PointerInfo->pointerId = PointerId;
	PointerInfo->frameId = 0;
	PointerInfo->pointerFlags = POINTER_FLAG_NONE;
	PointerInfo->sourceDevice = NULL;
	PointerInfo->hwndTarget = NULL;
	GetCursorPos(&PointerInfo->ptPixelLocation);
	GetCursorPos(&PointerInfo->ptHimetricLocation);
	GetCursorPos(&PointerInfo->ptPixelLocationRaw);
	GetCursorPos(&PointerInfo->ptHimetricLocationRaw);
	PointerInfo->dwTime = 0;
	PointerInfo->historyCount = 1;
	PointerInfo->InputData = 0;
	PointerInfo->dwKeyStates = 0;
	PointerInfo->PerformanceCount = 0;
	PointerInfo->ButtonChangeType = POINTER_CHANGE_NONE;

	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerTouchInfo(
	IN	DWORD	PointerId,
	OUT	LPVOID	TouchInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfo(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	DWORD	EntriesCount,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfo(
	IN	DWORD	PointerId,
	OUT	LPVOID	PenInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD	EntriesCount,
	OUT		LPVOID	PenInfo)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI SkipPointerFrameMessages(
	IN	DWORD	PointerId)
{
	return TRUE;
}

KXUSERAPI BOOL WINAPI GetPointerDeviceRects(
	IN	HANDLE	Device,
	OUT	LPRECT	PointerDeviceRect,
	OUT	LPRECT	DisplayRect)
{
	PointerDeviceRect->top = 0;
	PointerDeviceRect->left = 0;
	PointerDeviceRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	PointerDeviceRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	DisplayRect->top = 0;
	DisplayRect->left = 0;
	DisplayRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	DisplayRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	return TRUE;
}

KXUSERAPI BOOL WINAPI EnableMouseInPointer(
	IN	BOOL	Enable)
{
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI RegisterPointerDeviceNotifications(
	IN	HWND	Window,
	IN	BOOL	NotifyRange)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetWindowFeedbackSetting(
	IN		HWND			Window,
	IN		FEEDBACK_TYPE	FeedbackType,
	IN		ULONG			Flags,
	IN OUT	PULONG			ConfigurationSize,
	IN		PCVOID			Configuration OPTIONAL)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}

	if (FeedbackType == 0 ||
		FeedbackType > 12 ||
		ConfigurationSize == NULL ||
		(Flags & ~GWFS_INCLUDE_ANCESTORS) != 0) {

		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI SetWindowFeedbackSetting(
	IN	HWND			Window,
	IN	FEEDBACK_TYPE	FeedbackType,
	IN	ULONG			Flags,
	IN	ULONG			ConfigurationSize,
	IN	PCVOID			Configuration OPTIONAL)
{
	if (!IsWindow(Window)) {
		RtlSetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}

	if (FeedbackType == 0 ||
		FeedbackType > 12 ||
		(ConfigurationSize != 0 && Configuration != NULL) ||
		Flags != 0 ||
		(ConfigurationSize != 0 && ConfigurationSize != 4)) {

		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	// In Windows 8 the feedback settings are stored in a window property
	// (i.e. GetProp/SetProp). The property is an atom named "SysFeedbackSettings".
	// Of course, we won't bother actually doing that, since the window feedback
	// stuff is only relevant for touch screens and pens.

	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}