///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexDll.h
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
//     vxiiduu               06-Nov-2022  Refactor and create KexLdr* section
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <KexComm.h>

#ifndef KEXAPI
#  pragma comment(lib, "KexDll.lib")
#  define KEXAPI DECLSPEC_IMPORT
#endif

#ifndef KEX_COMPONENT
#  error You must define a Unicode component name as the KEX_COMPONENT macro.
#endif

#pragma region Macros and Structure Definitions

//
// Uncomment the macro definition to keep debug logging enabled in release
// builds.
//
// Note that this only affects debug logs made through the KexLogDebugEvent
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
#define STATUS_VERSION_MISMATCH					DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 4)
#define STATUS_SOURCE_APPLICATION_MISMATCH		DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 5)
#define STATUS_TOO_MANY_INDICES					DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 6)
#define STATUS_INVALID_OPEN_MODE				DEFINE_KEX_NTSTATUS(NTSTATUS_ERROR, 7)

#define KEXDATA_FLAG_PROPAGATED			1

#define KEX_STRONGSPOOF_SHAREDUSERDATA	1
#define KEX_STRONGSPOOF_REGISTRY		2

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

#define VXLL_VERSION 3

typedef enum _VXLLOGINFOCLASS {
	LogLibraryVersion,
	LogNumberOfCriticalEvents,
	LogNumberOfErrorEvents,
	LogNumberOfWarningEvents,
	LogNumberOfInformationEvents,
	LogNumberOfDetailEvents,
	LogNumberOfDebugEvents,
	LogTotalNumberOfEvents,
	LogSourceApplication,
	MaxLogInfoClass
} VXLLOGINFOCLASS;

typedef enum _VXLSEVERITY {
	LogSeverityInvalidValue = -1,
	LogSeverityCritical,
	LogSeverityError,
	LogSeverityWarning,
	LogSeverityInformation,
	LogSeverityDetail,
	LogSeverityDebug,
	LogSeverityMaximumValue
} VXLSEVERITY;

// All UNICODE_STRINGs in VXLLOGENTRY are guaranteed to be null terminated.
// So you can pass the buffers directly to Win32 functions.
typedef struct _VXLLOGENTRY {
	UNICODE_STRING			TextHeader;
	UNICODE_STRING			Text;
	UCHAR					SourceComponentIndex;
	UCHAR					SourceFileIndex;
	UCHAR					SourceFunctionIndex;
	ULONG					SourceLine;
	CLIENT_ID				ClientId;
	VXLSEVERITY				Severity;
	SYSTEMTIME				Time;
} TYPEDEF_TYPE_NAME(VXLLOGENTRY);

typedef struct _VXLLOGFILEHEADER {
	CHAR		Magic[4];
	ULONG		Version;
	ULONG		EventSeverityTypeCount[LogSeverityMaximumValue];
	WCHAR		SourceApplication[32];
	WCHAR		SourceComponents[48][16];
	WCHAR		SourceFiles[96][16];
	WCHAR		SourceFunctions[96][32];
	BOOLEAN		Dirty;
} TYPEDEF_TYPE_NAME(VXLLOGFILEHEADER);

typedef struct _VXLLOGFILEENTRY {
	union {
		FILETIME	Time;
		LONGLONG	Time64;
	};

	// Do not directly use CLIENT_ID here since its size varies with
	// bitness. (contains HANDLE members)
	ULONG		ProcessId;
	ULONG		ThreadId;

	VXLSEVERITY	Severity;
	ULONG		SourceLine;
	UCHAR		SourceComponentIndex;
	UCHAR		SourceFileIndex;
	UCHAR		SourceFunctionIndex;

	USHORT		TextHeaderCch;
	USHORT		TextCch;

	WCHAR		Text[];
} TYPEDEF_TYPE_NAME(VXLLOGFILEENTRY);

// index cache (EntryIndexToFileOffset) makes reading and sorting the
// log file faster. Without it, writing the log file is very fast but
// read and export performance is unacceptably bad.
typedef struct _VXLCONTEXT {
	RTL_SRWLOCK				Lock;
	HANDLE					FileHandle;
	PULONG					EntryIndexToFileOffset;	// only populated when file is opened for READ ONLY, otherwise NULL

	union {
		PVXLLOGFILEHEADER		Header;
		PBYTE					MappedFile;			// only populated in READ ONLY mode, otherwise NULL
		PVOID					MappedSection;		// ^
	};

	ULONG					OpenMode;				// GENERIC_READ or GENERIC_WRITE
} TYPEDEF_TYPE_NAME(VXLCONTEXT);

