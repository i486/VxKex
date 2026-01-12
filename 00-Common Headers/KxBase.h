///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     BaseDll.h
//
// Abstract:
//
//     Win32 base API (i.e. kernel32 and kernelbase)
//
// Author:
//
//     vxiiduu (07-Nov-2022)
//
// Revision History:
//
//     vxiiduu               07-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <powrprof.h>
#include <cfgmgr32.h>
#include <bcrypt.h>

#pragma region Macro Definitions

#if !defined(KXBASEAPI) && defined(KEX_ENV_WIN32)
#  define KXBASEAPI
#  pragma comment(lib, "KxBase.lib")
#endif

#define PROCESS_CREATION_MITIGATION_POLICY_VALID_MASK \
	(PROCESS_CREATION_MITIGATION_POLICY_DEP_ENABLE | \
	 PROCESS_CREATION_MITIGATION_POLICY_DEP_ATL_THUNK_ENABLE | \
	 PROCESS_CREATION_MITIGATION_POLICY_SEHOP_ENABLE)

#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR		0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR		0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS			0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32			0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS		0x00001000
#define LOAD_LIBRARY_SAFE_CURRENT_DIRS			0x00002000

#define IOCTL_KSEC_RANDOM_FILL_BUFFER			CTL_CODE(FILE_DEVICE_KSEC, 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define APPMODEL_ERROR_NO_PACKAGE				15700L
#define APPMODEL_ERROR_NO_APPLICATION			15703L

#define DEVICE_NOTIFY_CALLBACK					2

#pragma endregion

#pragma region Structures and Typedefs

//
// With the exception of the type of BaseDllDirectoryLock being different, the
// location and position of all of these members is consistent across every known
// build of Windows 7.
//

typedef struct _KERNELBASE_GLOBAL_DATA {
	//
	// Example of what could be in here: "C:\Windows\system32;C:\Windows\system;C:\Windows;"
	// The buffer of this string is allocated from the RtlProcessHeap().
	// The trailing semicolon is important.
	//

	PUNICODE_STRING			BaseDefaultPath;
	PRTL_SRWLOCK			BaseDefaultPathLock;

	//
	// This is what gets accessed when you call SetDllDirectory and GetDllDirectory.
	// Note: BaseDllDirectoryLock points to an RTL_CRITICAL_SECTION in early (RTM & SP1)
	// builds of Windows 7, but to an RTL_SRWLOCK in later builds.
	//

	PUNICODE_STRING			BaseDllDirectory;
	PVOID					BaseDllDirectoryLock;

	//
	// This is what gets set when you call SetSearchPathMode.
	// The names of the valid flags start with BASE_SEARCH_PATH_*.
	//

	PULONG					BaseSearchPathMode;
	PRTL_SRWLOCK			BaseSearchPathModeLock;

	//
	// These are all function pointers to functions either in NTDLL or in
	// Kernelbase. For the most part, they are not useful, or it is easier
	// to get them through other means.
	//

	PVOID					RtlAnsiStringToUnicodeString;
	PVOID					RtlUnicodeStringToAnsiString;
	PVOID					BasepAnsiStringToUnicodeSize;
	PVOID					BasepUnicodeStringToAnsiSize;
	PVOID					BasepConvertWin32AttributeList; // used during CreateProcess

	//
	// This is passed to most calls to RtlAllocateHeap.
	// It originates from a call to RtlCreateTagHeap.
	//

	ULONG					BaseDllTag;

	//
	// Presumably this is only set to TRUE when this is the CSRSS.EXE process.
	// The only time other than initialization when this variable is accessed
	// is inside CreateRemoteThreadEx.
	//

	BOOLEAN					BaseRunningInServerProcess;

	//
	// These strings are what gets queried when you call GetSystemDirectory or
	// GetWindowsDirectory.
	//

	UNICODE_STRING			BaseWindowsDirectory;
	UNICODE_STRING			BaseWindowsSystemDirectory;
} TYPEDEF_TYPE_NAME(KERNELBASE_GLOBAL_DATA);

typedef struct _BASE_STATIC_SERVER_DATA {
	// e.g. "C:\Windows"
	UNICODE_STRING			WindowsDirectory;

	// e.g. "C:\Windows\system32"
	UNICODE_STRING			WindowsSystemDirectory;

	// e.g. "\Sessions\1\BaseNamedObjects"
	UNICODE_STRING			NamedObjectDirectory;

	//
	// More members follow, but I don't consider them so interesting so
	// I didn't bother to include them.
	//
} TYPEDEF_TYPE_NAME(BASE_STATIC_SERVER_DATA);

