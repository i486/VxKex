///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     sspihack.c
//
// Abstract:
//
//     You'd think that writing a SSP would be easy because it's all documented
//     by Microsoft. But NOOOOOOO it turns out that if you write a SSP and you
//     expose EncryptMessage and DecryptMessage APIs, as we do, then SSPICLI
//     will not allow the target application to use those APIs. It overwrites
//     the function pointers with ones that do nothing and return a failure
//     status IF the security package DLL isn't "trusted".
//
//     Of course, this is pointless, because at that point our DLL is already
//     loaded and we can do whatever we want, so it's extremely useless as a
//     "security" feature. Windows 10 doesn't do this signature check so they
//     probably realized that it's useless and deleted the code.
//
//     In order to fix this we have to find the location of SSPI's function
//     tables and go change EncryptMessage and DecryptMessage back to what they
//     are supposed to be.
//
//     For reference, the function that does the meddling with our function
//     tables is sspicli!SecpBindSspiDll.
//
// Author:
//
//     vxiiduu (13-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               13-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

STATIC INLINE BOOLEAN SppValidDllSecPkg(
	IN	PCDLL_SECURITY_PACKAGE	DllSecPkg)
{
	BOOLEAN IsValid;

	//
	// SSPICLI.dll has a check inside for if dwLower is the current thread pseudo-
	// handle (-2). I don't know under what circumstance that becomes true, but
	// it will be a problem for us.
	//
	// When dwLower is NtCurrentThread(), SSPICLI uses a TLS slot to get the actual
	// pointer to the DLL_SECURITY_PACKAGE structure. We don't know what that TLS
	// slot index is, so we can't find the right pointer.
	//

	ASSERT (DllSecPkg != NtCurrentThread());

	if (DllSecPkg == NtCurrentThread()) {
		return FALSE;
	}

	IsValid = FALSE;

	try {
		UNICODE_STRING ExpectedPackageName;
		UNICODE_STRING ExpectedPackageComment;

		if (DllSecPkg->Version != SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION ||
			DllSecPkg->RpcId != SECPKG_ID_NONE ||
			DllSecPkg->TokenSize != KXSCHANL_PACKAGE_INFO_CB_MAX_TOKEN) {

			leave;
		}

		RtlInitConstantUnicodeString(&ExpectedPackageName, KXSCHANL_NAME_W);
		RtlInitConstantUnicodeString(&ExpectedPackageComment, KXSCHANL_DESC_W);

		if (!RtlEqualUnicodeString(&DllSecPkg->PackageName, &ExpectedPackageName, FALSE) ||
			!RtlEqualUnicodeString(&DllSecPkg->PackageComment, &ExpectedPackageComment, FALSE)) {

			leave;
		}

		if (!StringEqualA(DllSecPkg->PackageNameA, KXSCHANL_NAME_A) || 
			!StringEqualA(DllSecPkg->PackageCommentA, KXSCHANL_DESC_A)) {

			leave;
		}

		if (DllSecPkg->PackageNameACch != sizeof(KXSCHANL_NAME_A) ||
			DllSecPkg->PackageCommentACch != sizeof(KXSCHANL_DESC_A)) {

			leave;
		}

		if (DllSecPkg->FunctionTableA == NULL ||
			DllSecPkg->FunctionTableW == NULL ||
			DllSecPkg->FunctionTable == NULL ||
			DllSecPkg->FunctionTable != DllSecPkg->FunctionTableW) {

			leave;
		}

		IsValid = TRUE;
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) {
		// Probably some invalid/undefined pointer caused by not being invoked
		// through the SSPI infrastructure.
		NOTHING;
	}

	return IsValid;
}

VOID SppFixupFunctionPointers(
	IN	PCredHandle	CredentialHandle,
	IN	PVOID		RetAddr)
{
	STATIC VOLATILE BOOLEAN AlreadyFixedUp = FALSE;
	PDLL_SECURITY_PACKAGE DllSecPkg;

	if (AlreadyFixedUp || InterlockedCompareExchange8(&AlreadyFixedUp, TRUE, FALSE) == TRUE) {
		// Already done it.
		return;
	}

	ASSERT (CredentialHandle != NULL);

	DllSecPkg = (PDLL_SECURITY_PACKAGE) CredentialHandle->dwLower;

	if (DllSecPkg == NULL) {
		// No need to do anything. Maybe we were not invoked through the SSPI
		// infrastructure.
		return;
	}

	if (!AshModuleBaseNameIs(RetAddr, L"sspicli.dll")) {
		// Not invoked through SSPI infrastructure
		return;
	}

	ASSERT (SppValidDllSecPkg(DllSecPkg));

	if (!SppValidDllSecPkg(DllSecPkg)) {
		// We found something we don't like, probably not the right structure
		return;
	}

	//
	// When you call EncryptMessage and DecryptMessage in sspicli.dll (or secur32.dll),
	// it actually calls Reserved3 and Reserved4 for some reason. No idea why they
	// name it as reserved if it's actually used.
	//

	DllSecPkg->FunctionTableA->Reserved3 = SpEncryptMessage;
	DllSecPkg->FunctionTableA->Reserved4 = SpDecryptMessage;
	DllSecPkg->FunctionTableA->EncryptMessage = SpEncryptMessage;
	DllSecPkg->FunctionTableA->DecryptMessage = SpDecryptMessage;

	DllSecPkg->FunctionTableW->Reserved3 = SpEncryptMessage;
	DllSecPkg->FunctionTableW->Reserved4 = SpDecryptMessage;
	DllSecPkg->FunctionTableW->EncryptMessage = SpEncryptMessage;
	DllSecPkg->FunctionTableW->DecryptMessage = SpDecryptMessage;

	ASSERT (DllSecPkg->FunctionTable == DllSecPkg->FunctionTableW);
}