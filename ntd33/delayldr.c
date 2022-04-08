#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

// TODO: Implement these functions properly. But for now, basically no apps
// actually use these so it's low priority.

PVOID WINAPI LdrResolveDelayLoadedAPI(
	IN	PVOID							ParentModuleBase,
	IN	PVOID							DelayloadDescriptor,
	IN	PVOID							FailureDllHook,
	IN	PVOID							FailureSystemHook,
	OUT	PIMAGE_THUNK_DATA				ThunkAddress,
	IN	ULONG							Flags)
{
	ODS_ENTRY(L"(%p, %p, %p, %p, %p, %I32u)", ParentModuleBase, DelayloadDescriptor, FailureDllHook, FailureSystemHook, ThunkAddress, Flags);
	return NULL;
}