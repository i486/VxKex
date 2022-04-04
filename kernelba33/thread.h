#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

typedef enum _THREAD_INFORMATION_CLASS {	// THREADINFOCLASS value		Min. NT version
	ThreadMemoryPriority,					// ThreadPagePriority			6.0+
	ThreadAbsoluteCpuPriority,				// ThreadActualBasePriority		6.0+
	ThreadDynamicCodePolicy,				// ThreadDynamicCodePolicyInfo	10.0+
	ThreadPowerThrottling,					// ThreadDynamicCodePolicyInfo	10.0+
	ThreadInformationClassMax
} THREAD_INFORMATION_CLASS;