#include <Windows.h>
#include <Shlwapi.h>
#include <KexDll.h>
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);

WINUSERAPI BOOL WINAPI AdjustWindowRectExForDpi(
	IN OUT	LPRECT	lpRect,
	IN		DWORD	dwStyle,
	IN		BOOL	bMenu,
	IN		DWORD	dwExStyle,
	IN		UINT	dpi)
{
	return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
}

WINUSERAPI INT WINAPI GetSystemMetricsForDpi(
	IN	INT		nIndex,
	IN	UINT	dpi)
{
	return GetSystemMetrics(nIndex);
}

#define DPI_AWARENESS_CONTEXT_UNAWARE				((DPI_AWARENESS_CONTEXT) -1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE			((DPI_AWARENESS_CONTEXT) -2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE		((DPI_AWARENESS_CONTEXT) -3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2	((DPI_AWARENESS_CONTEXT) -4)
#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED		((DPI_AWARENESS_CONTEXT) -5)

typedef enum _DPI_AWARENESS {
	DPI_AWARENESS_INVALID			= -1,
	DPI_AWARENESS_UNAWARE			= 0,
	DPI_AWARENESS_SYSTEM_AWARE		= 1,
	DPI_AWARENESS_PER_MONITOR_AWARE	= 2
} DPI_AWARENESS;

WINUSERAPI BOOL WINAPI SetProcessDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	value)
{
	if (value == DPI_AWARENESS_CONTEXT_SYSTEM_AWARE) {
		return SetProcessDPIAware();
	}

	return TRUE;
}

WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI SetThreadDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	value)
{
	return value;
}

WINUSERAPI UINT WINAPI GetDpiForSystem(
	VOID)
{
	HDC hdcScreen = GetDC(NULL);
	UINT uDpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
	ReleaseDC(NULL, hdcScreen);
	return uDpi;
}

WINUSERAPI UINT WINAPI GetDpiForWindow(
	IN	HWND	hWnd)
{
	HDC hdcWindow = GetDC(hWnd);
	UINT uDpi = GetDeviceCaps(hdcWindow, LOGPIXELSX);
	ReleaseDC(hWnd, hdcWindow);
	return uDpi;
}

WINUSERAPI DPI_AWARENESS_CONTEXT WINAPI GetWindowDpiAwarenessContext(
	IN	HWND	hWnd)
{
	if (GetDpiForSystem() == 96 && GetDpiForWindow(hWnd) != 96) {
		return DPI_AWARENESS_CONTEXT_SYSTEM_AWARE;
	} else {
		return DPI_AWARENESS_CONTEXT_UNAWARE;
	}
}

