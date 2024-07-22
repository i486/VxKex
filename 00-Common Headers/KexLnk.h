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
//     vxiiduu               31-Oct-2022  Fix 32-bit SEH.
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
#elif defined(KEX_TARGET_TYPE_SYS)
#  pragma comment(linker, "/ENTRY:DriverEntry")
#elif !defined(KEX_TARGET_TYPE_LIB)
// Did you forget to include buildcfg.h?
#  error You must specify a target file type (EXE, DLL or LIB) for this project by defining a KEX_TARGET_TYPE_* macro before including KexLnk.h or KexComm.h.
#endif

#pragma region Library Inputs
#if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#  ifdef KEX_ENV_WIN32
#    if defined(KEX_TARGET_TYPE_EXE) || defined(KEX_TARGET_TYPE_DLL)
#      ifdef _M_X64
#        pragma comment(lib, "kernel32_x64.lib")
#        pragma comment(lib, "kernelbase_x64.lib")
#      else
#        pragma comment(lib, "kernel32_x86.lib")
#        pragma comment(lib, "kernelbase_x86.lib")
#      endif
#      pragma comment(lib, "ole32.lib")
#    endif
#  endif
#elif defined(KEX_TARGET_TYPE_SYS)
#  ifdef _M_X64
#    pragma comment(lib, "ntoskrnl_x64.lib")
#    pragma comment(lib, "ntstrsafe_x64.lib")
#  else
#    pragma comment(lib, "ntoskrnl_x86.lib")
#    pragma comment(lib, "ntstrsafe_x86.lib")
#  endif
#endif

#ifdef KEX_ARCH_X86
#  pragma comment(lib, "seh32.lib")
#endif
#pragma endregion