#include "buildcfg.h"
#include "kxuserp.h"

STATIC BOOLEAN g_MouseInPointerEnabled = FALSE;

KXUSERAPI BOOL WINAPI GetPointerDevices(
	IN OUT	PULONG					DeviceCount,
	OUT		PPOINTER_DEVICE_INFO	PointerDevices OPTIONAL)
{
	if (DeviceCount == NULL) {
		RtlSetLastWin32Error(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	//
	// I don't think mice are returned by this function, considering there
	// is no POINTER_DEVICE_TYPE enum value for mice.
	//

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
	PointerInfo->pointerFlags = POINTER_FLAG_PRIMARY | POINTER_FLAG_CONFIDENCE;
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
	IN	ULONG				PointerId,
	OUT	PPOINTER_TOUCH_INFO	TouchInfo)
{
	KexDebugCheckpoint();
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

//
// Added based on a user report that certain Unity 6 games require it.
// https://github.com/i486/VxKex/issues/279#issuecomment-4778250083
//
// According to MSDN docs, this function is only applicable for PT_TOUCH
// pointer types, which we don't support. Apps shouldn't ever call this
// function.
//
KXUSERAPI BOOL WINAPI GetPointerTouchInfoHistory(
	IN		ULONG				PointerId,
	IN OUT	ULONG				NumberOfTouchInfo,
	OUT		PPOINTER_TOUCH_INFO	TouchInfo)
{
	KexDebugCheckpoint();
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfo(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	KexDebugCheckpoint();
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerFrameTouchInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	DWORD	EntriesCount,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	KexDebugCheckpoint();
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfo(
	IN	DWORD	PointerId,
	OUT	LPVOID	PenInfo)
{
	KexDebugCheckpoint();
	RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
	return FALSE;
}

KXUSERAPI BOOL WINAPI GetPointerPenInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD	EntriesCount,
	OUT		LPVOID	PenInfo)
{
	KexDebugCheckpoint();
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

KXUSERAPI BOOL WINAPI IsMouseInPointerEnabled(
	VOID)
{
	return g_MouseInPointerEnabled;
}

KXUSERAPI BOOL WINAPI EnableMouseInPointer(
	IN	BOOL	Enabled)
{
	STATIC BOOLEAN AlreadyCalled = FALSE;

	// normalize boolean into the range (0,1)
	Enabled = !!Enabled;

	if (AlreadyCalled) {
		if (Enabled == g_MouseInPointerEnabled) {
			return TRUE;
		}

		RtlSetLastWin32Error(ERROR_NOT_SUPPORTED);
		return FALSE;
	}

	AlreadyCalled = TRUE;
	g_MouseInPointerEnabled = Enabled;

	if (Enabled) {
		// Mouse-in-pointer mode requires window message interception to work.
		EnableWindowMessageInterception();
	}

	return TRUE;
}