// Applicable for 32-bit program on 64-bit OS only.
typedef struct _BASE_STATIC_SERVER_DATA_WOW64 {
	struct {
		USHORT				Length;
		USHORT				MaximumLength;
		ULONG				Padding;
		PWCHAR				Buffer;
		ULONG				Zero;
	} WindowsDirectory;

	struct {
		USHORT				Length;
		USHORT				MaximumLength;
		ULONG				Padding;
		PWCHAR				Buffer;
		ULONG				Zero;
	} WindowsSystemDirectory;

	struct {
		USHORT				Length;
		USHORT				MaximumLength;
		ULONG				Padding;
		PWCHAR				Buffer;
		ULONG				Zero;
	} NamedObjectDirectory;
} TYPEDEF_TYPE_NAME(BASE_STATIC_SERVER_DATA_WOW64);

typedef enum _OFFER_PRIORITY {
	VMOfferPriorityVeryLow			= 1,
	VMOfferPriorityLow,
	VMOfferPriorityBelowNormal,
	VMOfferPriorityNormal,
	VMOfferPriorityMaximum
} TYPEDEF_TYPE_NAME(OFFER_PRIORITY);

typedef MEMORY_RANGE_ENTRY TYPEDEF_TYPE_NAME(WIN32_MEMORY_RANGE_ENTRY);

typedef struct _CREATEFILE2_EXTENDED_PARAMETERS {
	DWORD					dwSize;
	DWORD					dwFileAttributes;
	DWORD					dwFileFlags;
	DWORD					dwSecurityQosFlags;
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes;
	HANDLE					hTemplateFile;
} TYPEDEF_TYPE_NAME(CREATEFILE2_EXTENDED_PARAMETERS);

typedef enum _COPYFILE2_MESSAGE_ACTION {
	COPYFILE2_PROGRESS_CONTINUE,
	COPYFILE2_PROGRESS_CANCEL,
	COPYFILE2_PROGRESS_STOP,
	COPYFILE2_PROGRESS_QUIET,
	COPYFILE2_PROGRESS_PAUSE
} TYPEDEF_TYPE_NAME(COPYFILE2_MESSAGE_ACTION);

typedef enum _COPYFILE2_COPY_PHASE {
	COPYFILE2_PHASE_NONE = 0,
	COPYFILE2_PHASE_PREPARE_SOURCE,
	COPYFILE2_PHASE_PREPARE_DEST,
	COPYFILE2_PHASE_READ_SOURCE,
	COPYFILE2_PHASE_WRITE_DESTINATION,
	COPYFILE2_PHASE_SERVER_COPY,
	COPYFILE2_PHASE_NAMEGRAFT_COPY,
	COPYFILE2_PHASE_MAX
} TYPEDEF_TYPE_NAME(COPYFILE2_COPY_PHASE);

typedef enum _COPYFILE2_MESSAGE_TYPE {
	COPYFILE2_CALLBACK_NONE = 0,
	COPYFILE2_CALLBACK_CHUNK_STARTED,
	COPYFILE2_CALLBACK_CHUNK_FINISHED,
	COPYFILE2_CALLBACK_STREAM_STARTED,
	COPYFILE2_CALLBACK_STREAM_FINISHED,
	COPYFILE2_CALLBACK_POLL_CONTINUE,
	COPYFILE2_CALLBACK_ERROR,
	COPYFILE2_CALLBACK_MAX
} TYPEDEF_TYPE_NAME(COPYFILE2_MESSAGE_TYPE);

