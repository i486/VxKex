#pragma once

#include <KexComm.h>

#define STATUS_SUCCESS						((NTSTATUS) 0)
#define STATUS_IMAGE_MP_UP_MISMATCH			((NTSTATUS) 0xC0000249L)
#define STATUS_INVALID_IMAGE_FORMAT			((NTSTATUS) 0xC000007BL)
#define STATUS_IMAGE_MACHINE_TYPE_MISMATCH	((NTSTATUS) 0x4000000EL)
#define STATUS_WX86_BREAKPOINT				((NTSTATUS) 0x4000001FL)
#define STATUS_BUFFER_TOO_SMALL				((NTSTATUS) 0xC0000023L)
#define NT_SUCCESS(st) (((NTSTATUS) (st)) >= 0)

#define EXCEPTION_WX86_BREAKPOINT			STATUS_WX86_BREAKPOINT

#define RTL_MAX_DRIVE_LETTERS 32
#define PROCESSOR_FEATURE_MAX 64
#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#define PF_FLOATING_POINT_PRECISION_ERRATA			0
#define PF_FLOATING_POINT_EMULATED					1
#define PF_COMPARE_EXCHANGE_DOUBLE					2
#define PF_MMX_INSTRUCTIONS_AVAILABLE				3
#define PF_PPC_MOVEMEM_64BIT_OK						4
#define PF_ALPHA_BYTE_INSTRUCTIONS					5
#define PF_XMMI_INSTRUCTIONS_AVAILABLE				6
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE				7
#define PF_RDTSC_INSTRUCTION_AVAILABLE				8
#define PF_PAE_ENABLED								9
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE			10
#define PF_SSE_DAZ_MODE_AVAILABLE					11
#define PF_NX_ENABLED								12
#define PF_SSE3_INSTRUCTIONS_AVAILABLE				13
#define PF_COMPARE_EXCHANGE128						14
#define PF_COMPARE64_EXCHANGE128					15
#define PF_CHANNELS_ENABLED							16
#define PF_XSAVE_ENABLED							17

#ifdef _M_X64
#  define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#else
#  define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#endif

//
// Data type definitions
//

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];
typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];
typedef VOID (*PPS_POST_PROCESS_INIT_ROUTINE) (VOID);

