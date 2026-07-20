#include "buildcfg.h"
#include "kexdllp.h"

//
// This function is called with a namespace string such as
// L"ext-ms-win-firewallapi-webproxy-l1-1-0".
//
NTSTATUS NTAPI ApiSetQueryApiSetPresence(
	IN	PUNICODE_STRING	Namespace,
	OUT	PBOOLEAN		Present)
{
	UNICODE_STRING Api;
	UNICODE_STRING Ext;

	KexLogDetailEvent(L"ApiSetQueryApiSetPresence called with \"%wZ\"", Namespace);

	//
	// Ensure it begins with api- or ext-
	//

	RtlInitConstantUnicodeString(&Api, L"api-");
	RtlInitConstantUnicodeString(&Ext, L"ext-");

	if (!RtlPrefixUnicodeString(&Api, Namespace, TRUE) &&
		!RtlPrefixUnicodeString(&Ext, Namespace, TRUE)) {

		return STATUS_INVALID_PARAMETER;
	}

	//
	// Check for existence of a DLL rewrite entry of this API set.
	//

	*Present = KexDoesDllRewriteEntryExist(Namespace);
	return STATUS_SUCCESS;
}