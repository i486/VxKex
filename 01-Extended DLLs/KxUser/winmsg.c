///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     winmsg.c
//
// Abstract:
//
//     Functions which deal with the extension or modification of window
//     messages, window procedures, etc. This code was added to support some
//     Unity games which call EnableMouseInPointer and require WM_POINTERxxx
//     messages in order to register mouse clicks.
//
//     Important note: GetMessage returns -1 on error, 0 if it receives WM_QUIT,
//     and any other value if it receives a valid message other than WM_QUIT.
//
//     PeekMessage returns 0 if there are no messages, and any other value if
//     it receives a message.
//
// Author:
//
//     vxiiduu (28-Apr-2026)
//
// Revision History:
//
//     vxiiduu              28-Apr-2026  Initial creation.
//     vxiiduu              29-Apr-2026  Enhance KernelUserDispatchWindowMessageCallback
//                                       so that it does not intercept messages
//                                       destined for system-provided controls.
//     vxiiduu              22-May-2026  Hook DefWindowProc to turn unprocessed
//                                       pointer messages back into mouse messages.
//     vxiiduu              02-Jul-2026  Fix mismatch between VxKex behavior and
//                                       real Windows behavior. Fixes Cast n Chill.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxuserp.h"
#include <WindowsX.h>

STATIC BOOLEAN g_WindowMessageInterceptionEnabled = FALSE;
STATIC PFN_DISPATCH g_pfnDWORD = NULL;

#define VXKEX_DEFAULT_POINTER_FLAGS ( \
	POINTER_FLAG_PRIMARY | \
	POINTER_FLAG_CONFIDENCE)

STATIC WORD PointerFlagsFromWParam(
	IN	WPARAM	WParam)
{
	USHORT PointerFlags;

	PointerFlags = 0;

	PointerFlags |= (WParam & MK_LBUTTON)	? POINTER_FLAG_FIRSTBUTTON : 0;
	PointerFlags |= (WParam & MK_RBUTTON)	? POINTER_FLAG_SECONDBUTTON : 0;
	PointerFlags |= (WParam & MK_MBUTTON)	? POINTER_FLAG_THIRDBUTTON : 0;
	PointerFlags |= (WParam & MK_XBUTTON1)	? POINTER_FLAG_FOURTHBUTTON : 0;
	PointerFlags |= (WParam & MK_XBUTTON2)	? POINTER_FLAG_FIFTHBUTTON : 0;
	
	return PointerFlags;
}

//
// This is a bit of a hack.
// Basically when we're transforming mouse messages into pointer messages,
// when a WM_xxxBUTTONUP message gets transformed, we essentially *lose* the
// information about which button was released, as well as whether Ctrl or
// Shift were held down.
//
// In order for us to easily track this information, we'll encode which button
// was pressed/released in the pointer ID (HIWORD(WParam)), as well as the
// state of the modifier keys. We'll encode this as a combination of MK_*
// bitflags.
//
// The app uses the pointer ID for passing to GetPointerType/GetPointerInfo etc.,
// all of which ignore the pointer ID parameter. So it should be fine unless
// some app depends on the values staying the same for example.
//
STATIC WORD PointerIdFromWParamAndWindowMessage(
	IN	WPARAM	WParam,
	IN	UINT	Message)
{
	WORD PointerId;

	PointerId = 0;
	PointerId |= WParam & (MK_CONTROL | MK_SHIFT);

	switch (Message) {
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		PointerId |= MK_LBUTTON;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		PointerId |= MK_RBUTTON;
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		PointerId |= MK_MBUTTON;
		break;
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		switch (HIWORD(WParam)) {
		case XBUTTON1:
			PointerId |= MK_XBUTTON1;
			break;
		case XBUTTON2:
			PointerId |= MK_XBUTTON2;
			break;
		default:
			NOT_REACHED;
		}
	}

	return PointerId;
}

STATIC WPARAM WParamFromPointerFlagsAndPointerId(
	IN	WORD	PointerFlags,
	IN	WORD	PointerId)
{
	WPARAM WParam;

	WParam = 0;

	WParam |= (PointerFlags & POINTER_FLAG_FIRSTBUTTON)		? MK_LBUTTON : 0;
	WParam |= (PointerFlags & POINTER_FLAG_SECONDBUTTON)	? MK_RBUTTON : 0;
	WParam |= (PointerFlags & POINTER_FLAG_THIRDBUTTON)		? MK_MBUTTON : 0;
	WParam |= (PointerFlags & POINTER_FLAG_FOURTHBUTTON)	? MK_XBUTTON1 : 0;
	WParam |= (PointerFlags & POINTER_FLAG_FIFTHBUTTON)		? MK_XBUTTON2 : 0;

	WParam |= PointerId & (MK_CONTROL | MK_SHIFT);

	return WParam;
}