typedef struct _COPYFILE2_MESSAGE {
	COPYFILE2_MESSAGE_TYPE Type;
	DWORD                  dwPadding;

	union {
		struct {
			DWORD          dwStreamNumber;
			DWORD          dwReserved;
			HANDLE         hSourceFile;
			HANDLE         hDestinationFile;
			ULARGE_INTEGER uliChunkNumber;
			ULARGE_INTEGER uliChunkSize;
			ULARGE_INTEGER uliStreamSize;
			ULARGE_INTEGER uliTotalFileSize;
		} ChunkStarted;

		struct {
			DWORD          dwStreamNumber;
			DWORD          dwFlags;
			HANDLE         hSourceFile;
			HANDLE         hDestinationFile;
			ULARGE_INTEGER uliChunkNumber;
			ULARGE_INTEGER uliChunkSize;
			ULARGE_INTEGER uliStreamSize;
			ULARGE_INTEGER uliStreamBytesTransferred;
			ULARGE_INTEGER uliTotalFileSize;
			ULARGE_INTEGER uliTotalBytesTransferred;
		} ChunkFinished;

		struct {
			DWORD          dwStreamNumber;
			DWORD          dwReserved;
			HANDLE         hSourceFile;
			HANDLE         hDestinationFile;
			ULARGE_INTEGER uliStreamSize;
			ULARGE_INTEGER uliTotalFileSize;
		} StreamStarted;

		struct {
			DWORD          dwStreamNumber;
			DWORD          dwReserved;
			HANDLE         hSourceFile;
			HANDLE         hDestinationFile;
			ULARGE_INTEGER uliStreamSize;
			ULARGE_INTEGER uliStreamBytesTransferred;
			ULARGE_INTEGER uliTotalFileSize;
			ULARGE_INTEGER uliTotalBytesTransferred;
		} StreamFinished;

		struct {
			DWORD dwReserved;
		} PollContinue;

		struct {
			COPYFILE2_COPY_PHASE CopyPhase;
			DWORD                dwStreamNumber;
			HRESULT              hrFailure;
			DWORD                dwReserved;
			ULARGE_INTEGER       uliChunkNumber;
			ULARGE_INTEGER       uliStreamSize;
			ULARGE_INTEGER       uliStreamBytesTransferred;
			ULARGE_INTEGER       uliTotalFileSize;
			ULARGE_INTEGER       uliTotalBytesTransferred;
		} Error;
	} Info;
} TYPEDEF_TYPE_NAME(COPYFILE2_MESSAGE);

typedef COPYFILE2_MESSAGE_ACTION (CALLBACK *PCOPYFILE2_PROGRESS_ROUTINE) (
	IN	PCCOPYFILE2_MESSAGE	Message,
	IN	PVOID				Context OPTIONAL);

#define COPY_FILE_DIRECTORY						0x00000080		// Win10
#define COPY_FILE_REQUEST_SECURITY_PRIVILEGES	0x00002000		// Win8
#define COPY_FILE_RESUME_FROM_PAUSE				0x00004000		// Win8
#define COPY_FILE_SKIP_ALTERNATE_STREAMS		0x00008000		// Win10
#define COPY_FILE_NO_OFFLOAD					0x00040000		// Win8
#define COPY_FILE_OPEN_AND_COPY_REPARSE_POINT	0x00200000		// Win10
#define COPY_FILE_IGNORE_EDP_BLOCK				0x00400000		// Win10
#define COPY_FILE_IGNORE_SOURCE_ENCRYPTION		0x00800000		// Win10
#define COPY_FILE_DONT_REQUEST_DEST_WRITE_DAC	0x02000000		// Win10
#define COPY_FILE_DISABLE_PRE_ALLOCATION		0x04000000		// Win10
#define COPY_FILE_ENABLE_LOW_FREE_SPACE_MODE	0x08000000		// Win10
#define COPY_FILE_REQUEST_COMPRESSED_TRAFFIC	0x10000000		// Win10
#define COPY_FILE_ENABLE_SPARSE_COPY			0x20000000		// Win11

#define COPY_FILE_WIN7_VALID_FLAGS				(COPY_FILE_FAIL_IF_EXISTS | COPY_FILE_RESTARTABLE | \
												 COPY_FILE_OPEN_SOURCE_FOR_WRITE | COPY_FILE_ALLOW_DECRYPTED_DESTINATION | \
												 COPY_FILE_COPY_SYMLINK | COPY_FILE_NO_BUFFERING)

#define COPY_FILE_WIN8_VALID_FLAGS				(COPY_FILE_WIN7_VALID_FLAGS | COPY_FILE_REQUEST_SECURITY_PRIVILEGES | \
												 COPY_FILE_RESUME_FROM_PAUSE | COPY_FILE_NO_OFFLOAD)

