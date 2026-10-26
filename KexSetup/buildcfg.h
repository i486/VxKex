#pragma once

#ifndef NO_KEXDLL_LIB
#  define NO_KEXDLL_LIB
#endif

#pragma comment(lib, "KexMLS.lib")
#pragma comment(lib, "KexSmp.lib")
#pragma comment(lib, "KexW32ML.lib")
#pragma comment(lib, "KexGui.lib")
#pragma comment(lib, "KexPathCch.lib")
#pragma comment(lib, "KxCfgHlp.lib")

#define FRIENDLYAPPNAME L"VxKex Installer"
#define KEX_COMPONENT L"KexSetup"
#define KEX_ENV_WIN32
#define KEX_TARGET_TYPE_EXE
