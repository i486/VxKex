///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kxuserp.h
//
// Abstract:
//
//     Private header file for KxUser.
//
// Author:
//
//     vxiiduu (10-Feb-2022)
//
// Revision History:
//
//     vxiiduu              10-Feb-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxUser.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef struct _FNDWORDMSG {
	PVOID		pwnd;
	UINT		msg;
	WPARAM		wParam;
	LPARAM		lParam;
	ULONG_PTR	xParam;
	PVOID		xpfnProc;
} TYPEDEF_TYPE_NAME(FNDWORDMSG);

NTSTATUS EnableWindowMessageInterception(
	VOID);

