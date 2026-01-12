#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>
#include <powrprof.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef enum _WLDP_WINDOWS_LOCKDOWN_MODE {
	WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED,
	WLDP_WINDOWS_LOCKDOWN_MODE_TRIAL,
	WLDP_WINDOWS_LOCKDOWN_MODE_LOCKED,
	WLDP_WINDOWS_LOCKDOWN_MODE_MAX
} TYPEDEF_TYPE_NAME(WLDP_WINDOWS_LOCKDOWN_MODE);

typedef struct _VERHEAD {
	WORD				wTotLen;
	WORD				wValLen;
	WORD				wType;
	WCHAR				szKey[(sizeof("VS_VERSION_INFO") + 3) & ~3];
	VS_FIXEDFILEINFO	vsf;
} TYPEDEF_TYPE_NAME(VERHEAD);

#define DEVICE_NOTIFY_CALLBACK 2

typedef enum {
	NotificationKind_ItemAdded = 0,
	NotificationKind_ItemRemoved = 1,
	NotificationKind_ActionCompleted = 2,
	NotificationKind_ActionAborted = 3,
	NotificationKind_Other = 4
} NotificationKind;

typedef enum {
	NotificationProcessing_ImportantAll = 0,
	NotificationProcessing_ImportantMostRecent = 1,
	NotificationProcessing_All = 2,
	NotificationProcessing_MostRecent = 3,
	NotificationProcessing_CurrentThenMostRecent = 4,
	NotificationProcessing_ImportantCurrentThenMostRecent = 5
} NotificationProcessing;