#define COPY_FILE_WIN10_VALID_FLAGS				(COPY_FILE_WIN8_VALID_FLAGS | COPY_FILE_DIRECTORY | COPY_FILE_SKIP_ALTERNATE_STREAMS | \
												 COPY_FILE_OPEN_AND_COPY_REPARSE_POINT | COPY_FILE_IGNORE_EDP_BLOCK | \
												 COPY_FILE_IGNORE_SOURCE_ENCRYPTION | COPY_FILE_DONT_REQUEST_DEST_WRITE_DAC | \
												 COPY_FILE_DISABLE_PRE_ALLOCATION | COPY_FILE_ENABLE_LOW_FREE_SPACE_MODE | \
												 COPY_FILE_REQUEST_COMPRESSED_TRAFFIC)

#define COPY_FILE_WIN11_VALID_FLAGS				(COPY_FILE_WIN10_VALID_FLAGS | COPY_FILE_ENABLE_SPARSE_COPY)

#define COPY_FILE_ALL_VALID_FLAGS				(COPY_FILE_WIN11_VALID_FLAGS)

typedef struct _COPYFILE2_EXTENDED_PARAMETERS {
	DWORD						dwSize;
	DWORD						dwCopyFlags;		// COPY_FILE_*
	PBOOL						pfCancel;
	PCOPYFILE2_PROGRESS_ROUTINE	pProgressRoutine;
	PVOID						pvCallbackContext;
} TYPEDEF_TYPE_NAME(COPYFILE2_EXTENDED_PARAMETERS);

typedef PVOID TYPEDEF_TYPE_NAME(DLL_DIRECTORY_COOKIE);

// Don't confuse this with THREADINFOCLASS from NtDll.h.
// They are two completely separate things, and yes, this is the official
// naming and not just something I made up.
typedef enum _THREAD_INFORMATION_CLASS {
	ThreadMemoryPriority,
	ThreadAbsoluteCpuPriority,
	ThreadDynamicCodePolicy,
	ThreadPowerThrottling,
	ThreadInformationClassMax
} TYPEDEF_TYPE_NAME(THREAD_INFORMATION_CLASS);

typedef enum _PROCESS_INFORMATION_CLASS {
	ProcessMemoryPriority,
	ProcessMemoryExhaustionInfo,
	ProcessAppMemoryInfo,
	ProcessInPrivateInfo,
	ProcessPowerThrottling,
	ProcessReservedValue1,
	ProcessTelemetryCoverageInfo,
	ProcessProtectionLevelInfo,
	ProcessLeapSecondInfo,
	ProcessMachineTypeInfo,
	ProcessOverrideSubsequentPrefetchParameter,
	ProcessMaxOverridePrefetchParameter,
	ProcessInformationClassMax
} TYPEDEF_TYPE_NAME(PROCESS_INFORMATION_CLASS);

typedef struct _THREAD_POWER_THROTTLING_STATE {
	ULONG	Version;
	ULONG	ControlMask;
	ULONG	StateMask;
} TYPEDEF_TYPE_NAME(THREAD_POWER_THROTTLING_STATE);

typedef ULONG (CALLBACK *PDEVICE_NOTIFY_CALLBACK_ROUTINE) (
    IN	PVOID	Context OPTIONAL,
    IN	ULONG	Type,
    IN	PVOID	Setting);

typedef struct _DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS {
	PDEVICE_NOTIFY_CALLBACK_ROUTINE	Callback;
	PVOID							Context;
} TYPEDEF_TYPE_NAME(DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS);

typedef enum _PROCESS_MITIGATION_POLICY {
	ProcessDEPPolicy,
	ProcessASLRPolicy,
	ProcessDynamicCodePolicy,
	ProcessStrictHandleCheckPolicy,
	ProcessSystemCallDisablePolicy,
	ProcessMitigationOptionsMask,
	ProcessExtensionPointDisablePolicy,
	ProcessControlFlowGuardPolicy,
	ProcessSignaturePolicy,
	ProcessFontDisablePolicy,
	ProcessImageLoadPolicy,
	ProcessSystemCallFilterPolicy,
	ProcessPayloadRestrictionPolicy,
	ProcessChildProcessPolicy,
	ProcessSideChannelIsolationPolicy,
	ProcessUserShadowStackPolicy,
	ProcessRedirectionTrustPolicy,
	MaxProcessMitigationPolicy
} TYPEDEF_TYPE_NAME(PROCESS_MITIGATION_POLICY);

