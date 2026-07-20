#pragma once

#ifndef NO_KEXDLL_LIB
#  define NO_KEXDLL_LIB
#endif

#pragma comment(lib, "KexMLS.lib")
#pragma comment(lib, "KexSmp.lib")

#define FRIENDLYAPPNAME L"VxKex Installer"
#define KEX_COMPONENT L"KexSetup"
#define KEX_ENV_WIN32
#define KEX_TARGET_TYPE_EXE