typedef PVXLCONTEXT TYPEDEF_TYPE_NAME(VXLHANDLE);

typedef enum _KEX_WIN_VER_SPOOF {
	WinVerSpoofNone,	// do not spoof
	WinVerSpoofWin7,	// for testing - not useful in practice
	WinVerSpoofWin8,
	WinVerSpoofWin8Point1,
	WinVerSpoofWin10,
	WinVerSpoofWin11,
	WinVerSpoofMax		// should always be the last value
} TYPEDEF_TYPE_NAME(KEX_WIN_VER_SPOOF);

//
// These variable names are present under the IFEO key for each executable
// with a KEX_ prefix. For example:
//
//   HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\
//   Image File Execution Options\application.exe\{id}\KEX_WinVerSpoof
//

typedef struct _KEX_IFEO_PARAMETERS {
	ULONG				DisableForChild;
	ULONG				DisableAppSpecific;
	KEX_WIN_VER_SPOOF	WinVerSpoof;
	ULONG				StrongVersionSpoof;				// KEX_STRONGSPOOF_*
} TYPEDEF_TYPE_NAME(KEX_IFEO_PARAMETERS);

//
// A KEX_PROCESS_DATA structure is exported from KexDll under the name _KexData.
//

typedef struct _KEX_PROCESS_DATA {
	ULONG					Flags;						// KEXDATA_FLAG_*
	ULONG					InstalledVersion;			// version dword of VxKex
	KEX_IFEO_PARAMETERS		IfeoParameters;
	UNICODE_STRING			WinDir;						// e.g. C:\Windows
	UNICODE_STRING			KexDir;						// e.g. C:\Program Files\VxKex
	UNICODE_STRING			LogDir;
	UNICODE_STRING			ImageBaseName;				// e.g. program.exe
	HANDLE					SrvChannel;
	VXLHANDLE				LogHandle;
	PVOID					KexDllBase;
	PVOID					SystemDllBase;				// NTDLL base
} TYPEDEF_TYPE_NAME(KEX_PROCESS_DATA);

#pragma endregion

#pragma region KexData* functions

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

#pragma endregion

#pragma region KexRtl* functions

KEXAPI NTSTATUS NTAPI KexRtlPathFindFileName(
	IN	PCUNICODE_STRING Path,
	OUT	PUNICODE_STRING FileName);

KEXAPI NTSTATUS NTAPI KexRtlPathRemoveExtension(
	IN	PCUNICODE_STRING	Path,
	OUT	PUNICODE_STRING		PathWithoutExtension);

KEXAPI BOOLEAN NTAPI KexRtlPathReplaceIllegalCharacters(
	IN		PCUNICODE_STRING	Path,
	OUT		PUNICODE_STRING		SanitizedPath,
	IN		WCHAR				ReplacementCharacter OPTIONAL,
	IN		BOOLEAN				AllowPathSeparators);

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

KEXAPI NTSTATUS NTAPI KexRtlShiftUnicodeString(
	IN OUT	PUNICODE_STRING	String,
	IN		USHORT			ShiftCch,
	IN		WCHAR			LeftFillCharacter OPTIONAL);

KEXAPI ULONG NTAPI KexRtlRemoteProcessBitness(
	IN	HANDLE	ProcessHandle);

KEXAPI NTSTATUS NTAPI KexRtlWriteProcessMemory(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	Destination,
	IN	PVOID		Source,
	IN	SIZE_T		Cb);

KEXAPI NTSTATUS NTAPI KexRtlCreateDirectoryRecursive(
	OUT	PHANDLE				DirectoryHandle,
	IN	ACCESS_MASK			DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	ULONG				ShareAccess);

KEXAPI PCWSTR NTAPI KexRtlNtStatusToString(
	IN	NTSTATUS	Status);

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
	IN		PKEX_RTL_STRING_MAPPER				StringMapper,
	IN		CONST KEX_RTL_STRING_MAPPER_ENTRY	Entries[],
	IN		ULONG								EntryCount);

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

