#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>

#undef WINBASEAPI
#define WINBASEAPI
#undef WINUSERAPI
#define WINUSERAPI
#undef WINPATHCCHAPI
#define WINPATHCCHAPI
#undef DLLAPI
#define DLLAPI

#define PROXY_FUNCTION(ExportedName) P_##ExportedName

#define _STR(x) #x
#define STR(x) _STR(x)

#ifdef _DEBUG
#  define ODS(...) { WCHAR __ods_buf[256]; snwprintf_s(__ods_buf, ARRAYSIZE(__ods_buf), _TRUNCATE, L(__FUNCTION__) L"(" L(__FILE__) L":" L(STR(__LINE__)) L"): " __VA_ARGS__); OutputDebugString(__ods_buf); }
#  define ODS_ENTRY(...) { WCHAR __ods_buf[256]; snwprintf_s(__ods_buf, ARRAYSIZE(__ods_buf), _TRUNCATE, L"Function called: " L(__FUNCTION__) __VA_ARGS__); OutputDebugString(__ods_buf); }
#else
#  define ODS(...)
#  define ODS_ENTRY(...)
#endif