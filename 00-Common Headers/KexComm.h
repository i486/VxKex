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

#  define COBJMACROS
#  define CINTERFACE

#  define STRICT
#  define WIN32_NO_STATUS

#  ifdef __INTELLISENSE__
#    undef __cplusplus
#  endif

// attempt to make vxkex build with newer sdk's as well.
// no guarantees. if you install the win10 sdk and it doesn't build, tough luck.
// don't ask me to make shit build on your windows 11 with newest SDK.
#  define _WIN32_WINNT 0x0601

#  include <Windows.h>
#  include <NtDll.h>
#  include <StdArg.h>

#  ifdef KEX_TARGET_TYPE_SYS
#    include <NtKrnl.h>
#  endif

#  ifdef __INTELLISENSE__
#    undef NULL
#    define NULL 0
#  endif

#  if defined(KEX_ENV_WIN32) && !defined(NOUSER)
     // Generally speaking we import from SHLWAPI only for the Path*
     // functions. In the future we will dogfood our own PathCch* functions
     // and avoid importing from SHLWAPI altogether.

     // SHLWAPI string functions are EXTREMELY slow. Do not use them, ever.
#    define NO_SHLWAPI_STRFCNS

     // Ok, we'll make an exception for StrFormatByteSizeW -_-
#    define StrFormatByteSize StrFormatByteSizeW
     PWSTR StrFormatByteSizeW(LONGLONG qdw, PWSTR pszBuf, UINT cchBuf);

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

#  if defined(KEX_ENV_WIN32) && !defined(KEX_TARGET_TYPE_LIB)
#    include <KexGui.h>
#    include <KexW32ML.h>
#    include <KxCfgHlp.h>
#  endif

#  include <KexAssert.h>

#  ifdef KEX_TARGET_TYPE_SYS
#    include <NtStrSafe.h>
#  else
#    include <KexStrSafe.h>
#    include <KexPathCch.h>
#    include <SafeAlloc.h>
#  endif
#pragma endregion

#pragma region Extended Language Constructs
#  define try __try
#  define except __except
#  define finally __finally
#  define leave __leave
#  define throw(Status) RtlRaiseStatus(Status)
#  define asm __asm

#  ifndef until
#    define until(Condition) while (!(Condition))
#  endif

#  define unless(Condition) if (!(Condition))
#  define ReturnAddress _ReturnAddress

#  define PopulationCount16 __popcnt16
#  define PopulationCount __popcnt
#  define PopulationCount64 __popcnt64

#  define InterlockedIncrement _InterlockedIncrement
#  define InterlockedIncrement16 _InterlockedIncrement16
#  define InterlockedIncrement64 _InterlockedIncrement64

#  define InterlockedDecrement _InterlockedDecrement
#  define InterlockedDecrement16 _InterlockedDecrement16
#  define InterlockedDecrement64 _InterlockedDecrement64

#  define InterlockedAnd _InterlockedAnd
#  define InterlockedAnd8 _InterlockedAnd8
#  define InterlockedAnd16 _InterlockedAnd16
#  define InterlockedAnd64 _InterlockedAnd64

#  define InterlockedOr _InterlockedOr
#  define InterlockedOr8 _InterlockedOr8
#  define InterlockedOr16 _InterlockedOr16
#  define InterlockedOr64 _InterlockedOr64

#  define InterlockedXor _InterlockedXor
#  define InterlockedXor8 _InterlockedXor8
#  define InterlockedXor16 _InterlockedXor16
#  define InterlockedXor64 _InterlockedXor64

#  define InterlockedCompareExchange _InterlockedCompareExchange
#  define InterlockedCompareExchange8 _InterlockedCompareExchange8
#  define InterlockedCompareExchange16 _InterlockedCompareExchange16
#  define InterlockedCompareExchange64 _InterlockedCompareExchange64

#  undef InterlockedCompareExchangePointer
#  ifdef _M_X64
#    define InterlockedCompareExchangePointer(PointerToPointer, Pointer, Compare) ((PVOID) _InterlockedCompareExchange64((LONGLONG VOLATILE *) (PointerToPointer), (LONGLONG) (Pointer), (LONGLONG) Compare))
#  else
#    define InterlockedCompareExchangePointer(PointerToPointer, Pointer, Compare) ((PVOID) _InterlockedCompareExchange((LONG VOLATILE *) (PointerToPointer), (LONG) (Pointer), (LONG) Compare))
#  endif
#pragma endregion

#pragma region Convenience Macros
//
// Convert a HRESULT code to a Win32 error code.
// Note that not all HRESULT codes can be mapped to a valid Win32 error code.
//
#  define WIN32_FROM_HRESULT(x) (HRESULT_CODE(x))

#  define LODWORD(ull) ((ULONG) (ull))
#  define HIDWORD(ull) ((ULONG) ((ULONGLONG) (ull) >> 32))

//
// Convert a relative virtual address (e.g. as found in PE image files) to a
// real virtual address which can be read, written, dereferenced etc.
// VA_TO_RVA does the opposite.
//
#  define RVA_TO_VA(base, rva) ((PVOID) (((PBYTE) (base)) + (rva)))
#  define VA_TO_RVA(base, va) ((ULONG_PTR) (((PBYTE) (va)) - ((PBYTE) (base))))

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

//
// Check that a kernel handle is not NULL or INVALID_HANDLE_VALUE.
// Keep in mind that NtCurrentProcess() will not be valid if checked with
// this macro, so you will have to special-case it in any code that uses
// process handles.
//
#  define VALID_HANDLE(Handle) \
	(((Handle) != NULL) && \
	 ((Handle) != INVALID_HANDLE_VALUE) && \
	 !(((ULONG_PTR) (Handle)) & 3) && \
	 (((ULONG_PTR) (Handle)) <= ULONG_MAX))

#  define KexDebugCheckpoint() if (KexIsDebugBuild && NtCurrentPeb()->BeingDebugged) __debugbreak()

#  define InterlockedIncrement16 _InterlockedIncrement16
#  define InterlockedDecrement16 _InterlockedDecrement16
#pragma endregion