#include "VxKexLdr.h"

STATIC ULONG_PTR vaChildKexData = 0;

VOID UpdateKexDataInChild(
	VOID)
{
	if (vaChildKexData) {
		VaWrite(vaChildKexData, &g_KexData, sizeof(KEX_PROCESS_DATA));
	}
}

VOID CopyKexDataToChild(
	VOID)
{
	ULONG_PTR vaSubSystemData;

	// allocate memory in child process for KexData structure
	vaChildKexData = (ULONG_PTR) VirtualAllocEx(g_hProc, NULL, sizeof(KEX_PROCESS_DATA), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!vaChildKexData) {
		CriticalErrorBoxF(L"Failed to allocate memory inside the child process to store VxKex environment data.");
	}

	// write the data
	UpdateKexDataInChild();

	// put the pointer to the data in Peb->SubSystemData
	vaSubSystemData = g_vaPebBase - (g_bExe64 ? 0 : 0x1000) + (g_bExe64 ? 0x28 : 0x14);
	VaWrite(vaSubSystemData, &vaChildKexData, (g_bExe64 ? sizeof(QWORD) : sizeof(DWORD)));
}