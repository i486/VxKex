///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KxAdvApi.h
//
// Abstract:
//
//     Public header file for KxAdvApi
//
// Author:
//
//     vxiiduu (22-Oct-2024)
//
// Revision History:
//
//     vxiiduu              22-Oct-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>

#if !defined(KXADVAPI) && defined(KEX_ENV_WIN32)
#  define KXADVAPI
#  pragma comment(lib, "KxAdvApi.lib")
#endif

KXADVAPI LSTATUS WINAPI Ext_RegGetValueA(
	IN		HKEY	KeyHandle,
	IN		PCSTR	SubKeyName OPTIONAL,
	IN		PCSTR	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	Data OPTIONAL,
	IN OUT	PULONG	DataCb OPTIONAL);

KXADVAPI LSTATUS WINAPI Ext_RegGetValueW(
	IN		HKEY	KeyHandle,
	IN		PCWSTR	SubKeyName OPTIONAL,
	IN		PCWSTR	ValueName OPTIONAL,
	IN		ULONG	Flags OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PVOID	Data OPTIONAL,
	IN OUT	PULONG	DataCb OPTIONAL);