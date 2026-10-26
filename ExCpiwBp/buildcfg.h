#pragma once

#pragma comment(lib, "KexDll.lib")
#define FRIENDLYAPPNAME L"Post-Install Explorer CPIW Bypass DLL"

#ifdef _M_X64
#  define KEX_COMPONENT L"ExCpBp64"
#else
#  define KEX_COMPONENT L"ExCpBp32"
#endif

#define KEX_ENV_WIN32
#define KEX_TARGET_TYPE_DLL
