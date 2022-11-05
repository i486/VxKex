///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     NtDll.h
//
// Abstract:
//
//     VxKex base API
//
// Author:
//
//     vxiiduu (11-Oct-2022)
//
// Revision History:
//
//     vxiiduu               11-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexComm.h>
#include <KexLog.h>

#ifndef KEXAPI
#  pragma comment(lib, "KexDll.lib")
#  define KEXAPI DECLSPEC_IMPORT
#endif

#ifndef KEX_COMPONENT
#  error You must define a Unicode component name as the KEX_COMPONENT macro.
#endif

//
// Uncomment the macro definition to keep debug logging enabled in release
// builds.
//
// Note that this only affects debug logs made through the KexSrvLogDebugEvent
// macro.
//
//#define RELEASE_DEBUGLOGS_ENABLED

#define REG_RESTRICT_NONE						(1 << REG_NONE)
#define REG_RESTRICT_SZ							(1 << REG_SZ)
#define REG_RESTRICT_EXPAND_SZ					(1 << REG_EXPAND_SZ)
#define REG_RESTRICT_BINARY						(1 << REG_BINARY)
#define REG_RESTRICT_DWORD						(1 << REG_DWORD)
#define REG_RESTRICT_DWORD_BE					(1 << REG_DWORD_BIG_ENDIAN)
#define REG_RESTRICT_LINK						(1 << REG_LINK)
#define REG_RESTRICT_MULTI_SZ					(1 << REG_MULTI_SZ)
#define REG_RESTRICT_RESOURCE_LIST				(1 << REG_RESOURCE_LIST)
#define REG_RESTRICT_FULL_RESOURCE_DESCRIPTOR	(1 << REG_FULL_RESOURCE_DESCRIPTOR)
#define REG_RESTRICT_RESOURCE_REQUIREMENTS_LIST	(1 << REG_RESOURCE_REQUIREMENTS_LIST)
#define REG_RESTRICT_QWORD						(1 << REG_QWORD)

#define LEGAL_REG_RESTRICT_MASK \
	(REG_RESTRICT_NONE | \
	 REG_RESTRICT_SZ | \
	 REG_RESTRICT_EXPAND_SZ | \
	 REG_RESTRICT_BINARY | \
	 REG_RESTRICT_DWORD | \
	 REG_RESTRICT_DWORD_BE | \
	 REG_RESTRICT_LINK | \
	 REG_RESTRICT_MULTI_SZ | \
	 REG_RESTRICT_RESOURCE_LIST | \
	 REG_RESTRICT_FULL_RESOURCE_DESCRIPTOR | \
	 REG_RESTRICT_RESOURCE_REQUIREMENTS_LIST | \
	 REG_RESTRICT_QWORD)

#define REG_RESTRICT_ANY LEGAL_REG_RESTRICT_MASK

#define QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST 1

#define QUERY_KEY_MULTIPLE_VALUE_VALID_MASK \
	(QUERY_KEY_MULTIPLE_VALUE_FAIL_FAST)

#define KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS 1
#define KEX_RTL_STRING_MAPPER_FLAGS_VALID_MASK (KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS)

#define NTSTATUS_SUCCESS			(0x00000000L)
#define NTSTATUS_INFORMATIONAL		(0x40000000L)
#define NTSTATUS_WARNING			(0x80000000L)
#define NTSTATUS_ERROR				(0xC0000000L)
#define NTSTATUS_CUSTOMER			(0x20000000L)
#define DEFINE_KEX_NTSTATUS(Severity, Number) (NTSTATUS_CUSTOMER | Severity | Number)

#define STATUS_USER_DISABLED					DEFINE_KEX_NTSTATUS(NTSTATUS_INFORMATIONAL, 0)

#define STATUS_IMAGE_NO_IMPORT_DIRECTORY		DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 0)
#define STATUS_STRING_MAPPER_ENTRY_NOT_FOUND	DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 1)
#define STATUS_REG_DATA_TYPE_MISMATCH			DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 2)
#define STATUS_KEXDLL_INITIALIZATION_FAILURE	DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 3)

