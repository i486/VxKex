#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOMB
#define NOMEMMGR
#define NOOPENFILE
#define NOSCROLL
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
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

#define KXBASEAPI

#ifdef _M_X64
#  pragma comment(lib, "cfgmgr32_x64.lib")
#else
#  pragma comment(lib, "cfgmgr32_x86.lib")
#endif

#define KEX_COMPONENT L"KxBase"
#define KEX_ENV_WIN32
#define KEX_TARGET_TYPE_DLL
