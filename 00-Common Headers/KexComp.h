///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexComp.h
//
// Abstract:
//
//     Defines compiler settings for VxKex projects.
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

#ifdef _DEBUG
#  pragma optimize("", off)
#else
#  pragma optimize("y", off)
#  pragma optimize("gs", on)
#endif

//
// We can use stack checking in VxKex because NTDLL exports the __chkstk
// symbol in both x86 and x64.
//
#ifdef _DEBUG
#  pragma check_stack(on)
#else
#  pragma check_stack(off)
#endif

//
// Disable runtime checking. These features require the CRT,
// which we do not use.
//
#pragma runtime_checks("", off)

//
// Note: You still need to disable /GS (buffer security check) in the project
// configuration, because unfortunately, there is no pragma which can do this.
//