typedef struct _PROCESS_MITIGATION_DEP_POLICY {
	union {
		ULONG		AsUlong;

		struct {
			ULONG	Enable : 1;
			ULONG	DisableAtlThunkEmulation : 1;
			ULONG	ReservedFlags : 30;
		};
	} Flags;

	BOOLEAN			Permanent;
} TYPEDEF_TYPE_NAME(PROCESS_MITIGATION_DEP_POLICY);

DECLARE_HANDLE(HPSS);
DECLARE_HANDLE(HPSSWALK);
GEN_STD_TYPEDEFS(HPSS);
GEN_STD_TYPEDEFS(HPSSWALK);

typedef enum _PSS_CAPTURE_FLAGS {
	PSS_CAPTURE_NONE								= 0x00000000,
	PSS_CAPTURE_VA_CLONE							= 0x00000001,
	PSS_CAPTURE_RESERVED_00000002					= 0x00000002,
	PSS_CAPTURE_HANDLES								= 0x00000004,
	PSS_CAPTURE_HANDLE_NAME_INFORMATION				= 0x00000008,
	PSS_CAPTURE_HANDLE_BASIC_INFORMATION			= 0x00000010,
	PSS_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION	= 0x00000020,
	PSS_CAPTURE_HANDLE_TRACE						= 0x00000040,
	PSS_CAPTURE_THREADS								= 0x00000080,
	PSS_CAPTURE_THREAD_CONTEXT						= 0x00000100,
	PSS_CAPTURE_THREAD_CONTEXT_EXTENDED				= 0x00000200,
	PSS_CAPTURE_RESERVED_00000400					= 0x00000400,
	PSS_CAPTURE_VA_SPACE							= 0x00000800,
	PSS_CAPTURE_VA_SPACE_SECTION_INFORMATION		= 0x00001000,
	PSS_CAPTURE_IPT_TRACE							= 0x00002000,
	PSS_CAPTURE_RESERVED_00004000					= 0x00004000,

	PSS_CREATE_BREAKAWAY_OPTIONAL					= 0x04000000,
	PSS_CREATE_BREAKAWAY							= 0x08000000,
	PSS_CREATE_FORCE_BREAKAWAY						= 0x10000000,
	PSS_CREATE_USE_VM_ALLOCATIONS					= 0x20000000,
	PSS_CREATE_MEASURE_PERFORMANCE					= 0x40000000,
	PSS_CREATE_RELEASE_SECTION						= 0x80000000
} TYPEDEF_TYPE_NAME(PSS_CAPTURE_FLAGS);

typedef enum _PSS_QUERY_INFORMATION_CLASS {
	PSS_QUERY_PROCESS_INFORMATION			= 0,
	PSS_QUERY_VA_CLONE_INFORMATION			= 1,
	PSS_QUERY_AUXILIARY_PAGES_INFORMATION	= 2,
	PSS_QUERY_VA_SPACE_INFORMATION			= 3,
	PSS_QUERY_HANDLE_INFORMATION			= 4,
	PSS_QUERY_THREAD_INFORMATION			= 5,
	PSS_QUERY_HANDLE_TRACE_INFORMATION		= 6,
	PSS_QUERY_PERFORMANCE_COUNTERS			= 7
} TYPEDEF_TYPE_NAME(PSS_QUERY_INFORMATION_CLASS);

typedef enum _CM_NOTIFY_FILTER_TYPE {
	CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE,
	CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE,
	CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE,
	CM_NOTIFY_FILTER_TYPE_MAX
} CM_NOTIFY_FILTER_TYPE, *PCM_NOTIFY_FILTER_TYPE;

DECLARE_HANDLE(HCMNOTIFICATION);
GEN_STD_TYPEDEFS(HCMNOTIFICATION);


