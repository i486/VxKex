///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexLog.h
//
// Abstract:
//
//     Logging convenience functions and macros.
//
// Author:
//
//     vxiiduu (26-Sep-2022)
//
// Environment:
//
//     Win32 mode, mainly in KexSrv.
//     Do not call any logging functions from inside a kex process. Communicate
//     with KexSrv to do that.
//     Do not include this header file inside of KexComm.h.
//
// Revision History:
//
//     vxiiduu               26-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexComm.h>
#include <VXLL.h>

#ifdef KEX_ENV_WIN32
#  pragma comment(lib, "VXLL.lib")
#endif