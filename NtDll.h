#pragma once
#include <Windows.h>
#include <KexComm.h>

typedef LONG NTSTATUS;
#define STATUS_SUCCESS ((NTSTATUS) 0)
#define NT_SUCCESS(st) (((NTSTATUS) (st)) >= 0)

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

typedef struct _UNICODE_STRING {
	USHORT								Length;
	USHORT								MaximumLength;
	PWCHAR								Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _UNICODE_STRING32 {
	USHORT								Length;
	USHORT								MaximumLength;
	DWORD								Buffer;
} UNICODE_STRING32, *PUNICODE_STRING32;

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

STATIC CONST PKUSER_SHARED_DATA SharedUserData = (PKUSER_SHARED_DATA) 0x7FFE0000;

//
// NTDLL function declarations
//

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

ULONG NTAPI RtlNtStatusToDosError(
	IN	NTSTATUS	Status);

LONG NTAPI RtlGetLastWin32Error(
	VOID);

VOID NTAPI RtlSetLastWin32Error(
	IN	LONG	Win32Error);

VOID NTAPI RtlInitUnicodeString(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCWSTR				SourceString OPTIONAL);

PVOID NTAPI RtlAllocateHeap(
	IN	PVOID	HeapHandle,
	IN	ULONG	Flags OPTIONAL,
	IN	SIZE_T	Size);

BOOLEAN NTAPI RtlFreeHeap(
	IN	PVOID	HeapHandle,
	IN	ULONG	Flags OPTIONAL,
	IN	PVOID	BaseAddress OPTIONAL);

//
// Miscellaneous macros and inline functions
//

#define GetProcessHeap() (NtCurrentPeb()->ProcessHeap)
#define HeapAlloc(hHeap, dwFlags, cbSize) RtlAllocateHeap((hHeap), (dwFlags), (cbSize))
#define HeapFree(hHeap, dwFlags, lpBase) RtlFreeHeap((hHeap), (dwFlags), (lpBase))
#define GetCurrentProcess() ((HANDLE) -1)

FORCEINLINE PPEB NtCurrentPeb(
	VOID)
{
#ifdef _M_X64
	return (PPEB) __readgsqword(0x60);
#else
	return (PPEB) __readfsdword(0x30);
#endif
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