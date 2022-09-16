#include <KexDll.h>
#include <powrprof.h>

POWER_PLATFORM_ROLE DLLAPI PowerDeterminePlatformRoleEx(
	IN	ULONG	Version)
{
	ODS_ENTRY();
	return PowerDeterminePlatformRole();
}