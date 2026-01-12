#include "buildcfg.h"
#include "kxuserp.h"

//
// Coalescable timers are a power-saving feature. They reduce the timer precision
// in exchange for greater energy efficiency, since timers can be "coalesced" and
// batches of timers can be handled at once. This also means that we can just proxy
// it directly to SetTimer() without causing any big problems.
//
// As an aside, on Windows 8, the uToleranceDelay parameter was added to the win32k
// function NtUserSetTimer, and SetCoalescableTimer is directly forwarded to
// NtUserSetTimer. The ordinary SetTimer is just a stub that calls NtUserSetTimer
// with the uToleranceDelay parameter set to zero.
//
// Windows 7 supports coalescable timers with SetWaitableTimerEx, but it is not
// simple to integrate that functionality with the User timers, so we don't bother.
//

KXUSERAPI UINT_PTR WINAPI SetCoalescableTimer(
	IN	HWND		hwnd,
	IN	UINT_PTR	nIdEvent,
	IN	UINT		uElapse,
	IN	TIMERPROC	lpTimerFunc,
	IN	ULONG		uToleranceDelay)
{
	return SetTimer(hwnd, nIdEvent, uElapse, lpTimerFunc);
}