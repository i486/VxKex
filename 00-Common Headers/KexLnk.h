///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     NtDll.h
//
// Abstract:
//
//     Defines linker settings for VxKex projects.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu               30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Disable CRT across the board. It is not used in VxKex.
//
#pragma comment(linker, "/NODEFAULTLIB:LIBCMT.LIB")
#pragma comment(linker, "/NODEFAULTLIB:LIBCMTD.LIB")
#pragma comment(linker, "/NODEFAULTLIB:MSVCRT.LIB")
#pragma comment(linker, "/NODEFAULTLIB:MSVCRTD.LIB")

#ifdef KEX_ENV_WIN32
#  pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#else
#  pragma comment(linker, "/SUBSYSTEM:NATIVE")
#endif

//
// Set appropriate entry point.
//
#if defined(KEX_TARGET_TYPE_EXE)
#  pragma comment(linker, "/ENTRY:EntryPoint")
#elif defined(KEX_TARGET_TYPE_DLL)
#  pragma comment(linker, "/ENTRY:DllMain")
#elif !defined(KEX_TARGET_TYPE_LIB)
#  error You must specify a target file type (EXE, DLL or LIB) for this project by defining a KEX_TARGET_TYPE_* macro before including KexLnk.h or KexComm.h.
#endif

#pragma region Library Inputs
#  ifdef KEX_ENV_WIN32
#    if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#      pragma comment(lib, "kernel32.lib")
#      pragma comment(lib, "advapi32.lib")
#    endif
#  endif
#pragma endregion