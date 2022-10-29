///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexdllp.h
//
// Abstract:
//
//     Private header file for KexDll.
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexDll.h>

extern PKEX_PROCESS_DATA KexData;

VOID NTAPI KexDllNotificationCallback(
	IN	LDR_DLL_NOTIFICATION_REASON	Reason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context);

NTSTATUS KexInitializeDllRewrite(
	VOID);

BOOLEAN KexShouldRewriteImportsOfDll(
	IN	PCUNICODE_STRING	BaseDllName,
	IN	PCUNICODE_STRING	FullDllName);

NTSTATUS KexRewriteImageImportDirectory(
	IN	PVOID				ImageBase,
	IN	PCUNICODE_STRING	BaseImageName,
	IN	PCUNICODE_STRING	FullImageName);