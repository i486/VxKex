#include "buildcfg.h"
#include "kxbasep.h"
#include <WinDNS.h>

#include "dnsp.h"

//
// This DnsQueryEx implementation mainly exists to support Qt6Network.
// It does not support asynchronous queries. If we need to support any
// extra functionality in the future, have a look into DnsQueryExW which
// is present in nt5src - this has the capabilities to do everything we
// want, but requires a lot more effort and research since it's not
// documented.
//
KXBASEAPI DNS_STATUS WINAPI DnsQueryEx(
	IN		PDNS_QUERY_REQUEST	Request,
	IN OUT	PDNS_QUERY_RESULT	Result,
	IN OUT	PDNS_QUERY_CANCEL	Cancel OPTIONAL)
{
	DNS_STATUS DnsStatus;

	ASSERT (Request != NULL);
	ASSERT (Result != NULL);
	ASSERT (Cancel == NULL);
	ASSERT (Request->Version == 1);
	ASSERT (Request->QueryCompletionCallback == NULL);
	ASSERT (Result->Version == 1);

	//
	// Validate parameters.
	//

	if (!Request || !Result) {
		return ERROR_INVALID_PARAMETER;
	}

	KexLogDebugEvent(
		L"DnsQueryEx called\r\n\r\n"
		L"Request->Version:                 %d\r\n"
		L"Request->QueryName:               %s\r\n"
		L"Request->QueryType:               %hu\r\n"
		L"Request->QueryOptions:            0x%016I64u\r\n"
		L"Request->DnsServerList:           0x%p\r\n"
		L"Request->InterfaceIndex:          %d\r\n"
		L"Request->QueryCompletionCallback: 0x%p\r\n"
		L"Request->QueryContext:            0x%p",
		Request->Version,
		Request->QueryName,
		Request->QueryType,
		Request->QueryOptions,
		Request->DnsServerList,
		Request->InterfaceIndex,
		Request->QueryCompletionCallback,
		Request->QueryContext);

	if (Request->Version != 1 || Result->Version != 1) {
		return ERROR_INVALID_PARAMETER;
	}

	if (Request->QueryCompletionCallback) {
		return DNS_RCODE_NOT_IMPLEMENTED;
	}

	if (Cancel) {
		return DNS_RCODE_NOT_IMPLEMENTED;
	}

	//
	// Perform the query.
	// Note: the current implementation ignores DnsServerList and InterfaceIndex.
	//

	DnsStatus = DnsQuery(
		Request->QueryName,
		Request->QueryType,
		(ULONG) Request->QueryOptions,
		NULL,
		&Result->QueryRecords,
		NULL);

	//
	// Fill out remaining fields of DNS_QUERY_RESULT.
	//

	Result->QueryStatus = DnsStatus;
	Result->QueryOptions = Request->QueryOptions;
	Result->Reserved = NULL;

	return DnsStatus;
}