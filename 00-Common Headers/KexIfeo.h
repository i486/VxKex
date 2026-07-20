///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexIfeo.h
//
// Abstract:
//
//     This file contains the definitions for all VxKex IFEO key options (the
//     ones which begin with KEX_*). It's similar to redirects.h from KexDll.
//     Search for inclusions of this header file for usage examples.
//
//     This was created to reduce the duplication of VxKex IFEO options and
//     the spread of IFEO checking code throughout the codebase.
//
// Author:
//
//     vxiiduu (23-May-2026)
//
// Revision History:
//
//     Author               23-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef IFEO_PARAMETER
#  ifndef IFEO_PARAMETER_BASE_POINTER
#    error "Define either IFEO_PARAMETER or IFEO_PARAMETER_BASE_POINTER"
#  endif
#  define IFEO_PARAMETER(DATA_TYPE, MEMBER, REGISTRY_DATA_TYPE) \
		{ \
			_L(_STR(KEX_##MEMBER)), \
			REGISTRY_DATA_TYPE, \
			&(IFEO_PARAMETER_BASE_POINTER)->MEMBER, \
			sizeof((IFEO_PARAMETER_BASE_POINTER)->MEMBER) \
		}, // The trailing comma is deliberate, do not remove.

#  define KEX_IFEO_SHOULD_CLEANUP
#else
#  ifdef IFEO_PARAMETER_BASE_POINTER
#    error "Defining both IFEO_PARAMETER and IFEO_PARAMETER_BASE_POINTER is useless"
#  endif
#endif

// Increment KXCFG_PRESERVED_CONFIGURATION_VERSION whenever any changes
// are made to the KEX_IFEO_PARAMETERS structure.

IFEO_PARAMETER(ULONG,				DisableForChild,			REG_DWORD) // boolean
IFEO_PARAMETER(ULONG,				DisableAppSpecific,			REG_DWORD) // boolean
IFEO_PARAMETER(KEX_WIN_VER_SPOOF,	WinVerSpoof,				REG_DWORD)
IFEO_PARAMETER(ULONG,				StrongVersionSpoof,			REG_DWORD) // KEX_STRONGSPOOF_*
IFEO_PARAMETER(ULONG,				DisableConsoleEnhancements,	REG_DWORD) // boolean
IFEO_PARAMETER(ULONG,				TlsForceEnabledProtocols,	REG_DWORD) // SP_PROT_*
IFEO_PARAMETER(ULONG,				TlsForceDisabledProtocols,	REG_DWORD) // SP_PROT_*
IFEO_PARAMETER(IFEO_PATH_BUFFER,	DllRewriteEntries,			REG_SZ)
IFEO_PARAMETER(IFEO_PATH_BUFFER,	DllRewriteExemptions,		REG_SZ)

#ifdef KEX_IFEO_SHOULD_CLEANUP
#  undef IFEO_PARAMETER
#  undef KEX_IFEO_SHOULD_CLEANUP
#endif