//
// Certain mouse-based window messages give us client coordinates.
// Pointer-based window messages are supposed to use screen coordinates.
// Helper function to convert client to screen coordinates.
//
STATIC LPARAM MouseToPointerCoordinates(
	IN	HWND	Window,
	IN	LPARAM	Coordinates)
{
	POINT Point;
	BOOL Success;

	ASSERT (IsWindow(Window));

	Point.x = GET_X_LPARAM(Coordinates);
	Point.y = GET_Y_LPARAM(Coordinates);
	Success = ClientToScreen(Window, &Point);
	ASSERT (Success);

	return MAKELPARAM(Point.x, Point.y);
}

//
// Do the inverse of MouseToPointerCoordinates.
//
STATIC LPARAM PointerToMouseCoordinates(
	IN	HWND	Window,
	IN	LPARAM	Coordinates)
{
	POINT Point;
	BOOL Success;

	ASSERT (IsWindow(Window));

	Point.x = GET_X_LPARAM(Coordinates);
	Point.y = GET_Y_LPARAM(Coordinates);
	Success = ScreenToClient(Window, &Point);
	ASSERT (Success);

	return MAKELPARAM(Point.x, Point.y);
}

//
// This function is responsible for modifying window messages in place, if it
// is necessary, before they are passed to the application. All window messages
// received by VxKex applications should pass through here.
//
// This function is forbidden from reading any fields of the MSG structure
// other than hwnd, wParam, and lParam. This is because any messages sent from
// the kernel (such as scroll messages from touchpad drivers, for example)
// go through KernelUserDispatchWindowMessageCallback, and that function does
// not populate any other members of the MSG structure.
//
// If this function modifies a window message, it returns TRUE.
// If not, it returns FALSE.
//
STATIC BOOLEAN ProcessWindowMessageInPlace(
	IN OUT	PMSG	Message,
	IN		BOOLEAN	Unicode)
{
	ASSERT (Message != NULL);

	if (g_WindowMessageInterceptionEnabled == FALSE) {
		// Window message interception is not enabled. Do nothing.
		return FALSE;
	}

	//
	// If the application has called EnableMouseInPointer(TRUE), then we need to
	// convert mouse-related window messages into the new WM_POINTERxxx messages
	// in order for them to work. New versions of Unity require this for mouse
	// clicks to be registered.
	//
	if (IsMouseInPointerEnabled()) {
		WORD PointerFlags;
		WORD PointerId;

		PointerFlags = VXKEX_DEFAULT_POINTER_FLAGS;
		PointerId = 0;

		switch (Message->message) {
		case WM_MOUSEACTIVATE: {
			UINT_PTR TopLevelParentWindow;
			WORD HitTest;

			// gather information from original message
			TopLevelParentWindow = Message->wParam;
			HitTest = LOWORD(Message->lParam);

			// modify the message
			Message->message = WM_POINTERACTIVATE;
			Message->lParam = TopLevelParentWindow;
			Message->wParam = MAKEWPARAM(PointerId, HitTest);
			break;
							   }
		case WM_CAPTURECHANGED: {
			Message->message = WM_POINTERCAPTURECHANGED;
			Message->wParam = MAKEWPARAM(PointerId, PointerFlags);
			// lParam does not need to be changed - it's a HWND
			break;
								}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN: {
			PointerFlags |= PointerFlagsFromWParam(Message->wParam);
			PointerFlags |= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
			PointerId = PointerIdFromWParamAndWindowMessage(Message->wParam, Message->message);

			Message->message = WM_POINTERDOWN;
			Message->wParam = MAKEWPARAM(PointerId, PointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
							 }
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP: {
			PointerFlags |= PointerFlagsFromWParam(Message->wParam);
			PointerFlags |= POINTER_FLAG_INRANGE;
			PointerId = PointerIdFromWParamAndWindowMessage(Message->wParam, Message->message);

			Message->message = WM_POINTERUP;
			Message->wParam = MAKEWPARAM(PointerId, PointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
						   }
		case WM_MOUSEMOVE: {
			PointerFlags |= PointerFlagsFromWParam(Message->wParam);

			if (Message->wParam & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON |
								   MK_XBUTTON1 | MK_XBUTTON2)) {

				PointerFlags |= POINTER_FLAG_INCONTACT;
			}

			PointerFlags |= POINTER_FLAG_INRANGE;
			PointerId = LOWORD(Message->wParam);

			Message->message = WM_POINTERUPDATE;
			Message->wParam = MAKEWPARAM(PointerId, PointerFlags);
			Message->lParam = MouseToPointerCoordinates(Message->hwnd, Message->lParam);
			break;
						   }
		case WM_MOUSEWHEEL: {
			Message->message = WM_POINTERWHEEL;

			// wParam does not need to be changed: LOWORD will remain MK_* which we can
			// use to convert back in the DefWindowProc extension, and HIWORD has the
			// same format between the two messages.

			// lParam does not need to be changed because WM_MOUSEWHEEL and
			// WM_POINTERWHEEL both use screen coordinates.
			break;
							}
		case WM_MOUSEHWHEEL: {
			Message->message = WM_POINTERHWHEEL;
			break;
							 }
		default:
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

// See __fnDWORD (sometimes called __fnEMPTY or something like that) in user32
// See comment above EnableWindowMessageInterception
STATIC NTSTATUS NTAPI KernelUserDispatchWindowMessageCallback(
	IN	PVOID	Context OPTIONAL)
{
	PFNDWORDMSG DwordMsg;
	HWND WindowHandle;
	WNDPROC WindowProc;
	MSG Msg;
	BOOLEAN MessageWasModified;

	ASSERT (g_pfnDWORD != NULL);

	DwordMsg = (PFNDWORDMSG) Context;

	if (DwordMsg == NULL || DwordMsg->pwnd == NULL) {
		//
		// Some messages, such as WM_TIMER, have pwnd set to NULL.
		// We don't care about these messages, and pwnd being NULL is annoying to
		// deal with in our later code so we will just bail out.
		//

		goto CallOriginalProc;
	}

	//
	// We are only provided a PWND. We need to convert this to a HWND.
	//

	WindowHandle = PWND_TO_HWND(DwordMsg->pwnd);
	ASSERT (WindowHandle != NULL);

	if (WindowHandle == NULL) {
		goto CallOriginalProc;
	}

	//
	// Place all the necessary information into a temporary MSG structure
	// so that we can call ProcessWindowMessageInPlace on it.
	//

	RtlZeroMemory(&Msg, sizeof(Msg));

	Msg.hwnd			= WindowHandle;
	Msg.message			= DwordMsg->msg;
	Msg.wParam			= DwordMsg->wParam;
	Msg.lParam			= DwordMsg->lParam;

	//
	// Process the window message.
	//

	MessageWasModified = ProcessWindowMessageInPlace(&Msg, TRUE);

	if (MessageWasModified) {
		//
		// Get the address of the window's window procedure.
		// We want to check which module this window procedure is in.
		// If it's in a Windows DLL (e.g. user32.dll or comctl32.dll), then we
		// do not want to mess with this window message.
		//
		// This check is done after ProcessWindowMessageInPlace because the call
		// to AshModuleIsWindowsModule is fairly expensive, since it involves
		// a loader call.
		//
	
		WindowProc = (WNDPROC) GetWindowLongPtr(WindowHandle, GWLP_WNDPROC);
		ASSERT (WindowProc != NULL);

		if (WindowProc == NULL) {
			goto CallOriginalProc;
		}

		if (AshModuleIsWindowsModule(WindowProc)) {
			//
			// The window procedure for this window is inside a Windows DLL, which
			// probably means that this window is a common control or common dialog.
			// We don't want to interfere with messages for such windows, so we will
			// bail out.
			//

			goto CallOriginalProc;
		}

		//
		// The window message was destined for an application-created window procedure.
		// We will proceed to copy all the necessary data from the MSG structure back
		// into the FNDWORDMSG structure.
		//

		DwordMsg->msg		= Msg.message;
		DwordMsg->wParam	= Msg.wParam;
		DwordMsg->lParam	= Msg.lParam;
	}

CallOriginalProc:
	return g_pfnDWORD(Context);
}

//
// Hooks on GetMessage and PeekMessage work for most messages queued in a
// message queue, but some, such as scroll messages from certain touchpad drivers,
// are sent directly from the kernel and therefore GetMessage/PeekMessage hooks
// do not work for intercepting those.
//
// Such messages pass through a function called fnDWORD in user32, the address of
// which is stored in the Peb->KernelCallbackTable. Overwriting this function
// pointer will allow us to hook the function and run our window message handling
// code.
//
// Window message interception is disabled by default until this function is called.
// This allows us to avoid performance and compatibility issues with hooking all
// window messages. Bugs in KernelUserDispatchWindowMessageCallback can cause
// confusing error messages, and it's best not to risk it for the 99% of
// applications that do not need this compatibility fix.
//
NTSTATUS EnableWindowMessageInterception(
	VOID)
{
	STATIC RTL_SRWLOCK Lock = RTL_SRWLOCK_INIT;
	NTSTATUS Status;
	PFN_DISPATCH *KernelCallbackTable;
	PVOID BaseAddress;
	SIZE_T RegionSize;
	ULONG OldProtect;

	if (g_WindowMessageInterceptionEnabled) {
		return STATUS_ALREADY_INITIALIZED;
	}

	KernelCallbackTable = NtCurrentPeb()->KernelCallbackTable;
	ASSERT (KernelCallbackTable != NULL);
	
	if (KernelCallbackTable == NULL) {
		KexLogErrorEvent(L"Failed to initialize window message interception because KernelCallbackTable is NULL");
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Get the original fnDWORD pointer
	//

	ASSERT (g_pfnDWORD == NULL);
	g_pfnDWORD = KernelCallbackTable[2];
	ASSERT (g_pfnDWORD != NULL);

	if (g_pfnDWORD == NULL) {
		KexLogErrorEvent(L"Failed to initialize window message interception because the pointer to fnDWORD is NULL");
		return STATUS_UNSUCCESSFUL;
	}

	//
	// Replace it with our hook. The callback table is usually read-only so we
	// need to set the memory protection.
	//

	BaseAddress = &KernelCallbackTable[2];
	RegionSize = sizeof(KernelCallbackTable[2]);

	RtlAcquireSRWLockExclusive(&Lock);

	try {
		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			PAGE_READWRITE,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"Failed to initialize window message interception.\r\n\r\n"
				L"NTSTATUS error code: %s (0x%08lx)",
				KexRtlNtStatusToString(Status), Status);

			return Status;
		}

		KernelCallbackTable[2] = KernelUserDispatchWindowMessageCallback;

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));
	} finally {
		RtlReleaseSRWLockExclusive(&Lock);
	}

	KexLogInformationEvent(L"Window message interception enabled");

	g_WindowMessageInterceptionEnabled = TRUE;
	return STATUS_SUCCESS;
}

KXUSERAPI BOOL WINAPI Ext_GetMessageA(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax)
{
	INT ReturnValue;

	ReturnValue = GetMessageA(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax);

	if (ReturnValue != -1) {
		ProcessWindowMessageInPlace(Message, FALSE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_GetMessageW(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax)
{
	INT ReturnValue;

	ReturnValue = GetMessageW(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax);

	if (ReturnValue != -1) {
		ProcessWindowMessageInPlace(Message, TRUE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_PeekMessageA(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax,
	IN	UINT	MessageRemoveFlags)
{
	INT ReturnValue;

	ReturnValue = PeekMessageA(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax,
		MessageRemoveFlags);
	
	if (ReturnValue != 0) {
		ProcessWindowMessageInPlace(Message, FALSE);
	}

	return ReturnValue;
}

KXUSERAPI BOOL WINAPI Ext_PeekMessageW(
	OUT	PMSG	Message,
	IN	HWND	Window OPTIONAL,
	IN	UINT	MessageFilterMin,
	IN	UINT	MessageFilterMax,
	IN	UINT	MessageRemoveFlags)
{
	INT ReturnValue;

	ReturnValue = PeekMessageW(
		Message,
		Window,
		MessageFilterMin,
		MessageFilterMax,
		MessageRemoveFlags);
	
	if (ReturnValue != 0) {
		ProcessWindowMessageInPlace(Message, TRUE);
	}

	return ReturnValue;
}

//
// DefWindowProc on Windows 8 or above changes WM_POINTERxxx messages
// back into their equivalent mouse messages.
//
// Applications which call EnableMouseInPointer but then don't handle
// one or more WM_POINTERxxx messages depend on this behavior.
//
// The game "Haste" (Unity) requires this.
//
// https://devblogs.microsoft.com/oldnewthing/20210728-00/?p=105487
//
STATIC LRESULT KxUserDefWindowProcAorW(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam,
	IN	BOOLEAN	Unicode)
{
	MSG Msg;

	if (g_WindowMessageInterceptionEnabled == FALSE) {
		// Window message interception is not enabled. Do nothing.
		goto CallOriginalDefWindowProc;
	}

	KexRtlZeroMemory(&Msg, sizeof(Msg));
	Msg.hwnd = Window;
	Msg.message = Message;
	Msg.wParam = WParam;
	Msg.lParam = LParam;

	switch (Message) {
	case WM_POINTERACTIVATE: {
		Msg.message = WM_MOUSEACTIVATE;
		Msg.wParam = (WPARAM) LParam;
		Msg.lParam = MAKELPARAM(HIWORD(WParam), 0);
		break;
							 }
	case WM_POINTERCAPTURECHANGED: {
		Msg.message = WM_CAPTURECHANGED;
		Msg.wParam = 0;
		// leave lParam unchanged - same in both messages
		break;
								   }
	case WM_POINTERDOWN:
	case WM_POINTERUP: {
		WORD PointerFlags;
		WORD PointerId;

		PointerFlags = HIWORD(WParam);
		PointerId = LOWORD(WParam);

		Msg.wParam = WParamFromPointerFlagsAndPointerId(PointerFlags, PointerId);
		Msg.lParam = PointerToMouseCoordinates(Window, LParam);

		if (PointerId & MK_LBUTTON) {
			Msg.message = WM_LBUTTONDOWN;
		} else if (PointerId & MK_RBUTTON) {
			Msg.message = WM_RBUTTONDOWN;
		} else if (PointerId & MK_MBUTTON) {
			Msg.message = WM_MBUTTONDOWN;
		} else if (PointerId & MK_XBUTTON1) {
			Msg.message = WM_XBUTTONDOWN;
			Msg.wParam = MAKEWPARAM(Msg.wParam, XBUTTON1);
		} else if (PointerId & MK_XBUTTON2) {
			Msg.message = WM_XBUTTONDOWN;
			Msg.wParam = MAKEWPARAM(Msg.wParam, XBUTTON2);
		} else {
			NOT_REACHED;
		}

		if (Message == WM_POINTERUP) {
			// adding 1 turns a WM_xxBUTTONDOWN into a WM_xxBUTTONUP message
			++Msg.message;
		}

		break;
					   }
	case WM_POINTERUPDATE: {
		WORD PointerFlags;
		WORD PointerId;

		PointerFlags = HIWORD(WParam);
		PointerId = LOWORD(WParam);

		Msg.message = WM_MOUSEMOVE;
		Msg.wParam = WParamFromPointerFlagsAndPointerId(PointerFlags, PointerId);
		Msg.lParam = PointerToMouseCoordinates(Window, LParam);
		break;
						   }
	case WM_POINTERWHEEL: {
		Msg.message = WM_MOUSEWHEEL;
		// No need to change wParam or lParam
		break;
						  }
	case WM_POINTERHWHEEL: {
		Msg.message = WM_MOUSEHWHEEL;
		break;
						   }
	default:
		goto CallOriginalDefWindowProc;
	}

	//
	// TODO: Technically, we should be using PostMessage here. However, that
	// causes an infinite loop because we're posting a mouse message, then
	// convert it to a pointer message, it gets passed to DefWindowProc (us)
	// and then convert it to a mouse message over and over again.
	//

	if (Unicode) {
		SendMessageW(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
	} else {
		SendMessageA(Msg.hwnd, Msg.message, Msg.wParam, Msg.lParam);
	}

	return 0;

CallOriginalDefWindowProc:
	if (Unicode) {
		return DefWindowProcW(Window, Message, WParam, LParam);
	} else {
		return DefWindowProcA(Window, Message, WParam, LParam);
	}
}

KXUSERAPI LRESULT WINAPI Ext_DefWindowProcA(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	return KxUserDefWindowProcAorW(Window, Message, WParam, LParam, FALSE);
}

KXUSERAPI LRESULT WINAPI Ext_DefWindowProcW(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	return KxUserDefWindowProcAorW(Window, Message, WParam, LParam, TRUE);
}