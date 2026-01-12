#pragma once

#define MLSAPI
#define NO_KEXDLL_LIB // Prevent inclusion of KexDll.lib

#define KEX_COMPONENT L"KexMls"
#define KEX_ENV_WIN32
#define KEX_TARGET_TYPE_LIB

#pragma comment(lib, "KexSmp.lib")