typedef struct _KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY {
	IN		CONST UNICODE_STRING			ValueName;
	OUT		NTSTATUS						Status;
	IN OUT	ULONG							ValueDataCb;

	//
	// If you don't want to store data outside the structure, and if
	// you are querying value data that you know should be 8 bytes or
	// less, then you can set ValueData to point to ValueDataRawBytes.
	// Make sure you set ValueDataCb and ValueDataTypeRestrict as
	// appropriate.
	//
	union {
		OUT		PVOID							ValueData;
		OUT		ULONG							ValueDataAsDword;
		OUT		ULONGLONG						ValueDataAsQword;
		OUT		BYTE							ValueDataRawBytes[8];
	};

	IN		ULONG							ValueDataTypeRestrict;
	OUT		ULONG							ValueDataType;
} TYPEDEF_TYPE_NAME(KEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY);

typedef struct _KEX_RTL_STRING_MAPPER {
	RTL_DYNAMIC_HASH_TABLE	HashTable;
	ULONG					Flags;
} TYPEDEF_TYPE_NAME(KEX_RTL_STRING_MAPPER);

typedef struct _KEX_RTL_STRING_MAPPER_ENTRY {
	UNICODE_STRING	Key;
	UNICODE_STRING	Value;
} TYPEDEF_TYPE_NAME(KEX_RTL_STRING_MAPPER_ENTRY);

typedef struct _KEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY {
	RTL_DYNAMIC_HASH_TABLE_ENTRY	HashTableEntry;
	UNICODE_STRING					Key;
	UNICODE_STRING					Value;
} TYPEDEF_TYPE_NAME(KEX_RTL_STRING_MAPPER_HASH_TABLE_ENTRY);

//
// External code should access this data through a pointer because
// it is more efficient.
//
// Example:
//   PCKEX_PROCESS_DATA KexData = &_KexData;
//
extern KEX_PROCESS_DATA _KexData;

KEXAPI NTSTATUS NTAPI KexDataInitialize(
	OUT	PPKEX_PROCESS_DATA	KexDataOut OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlPathFindFileName(
	IN	PCUNICODE_STRING Path,
	OUT	PUNICODE_STRING FileName);

KEXAPI NTSTATUS NTAPI KexRtlGetProcessImageBaseName(
	OUT	PUNICODE_STRING	FileName);

KEXAPI NTSTATUS NTAPI KexRtlQueryKeyValueData(
	IN		HANDLE				KeyHandle,
	IN		PCUNICODE_STRING	ValueName,
	IN OUT	PULONG				ValueDataCb,
	OUT		PVOID				ValueData OPTIONAL,
	IN		ULONG				ValueDataTypeRestrict,
	OUT		PULONG				ValueDataType OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlQueryKeyMultipleValueData(
	IN		HANDLE												KeyHandle,
	IN		PKEX_RTL_QUERY_KEY_MULTIPLE_VARIABLE_TABLE_ENTRY	QueryTable,
	IN OUT	PULONG												NumberOfQueryTableElements,
	IN		ULONG												Flags);

KEXAPI BOOLEAN NTAPI KexRtlUnicodeStringEndsWith(
	IN	PCUNICODE_STRING	String,
	IN	PCUNICODE_STRING	EndsWith,
	IN	BOOLEAN				CaseInsensitive);

KEXAPI PWCHAR NTAPI KexRtlFindUnicodeSubstring(
	PCUNICODE_STRING	Haystack,
	PCUNICODE_STRING	Needle,
	BOOLEAN				CaseInsensitive);

KEXAPI VOID NTAPI KexRtlAdvanceUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			AdvanceCb);

KEXAPI VOID NTAPI KexRtlRetreatUnicodeString(
	OUT	PUNICODE_STRING	String,
	IN	USHORT			RetreatCb);

KEXAPI PVOID NTAPI KexRtlGetNativeSystemDllBase(
	VOID);

KEXAPI NTSTATUS NTAPI KexRtlMiniGetProcedureAddress(
	IN	PVOID	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PPVOID	ProcedureAddress);

KEXAPI ULONG NTAPI KexRtlRemoteProcessBitness(
	IN	HANDLE	ProcessHandle);

NTSTATUS NTAPI KexRtlWriteProcessMemory(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	Destination,
	IN	PVOID		Source,
	IN	SIZE_T		Cb);

KEXAPI NTSTATUS NTAPI KexRtlCreateStringMapper(
	OUT		PPKEX_RTL_STRING_MAPPER		StringMapper,
	IN		ULONG						Flags OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlDeleteStringMapper(
	IN		PPKEX_RTL_STRING_MAPPER		StringMapper);

KEXAPI NTSTATUS NTAPI KexRtlInsertEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER		StringMapper,
	IN		PCUNICODE_STRING			Key,
	IN		PCUNICODE_STRING			Value OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlLookupEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key,
	OUT		PUNICODE_STRING					Value OPTIONAL);

KEXAPI NTSTATUS NTAPI KexRtlRemoveEntryStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		PCUNICODE_STRING				Key);

