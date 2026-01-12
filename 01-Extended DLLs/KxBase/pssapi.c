///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     pssapi.c
//
// Abstract:
//
//     This file contains the kxbase-side implementation of the Process
//     Snapshots API introduced in Windows 8.1. These APIs are used by Python.
//
//     Currently these APIs are all stubs. It is, however, possible to fully
//     implement them under Windows 7 if it is found that programs are making
//     significant use of them.
//
// Author:
//
//     vxiiduu (01-Mar-2024)
//
// Environment:
//
//     WHERE CAN THIS PROGRAM/LIBRARY/CODE RUN?
//
// Revision History:
//
//     vxiiduu              01-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI ULONG WINAPI PssCaptureSnapshot(
	IN	HANDLE				ProcessHandle,
	IN	PSS_CAPTURE_FLAGS	CaptureFlags,
	IN	ULONG				ThreadContextFlags OPTIONAL,
	OUT	PHPSS				SnapshotHandle)
{
	KexLogWarningEvent(L"Stub API called: PssCaptureSnapshot");
	return ERROR_NOT_SUPPORTED;
}

KXBASEAPI ULONG WINAPI PssFreeSnapshot(
	IN	HANDLE				ProcessHandle,
	IN	HPSS				SnapshotHandle)
{
	KexLogWarningEvent(L"Stub API called: PssFreeSnapshot");
	return ERROR_NOT_SUPPORTED;
}

KXBASEAPI ULONG WINAPI PssQuerySnapshot(
	IN	HPSS						SnapshotHandle,
	IN	PSS_QUERY_INFORMATION_CLASS	InformationClass,
	OUT	PVOID						Buffer,
	IN	ULONG						BufferLength)
{
	KexLogWarningEvent(L"Stub API called: PssQuerySnapshot");
	return ERROR_NOT_SUPPORTED;
}