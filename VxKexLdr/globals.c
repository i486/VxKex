#include "VxKexLdr.h"

//
// Global vars
//

WCHAR g_szExeFullPath[MAX_PATH];
LPWSTR g_lpszExeBaseName = NULL;
ULONG_PTR g_vaExeBase;
ULONG_PTR g_vaPebBase;
HANDLE g_hProc = NULL;
HANDLE g_hThread = NULL;
DWORD g_dwProcId;
DWORD g_dwThreadId;
BOOL g_bExe64 = -1;
HANDLE g_hLogFile = NULL;
KEX_PROCESS_DATA g_KexData;