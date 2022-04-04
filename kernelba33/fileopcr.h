#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>

typedef struct _CREATEFILE2_EXTENDED_PARAMETERS {
	DWORD					dwSize;
	DWORD					dwFileAttributes;
	DWORD					dwFileFlags;
	DWORD					dwSecurityQosFlags;
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes;
	HANDLE					hTemplateFile;
} CREATEFILE2_EXTENDED_PARAMETERS, *PCREATEFILE2_EXTENDED_PARAMETERS, *LPCREATEFILE2_EXTENDED_PARAMETERS;