typedef enum _CM_NOTIFY_ACTION {
	// Filter type: CM_NOTIFY_FILTER_TYPE_DEVICEINTERFACE
	CM_NOTIFY_ACTION_DEVICEINTERFACEARRIVAL,
	CM_NOTIFY_ACTION_DEVICEINTERFACEREMOVAL,

	// Filter type: CM_NOTIFY_FILTER_TYPE_DEVICEHANDLE
	CM_NOTIFY_ACTION_DEVICEQUERYREMOVE,
	CM_NOTIFY_ACTION_DEVICEQUERYREMOVEFAILED,
	CM_NOTIFY_ACTION_DEVICEREMOVEPENDING,
	CM_NOTIFY_ACTION_DEVICEREMOVECOMPLETE,
	CM_NOTIFY_ACTION_DEVICECUSTOMEVENT,

	// Filter type: CM_NOTIFY_FILTER_TYPE_DEVICEINSTANCE
	CM_NOTIFY_ACTION_DEVICEINSTANCEENUMERATED,
	CM_NOTIFY_ACTION_DEVICEINSTANCESTARTED,
	CM_NOTIFY_ACTION_DEVICEINSTANCEREMOVED,

	CM_NOTIFY_ACTION_MAX
} TYPEDEF_TYPE_NAME(CM_NOTIFY_ACTION);

typedef struct _CM_NOTIFY_EVENT_DATA {
	CM_NOTIFY_FILTER_TYPE	FilterType;
	DWORD					Reserved;

	union {
		struct {
			GUID			ClassGuid;
			WCHAR			SymbolicLink[ANYSIZE_ARRAY];
		} DeviceInterface;

		struct {
			GUID			EventGuid;
			LONG			NameOffset;
			DWORD			DataSize;
			BYTE			Data[ANYSIZE_ARRAY];
		} DeviceHandle;

		struct {
			WCHAR			InstanceId[ANYSIZE_ARRAY];
		} DeviceInstance;
	};
} TYPEDEF_TYPE_NAME(CM_NOTIFY_EVENT_DATA);

typedef DWORD (CALLBACK *PCM_NOTIFY_CALLBACK) (
	IN	HCMNOTIFICATION			hNotify,
	IN	PVOID					Context OPTIONAL,
	IN	CM_NOTIFY_ACTION		Action,
	IN	PCM_NOTIFY_EVENT_DATA	EventData,
	IN	DWORD					EventDataSize);

typedef struct _CM_NOTIFY_FILTER {
	DWORD                 cbSize;
	DWORD                 Flags;
	CM_NOTIFY_FILTER_TYPE FilterType;
	DWORD                 Reserved;

	union {
		struct {
			GUID ClassGuid;
		} DeviceInterface;
		struct {
			HANDLE hTarget;
		} DeviceHandle;
		struct {
			WCHAR InstanceId[MAX_DEVICE_ID_LEN];
		} DeviceInstance;
	};
} TYPEDEF_TYPE_NAME(CM_NOTIFY_FILTER);

typedef enum _WLDP_WINDOWS_LOCKDOWN_MODE {
	WLDP_WINDOWS_LOCKDOWN_MODE_UNLOCKED,
	WLDP_WINDOWS_LOCKDOWN_MODE_TRIAL,
	WLDP_WINDOWS_LOCKDOWN_MODE_LOCKED,
	WLDP_WINDOWS_LOCKDOWN_MODE_MAX
} TYPEDEF_TYPE_NAME(WLDP_WINDOWS_LOCKDOWN_MODE);

typedef enum _FIRMWARE_TYPE {
	FirmwareTypeUnknown,
	FirmwareTypeBios,
	FirmwareTypeUefi,
	FirmwareTypeMax
} TYPEDEF_TYPE_NAME(FIRMWARE_TYPE);

typedef struct _VERHEAD {
	WORD				wTotLen;
	WORD				wValLen;
	WORD				wType;
	WCHAR				szKey[(sizeof("VS_VERSION_INFO") + 3) & ~3];
	VS_FIXEDFILEINFO	vsf;
} TYPEDEF_TYPE_NAME(VERHEAD);

#pragma endregion

#if defined(KEX_ENV_WIN32)

WINBASEAPI PKERNELBASE_GLOBAL_DATA WINAPI KernelBaseGetGlobalData(
	VOID);

WINBASEAPI ULONG WINAPI BaseSetLastNTError(
	IN	NTSTATUS	Status);

//
// thread.c
//

KXBASEAPI HRESULT WINAPI GetThreadDescription(
	IN	HANDLE	ThreadHandle,
	OUT	PPWSTR	ThreadDescription);

KXBASEAPI HRESULT WINAPI SetThreadDescription(
	IN	HANDLE	ThreadHandle,
	IN	PCWSTR	ThreadDescription);

KXBASEAPI VOID WINAPI GetCurrentThreadStackLimits(
	OUT	PULONG_PTR	LowLimit,
	OUT	PULONG_PTR	HighLimit);

