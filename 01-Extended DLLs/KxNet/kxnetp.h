#include "buildcfg.h"
#include <KexComm.h>
#include <WinDNS.h>
#include <WinHTTP.h>

EXTERN PKEX_PROCESS_DATA KexData;

#define DNS_ADDR_MAX_SOCKADDR_LENGTH 32

typedef struct _DNS_ADDR {
	CHAR		MaxSa[DNS_ADDR_MAX_SOCKADDR_LENGTH];
	ULONG		DnsAddrUserDword[8];
} TYPEDEF_TYPE_NAME(DNS_ADDR);

typedef struct _DNS_ADDR_ARRAY {
	ULONG		MaxCount;
	ULONG		NumberOfAddresses;
	ULONG		Tag;
	USHORT		Family;
	USHORT		Reserved1;
	ULONG		Flags;
	ULONG		MatchFlag;
	ULONG		Reserved2;
	ULONG		Reserved3;
	DNS_ADDR	Addresses[];
} TYPEDEF_TYPE_NAME(DNS_ADDR_ARRAY);

typedef struct _DNS_QUERY_RESULT {
	ULONG							Version;
	DNS_STATUS						QueryStatus;
	ULONGLONG						QueryOptions;
	PDNS_RECORD						QueryRecords;
	PVOID							Reserved;
} TYPEDEF_TYPE_NAME(DNS_QUERY_RESULT);

typedef VOID (CALLBACK *PDNS_QUERY_COMPLETION_ROUTINE) (
	IN		PVOID				QueryContext,
	IN OUT	PDNS_QUERY_RESULT	QueryResults);

typedef struct _DNS_QUERY_REQUEST {
	ULONG							Version;
	PCWSTR							QueryName;
	USHORT							QueryType;
	ULONGLONG						QueryOptions;
	PDNS_ADDR_ARRAY					DnsServerList;
	ULONG							InterfaceIndex;
	PDNS_QUERY_COMPLETION_ROUTINE	QueryCompletionCallback;
	PVOID							QueryContext;
} TYPEDEF_TYPE_NAME(DNS_QUERY_REQUEST);

typedef struct _DNS_QUERY_CANCEL {
	BYTE							Unknown[32];
} TYPEDEF_TYPE_NAME(DNS_QUERY_CANCEL);

typedef struct _DNS_RESULTS_BASIC {
	DNS_STATUS		Status;
	USHORT			ResponseCode;
	DNS_ADDR		ServerAddr;
} TYPEDEF_TYPE_NAME(DNS_RESULTS_BASIC);

#define DNS_MAX_ALIAS_COUNT 8

typedef struct _DNS_SOCKADDR_RESULTS {
	PWSTR			Name;
	PDNS_ADDR_ARRAY	AddressArray;
	PVOID			Hostent;
	ULONG			AliasCount;
	ULONG			Reserved;
	PWSTR			AliasArray[DNS_MAX_ALIAS_COUNT];
} TYPEDEF_TYPE_NAME(DNS_SOCKADDR_RESULTS);

typedef struct _DNS_EXTRA_INFO *PDNS_EXTRA_INFO;
#define DNS_MAX_PRIVATE_EXTRA_INFO_SIZE 72

typedef struct _DNS_EXTRA_INFO {
	PDNS_EXTRA_INFO	Next;
	ULONG			Id;

	union {
		CHAR					Flat[DNS_MAX_PRIVATE_EXTRA_INFO_SIZE];

		struct {
			DNS_STATUS	Status;
			USHORT		ResponseCode;
			IP4_ADDRESS	ServerIp4;
			IP6_ADDRESS	ServerIp6;
		} ResultsV1;

		DNS_RESULTS_BASIC		ResultsBasic;
		DNS_SOCKADDR_RESULTS	SaResults;

		PDNS_ADDR_ARRAY			ServerList;
		PIP4_ARRAY				ServerList4;
	};
} TYPEDEF_TYPE_NAME(DNS_EXTRA_INFO);

typedef struct _DNS_QUERY_INFO {
	ULONG			Size;
	ULONG			Version;
	PWSTR			QueryName;
	USHORT			QueryType;
	USHORT			ResponseCode;
	ULONG			Flags;
	DNS_STATUS		Status;
	DNS_CHARSET		CharSet;

	PDNS_RECORD		AnswerRecords;
	PDNS_RECORD		AliasRecords;
	PDNS_RECORD		AdditionalRecords;
	PDNS_RECORD		AuthorityRecords;

	HANDLE			EventHandle;
	PDNS_EXTRA_INFO	ExtraInfo;

	PVOID			ServerList;
	PIP4_ARRAY		ServerListIp4;

	PVOID			Message;
	PVOID			ReservedName;
} TYPEDEF_TYPE_NAME(DNS_QUERY_INFO);

// Undocumented function from dnsapi.dll
DNS_STATUS WINAPI DnsQueryExW(
	IN OUT	PDNS_QUERY_INFO	QueryInfo);