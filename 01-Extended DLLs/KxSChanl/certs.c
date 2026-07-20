///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     certs.c
//
// Abstract:
//
//     Contains utility functions for dealing with certificates.
//
// Author:
//
//     vxiiduu (14-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               14-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Load the root certificate bundle into the given KxSchanl credential.
//
BOOLEAN SppLoadRootCertificates(
	IN OUT	PKXSCHANL_CREDENTIAL	Credential)
{
	WCHAR BundlePath[MAX_PATH];

	StringCchCopy(BundlePath, ARRAYSIZE(BundlePath), KexData->KexDir.Buffer);
	PathCchAppend(BundlePath, ARRAYSIZE(BundlePath), L"Certificates\\ROOT.sst");

	Credential->RootCertStore = CertOpenStore(
		CERT_STORE_PROV_FILENAME,
		0,
		0,
		CERT_STORE_OPEN_EXISTING_FLAG | CERT_STORE_READONLY_FLAG | CERT_STORE_SHARE_CONTEXT_FLAG,
		BundlePath);

	ASSERT (Credential->RootCertStore != NULL);

	//
	// Don't fail if we couldn't open ROOT.sst.
	// There is a fallback in TlspCheckServerCertificateOk which uses the system
	// root store if Credential->RootCertStore is NULL.
	//

	return TRUE;
}