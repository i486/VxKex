#include "buildcfg.h"
#include "kxmip.h"

KXMIAPI BOOL WINAPI ImmDisableLegacyIME(
	VOID)
{
	// The parameter to ImmDisableIME (and NtUserDisableThreadIme) is
	// the thread ID to disable IME for. This isn't documented.
	// 0 means the current thread.
	// -1 means all threads in the process.

	return ImmDisableIME(-1);
}