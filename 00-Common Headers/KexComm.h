///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexComm.h
//
// Abstract:
//
//     VxKex common header file. Every project which is a part of VxKex must
//     include this file unless there is a very good reason not to.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Environment:
//
//     Refer to included header files for information about their contents.
//
// Revision History:
//
//     vxiiduu               30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#if !defined(KEX_ARCH_X64) && !defined(KEX_ARCH_X86)
#  if defined(_M_X64)
#    define KEX_ARCH_X64
#  elif defined(_M_IX86)
#    define KEX_ARCH_X86
#  else
#    error Invalid CPU architecture for VxKex. Only x86 and x64 are supported.
#  endif
#endif

#ifndef KEX_ENV_NATIVE
#  define KEX_ENV_WIN32
#endif

#if defined(KEX_ENV_NATIVE) && defined(KEX_ENV_WIN32)
#  error Only one of the KEX_ENV_ macros may be defined.
#endif

// Define Unicode macros.
// __FILEW__ and __FUNCTIONW__ are already defined somewhere inside the windows
// headers.
#define CONCAT(a,b) a##b
#define _L(str) CONCAT(L,str)
#define __DATEW__ _L(__DATE__)
#define __TIMEW__ _L(__TIME__)
#define __TIMESTAMPW__ _L(__TIMESTAMP__)
#define __FUNCDNAMEW__ _L(__FUNCDNAME__)
#define __FUNCSIGW__ _L(__FUNCSIG__)

//
// Define convenience macros so we don't need #ifdef in the middle of code.
//
#ifdef _DEBUG
#  define KexIsDebugBuild TRUE
#  define KexIsReleaseBuild FALSE
#else
#  define KexIsDebugBuild FALSE
#  define KexIsReleaseBuild TRUE
#endif

#ifdef KEX_ARCH_X64
#  define KexIs32BitBuild FALSE
#  define KexIs64BitBuild TRUE
#else
#  define KexIs32BitBuild TRUE
#  define KexIs64BitBuild FALSE
#endif

#pragma region Header Includes
// these two must be included before other headers (esp. KexStrSafe.h), or
// undefined symbol errors can occur
#  include <KexComp.h>
#  include <KexLnk.h>

#  ifdef KEX_ENV_NATIVE
     //
     // We can't use any of this stuff in a native environment,
     // so we disable it to avoid mistakes/accidentally calling
     // unusable APIs.
     //
#    define NOGDICAPMASKS
#    define NOVIRTUALKEYCODES
#    define NOWINMESSAGES
#    define NOWINSTYLES
#    define NOSYSMETRICS
#    define NOMENUS
#    define NOICONS
#    define NOKEYSTATES
#    define NOSYSCOMMANDS
#    define NORASTEROPS
#    define NOSHOWWINDOW
#    define NOATOM
#    define NOCLIPBOARD
#    define NOCOLOR
#    define NOCTLMGR
#    define NODRAWTEXT
#    define NOGDI
#    define NOKERNEL
#    define NOUSER
#    define NONLS
#    define NOMB
#    define NOMEMMGR
#    define NOMSG
#    define NOOPENFILE
#    define NOSCROLL
#    define NOTEXTMETRIC
#    define NOWH
#    define NOWINOFFSETS
#    define NOCOMM
#    define NOKANJI
#    define NOHELP
#    define NOPROFILER
#    define NODEFERWINDOWPOS
#    define NOMCX
#    define NOCRYPT
#    define NOMETAFILE
#    define NOSERVICE
#    define NOSOUND
#  endif

#  define STRICT
#  include <Windows.h>
#  include <NtDll.h>
#  include <StdArg.h>

#  if defined(KEX_ENV_WIN32) && !defined(NOUSER)
     // Generally speaking we import from SHLWAPI only for the Path*
     // functions. In the future we will dogfood our own PathCch* functions
     // and avoid importing from SHLWAPI altogether.

     // SHLWAPI string functions are EXTREMELY slow. Do not use them, ever.
#    define NO_SHLWAPI_STRFCNS

#    define NO_SHLWAPI_ISOS
#    define NO_SHLWAPI_STREAM
#    define NO_SHLWAPI_HTTP
#    define NO_SHLWAPI_GDI
#    define NO_SHLWAPI_STOPWATCH
#    pragma comment(lib, "shlwapi.lib")
#    include <Shlwapi.h>
#  endif

// Some bullshit warning about a "deprecated" function in intrin.h
// get rid of the stupid warning
#  pragma warning(push)
#  pragma warning(disable:4995)
#    include <Intrin.h>
#  pragma warning(pop)

#  include <KexTypes.h>
#  include <KexVer.h>
#  include <KexData.h>

#  ifdef KEX_TARGET_TYPE_EXE
#    include <KexGui.h>
#    include <KexW32ML.h>
#  endif

#  include <KexAssert.h>
#  include <KexStrSafe.h>
#  include <SafeAlloc.h>

#  include <KexSrv.h>
#pragma endregion

#pragma region Extended Language Constructs
//
// SEH in native mode is possible without the CRT only on x64.
// This is because NTDLL exports __C_specific_handler (required for SEH on x64),
// but doesn't export _except_handler3 (required for SEH on x86) on either
// native x86 or WOW64 variants.
//
// Importing from MSVCRT is done in KexLnk.h.
//
#  if defined(KEX_ARCH_X64) || defined(KEX_ENV_WIN32)
#    define try __try
#    define except __except
#    define finally __finally
#    define leave __leave
#  else
#    define try do
#    define except(x) while (0); if (0)
#    define finally while (0);
#    define leave break
#  endif

#  define until(Condition) while (!(Condition))
#  define unless(Condition) if (!(Condition))
#pragma endregion

#pragma region Convenience Macros
//
// Convert a HRESULT code to a Win32 error code.
// Note that not all HRESULT codes can be mapped to a valid Win32 error code.
//
#  define WIN32_FROM_HRESULT(x) (HRESULT_CODE(x))

//
// Convert a relative virtual address (e.g. as found in PE image files) to a
// real virtual address which can be read, written, dereferenced etc.
//
#  define RVA_TO_VA(base, rva) ((LPVOID) (((LPBYTE) (base)) + (rva)))

//
// Convert a boolean to a string which can be displayed to the user.
//
#  define BOOLEAN_AS_STRING(b) ((b) ? L"TRUE" : L"FALSE")

//
// Lookup an entry in a static entry table with bounds clamping.
//
#  define ARRAY_LOOKUP_BOUNDS_CHECKED(Array, Index) Array[max(0, min((Index), ARRAYSIZE(Array) - 1))]

//
// Convert a byte count to a character count (assuming a character is
// 2 bytes long, as it always is when dealing with windows unicode strings)
// and vice versa.
//
#  define CB_TO_CCH(Cb) ((Cb) >> 1)
#  define CCH_TO_CB(Cch) ((Cch) << 1)

#  define InterlockedIncrement16 _InterlockedIncrement16
#  define InterlockedDecrement16 _InterlockedDecrement16
#pragma endregion