WINUSERAPI DPI_AWARENESS WINAPI GetAwarenessFromDpiAwarenessContext(
	IN	DPI_AWARENESS_CONTEXT	value)
{
	switch ((QWORD) value) {
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

WINUSERAPI BOOL WINAPI SystemParametersInfoForDpi(
	IN		UINT	uiAction,
	IN		UINT	uiParam,
	IN OUT	LPVOID	lpParam,
	IN		UINT	fWinIni,
	IN		UINT	dpi)
{
	return SystemParametersInfo(uiAction, uiParam, lpParam, fWinIni);
}

// Coalescable timers are a power-saving feature. They reduce the timer precision
// in exchange for greater energy efficiency, since timers can be "coalesced" and
// batches of timers can be handled at once. This also means that we can just proxy
// it directly to SetTimer() without causing any big problems.
WINUSERAPI UINT_PTR WINAPI SetCoalescableTimer(
	IN	HWND		hwnd,
	IN	UINT_PTR	nIdEvent,
	IN	UINT		uElapse,
	IN	TIMERPROC	lpTimerFunc,
	IN	ULONG		uToleranceDelay)
{
	return SetTimer(hwnd, nIdEvent, uElapse, lpTimerFunc);
}

WINUSERAPI BOOL WINAPI EnableNonClientDpiScaling(
	IN	HWND	hWnd)
{
	return TRUE;
}

WINUSERAPI BOOL WINAPI EnableMouseInPointer(
	IN	BOOL	bEnable)
{
	return TRUE;
}

typedef enum _ORIENTATION_PREFERENCE {
	ORIENTATION_PREFERENCE_NONE,
	ORIENTATION_PREFERENCE_LANDSCAPE,
	ORIENTATION_PREFERENCE_PORTRAIT,
	ORIENTATION_PREFERENCE_LANDSCAPE_FLIPPED,
	ORIENTATION_PREFERENCE_PORTRAIT_FLIPPED
} ORIENTATION_PREFERENCE;

WINUSERAPI BOOL WINAPI GetDisplayAutoRotationPreferences(
	OUT	ORIENTATION_PREFERENCE	*pOrientation)
{
	*pOrientation = ORIENTATION_PREFERENCE_NONE;
	return TRUE;
}

WINUSERAPI BOOL WINAPI SetDisplayAutoRotationPreferences(
	IN	ORIENTATION_PREFERENCE	orientation)
{
	return TRUE;
}

typedef enum _POINTER_INPUT_TYPE {
	PT_POINTER,
	PT_TOUCH,
	PT_PEN,
	PT_MOUSE,
	PT_TOUCHPAD
} POINTER_INPUT_TYPE;

typedef enum _POINTER_BUTTON_CHANGE_TYPE {
	POINTER_CHANGE_NONE,
	POINTER_CHANGE_FIRSTBUTTON_DOWN,
	POINTER_CHANGE_FIRSTBUTTON_UP,
	POINTER_CHANGE_SECONDBUTTON_DOWN,
	POINTER_CHANGE_SECONDBUTTON_UP,
	POINTER_CHANGE_THIRDBUTTON_DOWN,
	POINTER_CHANGE_THIRDBUTTON_UP,
	POINTER_CHANGE_FOURTHBUTTON_DOWN,
	POINTER_CHANGE_FOURTHBUTTON_UP,
	POINTER_CHANGE_FIFTHBUTTON_DOWN,
	POINTER_CHANGE_FIFTHBUTTON_UP
} POINTER_BUTTON_CHANGE_TYPE;

typedef UINT32 POINTER_FLAGS;
#define POINTER_FLAG_NONE               0x00000000 // Default
#define POINTER_FLAG_NEW                0x00000001 // New pointer
#define POINTER_FLAG_INRANGE            0x00000002 // Pointer has not departed
#define POINTER_FLAG_INCONTACT          0x00000004 // Pointer is in contact
#define POINTER_FLAG_FIRSTBUTTON        0x00000010 // Primary action
#define POINTER_FLAG_SECONDBUTTON       0x00000020 // Secondary action
#define POINTER_FLAG_THIRDBUTTON        0x00000040 // Third button
#define POINTER_FLAG_FOURTHBUTTON       0x00000080 // Fourth button
#define POINTER_FLAG_FIFTHBUTTON        0x00000100 // Fifth button
#define POINTER_FLAG_PRIMARY            0x00002000 // Pointer is primary for system
#define POINTER_FLAG_CONFIDENCE         0x00004000 // Pointer is considered unlikely to be accidental
#define POINTER_FLAG_CANCELED           0x00008000 // Pointer is departing in an abnormal manner
#define POINTER_FLAG_DOWN               0x00010000 // Pointer transitioned to down state (made contact)
#define POINTER_FLAG_UPDATE             0x00020000 // Pointer update
#define POINTER_FLAG_UP                 0x00040000 // Pointer transitioned from down state (broke contact)
#define POINTER_FLAG_WHEEL              0x00080000 // Vertical wheel
#define POINTER_FLAG_HWHEEL             0x00100000 // Horizontal wheel
#define POINTER_FLAG_CAPTURECHANGED     0x00200000 // Lost capture
#define POINTER_FLAG_HASTRANSFORM       0x00400000 // Input has a transform associated with it

typedef struct _POINTER_INFO {
	POINTER_INPUT_TYPE         pointerType;
	UINT32                     pointerId;
	UINT32                     frameId;
	POINTER_FLAGS              pointerFlags;
	HANDLE                     sourceDevice;
	HWND                       hwndTarget;
	POINT                      ptPixelLocation;
	POINT                      ptHimetricLocation;
	POINT                      ptPixelLocationRaw;
	POINT                      ptHimetricLocationRaw;
	DWORD                      dwTime;
	UINT32                     historyCount;
	INT32                      InputData;
	DWORD                      dwKeyStates;
	UINT64                     PerformanceCount;
	POINTER_BUTTON_CHANGE_TYPE ButtonChangeType;
} POINTER_INFO;

#define POINTER_DEVICE_PRODUCT_STRING_MAX 520

typedef enum tagPOINTER_DEVICE_TYPE {
	POINTER_DEVICE_TYPE_INTEGRATED_PEN = 0x00000001,
	POINTER_DEVICE_TYPE_EXTERNAL_PEN = 0x00000002,
	POINTER_DEVICE_TYPE_TOUCH = 0x00000003,
	POINTER_DEVICE_TYPE_TOUCH_PAD = 0x00000004,
	POINTER_DEVICE_TYPE_MAX = 0xFFFFFFFF
} POINTER_DEVICE_TYPE;

typedef struct tagPOINTER_DEVICE_INFO {
	DWORD               displayOrientation;
	HANDLE              device;
	POINTER_DEVICE_TYPE pointerDeviceType;
	HMONITOR            monitor;
	ULONG               startingCursorId;
	USHORT              maxActiveContacts;
	WCHAR               productString[POINTER_DEVICE_PRODUCT_STRING_MAX];
} POINTER_DEVICE_INFO;

WINUSERAPI BOOL WINAPI GetPointerDevices(
	IN OUT	UINT32				*deviceCount,
	OUT		POINTER_DEVICE_INFO	*pointerDevices)
{
	*deviceCount = 0;
	return TRUE;
}

WINUSERAPI BOOL WINAPI GetPointerType(
	IN	DWORD				dwPointerId,
	OUT	POINTER_INPUT_TYPE	*pointerType)
{
	*pointerType = PT_MOUSE;
	return TRUE;
}

WINUSERAPI BOOL WINAPI GetPointerInfo(
	IN	DWORD			dwPointerId,
	OUT	POINTER_INFO	*pointerInfo)
{
	pointerInfo->pointerType = PT_MOUSE;
	pointerInfo->pointerId = dwPointerId;
	pointerInfo->frameId = 0;
	pointerInfo->pointerFlags = POINTER_FLAG_NONE;
	pointerInfo->sourceDevice = NULL;
	pointerInfo->hwndTarget = NULL;
	GetCursorPos(&pointerInfo->ptPixelLocation);
	GetCursorPos(&pointerInfo->ptHimetricLocation);
	GetCursorPos(&pointerInfo->ptPixelLocationRaw);
	GetCursorPos(&pointerInfo->ptHimetricLocationRaw);
	pointerInfo->dwTime = 0;
	pointerInfo->historyCount = 1;
	pointerInfo->InputData = 0;
	pointerInfo->dwKeyStates = 0;
	pointerInfo->PerformanceCount = 0;
	pointerInfo->ButtonChangeType = POINTER_CHANGE_NONE;

	return TRUE;
}

WINUSERAPI BOOL WINAPI GetPointerDeviceRects(
	IN	HANDLE device,
	OUT	LPRECT	lprcPointerDeviceRect,
	OUT	LPRECT	lprcDisplayRect)
{
	lprcPointerDeviceRect->top = 0;
	lprcPointerDeviceRect->left = 0;
	lprcPointerDeviceRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	lprcPointerDeviceRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	lprcDisplayRect->top = 0;
	lprcDisplayRect->left = 0;
	lprcDisplayRect->bottom = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	lprcDisplayRect->right = GetSystemMetrics(SM_CXVIRTUALSCREEN);

	return TRUE;
}

WINUSERAPI BOOL WINAPI GetPointerTouchInfo(
	IN	DWORD	dwPointerId,
	OUT	LPVOID	lpTouchInfo)
{
	return FALSE;
}

WINUSERAPI BOOL WINAPI GetPointerFrameTouchInfo(
	IN		DWORD	dwPointerId,
	IN OUT	LPDWORD lpdwPointerCount,
	OUT		LPVOID	lpTouchInfo)
{
	return FALSE;
}

WINUSERAPI BOOL WINAPI GetPointerFrameTouchInfoHistory(
	IN		DWORD	dwPointerId,
	IN OUT	DWORD	dwEntriesCount,
	IN OUT	LPDWORD lpdwPointerCount,
	OUT		LPVOID	lpTouchInfo)
{
	return FALSE;
}

WINUSERAPI BOOL WINAPI GetPointerPenInfo(
	IN	DWORD	dwPointerId,
	OUT	LPVOID	lpPenInfo)
{
	return FALSE;
}

WINUSERAPI BOOL WINAPI GetPointerPenInfoHistory(
	IN		DWORD	dwPointerId,
	IN OUT	LPDWORD	lpdwEntriesCount,
	OUT		LPVOID	lpPenInfo)
{
	return FALSE;
}

WINUSERAPI BOOL WINAPI SkipPointerFrameMessages(
	IN	DWORD	dwPointerId)
{
	return TRUE;
}

WINUSERAPI BOOL WINAPI LogicalToPhysicalPointForPerMonitorDPI(
	IN		HWND	hWnd,
	IN OUT	LPPOINT	lpPoint)
{
	return LogicalToPhysicalPoint(hWnd, lpPoint);
}

WINUSERAPI BOOL WINAPI PhysicalToLogicalPointForPerMonitorDPI(
	IN		HWND	hWnd,
	IN OUT	LPPOINT	lpPoint)
{
	return PhysicalToLogicalPoint(hWnd, lpPoint);
}

WINUSERAPI HHOOK WINAPI PROXY_FUNCTION(SetWindowsHookExW) (
	IN	INT			idHook,
	IN	HOOKPROC	lpfn,
	IN	HINSTANCE	hMod,
	IN	DWORD		dwThreadId)
{
	STATIC BOOL bDisableHooks = -1;

	// APPSPECIFICHACK
	if (bDisableHooks == -1) {
		WCHAR szAppExe[MAX_PATH];
		wcscpy_s(szAppExe, ARRAYSIZE(szAppExe), NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);
		PathStripPath(szAppExe);
		bDisableHooks = !wcsicmp(szAppExe, L"KEYMOUSE.EXE");
	}

	if (bDisableHooks) {
		return NULL;
	} else {
		return SetWindowsHookExW(idHook, lpfn, hMod, dwThreadId);
	}
}