KEXAPI NTSTATUS NTAPI KexRtlApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	PUNICODE_STRING					KeyToValue);

KEXAPI NTSTATUS NTAPI KexRtlInsertMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN		KEX_RTL_STRING_MAPPER_ENTRY		Entries[],
	IN		ULONG							EntryCount);

KEXAPI NTSTATUS NTAPI KexRtlLookupMultipleEntriesStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	KEX_RTL_STRING_MAPPER_ENTRY		Entries[],
	IN		ULONG							EntryCount);

KEXAPI NTSTATUS NTAPI KexRtlBatchApplyStringMapper(
	IN		PKEX_RTL_STRING_MAPPER			StringMapper,
	IN OUT	UNICODE_STRING					KeyToValue[],
	IN		ULONG							KeyToValueCount);

#ifdef KEX_ARCH_X64
#  define KexRtlCurrentProcessBitness() (64)
#else
#  define KexRtlCurrentProcessBitness() (32)
#endif

#ifdef KEX_ARCH_X64
#  define KexRtlOperatingSystemBitness() (64)
#else
#  define KexRtlOperatingSystemBitness() (SharedUserData->SystemCall != 0 ? 32 : 64)
#endif

#define KexRtlUpdateNullTerminatedUnicodeStringLength(UnicodeString) ((UnicodeString)->Length = (USHORT) (wcslen((UnicodeString)->Buffer) << 1))
#define KexRtlUpdateNullTerminatedAnsiStringLength(AnsiString) ((AnsiString)->Length = (USHORT) wcslen((AnsiString)->Buffer))
#define KexRtlUnicodeStringCch(UnicodeString) ((UnicodeString)->Length / sizeof(WCHAR))
#define KexRtlUnicodeStringBufferCch(UnicodeString) ((UnicodeString)->MaximumLength / sizeof(WCHAR))
#define KexRtlAnsiStringCch(AnsiString) ((AnsiString)->Length)
#define KexRtlAnsiStringBufferCch(AnsiString) ((AnsiString)->MaximumLength)
#define KexRtlEndOfUnicodeString(UnicodeString) ((UnicodeString)->Buffer + KexRtlUnicodeStringCch(UnicodeString))
#define KexRtlCopyMemory(Destination, Source, Cb) __movsb((PUCHAR) (Destination), (PUCHAR) (Source), (Cb))

KEXAPI NTSTATUS NTAPI KexSrvOpenChannel(
	OUT	PHANDLE	ChannelHandle);

KEXAPI NTSTATUS NTAPI KexSrvSendMessage(
	IN	HANDLE				ChannelHandle,
	IN	PKEX_IPC_MESSAGE	Message);

KEXAPI NTSTATUS NTAPI KexSrvNotifyProcessStart(
	IN	HANDLE				ChannelHandle,
	IN	PCUNICODE_STRING	ApplicationName);

#define KexSrvLogEvent(Severity, ...) \
	KexSrvLogEventEx(KexData->SrvChannel, Severity, __LINE__, KEX_COMPONENT, __FILEW__, __FUNCTIONW__, __VA_ARGS__)

#define KexSrvLogCriticalEvent(...)		KexSrvLogEvent(LogSeverityCritical, __VA_ARGS__)
#define KexSrvLogErrorEvent(...)		KexSrvLogEvent(LogSeverityError, __VA_ARGS__)
#define KexSrvLogWarningEvent(...)		KexSrvLogEvent(LogSeverityWarning, __VA_ARGS__)
#define KexSrvLogInformationEvent(...)	KexSrvLogEvent(LogSeverityInformation, __VA_ARGS__)
#define KexSrvLogDetailEvent(...)		KexSrvLogEvent(LogSeverityDetail, __VA_ARGS__)

