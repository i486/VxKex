///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     isi.c
//
// Abstract:
//
//     InitSecurityInterfaceA and InitSecurityInterfaceW.
//
//     We need to extend these functions because applications have two ways
//     of obtaining functions from SSPICLI: the first is by importing them from
//     the DLL, and the second is by calling InitSecurityInterface to get a
//     function table (which contains the exact same functions as if you were
//     to obtain them by importing).
//
//     Modules such as WebIO.dll (used by WinHTTP) use the InitSecurityInterface
//     method, while most normal programs would import from secur32, which
//     forwards most of its functions to sspicli.
//
// Author:
//
//     vxiiduu (09-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               09-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxcrypp.h"

KXCRYPAPI PSECURITY_FUNCTION_TABLE_A SEC_ENTRY Ext_InitSecurityInterfaceA(
	VOID)
{
	STATIC SECURITY_FUNCTION_TABLE_A SecTableA = {
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
		EnumerateSecurityPackagesA,
		QueryCredentialsAttributesA,
		Ext_AcquireCredentialsHandleA,
		FreeCredentialsHandle,
		NULL,
		InitializeSecurityContextA,
		AcceptSecurityContext,
		CompleteAuthToken,
		DeleteSecurityContext,
		ApplyControlToken,
		QueryContextAttributesA,
		ImpersonateSecurityContext,
		RevertSecurityContext,
		MakeSignature,
		VerifySignature,
		FreeContextBuffer,
		QuerySecurityPackageInfoA,
		EncryptMessage,
		DecryptMessage,
		ExportSecurityContext,
		ImportSecurityContextA,
		AddCredentialsA,
		NULL,
		QuerySecurityContextToken,
		EncryptMessage,
		DecryptMessage
	};

	return &SecTableA;
}

KXCRYPAPI PSECURITY_FUNCTION_TABLE_W SEC_ENTRY Ext_InitSecurityInterfaceW(
	VOID)
{
	STATIC SECURITY_FUNCTION_TABLE_W SecTableW = {
		SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION,
		EnumerateSecurityPackagesW,
		QueryCredentialsAttributesW,
		Ext_AcquireCredentialsHandleW,
		FreeCredentialsHandle,
		NULL,
		InitializeSecurityContextW,
		AcceptSecurityContext,
		CompleteAuthToken,
		DeleteSecurityContext,
		ApplyControlToken,
		QueryContextAttributesW,
		ImpersonateSecurityContext,
		RevertSecurityContext,
		MakeSignature,
		VerifySignature,
		FreeContextBuffer,
		QuerySecurityPackageInfoW,
		EncryptMessage,
		DecryptMessage,
		ExportSecurityContext,
		ImportSecurityContextW,
		AddCredentialsW,
		NULL,
		QuerySecurityContextToken,
		EncryptMessage,
		DecryptMessage
	};

	return &SecTableW;
}