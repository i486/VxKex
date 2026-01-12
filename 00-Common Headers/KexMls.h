///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexMls.h
//
// Abstract:
//
//     Public header file for the multi-language support (MLS) component.
//
// Author:
//
//     vxiiduu (17-May-2025)
//
// Environment:
//
//     Setup, Native, Win32
//
// Revision History:
//
//     vxiiduu               17-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>

#pragma comment(lib, "KexSmp.lib")

#ifndef MLSAPI
#  define MLSAPI
#  pragma comment(lib, "KexMls.lib")
#endif

//
// Macros
//

// Convenience macro for string literals.
// e.g. MessageBox(NULL, _(L"Hello world"), NULL, 0);

#ifdef KEX_DISABLE_MLS_MACRO
#  ifndef _
#    define _(s) (s)
#  endif
#else
#  define _(s) MlsMapString(s)
#endif

// This flag indicates that Data is a mapped section view which needs to be
// unmapped when the dictionary is cleaned up.
#define MLSP_DICTIONARY_FLAG_MAPPED_FILE	0x00000001

// In the future we might want to put the entire hash table into this file.
// In that case we need to differentiate between "old" dictionaries and "new"
// ones. It's always a good idea to add a version field to a file format anyway.
#define MLSP_VERSION 1

//
// Data Types
//

typedef struct _MLSP_DICTIONARY_HEADER {
	CHAR	Magic[4]; // {'B', 'D', 'I', 'C'}
	ULONG	Version;
	ULONG	NumberOfKeyValuePairs;
} TYPEDEF_TYPE_NAME(MLSP_DICTIONARY_HEADER);

typedef struct _MLSP_DICTIONARY_KEY_OR_VALUE {
	ULONG	Cch;	// character count, not including null terminator
	WCHAR	Text[]; // Text[Cch+1], null terminated
} TYPEDEF_TYPE_NAME(MLSP_DICTIONARY_KEY_OR_VALUE);

typedef struct _MLSP_DICTIONARY {
	PCMLSP_DICTIONARY_HEADER	Header;
	PCVOID						Data;
	SIZE_T						DataCb;
	ULONG						Flags;	// MLSP_DICTIONARY_FLAG_*
} TYPEDEF_TYPE_NAME(MLSP_DICTIONARY);

typedef NTSTATUS (NTAPI *PMLSP_DICTIONARY_ITERATOR) (
	IN	PVOID				Context OPTIONAL,
	IN	PCUNICODE_STRING	Key,
	IN	PCUNICODE_STRING	Value);

//
// Functions
//

MLSAPI NTSTATUS NTAPI MlsInitialize(
	VOID);

MLSAPI NTSTATUS NTAPI MlsCleanup(
	VOID);

MLSAPI NTSTATUS NTAPI MlsGetCurrentLangId(
	OUT	PLANGID	LangId);

MLSAPI NTSTATUS NTAPI MlsSetCurrentLangId(
	IN	LANGID	LangId);

MLSAPI PCWSTR NTAPI MlsMapString(
	IN	PCWSTR	EnglishString);