KXBASEAPI BOOL WINAPI SetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	IN	PVOID						ThreadInformation,
	IN	ULONG						ThreadInformationSize);

KXBASEAPI BOOL WINAPI GetThreadInformation(
	IN	HANDLE						ThreadHandle,
	IN	THREAD_INFORMATION_CLASS	ThreadInformationClass,
	OUT	PVOID						ThreadInformation,
	IN	ULONG						ThreadInformationSize);

KXBASEAPI BOOL WINAPI SetThreadSelectedCpuSets(
	IN	HANDLE	ThreadHandle,
	IN	PULONG	CpuSetIds,
	IN	ULONG	NumberOfCpuSetIds);

KXBASEAPI BOOL WINAPI SetThreadSelectedCpuSetMasks(
	IN	HANDLE			ThreadHandle,
	IN	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			NumberOfCpuSetMasks);

KXBASEAPI BOOL WINAPI GetThreadSelectedCpuSets(
	IN	HANDLE	ThreadHandle,
	OUT	PULONG	CpuSetIds,
	IN	ULONG	CpuSetIdArraySize,
	OUT	PULONG	ReturnCount);

KXBASEAPI BOOL WINAPI GetThreadSelectedCpuSetMasks(
	IN	HANDLE			ThreadHandle,
	OUT	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			CpuSetMaskArraySize,
	OUT	PULONG			ReturnCount);

//
// process.c
//

KXBASEAPI BOOL WINAPI GetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	OUT	PVOID						ProcessInformation,
	IN	ULONG						ProcessInformationSize);

KXBASEAPI BOOL WINAPI SetProcessInformation(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_INFORMATION_CLASS	ProcessInformationClass,
	IN	PVOID						ProcessInformation,
	IN	ULONG						ProcessInformationSize);

KXBASEAPI BOOL WINAPI SetProcessDefaultCpuSets(
	IN	HANDLE	ProcessHandle,
	IN	PULONG	CpuSetIds,
	IN	ULONG	NumberOfCpuSetIds);

KXBASEAPI BOOL WINAPI SetProcessDefaultCpuSetMasks(
	IN	HANDLE			ProcessHandle,
	IN	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			NumberOfCpuSetMasks);

KXBASEAPI BOOL WINAPI GetProcessDefaultCpuSets(
	IN	HANDLE	ProcessHandle,
	OUT	PULONG	CpuSetIds,
	IN	ULONG	CpuSetIdArraySize,
	OUT	PULONG	ReturnCount);

KXBASEAPI BOOL WINAPI GetProcessDefaultCpuSetMasks(
	IN	HANDLE			ProcessHandle,
	OUT	PGROUP_AFFINITY	CpuSetMasks,
	IN	ULONG			CpuSetMaskArraySize,
	OUT	PULONG			ReturnCount);

KXBASEAPI BOOL WINAPI SetProcessMitigationPolicy(
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	IN	PVOID						Buffer,
	IN	SIZE_T						BufferCb);

KXBASEAPI BOOL WINAPI GetProcessMitigationPolicy(
	IN	HANDLE						ProcessHandle,
	IN	PROCESS_MITIGATION_POLICY	MitigationPolicy,
	OUT	PVOID						Buffer,
	IN	SIZE_T						BufferCb);

//
// file.c
//

KXBASEAPI HANDLE WINAPI CreateFile2(
	IN	PCWSTR								FileName,
	IN	ULONG								DesiredAccess,
	IN	ULONG								ShareMode,
	IN	ULONG								CreationDisposition,
	IN	PCREATEFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL);

KXBASEAPI ULONG WINAPI GetTempPath2A(
	IN	ULONG	BufferCch,
	OUT	PSTR	Buffer);

KXBASEAPI ULONG WINAPI GetTempPath2W(
	IN	ULONG	BufferCch,
	OUT	PWSTR	Buffer);

KXBASEAPI HRESULT WINAPI CopyFile2(
	IN	PCWSTR							ExistingFileName,
	IN	PCWSTR							NewFileName,
	IN	PCOPYFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL);

//
// time.c
//

KXBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(
	OUT	PFILETIME	SystemTimeAsFileTime);

//
// synch.c
//

KXBASEAPI BOOL WINAPI WaitOnAddress(
	IN	VOLATILE VOID	*Address,
	IN	PVOID			CompareAddress,
	IN	SIZE_T			AddressSize,
	IN	DWORD			Milliseconds OPTIONAL);