#define KEX_RTL_BAG(Type) struct { ULONG NumberOfItems; ULONG MaximumNumberOfItems; Type *Items; }
typedef KEX_RTL_BAG(VOID) TYPEDEF_TYPE_NAME(KEX_RTL_GENERIC_BAG);

FORCEINLINE VOID KexRtlInitializeBagHelper(
	IN OUT	PKEX_RTL_GENERIC_BAG	Bag,
	IN		ULONG					ElementSize)
{
	Bag->Items = RtlAllocateHeap(
		RtlProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS,
		12 * ElementSize);
}
 
FORCEINLINE VOID KexRtlResizeBagHelper(
	IN OUT	PKEX_RTL_GENERIC_BAG	Bag,
	IN		ULONG					ElementSize)
{
	if (Bag->NumberOfItems > Bag->MaximumNumberOfItems) {
		Bag->MaximumNumberOfItems *= 2;
	} else if (Bag->NumberOfItems < Bag->MaximumNumberOfItems / 4) {
		Bag->MaximumNumberOfItems /= 2;	
	} else {
		return;
	}

	Bag->Items = RtlReAllocateHeap(
		RtlProcessHeap(),
		HEAP_GENERATE_EXCEPTIONS,
		Bag->Items,
		Bag->MaximumNumberOfItems * ElementSize);
}

#define KexRtlInitializeBag(Bag, ItemCount) KexRtlInitializeBagHelper((PKEX_RTL_GENERIC_BAG) (Bag), sizeof((Bag)->Items[0]) * (ItemCount ? ItemCount : 4))
#define KexRtlDeleteBag(Bag) do { \
		RtlFreeHeap(RtlProcessHeap(), 0, (Bag)->Items); \
		RtlZeroMemory((Bag), sizeof(KEX_RTL_GENERIC_BAG)); \
	} while (0)

#define KexRtlInsertItemBag(Bag, Item) \
	do { \
		++(Bag)->NumberOfItems; \
		KexRtlResizeBagHelper((PKEX_RTL_GENERIC_BAG) (Bag), sizeof((Bag)->Items[0])); \
		(Bag)->Items[(Bag)->NumberOfItems - 1] = Item; \
	} while (0)

#define KexRtlRemoveItemBag(Bag, Index) \
	do { \
		(Bag)->Items[Index] = (Bag)->Items[--(Bag)->NumberOfItems]; \
		KexRtlResizeBagHelper((PKEX_RTL_GENERIC_BAG) (Bag), sizeof((Bag)->Items[0])); \
	} while(0)

#define ForEachBagItem(Bag, Item, Index) for (Index = 0; Index < (Bag)->NumberOfItems, Item = (Bag)->Items[Index]; ++Index)

#define ForEachArrayItem(Array, Index) for (Index = 0; Index < ARRAYSIZE(Array); ++Index)

#pragma endregion

#pragma region KexLdr* functions

KEXAPI PVOID NTAPI KexLdrGetNativeSystemDllBase(
	VOID);

KEXAPI NTSTATUS NTAPI KexLdrMiniGetProcedureAddress(
	IN	PVOID	DllBase,
	IN	PCSTR	ProcedureName,
	OUT	PPVOID	ProcedureAddress);

KEXAPI NTSTATUS NTAPI KexLdrFindDllInitRoutine(
	IN	PVOID	DllBase,
	OUT	PPVOID	InitRoutine);

NTSTATUS NTAPI KexLdrGetDllFullName(
	IN	PVOID			DllBase OPTIONAL,
	OUT	PUNICODE_STRING	DllFullPath);

KEXAPI NTSTATUS NTAPI KexLdrGetDllFullNameFromAddress(
	IN	PVOID			Address,
	OUT	PUNICODE_STRING	DllFullPath);

#pragma endregion

#pragma region KexSrv* functions

KEXAPI NTSTATUS NTAPI KexSrvOpenChannel(
	OUT	PHANDLE	ChannelHandle);

KEXAPI NTSTATUS NTAPI KexSrvSendMessage(
	IN	HANDLE				ChannelHandle,
	IN	PKEX_IPC_MESSAGE	Message);

