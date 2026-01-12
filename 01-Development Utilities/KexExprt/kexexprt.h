///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexexprt.h
//
// Abstract:
//
//     Private header file for KexExprt
//
// Author:
//
//     vxiiduu (29-Oct-2022)
//
// Revision History:
//
//     vxiiduu               29-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "buildcfg.h"
#include <KexComm.h>

typedef struct _LARGE_UNICODE_STRING {
	ULONG	Cch;
	ULONG	MaxCch;
	PWCHAR	Buffer;
} TYPEDEF_TYPE_NAME(LARGE_UNICODE_STRING);

typedef enum _KEXEXPRT_GENERATE_STYLE {
	GenerateStyleDef,
	GenerateStylePragma
} TYPEDEF_TYPE_NAME(KEXEXPRT_GENERATE_STYLE);

INT_PTR CALLBACK MainWndProc(
	IN	HWND	MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

VOID DumpExports(
	IN	HWND					MainWindow,
	IN	PCWSTR					DllPath,
	IN	KEXEXPRT_GENERATE_STYLE	Style,
	IN	BOOLEAN					IncludeOrdinals);