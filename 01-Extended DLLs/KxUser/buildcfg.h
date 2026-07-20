#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOWINSTYLES
#define NOMENUS
#define NOICONS
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NODRAWTEXT
#define NOKERNEL
#define NONLS
#define NOMEMMGR
#define NOOPENFILE
#define NOTEXTMETRIC
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#define NOCRYPT
#define NOMETAFILE
#define NOSERVICE
#define NOSOUND
#define _UXTHEME_H_

#ifdef _M_X64
#  pragma comment(lib, "user32_x64.lib")
#else
#  pragma comment(lib, "user32_x86.lib")
#endif

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "KexDll.lib")
#pragma comment(lib, "gdi32.lib")

#define KXUSERAPI

#define KEX_TARGET_TYPE_DLL
#define KEX_ENV_WIN32
#define KEX_COMPONENT L"KxUser"
#define KEX_NO_EXTRA_LIBS