KXBASEAPI VOID WINAPI WakeByAddressSingle(
	IN	PVOID			Address);

KXBASEAPI VOID WINAPI WakeByAddressAll(
	IN	PVOID			Address);

//
// wow64.c
//

KXBASEAPI BOOL WINAPI IsWow64Process2(
	IN	HANDLE	ProcessHandle,
	OUT	PUSHORT	ProcessMachine,
	OUT	PUSHORT	NativeMachine);

//
// appmodel.c
//

KXBASEAPI LONG WINAPI GetCurrentPackageFullName(
	IN OUT	PULONG	PackageFullNameLength,
	OUT		PWSTR	PackageFullName OPTIONAL);

KXBASEAPI LONG WINAPI GetCurrentPackageId(
	IN OUT	PULONG	BufferLength,
	OUT		PBYTE	Buffer OPTIONAL);

KXBASEAPI LONG WINAPI AppPolicyGetProcessTerminationMethod(
	IN	HANDLE	ProcessToken,
	OUT	PULONG	Policy);

KXBASEAPI HRESULT WINAPI CreateAppContainerProfile(
	IN	PCWSTR				AppContainerName,
	IN	PCWSTR				DisplayName,
	IN	PCWSTR				Description,
	IN	PSID_AND_ATTRIBUTES	Capabilities,
	IN	ULONG				NumberOfCapabilities,
	OUT	PSID				*AppContainerSid);

KXBASEAPI HRESULT WINAPI DeleteAppContainerProfile(
	IN	PCWSTR	AppContainerName);

KXBASEAPI HRESULT WINAPI GetAppContainerFolderPath(
	IN	PCWSTR	AppContainerName,
	OUT	PPWSTR	FolderPath);

KXBASEAPI HRESULT WINAPI GetAppContainerRegistryLocation(
	IN	REGSAM	DesiredAccess,
	OUT	PHKEY	AppContainerKey);

//
// crypto.c
//

KXBASEAPI BOOL WINAPI ProcessPrng(
	OUT	PBYTE	Buffer,
	IN	SIZE_T	BufferCb);

KXBASEAPI NTSTATUS WINAPI BCryptHash(
	IN	BCRYPT_ALG_HANDLE	Algorithm,
	IN	PBYTE				Secret OPTIONAL,
	IN	ULONG				SecretCb,
	IN	PBYTE				Input,
	IN	ULONG				InputCb,
	OUT	PBYTE				Output,
	IN	ULONG				OutputCb);

//
// vmem.c
//

KXBASEAPI ULONG WINAPI OfferVirtualMemory(
	IN	PVOID			VirtualAddress,
	IN	SIZE_T			Size,
	IN	OFFER_PRIORITY	Priority);

KXBASEAPI ULONG WINAPI DiscardVirtualMemory(
	IN	PVOID	VirtualAddress,
	IN	SIZE_T	Size);

KXBASEAPI ULONG WINAPI ReclaimVirtualMemory(
	IN	PCVOID	VirtualAddress,
	IN	SIZE_T	Size);

KXBASEAPI BOOL WINAPI PrefetchVirtualMemory(
	IN	HANDLE						ProcessHandle,
	IN	ULONG_PTR					NumberOfEntries,
	IN	PWIN32_MEMORY_RANGE_ENTRY	VirtualAddresses,
	IN	ULONG						Flags);

//
// power.c
//

KXBASEAPI POWER_PLATFORM_ROLE WINAPI PowerDeterminePlatformRoleEx(
	IN	ULONG	Version);

KXBASEAPI ULONG WINAPI PowerRegisterSuspendResumeNotification(
	IN	ULONG			Flags,
	IN	HANDLE			Recipient,
	OUT	PHPOWERNOTIFY	RegistrationHandle);

KXBASEAPI ULONG WINAPI PowerUnregisterSuspendResumeNotification(
	IN OUT	HPOWERNOTIFY	RegistrationHandle);

//
// misc.c
//

KXBASEAPI BOOL WINAPI GetOsSafeBootMode(
	OUT	PBOOL	IsSafeBootMode);

KXBASEAPI BOOL WINAPI GetFirmwareType(
	OUT	PFIRMWARE_TYPE	FirmwareType);

#endif // if defined(KEX_ENV_WIN32)