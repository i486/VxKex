///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexCpu.h
//
// Abstract:
//
//     Contains structure definitions for CPU-related bit flags and
//     registers.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Environment:
//
//     Any environment.
//
// Revision History:
//
//     vxiiduu               30-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexTypes.h>

// Debug Status register
typedef struct _DR6 {
	union {
		ULONG_PTR Entire;

		struct {
			UCHAR	B0 : 1;
			UCHAR	B1 : 1;
			UCHAR	B2 : 1;
			UCHAR	B3 : 1;
			USHORT	Reserved1 : 9;
			UCHAR	BD : 1;
			UCHAR	BS : 1;
			UCHAR	BT : 1;
			UCHAR	RTM : 1;
			USHORT	Reserved2 : 15;
#ifdef _M_X64
			ULONG	Reserved3 : 32;
#endif
		};
	};
} DR6;

// Debug Control register
typedef struct _DR7 {
	union {
		ULONG_PTR Entire;

		struct {
			UCHAR	L0 : 1;
			UCHAR	G0 : 1;
			UCHAR	L1 : 1;
			UCHAR	G1 : 1;
			UCHAR	L2 : 1;
			UCHAR	G2 : 1;
			UCHAR	L3 : 1;
			UCHAR	G3 : 1;
			UCHAR	LE : 1;
			UCHAR	GE : 1;
			UCHAR	Reserved1 : 1; // set to 1
			UCHAR	RTM : 1;
			UCHAR	Reserved2 : 1; // set to 0
			UCHAR	GD : 1;
			UCHAR	Reserved3 : 2; // both set to 0
			UCHAR	RW0 : 2;
			UCHAR	LEN0 : 2;
			UCHAR	RW1 : 2;
			UCHAR	LEN1 : 2;
			UCHAR	RW2 : 2;
			UCHAR	LEN2 : 2;
			UCHAR	RW3 : 2;
			UCHAR	LEN3 : 2;
#ifdef _M_X64
			ULONG	Reserved4 : 32;
#endif
		};
	};
} DR7;