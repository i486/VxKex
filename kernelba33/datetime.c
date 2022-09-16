#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

WINBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(
	OUT	LPFILETIME	lpSystemTimeAsFileTime)
{
	ODS_ENTRY();
	GetSystemTimeAsFileTime(lpSystemTimeAsFileTime);
}