#pragma once
#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>

#define DLLAPI DECLSPEC_EXPORT
#undef WINBASEAPI
#define WINBASEAPI DECLSPEC_EXPORT
#undef WINUSERAPI
#define WINUSERAPI DECLSPEC_EXPORT

#define _STR(x) #x
#define STR(x) _STR(x)

#ifdef _DEBUG
#  define ODS(...) { TCHAR __ods_buf[256]; sprintf_s(__ods_buf, ARRAYSIZE(__ods_buf), T(__FUNCTION__) T("(") T(__FILE__) T(":") T(STR(__LINE__)) T("): ") __VA_ARGS__); OutputDebugString(__ods_buf); }
#  define ODS_ENTRY(...) { TCHAR __ods_buf[256]; sprintf_s(__ods_buf, ARRAYSIZE(__ods_buf), T("Function called: ") T(__FUNCTION__) __VA_ARGS__); OutputDebugString(__ods_buf); }
#else
#  define ODS(...)
#  define ODS_ENTRY(...)
#endif