#if defined(_DEBUG) || defined(RELEASE_DEBUGLOGS_ENABLED)
#  define KexSrvLogDebugEvent(...)		KexSrvLogEvent(LogSeverityDebug, __VA_ARGS__)
#else
#  define KexSrvLogDebugEvent(...)
#endif

KEXAPI NTSTATUS CDECL KexSrvLogEventEx(
	IN	HANDLE		ChannelHandle,
	IN	VXLSEVERITY	Severity,
	IN	ULONG		SourceLine,
	IN	PCWSTR		SourceComponent,
	IN	PCWSTR		SourceFile,
	IN	PCWSTR		SourceFunction,
	IN	PCWSTR		Format,
	IN	...);

#ifdef KEX_ARCH_X64
#  define BASIC_HOOK_LENGTH 14
#  define BASIC_HOOK_DESTINATION_OFFSET 6
#else
#  define BASIC_HOOK_LENGTH 6
#  define BASIC_HOOK_DESTINATION_OFFSET 1
#endif

typedef struct _KEX_BASIC_HOOK_CONTEXT {
	PVOID OriginalApiAddress;
	BYTE OriginalInstructions[BASIC_HOOK_LENGTH];
} TYPEDEF_TYPE_NAME(KEX_BASIC_HOOK_CONTEXT);

KEXAPI NTSTATUS NTAPI KexHkInstallBasicHook(
	IN		PVOID					ApiAddress,
	IN		PVOID					RedirectedAddress,
	OUT		PKEX_BASIC_HOOK_CONTEXT	HookContext OPTIONAL);

KEXAPI NTSTATUS NTAPI KexHkRemoveBasicHook(
	IN		PKEX_BASIC_HOOK_CONTEXT	HookContext);

//
// KexNt* syscall stubs (see syscal32.c and syscal64.asm)
//

KEXAPI NTSTATUS NTAPI KexNtQuerySystemTime(
	OUT		PULONGLONG	CurrentTime);

KEXAPI NTSTATUS NTAPI KexNtCreateUserProcess(
	OUT		PHANDLE							ProcessHandle,
	OUT		PHANDLE							ThreadHandle,
	IN		ACCESS_MASK						ProcessDesiredAccess,
	IN		ACCESS_MASK						ThreadDesiredAccess,
	IN		POBJECT_ATTRIBUTES				ProcessObjectAttributes OPTIONAL,
	IN		POBJECT_ATTRIBUTES				ThreadObjectAttributes OPTIONAL,
	IN		ULONG							ProcessFlags,
	IN		ULONG							ThreadFlags,
	IN		PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN OUT	PPS_CREATE_INFO					CreateInfo,
	IN		PPS_ATTRIBUTE_LIST				AttributeList OPTIONAL);

KEXAPI NTSTATUS NTAPI KexNtProtectVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

KEXAPI NTSTATUS NTAPI KexNtAllocateVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

KEXAPI NTSTATUS NTAPI KexNtQueryVirtualMemory(
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

KEXAPI NTSTATUS NTAPI KexNtFreeVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

KEXAPI NTSTATUS NTAPI KexNtOpenKeyEx(
	OUT		PHANDLE						KeyHandle,
	IN		ACCESS_MASK					DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes,
	IN		ULONG						OpenOptions);

KEXAPI NTSTATUS NTAPI KexNtQueryObject(
	IN		HANDLE						ObjectHandle,
	IN		OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	OUT		PVOID						ObjectInformation,
	IN		ULONG						Length,
	OUT		PULONG						ReturnLength OPTIONAL);

KEXAPI NTSTATUS NTAPI KexNtOpenFile(
	OUT		PHANDLE				FileHandle,
	IN		ACCESS_MASK			DesiredAccess,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		ULONG				ShareAccess,
	IN		ULONG				OpenOptions);

KEXAPI NTSTATUS NTAPI KexNtWriteFile(
	IN		HANDLE				FileHandle,
	IN		HANDLE				Event OPTIONAL,
	IN		PIO_APC_ROUTINE		ApcRoutine OPTIONAL,
	IN		PVOID				ApcContext OPTIONAL,
	OUT		PIO_STATUS_BLOCK	IoStatusBlock,
	IN		PVOID				Buffer,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		ByteOffset OPTIONAL,
	IN		PULONG				Key OPTIONAL);

KEXAPI NTSTATUS NTAPI KexNtRaiseHardError(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);