KEXAPI NTSTATUS NTAPI KexSrvSendReceiveMessage(
	IN	HANDLE				ChannelHandle,
	IN	PKEX_IPC_MESSAGE	MessageToSend,
	OUT	PKEX_IPC_MESSAGE	MessageToReceive);

KEXAPI NTSTATUS NTAPI KexSrvNotifyProcessStart(
	IN	HANDLE				ChannelHandle,
	IN	PCUNICODE_STRING	ApplicationName);

#pragma endregion

#pragma region KexHk* functions

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

#pragma endregion

#pragma region KexNt* functions

KEXAPI NTSTATUS NTAPI KexNtQuerySystemTime(
	OUT		PLONGLONG	CurrentTime);

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

KEXAPI NTSTATUS NTAPI KexNtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

KEXAPI NTSTATUS NTAPI KexNtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

#pragma endregion

#pragma region Vxl* functions

//
// vxlopcl.c
//

KEXAPI NTSTATUS NTAPI VxlOpenLog(
	OUT		PVXLHANDLE			LogHandle,
	IN		PUNICODE_STRING		SourceApplication OPTIONAL,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	IN		ACCESS_MASK			DesiredAccess,
	IN		ULONG				CreateDisposition);

KEXAPI NTSTATUS NTAPI VxlCloseLog(
	IN OUT	PVXLHANDLE		LogHandle);

//
// vxlquery.c
//

KEXAPI NTSTATUS NTAPI VxlQueryInformationLog(
	IN		VXLHANDLE		LogHandle,
	IN		VXLLOGINFOCLASS	LogInformationClass,
	OUT		PVOID			Buffer OPTIONAL,
	IN OUT	PULONG			BufferSize);

//
// vxlwrite.c
//

#define VxlWriteLog(LogHandle, SourceComponent, Severity, ...) \
	VxlWriteLogEx( \
		LogHandle, \
		SourceComponent, \
		__FILEW__, \
		__LINE__, \
		__FUNCTIONW__, \
		Severity, \
		__VA_ARGS__)

#define KexLogEvent(Severity, ...) \
	VxlWriteLog(KexData->LogHandle, KEX_COMPONENT, Severity, __VA_ARGS__)

#define KexLogCriticalEvent(...)	KexLogEvent(LogSeverityCritical, __VA_ARGS__)
#define KexLogErrorEvent(...)		KexLogEvent(LogSeverityError, __VA_ARGS__)
#define KexLogWarningEvent(...)		KexLogEvent(LogSeverityWarning, __VA_ARGS__)
#define KexLogInformationEvent(...)	KexLogEvent(LogSeverityInformation, __VA_ARGS__)
#define KexLogDetailEvent(...)		KexLogEvent(LogSeverityDetail, __VA_ARGS__)

#if defined(_DEBUG) || defined(RELEASE_DEBUGLOGS_ENABLED)
#  define KexLogDebugEvent(...)		KexLogEvent(LogSeverityDebug, __VA_ARGS__)
#else
#  define KexLogDebugEvent(...)
#endif

KEXAPI NTSTATUS CDECL VxlWriteLogEx(
	IN		VXLHANDLE		LogHandle,
	IN		PCWSTR			SourceComponent OPTIONAL,
	IN		PCWSTR			SourceFile OPTIONAL,
	IN		ULONG			SourceLine,
	IN		PCWSTR			SourceFunction OPTIONAL,
	IN		VXLSEVERITY		Severity,
	IN		PCWSTR			Format,
	IN		...);

//
// vxlread.c
//

KEXAPI NTSTATUS NTAPI VxlReadLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndex,
	OUT		PVXLLOGENTRY	Entry);

KEXAPI NTSTATUS NTAPI VxlReadMultipleEntriesLog(
	IN		VXLHANDLE		LogHandle,
	IN		ULONG			LogEntryIndexStart,
	IN		ULONG			LogEntryIndexEnd,
	OUT		PVXLLOGENTRY	Entry[]);

//
// vxlsever.c
//

KEXAPI PCWSTR NTAPI VxlSeverityToText(
	IN		VXLSEVERITY		Severity,
	IN		BOOLEAN			LongDescription);

#pragma endregion