typedef struct _PEB *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION {
    NTSTATUS	ExitStatus;
    PPEB		PebBaseAddress;
    ULONG_PTR	AffinityMask;
    KPRIORITY	BasePriority;
    ULONG_PTR	UniqueProcessId;
    ULONG_PTR	InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PAGE_PRIORITY_INFORMATION {
    ULONG		PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

#pragma pack(push, 1)
typedef struct _PROCESS_DEVICEMAP_INFORMATION {
	union {
		struct {
			HANDLE DirectoryHandle;
		} Set;
		
		struct {
			ULONG DriveMap;
			UCHAR DriveType[32];
		} Query;
	};
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;
#pragma pack(pop)

typedef enum _PROCESSINFOCLASS {
	ProcessBasicInformation,					// PROCESS_BASIC_INFORMATION
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority,
	ProcessRaisePriority,
	ProcessDebugPort,
	ProcessExceptionPort,
	ProcessAccessToken,
	ProcessLdtInformation,
	ProcessLdtSize,
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL,
	ProcessEnableAlignmentFaultFixup,
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount,
	ProcessAffinityMask,
	ProcessPriorityBoost,
	ProcessDeviceMap,
	ProcessSessionInformation,
	ProcessForegroundInformation,
	ProcessWow64Information,
	ProcessImageFileName,
	ProcessLUIDDeviceMapsEnabled,
	ProcessBreakOnTermination,
	ProcessDebugObjectHandle,
	ProcessDebugFlags,
	ProcessHandleTracing,
	ProcessIoPriority,
	ProcessExecuteFlags,
	ProcessTlsInformation,
	ProcessCookie,
	ProcessImageInformation,
	ProcessCycleTime,
	ProcessPagePriority,						// PAGE_PRIORITY_INFORMATION
	ProcessInstrumentationCallback,
	ProcessThreadStackAllocation,
	ProcessWorkingSetWatchEx,
	ProcessImageFileNameWin32,
	ProcessImageFileMapping,
	ProcessAffinityUpdateMode,
	ProcessMemoryAllocationMode,
	ProcessGroupInformation,
	ProcessTokenVirtualizationEnabled,
	ProcessConsoleHostProcess,
	ProcessWindowInformation,
	MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef enum _THREADINFOCLASS {
	ThreadBasicInformation,
	ThreadTimes,
	ThreadPriority,
	ThreadBasePriority,
	ThreadAffinityMask,
	ThreadImpersonationToken,
	ThreadDescriptorTableEntry,
	ThreadEnableAlignmentFaultFixup,
	ThreadEventPair,
	ThreadQuerySetWin32StartAddress,
	ThreadZeroTlsCell,
	ThreadPerformanceCount,					// 3.51+
	ThreadAmlLastThread,
	ThreadIdealProcessor,					// 4.0+
	ThreadPriorityBoost,
	ThreadSetTlsArrayAddresses,
	ThreadIsIoPending,						// 5.0+
	ThreadHideFromDebugger,
	ThreadBreakOnTermination,				// 5.2+
	ThreadSwitchLegacyState,
	ThreadIsTerminated,
	ThreadLastSystemCall,					// 6.0+
	ThreadIoPriority,
	ThreadCycleTime,
	ThreadPagePriority,						// PAGE_PRIORITY_INFORMATION
	ThreadActualBasePriority,
	ThreadTebInformation,
	ThreadCSwitchMon,
	ThreadCSwitchPmu,						// 6.1+
	ThreadWow64Context,
	ThreadGroupInformation,
	ThreadUmsInformation,
	ThreadCounterProfiling,
	ThreadIdealProcessorEx,
	MaxThreadInfoClass
} THREADINFOCLASS;

typedef enum _SYSINFOCLASS {
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemMirrorMemoryInformation,
	SystemPerformanceTraceInformation,
	SystemObsolete0,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemVerifierAddDriverInformation,
	SystemVerifierRemoveDriverInformation,
	SystemProcessorIdleInformation,
	SystemLegacyDriverInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation,
	SystemTimeSlipNotification,
	SystemSessionCreate,
	SystemSessionDetach,
	SystemSessionInformation,
	SystemRangeStartInformation,
	SystemVerifierInformation,
	SystemVerifierThunkExtend,
	SystemSessionProcessInformation,
	SystemLoadGdiDriverInSystemSpace,
	SystemNumaProcessorMap,
	SystemPrefetcherInformation,
	SystemExtendedProcessInformation,
	SystemRecommendedSharedDataAlignment,
	SystemComPlusPackage,
	SystemNumaAvailableMemory,
	SystemProcessorPowerInformation,
	SystemEmulationBasicInformation,
	SystemEmulationProcessorInformation,
	SystemExtendedHandleInformation,
	SystemLostDelayedWriteInformation,
	SystemBigPoolInformation,
	SystemSessionPoolTagInformation,
	SystemSessionMappedViewInformation,
	SystemHotpatchInformation,
	SystemObjectSecurityMode,
	SystemWatchdogTimerHandler,
	SystemWatchdogTimerInformation,
	SystemLogicalProcessorInformation
} SYSINFOCLASS;

// I don't know how many of these are actually valid in Win7.
// All I know is that the first one is the only one which is used often
// and the first three are present in XP, the remainder are present
// in build 8175 (i.e. Win8)
typedef enum _MEMINFOCLASS {
	MemoryBasicInformation,
	MemoryWorkingSetInformation,
	MemoryMappedFilenameInformation,
	MemoryRegionInformation,
	MemoryWorkingSetExInformation
} MEMINFOCLASS;

typedef enum _NT_PRODUCT_TYPE {
	NtProductWinNt = 1,
	NtProductLanManNt,
	NtProductServer
} NT_PRODUCT_TYPE, *PNT_PRODUCT_TYPE;

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE {
	StandardDesign,
	NEC98x86
} ALTERNATIVE_ARCHITECTURE_TYPE;

typedef struct _STRING {
	USHORT								Length;
	USHORT								MaximumLength;
	PCHAR								Buffer;
} STRING, ANSI_STRING, *PSTRING, *PANSI_STRING;

typedef CONST ANSI_STRING *PCANSI_STRING;

typedef struct _UNICODE_STRING {
	USHORT								Length;
	USHORT								MaximumLength;
	PWCHAR								Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef CONST UNICODE_STRING *PCUNICODE_STRING;

typedef struct _UNICODE_STRING32 {
	USHORT								Length;
	USHORT								MaximumLength;
	DWORD								Buffer;
} UNICODE_STRING32, *PUNICODE_STRING32;

typedef CONST UNICODE_STRING32 *PCUNICODE_STRING32;

typedef struct _PEB_LDR_DATA {															// 3.51+
	ULONG								Length;
	BOOLEAN								Initialized;
	PVOID								SsHandle;
	LIST_ENTRY							InLoadOrderModuleList;
	LIST_ENTRY							InMemoryOrderModuleList;
	LIST_ENTRY							InInitializationOrderModuleList;
	PVOID								EntryInProgress;								// this and the following 5.1+
	BOOLEAN								ShutdownInProgress;								// 6.0SP1+
	HANDLE								ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _CURDIR {
	UNICODE_STRING						DosPath;
	HANDLE								Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
	USHORT								Flags;
	USHORT								Length;
	ULONG								TimeStamp;
	STRING								DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _PEB_FREE_BLOCK {
	struct _PEB_FREE_BLOCK				*Next;
	ULONG								Size;
} PEB_FREE_BLOCK, *PPEB_FREE_BLOCK;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	ULONG								MaximumLength;
	ULONG								Length;

	ULONG								Flags;
	ULONG								DebugFlags;

	HANDLE								ConsoleHandle;
	ULONG								ConsoleFlags;
	HANDLE								StandardInput;
	HANDLE								StandardOutput;
	HANDLE								StandardError;

	CURDIR								CurrentDirectory;
	UNICODE_STRING						DllPath;
	UNICODE_STRING						ImagePathName;
	UNICODE_STRING						CommandLine;
	PVOID								Environment;

	ULONG								StartingX;
	ULONG								StartingY;
	ULONG								CountX;
	ULONG								CountY;
	ULONG								CountCharsX;
	ULONG								CountCharsY;
	ULONG								FillAttribute;

	ULONG								WindowFlags;
	ULONG								ShowWindowFlags;
	UNICODE_STRING						WindowTitle;
	UNICODE_STRING						DesktopInfo;
	UNICODE_STRING						ShellInfo;
	UNICODE_STRING						RuntimeData;
	RTL_DRIVE_LETTER_CURDIR				CurrentDirectories[RTL_MAX_DRIVE_LETTERS];
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// for more info on this "API set" crap - https://lucasg.github.io/2017/10/15/Api-set-resolution
typedef struct _API_SET_NAMESPACE {
	ULONG	Version;
	ULONG	Size;
	ULONG	Flags;
	ULONG	Count;
	ULONG	EntryOffset;
	ULONG	HashOffset;
	ULONG	HashFactor;
} API_SET_NAMESPACE, *PAPI_SET_NAMESPACE;

typedef struct _API_SET_HASH_ENTRY {
	ULONG	Hash;
	ULONG	Index;
} API_SET_HASH_ENTRY, *PAPI_SET_HASH_ENTRY;

typedef struct _API_SET_NAMESPACE_ENTRY {
	ULONG	Flags;
	ULONG	NameOffset;
	ULONG	NameLength;
	ULONG	HashedLength;
	ULONG	ValueOffset;
	ULONG	ValueCount;
} API_SET_NAMESPACE_ENTRY, *PAPI_SET_NAMESPACE_ENTRY;

typedef struct _API_SET_VALUE_ENTRY {
	ULONG	Flags;
	ULONG	NameOffset;
	ULONG	NameLength;
	ULONG	ValueOffset;
	ULONG	ValueLength;
} API_SET_VALUE_ENTRY;

typedef struct _PEB {																	// 3.51+
	BOOLEAN								InheritedAddressSpace;
	BOOLEAN								ReadImageFileExecOptions;
	BOOLEAN								BeingDebugged;
	union {
		BOOLEAN							SpareBool;
		UCHAR							BitField;										// this and the following struct 5.2+
		struct {
			UCHAR						ImageUsedLargePages : 1;
			UCHAR						IsProtectedProcess : 1;							// this and the following bitfields 6.0+
			UCHAR						IsLegacyProcess : 1;
			UCHAR						IsImageDynamicallyRelocated : 1;
			UCHAR						SkipPatchingUser32Forwarders : 1;
			UCHAR						SpareBits0 : 3;
		};
	};

	HANDLE								Mutant;
	PVOID								ImageBaseAddress;
	PPEB_LDR_DATA						Ldr;
	PRTL_USER_PROCESS_PARAMETERS		ProcessParameters;
	PVOID								SubSystemData;
	HANDLE								ProcessHeap;
	PRTL_CRITICAL_SECTION				FastPebLock;

	union {
		PVOID							FastPebLockRoutine;								// 3.10-5.1
		PVOID							SparePtr1;										// 5.2
		PVOID							AtlThunkSListPtr;								// 5.2+
	};

	union {
		PVOID							FastPebUnlockRoutine;							// 3.10-5.1
		PVOID							SparePtr2;										// 5.2
		PVOID							IFEOKey;										// 5.2+
	};
	
	union {
		ULONG							EnvironmentUpdateCount;							// 3.50-5.2
		union {
			ULONG						CrossProcessFlags;								// this and the following struct 6.0+
			struct {
				ULONG					ProcessInJob : 1;
				ULONG					ProcessInitializing : 1;
				ULONG					ProcessUsingVEH : 1;							// this and the following bitfields 6.1+
				ULONG					ProcessUsingVCH : 1;
				ULONG					ProcessUsingFTH : 1;
				ULONG					ReservedBits0 : 27;
			};
		};
	};

	union {
		PVOID							KernelCallbackTable;
		PVOID							UserSharedInfoPtr;								// 6.0+
	};

	ULONG								SystemReserved[1];

	union {
		struct {																		// 5.1-5.2
			ULONG						ExecuteOptions : 2;
			ULONG						SpareBits1 : 30;
		};

		ULONG							SpareUlong;										// 5.2-6.0
		ULONG							AtlThunkSListPtr32;								// 6.1+
	};

	union {
		PPEB_FREE_BLOCK					FreeList;										// 3.10-6.0
		ULONG							SparePebPtr0;									// 6.0
		PAPI_SET_NAMESPACE				ApiSetMap;										// 6.1+
	};

	ULONG								TlsExpansionCounter;
	PVOID								TlsBitmap;
	ULONG								TlsBitmapBits[2];
	PVOID								ReadOnlySharedMemoryBase;

	union {
		PVOID							ReadOnlySharedMemoryHeap;						// 3.10-5.2
		PVOID							HotpatchInformation;							// 6.0-6.1
	};

	PPVOID								ReadOnlyStaticServerData;
	PVOID								AnsiCodePageData;
	PVOID								OemCodePageData;
	PVOID								UnicodeCaseTableData;

	ULONG								NumberOfProcessors;
	ULONG								NtGlobalFlag;

	LARGE_INTEGER						CriticalSectionTimeout;
	SIZE_T								HeapSegmentReserve;								// this and all following members 3.51+
	SIZE_T								HeapSegmentCommit;
	SIZE_T								HeapDeCommitTotalFreeThreshold;
	SIZE_T								HeapDeCommitFreeBlockThreshold;

	ULONG								NumberOfHeaps;
	ULONG								MaximumNumberOfHeaps;
	PPVOID								ProcessHeaps;

	PVOID								GdiSharedHandleTable;							// this and all following members 4.0+
	PVOID								ProcessStarterHelper;
	ULONG								GdiDCAttributeList;
	PRTL_CRITICAL_SECTION				LoaderLock;

	ULONG								OSMajorVersion;
	ULONG								OSMinorVersion;
	USHORT								OSBuildNumber;
	USHORT								OSCSDVersion;
	ULONG								OSPlatformId;
	ULONG								ImageSubsystem;
	ULONG								ImageSubsystemMajorVersion;
	ULONG								ImageSubsystemMinorVersion;
	ULONG_PTR							ImageProcessAffinityMask;
	GDI_HANDLE_BUFFER					GdiHandleBuffer;

	PPS_POST_PROCESS_INIT_ROUTINE		PostProcessInitRoutine;							// this and all following members 5.0+
	PVOID								TlsExpansionBitmap;
	ULONG								TlsExpansionBitmapBits[32];
	ULONG								SessionId;

	union {
		struct {
			PVOID						AppCompatInfo;
			UNICODE_STRING				CSDVersion;
		} nt50;

		struct {																		// 5.1+
			ULARGE_INTEGER				AppCompatFlags;
			ULARGE_INTEGER				AppCompatFlagsUser;
			PVOID						pShimData;

			PVOID						AppCompatInfo;
			UNICODE_STRING				CSDVersion;
		} nt51;
	} AppCompatUnion;

	PVOID								ActivationContextData;							// this and all following members 5.1+
	PVOID								ProcessAssemblyStorageMap;
	PVOID								SystemDefaultActivationContextData;
	PVOID								SystemAssemblyStorageMap;

	SIZE_T								MinimumStackCommit;

	PPVOID								FlsCallback;									// this and all following members 5.2+
	PVOID								SparePointers[4];
	LIST_ENTRY							FlsListHead;
	PVOID								FlsBitmap;
	ULONG								FlsBitmapBits[4];
	ULONG								FlsHighIndex;

	PVOID								WerRegistrationData;							// this and all following members 6.0+
	PVOID								WerShipAssetPtr;

	PVOID								pContextData;									// this and all following members 6.1+
	PVOID								pImageHeaderHash;

	union {
		ULONG							TracingFlags;
		struct {
			ULONG						HeapTracingEnabled : 1;
			ULONG						CritSecTracingEnabled : 1;
			ULONG						SpareTracingBits : 30;
		};
	};
} PEB, *PPEB;

typedef struct _CLIENT_ID {
	HANDLE								UniqueProcess;
	HANDLE								UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

typedef struct _TEB {
	NT_TIB								NtTib;
	PVOID								EnvironmentPointer;
	CLIENT_ID							ClientId;
	PVOID								ActiveRpcHandle;
	PVOID								ThreadLocalStoragePointer;
	PPEB								ProcessEnvironmentBlock;
	ULONG								LastErrorValue;
	ULONG								CountOfOwnedCriticalSections;
} TEB, *PTEB;

typedef struct _KSYSTEM_TIME {
	ULONG								LowPart;
	LONG								High1Time;
	LONG								High2Time;
} KSYSTEM_TIME, *PKSYSTEM_TIME;

typedef struct _KUSER_SHARED_DATA {														// 3.50+
	ULONG VOLATILE						TickCountLow;
	ULONG								TickCountMultiplier;
	KSYSTEM_TIME VOLATILE				InterruptTime;
	KSYSTEM_TIME VOLATILE				SystemTime;
	KSYSTEM_TIME VOLATILE				TimeZoneBias;

	USHORT								ImageNumberLow;									// 3.51+
	USHORT								ImageNumberHigh;
	WCHAR								NtSystemRoot[MAX_PATH];

	union {																				// 4.0+
		ULONG							DosDeviceMap;									// 4.0
		ULONG							MaxStackTraceDepth;								// 5.0+
	};

	ULONG								CryptoExponent;
	ULONG								TimeZoneId;

	ULONG								LargePageMinimum;								// 5.2+
	ULONG								Reserved2[7];

	NT_PRODUCT_TYPE						NtProductType;									// 4.0+
	BOOLEAN								ProductTypeIsValid;

	ULONG								NtMajorVersion;
	ULONG								NtMinorVersion;

	BOOLEAN								ProcessorFeatures[PROCESSOR_FEATURE_MAX];		// search for PF_

	ULONG								Reserved1;
	ULONG								Reserved3;

	ULONG VOLATILE						TimeSlip;
	ALTERNATIVE_ARCHITECTURE_TYPE		AlternativeArchitecture;
	LARGE_INTEGER						SystemExpirationDate;
	ULONG								SuiteMask;
	BOOLEAN								KdDebuggerEnabled;
	ULONG VOLATILE						ActiveConsoleId;
	ULONG VOLATILE						DismountCount;
	ULONG								ComPlusPackage;
	ULONG								LastSystemRITEventTickCount;
	ULONG								NumberOfPhysicalPages;
	BOOLEAN								SafeBootMode;
	ULONG								TraceLogging;
	ULONGLONG							Fill0;
	ULONGLONG							SystemCall[4];

	union {
		KSYSTEM_TIME VOLATILE			TickCount;
		ULONG64 VOLATILE				TickCountQuad;
	};
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

typedef struct _OBJECT_ATTRIBUTES {
	ULONG				Length;
	HANDLE				RootDirectory;
	PUNICODE_STRING		ObjectName;
	ULONG				Attributes;
	PVOID				SecurityDescriptor;
	PVOID				SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef struct _LDR_DLL_NOTIFICATION_DATA {
	ULONG				Flags;
	PCUNICODE_STRING	FullDllName;
	PCUNICODE_STRING	BaseDllName;
	PVOID				DllBase;
	ULONG				SizeOfImage;
} LDR_DLL_NOTIFICATION_DATA, *PLDR_DLL_NOTIFICATION_DATA;

typedef CONST LDR_DLL_NOTIFICATION_DATA *PCLDR_DLL_NOTIFICATION_DATA;

typedef enum _LDR_DLL_NOTIFICATION_REASON {
	LDR_DLL_NOTIFICATION_REASON_LOADED		= 1,
	LDR_DLL_NOTIFICATION_REASON_UNLOADED	= 2
} LDR_DLL_NOTIFICATION_REASON;

typedef VOID (NTAPI *PLDR_DLL_NOTIFICATION_FUNCTION)(
	IN	LDR_DLL_NOTIFICATION_REASON	NotificationReason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context);

STATIC CONST PKUSER_SHARED_DATA SharedUserData = (PKUSER_SHARED_DATA) 0x7FFE0000;

//
// NTDLL function declarations
//

NTSYSAPI NTSTATUS NTAPI NtQuerySystemInformation(
	IN	SYSINFOCLASS	SystemInformationClass,
	OUT	PVOID			SystemInformation,
	IN	ULONG			SystemInformationLength,
	OUT	PULONG			ReturnLength OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtQueryInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	OUT	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtSetInformationProcess(
	IN	HANDLE				ProcessHandle,
	IN	PROCESSINFOCLASS	ProcessInformationClass,
	IN	PVOID				ProcessInformation,
	IN	ULONG				ProcessInformationLength);

NTSYSAPI NTSTATUS NTAPI NtQueryInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	OUT	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength,
	OUT	PULONG				ReturnLength OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtSetInformationThread(
	IN	HANDLE				ThreadHandle,
	IN	THREADINFOCLASS		ThreadInformationClass,
	IN	PVOID				ThreadInformation,
	IN	ULONG				ThreadInformationLength);

NTSYSAPI NTSTATUS NTAPI NtSuspendProcess(
	IN	HANDLE	ProcessHandle);

NTSYSAPI NTSTATUS NTAPI NtResumeProcess(
	IN	HANDLE	ProcessHandle);

NTSYSAPI NTSTATUS NTAPI NtSuspendThread(
	IN	HANDLE	ThreadHandle,
	OUT	PULONG	PreviousSuspendCount OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtResumeThread(
	IN	HANDLE	ThreadHandle,
	IN	PULONG	PreviousSuspendCount OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtTerminateProcess(
	IN	HANDLE		ProcessHandle,
	IN	NTSTATUS	ExitStatus);

NTSYSAPI NTSTATUS NTAPI NtWaitForSingleObject(
	IN	HANDLE			Handle,
	IN	BOOLEAN			Alertable,
	IN	PLARGE_INTEGER	Timeout);

NTSYSAPI NTSTATUS NTAPI NtClose(
	IN	HANDLE	Handle);

NTSYSAPI NTSTATUS NTAPI NtRaiseHardError(
	IN	NTSTATUS	ErrorStatus,
	IN	ULONG		NumberOfParameters,
	IN	ULONG		UnicodeStringParameterMask,
	IN	PULONG_PTR	Parameters,
	IN	ULONG		ValidResponseOptions,
	OUT	PULONG		Response);

NTSYSAPI NTSTATUS NTAPI NtCreateUserProcess(
	OUT		PHANDLE							ProcessHandle,
	OUT		PHANDLE							ThreadHandle,
	IN		ACCESS_MASK						ProcessDesiredAccess,
	IN		ACCESS_MASK						ThreadDesiredAccess,
	IN		POBJECT_ATTRIBUTES				ProcessObjectAttributes,
	IN		POBJECT_ATTRIBUTES				ThreadObjectAttributes,
	IN		ULONG							ProcessFlags,
	IN		ULONG							ThreadFlags,
	IN		PRTL_USER_PROCESS_PARAMETERS	ProcessParameters,
	IN OUT	PVOID							CreateInfo,
	IN		PVOID							AttributeList);

NTSYSAPI NTSTATUS NTAPI NtAllocateVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN		ULONG_PTR	ZeroBits,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		AllocationType,
	IN		ULONG		Protect);

NTSYSAPI NTSTATUS NTAPI NtFreeVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		FreeType);

NTSYSAPI NTSTATUS NTAPI NtQueryVirtualMemory(
	IN		HANDLE			ProcessHandle,
	IN		PVOID			BaseAddress OPTIONAL,
	IN		MEMINFOCLASS	MemoryInformationClass,
	OUT		PVOID			MemoryInformation,
	IN		SIZE_T			MemoryInformationLength,
	OUT		PSIZE_T			ReturnLength OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtProtectVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN OUT	PVOID		*BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

NTSYSAPI NTSTATUS NTAPI NtReadVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN		PVOID		BaseAddress,
	OUT		PVOID		Buffer,
	IN		SIZE_T		BufferSize,
	OUT		PSIZE_T		NumberOfBytesRead OPTIONAL);

NTSYSAPI NTSTATUS NTAPI NtWriteVirtualMemory(
	IN		HANDLE		ProcessHandle,
	IN		PVOID		BaseAddress,
	IN		PCVOID		Buffer,
	IN		SIZE_T		BufferSize,
	OUT		PSIZE_T		NumberOfBytesWritten OPTIONAL);

NTSTATUS NTAPI RtlEnterCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection);

NTSTATUS NTAPI RtlLeaveCriticalSection(
	IN	PRTL_CRITICAL_SECTION	CriticalSection);

VOID NTAPI RtlAcquirePebLock(
	VOID);

VOID NTAPI RtlReleasePebLock(
	VOID);

ULONG NTAPI RtlNtStatusToDosError(
	IN	NTSTATUS	Status);

LONG NTAPI RtlGetLastWin32Error(
	VOID);

VOID NTAPI RtlSetLastWin32Error(
	IN	LONG	Win32Error);

VOID NTAPI RtlInitAnsiString(
	OUT	PANSI_STRING	DestinationString,
	IN	PCSTR			SourceString OPTIONAL);

VOID NTAPI RtlInitUnicodeString(
	OUT	PUNICODE_STRING	DestinationString,
	IN	PCWSTR			SourceString OPTIONAL);

NTSTATUS NTAPI RtlInitAnsiStringEx(
	OUT	PANSI_STRING	DestinationString,
	IN	PCSTR			SourceString OPTIONAL);

NTSTATUS NTAPI RtlInitUnicodeStringEx(
	OUT	PUNICODE_STRING	DestinationString,
	IN	PCWSTR			SourceString OPTIONAL);

NTSTATUS NTAPI RtlAnsiStringToUnicodeString(
	OUT	PUNICODE_STRING	DestinationString,
	IN	PCANSI_STRING	SourceString,
	IN	BOOLEAN			AllocateDestinationString);

BOOLEAN NTAPI RtlCreateUnicodeStringFromAsciiz(
	OUT	PUNICODE_STRING	DestinationString,
	IN	PCSTR			SourceString OPTIONAL);

BOOLEAN NTAPI RtlCreateUnicodeString(
	OUT	PUNICODE_STRING	DestinationString,
	IN	PCWSTR			SourceString);

NTSTATUS NTAPI RtlUpcaseUnicodeString(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCUNICODE_STRING	SourceString,
	IN	BOOLEAN				AllocateDestinationString);

NTSTATUS NTAPI RtlDowncaseUnicodeString(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCUNICODE_STRING	SourceString,
	IN	BOOLEAN				AllocateDestinationString);

VOID NTAPI RtlFreeUnicodeString(
	IN OUT	PUNICODE_STRING	UnicodeString);

BOOLEAN NTAPI RtlEqualUnicodeString(
	IN	PCUNICODE_STRING	String1,
	IN	PCUNICODE_STRING	String2,
	IN	BOOLEAN				CaseInsensitive);

BOOLEAN NTAPI RtlPrefixUnicodeString(
	IN	PCUNICODE_STRING	String1,
	IN	PCUNICODE_STRING	String2,
	IN	BOOLEAN				CaseInsensitive);

VOID NTAPI RtlCopyUnicodeString(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCUNICODE_STRING	SourceString OPTIONAL);

NTSTATUS NTAPI RtlAppendUnicodeToString(
	IN	PUNICODE_STRING		DestinationString,
	IN	PCWSTR				SourceString OPTIONAL);

NTSTATUS NTAPI RtlAppendUnicodeStringToString(
	IN OUT	PUNICODE_STRING		DestinationString,
	IN		PCUNICODE_STRING	SourceString);

NTSTATUS NTAPI RtlHashUnicodeString(
	IN	PCUNICODE_STRING	String,
	IN	BOOLEAN				CaseInsensitive,
	IN	ULONG				HashAlgorithm,
	OUT	PULONG				HashValue);

PVOID NTAPI RtlAllocateHeap(
	IN	PVOID	HeapHandle,
	IN	ULONG	Flags OPTIONAL,
	IN	SIZE_T	Size);

BOOLEAN NTAPI RtlFreeHeap(
	IN	PVOID	HeapHandle,
	IN	ULONG	Flags OPTIONAL,
	IN	PVOID	BaseAddress OPTIONAL);

NTSTATUS NTAPI RtlCreateEnvironment(
	IN	BOOLEAN	CloneCurrentEnvironment,
	OUT	PVOID	*Environment);

NTSTATUS NTAPI RtlDestroyEnvironment(
	IN	PVOID	Environment);

NTSTATUS NTAPI RtlSetCurrentEnvironment(
	IN	PVOID	Environment,
	OUT	PVOID	*PreviousEnvironment OPTIONAL);

NTSTATUS NTAPI RtlQueryEnvironmentVariable_U(
	IN		PVOID				Environment OPTIONAL,
	IN		PCUNICODE_STRING	Name,
	IN OUT	PUNICODE_STRING		Value);

NTSTATUS NTAPI RtlSetEnvironmentVariable(
	IN OUT	PVOID				*Environment OPTIONAL,
	IN		PCUNICODE_STRING	Name,
	IN		PCUNICODE_STRING	Value OPTIONAL);

NTSTATUS NTAPI RtlSetEnvironmentStrings(
	IN	PWCHAR	NewEnvironment,
	IN	SIZE_T	NewEnvironmentSize);

NTSTATUS NTAPI LdrRegisterDllNotification(
	IN	ULONG							Flags,					// Unused. Must be zero.
	IN	PLDR_DLL_NOTIFICATION_FUNCTION	NotificationFunction,	// function called when any DLL is loaded/unloaded
	IN	PVOID							Context OPTIONAL,		// opaque data passed to the notification function
	OUT	PVOID							*Cookie);				// Store a opaque cookie which you can pass to unregister

NTSTATUS NTAPI LdrUnregisterDllNotification(
	IN	PVOID	Cookie);

NTSTATUS NTAPI DbgUiConnectToDbg(
	VOID);

NTSTATUS NTAPI DbgUiDebugActiveProcess(
	IN	HANDLE	ProcessHandle);

#define LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS	(0x00000001)
#define LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY			(0x00000002) // Required to pass Disposition ptr with this flag

#define LDR_LOCK_LOADER_LOCK_DISPOSITION_INVALID			(0)
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED		(1)
#define LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_NOT_ACQUIRED	(2)

NTSTATUS NTAPI LdrLockLoaderLock(
	IN	ULONG	Flags,
	OUT	PULONG	Disposition OPTIONAL,
	OUT	PPVOID	Cookie);

NTSTATUS NTAPI LdrUnlockLoaderLock(
	IN	ULONG	Flags,
	IN	PVOID	Cookie);

//
// Miscellaneous macros and inline functions
//

#define GetProcessHeap() (NtCurrentPeb()->ProcessHeap)
#define HeapAlloc(hHeap, dwFlags, cbSize) RtlAllocateHeap((hHeap), (dwFlags), (cbSize))
#define HeapFree(hHeap, dwFlags, lpBase) RtlFreeHeap((hHeap), (dwFlags), (lpBase))
#define NtCurrentProcess() ((HANDLE) -1)
#define GetCurrentProcess NtCurrentProcess

FORCEINLINE PPEB NtCurrentPeb(
	VOID)
{
#ifdef _M_X64
	return (PPEB) __readgsqword(0x60);
#else
	return (PPEB) __readfsdword(0x30);
#endif
}

FORCEINLINE VOID InitializeObjectAttributes(
	OUT	POBJECT_ATTRIBUTES		p,
	IN	PUNICODE_STRING			n,
	IN	ULONG					a,
	IN	HANDLE					r,
	IN	PSECURITY_DESCRIPTOR	s)
{
	p->Length = sizeof(OBJECT_ATTRIBUTES);
	p->RootDirectory = r;
	p->Attributes = a;
	p->ObjectName = n;
	p->SecurityDescriptor = s;
	p->SecurityQualityOfService = NULL;
}

// doubly linked list functions

#ifndef __INTELLISENSE__
#  define ForEachListEntry(pListHead, pListEntry) for (((PLIST_ENTRY) (pListEntry)) = (pListHead)->Flink; ((PLIST_ENTRY) (pListEntry)) != (pListHead); ((PLIST_ENTRY) (pListEntry)) = ((PLIST_ENTRY) (pListEntry))->Flink)
#else
#  define ForEachListEntry(pListHead, pListEntry)
#endif

FORCEINLINE VOID InitializeListHead(
	OUT	PLIST_ENTRY ListHead)
{
	ListHead->Flink = ListHead;
	ListHead->Blink = ListHead;
}

FORCEINLINE BOOL IsListEmpty(
	IN	PLIST_ENTRY	ListHead)
{
	return ListHead->Flink == ListHead;
}

FORCEINLINE VOID RemoveEntryList(
	OUT	PLIST_ENTRY	Entry)
{
	PLIST_ENTRY Blink = Entry->Blink;
	PLIST_ENTRY Flink = Entry->Flink;
	Blink->Flink = Flink;
	Flink->Blink = Blink;
}

FORCEINLINE PLIST_ENTRY RemoveHeadList(
	OUT	PLIST_ENTRY ListHead)
{
	PLIST_ENTRY Ret = ListHead->Flink;
	RemoveEntryList(ListHead->Flink);
	return Ret;
}

FORCEINLINE PLIST_ENTRY RemoveTailList(
	OUT	PLIST_ENTRY	ListHead)
{
	PLIST_ENTRY Blink = ListHead->Blink;
	RemoveEntryList(ListHead->Blink);
	return Blink;
}

FORCEINLINE VOID InsertTailList(
	OUT	PLIST_ENTRY ListHead,
	IN	PLIST_ENTRY	Entry)
{
	Entry->Flink = ListHead;
	Entry->Blink = ListHead->Blink;
	ListHead->Blink->Flink = Entry;
	ListHead->Blink = Entry;
}

FORCEINLINE VOID InsertHeadList(
	OUT	PLIST_ENTRY	ListHead,
	IN	PLIST_ENTRY	Entry)
{
	Entry->Flink = ListHead->Flink;
	Entry->Blink = ListHead;
	ListHead->Flink->Blink = Entry;
	ListHead->Flink = Entry;
}

// singly linked list functions

FORCEINLINE VOID PushEntryList(
	OUT	PSINGLE_LIST_ENTRY	ListHead,
	IN	PSINGLE_LIST_ENTRY	Entry)
{
	Entry->Next = ListHead->Next;
	ListHead->Next = Entry;
}

FORCEINLINE PSINGLE_LIST_ENTRY PopEntryList(
	OUT	PSINGLE_LIST_ENTRY	ListHead)
{
	PSINGLE_LIST_ENTRY FirstEntry = ListHead->Next;

	if (FirstEntry) {
		ListHead->Next = FirstEntry->Next;
	}

	return FirstEntry;
}