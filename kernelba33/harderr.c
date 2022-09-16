#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

NORETURN VOID KexRaiseHardError(
	IN	LPCWSTR	lpszWindowTitle OPTIONAL,
	IN	LPCWSTR lpszBugLink OPTIONAL,
	IN	LPCWSTR	lpszFmt OPTIONAL, ...)
{
	CriticalErrorBoxF(L"hard error stub");
	ExitProcess(0);
}