///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     registry.c
//
// Abstract:
//
//     Registry convenience functions
//
// Author:
//
//     vxiiduu (02-Oct-2022)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               02-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegReadI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	OUT	PULONG	Data)
{
	DWORD Discard;
	
	Discard = sizeof(*Data);
	
	return RegGetValue(
		Key,
		SubKey,
		ValueName,
		RRF_RT_DWORD,
		NULL,
		Data,
		&Discard);
}

KW32MLDECLSPEC EXTERN_C ULONG KW32MLAPI RegWriteI32(
	IN	HKEY	Key,
	IN	PCWSTR	SubKey OPTIONAL,
	IN	PCWSTR	ValueName OPTIONAL,
	IN	ULONG	Data)
{
	return RegSetKeyValue(
		Key,
		SubKey,
		ValueName,
		REG_DWORD,
		&Data,
		sizeof(Data));
}