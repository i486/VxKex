#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

typedef struct _WIN32_MEMORY_RANGE_ENTRY {
	LPVOID	VirtualAddress;
	SIZE_T	NumberOfBytes;
} WIN32_MEMORY_RANGE_ENTRY, *PWIN32_MEMORY_RANGE_ENTRY;