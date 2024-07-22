#include "buildcfg.h"
#include "kxuserp.h"

BOOL WINAPI GetPointerDevices(
	IN OUT	UINT32				*DeviceCount,
	OUT		POINTER_DEVICE_INFO	*PointerDevices)
{
	*DeviceCount = 0;
	return TRUE;
}

BOOL WINAPI GetPointerType(
	IN	DWORD				PointerId,
	OUT	POINTER_INPUT_TYPE	*PointerType)
{
	*PointerType = PT_MOUSE;
	return TRUE;
}

BOOL WINAPI GetPointerInfo(
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

BOOL WINAPI GetPointerTouchInfo(
	IN	DWORD	PointerId,
	OUT	LPVOID	TouchInfo)
{
	return FALSE;
}

BOOL WINAPI GetPointerFrameTouchInfo(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	return FALSE;
}

BOOL WINAPI GetPointerFrameTouchInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	DWORD	EntriesCount,
	IN OUT	LPDWORD PointerCount,
	OUT		LPVOID	TouchInfo)
{
	return FALSE;
}

BOOL WINAPI GetPointerPenInfo(
	IN	DWORD	PointerId,
	OUT	LPVOID	PenInfo)
{
	return FALSE;
}

BOOL WINAPI GetPointerPenInfoHistory(
	IN		DWORD	PointerId,
	IN OUT	LPDWORD	EntriesCount,
	OUT		LPVOID	PenInfo)
{
	return FALSE;
}

BOOL WINAPI SkipPointerFrameMessages(
	IN	DWORD	PointerId)
{
	return TRUE;
}

BOOL WINAPI GetPointerDeviceRects(
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

BOOL WINAPI EnableMouseInPointer(
	IN	BOOL	Enable)
{
	return FALSE;
}