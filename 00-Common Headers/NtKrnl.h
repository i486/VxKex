#pragma once

typedef struct _DRIVER_OBJECT TYPEDEF_TYPE_NAME(DRIVER_OBJECT);
typedef struct _DEVICE_OBJECT TYPEDEF_TYPE_NAME(DEVICE_OBJECT);
typedef struct _IRP TYPEDEF_TYPE_NAME(IRP);
typedef struct _KDPC TYPEDEF_TYPE_NAME(KDPC);
typedef struct _FILE_OBJECT TYPEDEF_TYPE_NAME(FILE_OBJECT);
typedef struct _MDL TYPEDEF_TYPE_NAME(MDL);

typedef enum _IO_ALLOCATION_ACTION {
	KeepObject = 1,
	DeallocateObject,
	DeallocateObjectKeepRegisters,
} TYPEDEF_TYPE_NAME(IO_ALLOCATION_ACTION);

typedef IO_ALLOCATION_ACTION (NTAPI *PDRIVER_CONTROL) (
	IN		PDEVICE_OBJECT	DeviceObject,
	IN OUT	PIRP			Irp,
	IN		PVOID			MapRegisterBase,
	IN		PVOID			Context);

#define VPB_MOUNTED                     0x00000001
#define VPB_LOCKED                      0x00000002
#define VPB_PERSISTENT                  0x00000004
#define VPB_REMOVE_PENDING              0x00000008
#define VPB_RAW_MOUNT                   0x00000010
#define VPB_DIRECT_WRITES_ALLOWED       0x00000020

#define MAXIMUM_VOLUME_LABEL_LENGTH  (32 * sizeof(WCHAR)) // 32 characters

typedef struct _VPB {
	CSHORT			Type;
	CSHORT			Size;
	USHORT			Flags;				// VPB_*
	USHORT			VolumeLabelLength;	// in bytes
	PDEVICE_OBJECT	DeviceObject;
	PDEVICE_OBJECT	RealDevice;
	ULONG			SerialNumber;
	ULONG			ReferenceCount;
	WCHAR			VolumeLabel[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
} TYPEDEF_TYPE_NAME(VPB);

typedef struct _KDEVICE_QUEUE_ENTRY {
	LIST_ENTRY	DeviceListEntry;
	ULONG		SortKey;
	BOOLEAN		Inserted;
} TYPEDEF_TYPE_NAME(KDEVICE_QUEUE_ENTRY);

typedef VOID (NTAPI *PKDEFERRED_ROUTINE) (
	IN	PKDPC	Dpc,
	IN	PVOID	DeferredContext OPTIONAL,
	IN	PVOID	SystemArgument1 OPTIONAL,
	IN	PVOID	SystemArgument2 OPTIONAL);

typedef struct _KDPC {
	UCHAR				Type;
	UCHAR				Importance;
	VOLATILE USHORT		Number;
	LIST_ENTRY			DpcListEntry;
	PKDEFERRED_ROUTINE	DeferredRoutine;
	PVOID				DeferredContext;
	PVOID				SystemArgument1;
	PVOID				SystemArgument2;
	VOLATILE PVOID		DpcData;
} TYPEDEF_TYPE_NAME(KDPC);

typedef struct _WAIT_CONTEXT_BLOCK {
	KDEVICE_QUEUE_ENTRY	WaitQueueEntry;
	PDRIVER_CONTROL		DeviceRoutine;
	PVOID				DeviceContext;
	ULONG				NumberOfMapRegisters;
	PDEVICE_OBJECT		DeviceObject;
	PIRP				CurrentIrp;
	PKDPC				BufferChainingDpc;
} TYPEDEF_TYPE_NAME(WAIT_CONTEXT_BLOCK);

typedef VOID (NTAPI *PIO_TIMER_ROUTINE) (
	IN	PDEVICE_OBJECT	DeviceObject,
	IN	PVOID			Context OPTIONAL);

typedef struct _IO_TIMER {
	CSHORT				Type;
	CSHORT				TimerFlag;
	LIST_ENTRY			TimerList;
	PIO_TIMER_ROUTINE	TimerRoutine;
	PVOID				Context;
	PDEVICE_OBJECT		DeviceObject;
} TYPEDEF_TYPE_NAME(IO_TIMER);

typedef struct _KDEVICE_QUEUE {
	CSHORT				Type;
	CSHORT				Size;
	LIST_ENTRY			DeviceListHead;
	KSPIN_LOCK			Lock;

#ifdef _M_X64
	union {
		BOOLEAN			Busy;
		struct {
			LONGLONG	Reserved	: 8;
			LONGLONG	Hint		: 56;
		};
	};
#else
	BOOLEAN				Busy;
#endif
} TYPEDEF_TYPE_NAME(KDEVICE_QUEUE);

//
// Common dispatcher object header
//
// N.B. The size field contains the number of dwords in the structure.
//

#define TIMER_EXPIRED_INDEX_BITS		6
#define TIMER_PROCESSOR_INDEX_BITS		5

typedef struct _DISPATCHER_HEADER {
	union {
		struct {
			UCHAR	Type;	// All (accessible via KOBJECT_TYPE)

			union {
				union {	// Timer
					UCHAR	TimerControlFlags;

					struct {
						UCHAR	Absolute				: 1;
						UCHAR	Coalescable				: 1;
						UCHAR	KeepShifting			: 1;	// Periodic timer
						UCHAR	EncodedTolerableDelay	: 5;	// Periodic timer
					};
				};

				UCHAR	Abandoned;	// Queue
				BOOLEAN	Signalling;	// Gate/Events
			};

			union {
				union {
					UCHAR	ThreadControlFlags;	// Thread

					struct {
						UCHAR	CpuThrottled	: 1;
						UCHAR	CycleProfiling	: 1;
						UCHAR	CounterProfiling: 1;
						UCHAR	Reserved		: 5;
					};
				};

				UCHAR	Hand;	// Timer
				UCHAR	Size;	// All other objects
			};

			union {
				union {	// Timer
					UCHAR	TimerMiscFlags;

					struct {

#ifdef _M_X64
						UCHAR			Index		: TIMER_EXPIRED_INDEX_BITS;
#else
						UCHAR			Index		: 1;
						UCHAR			Processor	: TIMER_PROCESSOR_INDEX_BITS;
#endif

						UCHAR			Inserted	: 1;
						VOLATILE UCHAR	Expired		: 1;
					};
				};
				
				union {	// Thread
					BOOLEAN				DebugActive;

					struct {
						BOOLEAN	ActiveDR7	: 1;
						BOOLEAN	Instrumented: 1;
						BOOLEAN	Reserved2	: 4;
						BOOLEAN	UmsScheduled: 1;
						BOOLEAN	UmsPrimary	: 1;
					};
				};

				BOOLEAN	DpcActive;	// Mutant
			};
		};

		VOLATILE LONG	Lock;	// Interlocked
	};

	LONG		SignalState;	// Object lock
	LIST_ENTRY	WaitListHead;	// Object lock
} TYPEDEF_TYPE_NAME(DISPATCHER_HEADER);

typedef struct _KEVENT {
	DISPATCHER_HEADER	Header;
} TYPEDEF_TYPE_NAME(KEVENT);

typedef struct _DEVOBJ_EXTENSION {
	CSHORT			Type;
	USHORT			Size;
	PDEVICE_OBJECT	DeviceObject;
} TYPEDEF_TYPE_NAME(DEVOBJ_EXTENSION);

typedef struct DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) _DEVICE_OBJECT {
	CSHORT					Type;
	CSHORT					Size;
	LONG					ReferenceCount;

	PDRIVER_OBJECT			DriverObject;
	PDEVICE_OBJECT			NextDevice;
	PDEVICE_OBJECT			AttachedDevice;
	PIRP					CurrentIrp;
	PIO_TIMER				Timer;
	ULONG					Flags;
	ULONG					Characteristics;
	VOLATILE PVPB			Vpb;
	PVOID					DeviceExtension;
	DEVICE_TYPE				DeviceType;
	CCHAR					StackSize;

	union {
		LIST_ENTRY			ListEntry;
		WAIT_CONTEXT_BLOCK	Wcb;
	} Queue;

	ULONG					AlignmentRequirement;
	KDEVICE_QUEUE			DeviceQueue;
	KDPC					Dpc;

	ULONG					ActiveThreadCount;
	PSECURITY_DESCRIPTOR	SecurityDescriptor;
	KEVENT					DeviceLock;

	USHORT					SectorSize;
	USHORT					Spare1;

	PDEVOBJ_EXTENSION		DeviceObjectExtension;
	PVOID					Reserved;
} TYPEDEF_TYPE_NAME(DEVICE_OBJECT);

typedef NTSTATUS (NTAPI *PDRIVER_ADD_DEVICE) (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PDEVICE_OBJECT	PhysicalDeviceObject);

typedef struct _DRIVER_EXTENSION {
	PDRIVER_OBJECT		DriverObject;
	PDRIVER_ADD_DEVICE	AddDevice;
	ULONG				Count;
	UNICODE_STRING		ServiceKeyName;
} TYPEDEF_TYPE_NAME(DRIVER_EXTENSION);

typedef struct _SECTION_OBJECT_POINTERS {
	PVOID	DataSectionObject;
	PVOID	SharedCacheMap;
	PVOID	ImageSectionObject;
} TYPEDEF_TYPE_NAME(SECTION_OBJECT_POINTERS);

typedef struct _IO_COMPLETION_CONTEXT {
	PVOID	Port;
	PVOID	Key;
} TYPEDEF_TYPE_NAME(IO_COMPLETION_CONTEXT);

#define FO_FILE_OPEN                    0x00000001
#define FO_SYNCHRONOUS_IO               0x00000002
#define FO_ALERTABLE_IO                 0x00000004
#define FO_NO_INTERMEDIATE_BUFFERING    0x00000008
#define FO_WRITE_THROUGH                0x00000010
#define FO_SEQUENTIAL_ONLY              0x00000020
#define FO_CACHE_SUPPORTED              0x00000040
#define FO_NAMED_PIPE                   0x00000080
#define FO_STREAM_FILE                  0x00000100
#define FO_MAILSLOT                     0x00000200
#define FO_GENERATE_AUDIT_ON_CLOSE      0x00000400
#define FO_QUEUE_IRP_TO_THREAD          FO_GENERATE_AUDIT_ON_CLOSE
#define FO_DIRECT_DEVICE_OPEN           0x00000800
#define FO_FILE_MODIFIED                0x00001000
#define FO_FILE_SIZE_CHANGED            0x00002000
#define FO_CLEANUP_COMPLETE             0x00004000
#define FO_TEMPORARY_FILE               0x00008000
#define FO_DELETE_ON_CLOSE              0x00010000
#define FO_OPENED_CASE_SENSITIVE        0x00020000
#define FO_HANDLE_CREATED               0x00040000
#define FO_FILE_FAST_IO_READ            0x00080000
#define FO_RANDOM_ACCESS                0x00100000
#define FO_FILE_OPEN_CANCELLED          0x00200000
#define FO_VOLUME_OPEN                  0x00400000
#define FO_REMOTE_ORIGIN                0x01000000
#define FO_DISALLOW_EXCLUSIVE           0x02000000
#define FO_SKIP_COMPLETION_PORT         FO_DISALLOW_EXCLUSIVE
#define FO_SKIP_SET_EVENT               0x04000000
#define FO_SKIP_SET_FAST_IO             0x08000000

typedef struct _FILE_OBJECT {
    CSHORT							Type;
    CSHORT							Size;
    PDEVICE_OBJECT					DeviceObject;
    PVPB							Vpb;
    PVOID							FsContext;
    PVOID							FsContext2;
    PSECTION_OBJECT_POINTERS		SectionObjectPointer;
    PVOID							PrivateCacheMap;
    NTSTATUS						FinalStatus;
    PFILE_OBJECT					RelatedFileObject;
    BOOLEAN							LockOperation;
    BOOLEAN							DeletePending;
    BOOLEAN							ReadAccess;
    BOOLEAN							WriteAccess;
    BOOLEAN							DeleteAccess;
    BOOLEAN							SharedRead;
    BOOLEAN							SharedWrite;
    BOOLEAN							SharedDelete;
    ULONG							Flags; // FO_*
    UNICODE_STRING					FileName;
    LARGE_INTEGER					CurrentByteOffset;
    VOLATILE ULONG					Waiters;
    VOLATILE ULONG					Busy;
    PVOID							LastLock;
    KEVENT							Lock;
    KEVENT							Event;
    VOLATILE PIO_COMPLETION_CONTEXT	CompletionContext;
    KSPIN_LOCK						IrpListLock;
    LIST_ENTRY						IrpList;
    VOLATILE PVOID					FileObjectExtension;
} TYPEDEF_TYPE_NAME(FILE_OBJECT);

#ifdef _M_X64
#  define MAX_PROC_GROUPS 4
#else
#  define MAX_PROC_GROUPS 1
#endif

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ke/affinity/kaffinity_ex.htm
typedef struct _KAFFINITY_EX {
	USHORT		Count;
	USHORT		Size;
	ULONG		Reserved;
	KAFFINITY	Bitmap[MAX_PROC_GROUPS];
} TYPEDEF_TYPE_NAME(KAFFINITY_EX);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kexecute_options.htm
typedef union _KEXECUTE_OPTIONS {
	struct {
		UCHAR	ExecuteDisable					: 1;
		UCHAR	ExecuteEnable					: 1;
		UCHAR	DisableThunkEmulation			: 1;
		UCHAR	Permanent						: 1;
		UCHAR	ExecuteDispatchEnable			: 1;
		UCHAR	ImageDispatchEnable				: 1;
		UCHAR	DisableExceptionChainValidation	: 1;
		UCHAR	Spare							: 1;
	};

	VOLATILE UCHAR	ExecuteOptions;
	UCHAR			ExecuteOptionsNV;
} TYPEDEF_TYPE_NAME(KEXECUTE_OPTIONS);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kprocess_state.htm
typedef enum _KPROCESS_STATE {
	ProcessInMemory,
	ProcessOutOfMemory,
	ProcessInTransition,
	ProcessOutTransition,
	ProcessInSwap,
	ProcessOutSwap,
	ProcessAllSwapStates
} TYPEDEF_TYPE_NAME(KPROCESS_STATE);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kstack_count.htm
typedef union _KSTACK_COUNT {
	struct {
		VOLATILE ULONG	State		: 3;	// KPROCESS_STATE
		ULONG			StackCount	: 29;
	};

	VOLATILE LONG	Value;
} TYPEDEF_TYPE_NAME(KSTACK_COUNT);

typedef union _KGDTENTRY64 {
	struct {
		USHORT	LimitLow;
		USHORT	BaseLow;

		struct {
			CHAR	BaseMiddle;
			CHAR	Flags1;
			CHAR	Flags2;
			CHAR	BaseHigh;
		} Bytes;
	  
		ULONG		BaseUpper;
		ULONG		MustBeZero;
	};

	struct {
		BYTE		gap0[4];
		ULONG		BaseMiddle	: 8;
		ULONG		Type		: 5;
		ULONG		Dpl			: 2;
		ULONG		Present		: 1;
		ULONG		LimitHigh	: 4;
		ULONG		System		: 1;
		ULONG		LongMode	: 1;
		ULONG		DefaultBig	: 1;
		ULONG		Granularity	: 1;
		ULONG		BaseHigh	: 8;
	};

	ULONG_PTR Alignment;
} TYPEDEF_TYPE_NAME(KGDTENTRY64);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kwait_state.htm
typedef enum _KWAIT_STATE {
	WaitInProgress,
	WaitCommitted,
	WaitAborted,
	MaximumWaitState
} TYPEDEF_TYPE_NAME(KWAIT_STATE);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kwait_status_register.htm
typedef struct _KWAIT_STATUS_REGISTER {
	UCHAR	State			: 2;	// KWAIT_STATE
	UCHAR	Affinity		: 1;
	UCHAR	Priority		: 1;
	UCHAR	Apc				: 1;
	UCHAR	UserApc			: 1;
	UCHAR	Alert			: 1;
	UCHAR	Unused			: 1;
} TYPEDEF_TYPE_NAME(KWAIT_STATUS_REGISTER);

typedef struct _KQUEUE {
	DISPATCHER_HEADER	Header;
	LIST_ENTRY			EntryListHead;	// Object lock
	VOLATILE ULONG		CurrentCount;	// Interlocked
	ULONG				MaximumCount;
	LIST_ENTRY			ThreadListHead;	// Object lock
} TYPEDEF_TYPE_NAME(KQUEUE);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kthread/late52.htm
typedef struct _KTHREAD {
	DISPATCHER_HEADER					Header;
	VOLATILE ULONGLONG					CycleTime;
	VOLATILE ULONG						HighCycleTime;
	ULONGLONG							QuantumTarget;
	PVOID								InitialStack;
	VOLATILE PVOID						StackLimit;
	PVOID								KernelStack;
	KSPIN_LOCK							ThreadLock;
	KWAIT_STATUS_REGISTER				WaitRegister;
	VOLATILE BOOLEAN					Running;
	BOOLEAN								Alerted[2];

	union {
		struct {
			ULONG						KernelStackResident		: 1;
			ULONG						ProcessReadyQueue		: 1;
			ULONG						WaitNext				: 1;
			ULONG						SystemAffinityActive	: 1;
			ULONG						Alertable				: 1;
			ULONG						GdiFlushActive			: 1;
			ULONG						UserStackWalkActive		: 1;
			ULONG						ApcInterruptRequest		: 1;
			ULONG						ForceDeferSchedule		: 1;
			ULONG						QuantumEndMigrate		: 1;
			ULONG						UmsDirectedSwitchEnable	: 1;
			ULONG						TimerActive				: 1;
			ULONG						Reserved				: 19;
		};

		LONG							MiscFlags;
	};

	union {
		LIST_ENTRY						WaitListEntry;
		SINGLE_LIST_ENTRY				SwapListEntry;
		KQUEUE *VOLATILE				Queue;
	};

	// There are more members after this. I'm not clear on exactly how they are laid
	// out so I will omit them for now.
} TYPEDEF_TYPE_NAME(KTHREAD);

typedef struct _ETHREAD {
	KTHREAD						Tcb;
	LARGE_INTEGER				CreateTime;

	union {
		LARGE_INTEGER			ExitTime;
		LIST_ENTRY				KeyedWaitChain;
	};

	LONG						ExitStatus;

	// more members later on...
} TYPEDEF_TYPE_NAME(ETHREAD);

typedef struct _KGATE {
	DISPATCHER_HEADER	Header;
} TYPEDEF_TYPE_NAME(KGATE);

typedef struct _KGUARDED_MUTEX {
	VOLATILE INT	Count;
	PKTHREAD		Owner;
	ULONG			Contention;
	KGATE			Gate;

	union {
		struct {
			USHORT	KernelApcDisable;
			USHORT	SpecialApcDisable;
		};

		ULONG		CombinedApcDisable;
	};
} TYPEDEF_TYPE_NAME(KGUARDED_MUTEX);

// https://www.geoffchappell.com/studies/windows/km/ntoskrnl/inc/ntos/ke/kprocess/index.htm
typedef struct DECLSPEC_ALIGN(8) _KPROCESS {
	DISPATCHER_HEADER		Header;
	LIST_ENTRY				ProfileListHead;
	ULONG_PTR				DirectoryTableBase;
#ifndef _M_X64
	KGDTENTRY				LdtDescriptor;
	KIDTENTRY				Int21Descriptor;
#endif
	LIST_ENTRY				ThreadListHead;
	ULONG_PTR				ProcessLock;
	KAFFINITY_EX			Affinity;
	LIST_ENTRY				ReadyListHead;
	SINGLE_LIST_ENTRY		SwapListEntry;
	KAFFINITY_EX			ActiveProcessors;
	
	union {
		struct {
			VOLATILE LONG	AutoAlignment		: 1;
			VOLATILE LONG	DisableBoost		: 1;
			VOLATILE LONG	DisableQuantum		: 1;
			VOLATILE ULONG	ActiveGroupsMask	: MAX_PROC_GROUPS;
		};

		VOLATILE LONG		ProcessFlags;
	};

	CHAR					BasePriority;
	CHAR					QuantumReset;
	BOOLEAN					Visited;
	UCHAR					Unused3;
	ULONG					ThreadSeed[4];
	USHORT					IdealNode[4];
	USHORT					IdealGlobalNode;
	KEXECUTE_OPTIONS		Flags;
	UCHAR					Unused1;

#ifdef _M_X64
	ULONG					Unused2;
#else
	USHORT					IopmOffset;
#endif

	ULONG					Unused4;
	KSTACK_COUNT			StackCount;
	LIST_ENTRY				ProcessListEntry;
	VOLATILE ULONGLONG		CycleTime;
	ULONG					KernelTime;
	ULONG					UserTime;

#ifdef _M_X64
	PVOID					InstrumentationCallback;
	KGDTENTRY64				LdtSystemDescriptor;
	PVOID					LdtBaseAddress;
	KGUARDED_MUTEX			LdtProcessLock;
	USHORT					LdtFreeSelectorHint;
	USHORT					LdtTableLength;
#else
	PVOID					VdmTrapcHandler;
#endif
} TYPEDEF_TYPE_NAME(KPROCESS);

typedef union _EX_PUSH_LOCK {
	struct {
		ULONG_PTR	Locked			: 1;
		ULONG_PTR	Waiting			: 1;
		ULONG_PTR	Waking			: 1;
		ULONG_PTR	MultipleShared	: 1;
#ifdef _M_X64
		ULONG_PTR	Shared			: 60;
#else
		ULONG_PTR	Shared			: 28;
#endif
	};

	ULONG_PTR		Value;
	PVOID			Ptr;
} TYPEDEF_TYPE_NAME(EX_PUSH_LOCK);

typedef union _EX_RUNDOWN_REF {
	ULONG_PTR	Count;
	PVOID		Ptr;
} TYPEDEF_TYPE_NAME(EX_RUNDOWN_REF);

typedef struct _EPROCESS {
	KPROCESS						Pcb;
	EX_PUSH_LOCK					ProcessLock;
	LARGE_INTEGER					CreateTime;
	LARGE_INTEGER					ExitTime;
	EX_RUNDOWN_REF					RundownProtect;
	HANDLE							UniqueProcessId;
	LIST_ENTRY						ActiveProcessLinks;
	ULONG_PTR						ProcessQuotaUsage[2];
	ULONG_PTR						ProcessQuotaPeak[2];
	VOLATILE ULONG_PTR				CommitCharge;
	PVOID							QuotaBlock;
//	PS_CPU_QUOTA_BLOCK				CpuQuotaBlock;
//	ULONG_PTR						PeakVirtualSize;
//	ULONG_PTR						VirtualSize;
//	LIST_ENTRY						SessionProcessLinks;
//	PVOID							DebugPort;
//
//	union {
//		PVOID						ExceptionPortData;
//		ULONG_PTR					ExceptionPortValue;
//		ULONG_PTR					ExceptionPortState	: 3;
//	};
//
//	PHANDLE_TABLE					ObjectTable;
//	EX_FAST_REF						Token;
//	ULONG_PTR						WorkingSetPage;
//	EX_PUSH_LOCK					AddressCreationLock;
//	PETHREAD						RotateInProcess;
//	PETHREAD						ForkInProcess;
//	ULONG_PTR						HardwareTrigger;
//	PMM_AVL_TABLE					PhysicalVadRoot;
//	PVOID							CloneRoot;
//	VOLATILE ULONG_PTR				NumberOfPrivatePages;
//	VOLATILE ULONG_PTR				NumberOfLockedPages;
//	PVOID							Win32Process;
//	EJOB *VOLATILE					Job;
//	PVOID							SectionObject;
//	PVOID							SectionBaseAddress;
//	ULONG							Cookie;
//
//#ifdef _M_X64
//	ULONG							UmsScheduledThreads;
//#else
//	ULONG							Spare8;
//#endif
//
//	PPAGEFAULT_HISTORY				WorkingSetWatch;
//	PVOID							Win32WindowStation;
//	HANDLE							InheritedFromUniqueProcessId;
//	PVOID							LdtInformation;
//
//#ifdef _M_X64
//	PVOID							Spare;
//#else
//	PVOID							VdmObjects;
//#endif
//
//	ULONG_PTR						ConsoleHostProcess;
//	PVOID							DeviceMap;
//	PVOID							EtwDataSource;
//	PVOID							FreeTebHint;
//	PVOID							FreeUmsTebHint;
//
//	union {
//		HARDWARE_PTE				PageDirectoryPte;
//		ULONGLONG					Filler;
//	};
//
//	PVOID							Session;
//	UCHAR							ImageFileName[0x0F];
//	UCHAR							PriorityClass;
//	LIST_ENTRY						JobLinks;
//	PVOID							LockedPagesList;
//	LIST_ENTRY						ThreadListHead;
//	PVOID							SecurityPort;
//
//#ifdef _M_X64
//	PVOID							Wow64Process;
//#else
//	PVOID							PaeTop;
//#endif
//
//	VOLATILE ULONG					ActiveThreads;
//	ULONG							ImagePathHash;
//	ULONG							DefaultHardErrorProcessing;
//	LONG							LastThreadExitStatus;
//	PPEB							Peb;
//	EX_FAST_REF						PrefetchTrace;
//
//	LARGE_INTEGER					ReadOperationCount;
//	LARGE_INTEGER					WriteOperationCount;
//	LARGE_INTEGER					OtherOperationCount;
//	LARGE_INTEGER					ReadTransferCount;
//	LARGE_INTEGER					WriteTransferCount;
//	LARGE_INTEGER					OtherTransferCount;
//
//	ULONGLONG						CommitChargeLimit;
//	VOLATILE ULONGLONG				CommitChargePeak;
//	PVOID							AweInfo;
//	SE_AUDIT_PROCESS_CREATION_INFO	SeAuditProcessCreationInfo;
//	MMSUPPORT						Vm;
//	LIST_ENTRY						MmProcessLinks;
//	PVOID							HighestUserAddress;
//	ULONG							ModifiedPageCount;
//
//	union {
//		struct {
//			ULONG					JobNotReallyActive:1;
//			ULONG					AccountingFolded:1;
//			ULONG					NewProcessReported:1;
//			ULONG					ExitProcessReported:1;
//			ULONG					ReportCommitChanges:1;
//			ULONG					LastReportMemory:1;
//			ULONG					ReportPhysicalPageChanges:1;
//			ULONG					HandleTableRundown:1;
//			ULONG					NeedsHandleRundown:1;
//			ULONG					RefTraceEnabled:1;
//			ULONG					NumaAware:1;
//			ULONG					ProtectedProcess:1;
//			ULONG					DefaultPagePriority:3;
//			ULONG					PrimaryTokenFrozen:1;
//			ULONG					ProcessVerifierTarget:1;
//			ULONG					StackRandomizationDisabled:1;
//			ULONG					AffinityPermanent:1;
//			ULONG					AffinityUpdateEnable:1;
//			ULONG					PropagateNode:1;
//			ULONG					ExplicitAffinity:1;
//		};
//
//		ULONG						Flags2;
//	};
//
//	union {
//		struct
//		{
//			ULONG					CreateReported:1;
//			ULONG					NoDebugInherit:1;
//			ULONG					ProcessExiting:1;
//			ULONG					ProcessDelete:1;
//			ULONG					Wow64SplitPages:1;
//			ULONG					VmDeleted:1;
//			ULONG					OutswapEnabled:1;
//			ULONG					Outswapped:1;
//			ULONG					ForkFailed:1;
//			ULONG					Wow64VaSpace4Gb:1;
//			ULONG					AddressSpaceInitialized:2;
//			ULONG					SetTimerResolution:1;
//			ULONG					BreakOnTermination:1;
//			ULONG					DeprioritizeViews:1;
//			ULONG					WriteWatch:1;
//			ULONG					ProcessInSession:1;
//			ULONG					OverrideAddressSpace:1;
//			ULONG					HasAddressSpace:1;
//			ULONG					LaunchPrefetched:1;
//			ULONG					InjectInpageErrors:1;
//			ULONG					VmTopDown:1;
//			ULONG					ImageNotifyDone:1;
//			ULONG					PdeUpdateNeeded:1;
//			ULONG					VdmAllowed:1;
//			ULONG					CrossSessionCreate:1;
//			ULONG					ProcessInserted:1;
//			ULONG					DefaultIoPriority:3;
//			ULONG					ProcessSelfDelete:1;
//			ULONG					SetTimerResolutionLink:1;
//		};
//
//		ULONG						Flags;
//	};
//
//	LONG							ExitStatus;
//    MM_AVL_TABLE					VadRoot;
//    ALPC_PROCESS_CONTEXT			AlpcContext;
//    LIST_ENTRY						TimerResolutionLink;
//    ULONG							RequestedTimerResolution;
//    ULONG							ActiveThreadsHighWatermark;
//    ULONG							SmallestTimerResolution;
//    PPO_DIAG_STACK_RECORD			TimerResolutionStackRecord;
} TYPEDEF_TYPE_NAME(EPROCESS);

typedef struct _MDL {
	PMDL		Next;
	CSHORT		Size;
	CSHORT		MdlFlags;
	PEPROCESS	Process;
	PVOID		MappedSystemVa;
	PVOID		StartVa;
	ULONG		ByteCount;
	ULONG		ByteOffset;
} TYPEDEF_TYPE_NAME(MDL);

typedef struct _KMUTANT {
	DISPATCHER_HEADER	Header;
	LIST_ENTRY			MutantListEntry;
	PKTHREAD			OwnerThread;
	BOOLEAN				Abandoned;
	UCHAR				ApcDisable;
} TYPEDEF_TYPE_NAME(KMUTANT);

typedef struct _KSEMAPHORE {
	DISPATCHER_HEADER	Header;
	LONG				Limit;
} TYPEDEF_TYPE_NAME(KSEMAPHORE);

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
	ERESOURCE_THREAD OwnerThread;

	union {
		struct {
			ULONG IoPriorityBoosted	: 1;
			ULONG OwnerReferenced	: 1;
			ULONG OwnerCount		: 30;
		};

		ULONG TableSize;
	};
} TYPEDEF_TYPE_NAME(OWNER_ENTRY);

typedef struct _ERESOURCE {
	LIST_ENTRY				SystemResourcesList;
	POWNER_ENTRY			OwnerTable;

	SHORT					ActiveCount;
	USHORT					Flag;
	VOLATILE PKSEMAPHORE	SharedWaiters;
	VOLATILE PKEVENT		ExclusiveWaiters;

	OWNER_ENTRY				OwnerEntry;
	ULONG					ActiveEntries;
	ULONG					ContentionCount;
	ULONG					NumberOfSharedWaiters;
	ULONG					NumberOfExclusiveWaiters;

#ifdef _M_X64
	PVOID					Reserved2;
#endif

	union {
		PVOID				Address;
		ULONG_PTR			CreatorBackTraceIndex;
	};

	KSPIN_LOCK				SpinLock;
} TYPEDEF_TYPE_NAME(ERESOURCE);

typedef struct _COMPRESSED_DATA_INFO {
	//
	//  Code for the compression format (and engine) as
	//  defined in ntrtl.h.  Note that COMPRESSION_FORMAT_NONE
	//  and COMPRESSION_FORMAT_DEFAULT are invalid if
	//  any of the described chunks are compressed.
	//

	USHORT	CompressionFormatAndEngine;

	//
	//  Since chunks and compression units are expected to be
	//  powers of 2 in size, we express then log2.  So, for
	//  example (1 << ChunkShift) == ChunkSizeInBytes.  The
	//  ClusterShift indicates how much space must be saved
	//  to successfully compress a compression unit - each
	//  successfully compressed compression unit must occupy
	//  at least one cluster less in bytes than an uncompressed
	//  compression unit.
	//

	UCHAR	CompressionUnitShift;
	UCHAR	ChunkShift;
	UCHAR	ClusterShift;
	UCHAR	Reserved;

	//
	//  This is the number of entries in the CompressedChunkSizes
	//  array.
	//

	USHORT	NumberOfChunks;

	//
	//  This is an array of the sizes of all chunks resident
	//  in the compressed data buffer.  There must be one entry
	//  in this array for each chunk possible in the uncompressed
	//  buffer size.  A size of FSRTL_CHUNK_SIZE indicates the
	//  corresponding chunk is uncompressed and occupies exactly
	//  that size.  A size of 0 indicates that the corresponding
	//  chunk contains nothing but binary 0's, and occupies no
	//  space in the compressed data.  All other sizes must be
	//  less than FSRTL_CHUNK_SIZE, and indicate the exact size
	//  of the compressed data in bytes.
	//

	ULONG	CompressedChunkSizes[ANYSIZE_ARRAY];
} TYPEDEF_TYPE_NAME(COMPRESSED_DATA_INFO);

typedef BOOLEAN (NTAPI *PFAST_IO_CHECK_IF_POSSIBLE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG				Length,
	IN	BOOLEAN				Wait,
	IN	ULONG				LockKey,
	IN	BOOLEAN				CheckForReadOperation,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_READ) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG				Length,
	IN	BOOLEAN				Wait,
	IN	ULONG				LockKey,
	OUT	PVOID				Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef	BOOLEAN (NTAPI *PFAST_IO_WRITE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG				Length,
	IN	BOOLEAN				Wait,
	IN	ULONG				LockKey,
	IN	PVOID				Buffer,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_QUERY_BASIC_INFO) (
	IN	PFILE_OBJECT FileObject,
	IN	BOOLEAN Wait,
	OUT	PFILE_BASIC_INFORMATION Buffer,
	OUT	PIO_STATUS_BLOCK IoStatus,
	IN	PDEVICE_OBJECT DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_QUERY_STANDARD_INFO) (
	IN	PFILE_OBJECT				FileObject,
	IN	BOOLEAN						Wait,
	OUT	PFILE_STANDARD_INFORMATION	Buffer,
	OUT	PIO_STATUS_BLOCK			IoStatus,
	IN	PDEVICE_OBJECT				DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_LOCK) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	PEPROCESS			ProcessId,
	IN	ULONG				Key,
	IN	BOOLEAN				FailImmediately,
	IN	BOOLEAN				ExclusiveLock,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_UNLOCK_SINGLE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PLARGE_INTEGER		Length,
	IN	PEPROCESS			ProcessId,
	IN	ULONG				Key,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_UNLOCK_ALL) (
	IN	PFILE_OBJECT		FileObject,
	IN	PEPROCESS			ProcessId,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_UNLOCK_ALL_BY_KEY) (
	IN	PFILE_OBJECT		FileObject,
	IN	PVOID				ProcessId,
	IN	ULONG				Key,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_DEVICE_CONTROL) (
	IN	PFILE_OBJECT		FileObject,
	IN	BOOLEAN				Wait,
	IN	PVOID				InputBuffer OPTIONAL,
	IN	ULONG				InputBufferLength,
	OUT	PVOID				OutputBuffer OPTIONAL,
	IN	ULONG				OutputBufferLength,
	IN	ULONG				IoControlCode,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef VOID (NTAPI *PFAST_IO_ACQUIRE_FILE) (
	IN	PFILE_OBJECT		FileObject);

typedef VOID (NTAPI *PFAST_IO_RELEASE_FILE) (
	IN	PFILE_OBJECT		FileObject);

typedef VOID (NTAPI *PFAST_IO_DETACH_DEVICE) (
	IN	PDEVICE_OBJECT		SourceDevice,
	IN	PDEVICE_OBJECT		TargetDevice);

typedef BOOLEAN (NTAPI *PFAST_IO_QUERY_NETWORK_OPEN_INFO) (
	IN	PFILE_OBJECT					FileObject,
	IN	BOOLEAN							Wait,
	OUT	PFILE_NETWORK_OPEN_INFORMATION	Buffer,
	OUT	PIO_STATUS_BLOCK				IoStatus,
	IN	PDEVICE_OBJECT					DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_MDL_READ) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG				Length,
	IN	ULONG				LockKey,
	OUT	PMDL				*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_MDL_READ_COMPLETE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PMDL				MdlChain,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_PREPARE_MDL_WRITE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	ULONG				Length,
	IN	ULONG				LockKey,
	OUT	PMDL				*MdlChain,
	OUT	PIO_STATUS_BLOCK	IoStatus,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_MDL_WRITE_COMPLETE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		FileOffset,
	IN	PMDL				MdlChain,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef NTSTATUS (NTAPI *PFAST_IO_ACQUIRE_FOR_MOD_WRITE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PLARGE_INTEGER		EndingOffset,
	OUT	PERESOURCE			*ResourceToRelease,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef NTSTATUS (NTAPI *PFAST_IO_RELEASE_FOR_MOD_WRITE) (
	IN	PFILE_OBJECT		FileObject,
	IN	PERESOURCE			ResourceToRelease,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef NTSTATUS (NTAPI *PFAST_IO_ACQUIRE_FOR_CCFLUSH) (
	IN	PFILE_OBJECT		FileObject,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef NTSTATUS (NTAPI *PFAST_IO_RELEASE_FOR_CCFLUSH) (
	IN	PFILE_OBJECT		FileObject,
	IN	PDEVICE_OBJECT		DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_READ_COMPRESSED) (
	IN	PFILE_OBJECT			FileObject,
	IN	PLARGE_INTEGER			FileOffset,
	IN	ULONG					Length,
	IN	ULONG					LockKey,
	OUT	PVOID					Buffer,
	OUT	PMDL					*MdlChain,
	OUT	PIO_STATUS_BLOCK		IoStatus,
	OUT	PCOMPRESSED_DATA_INFO	CompressedDataInfo,
	IN	ULONG					CompressedDataInfoLength,
	IN	PDEVICE_OBJECT			DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_WRITE_COMPRESSED) (
	IN	PFILE_OBJECT			FileObject,
	IN	PLARGE_INTEGER			FileOffset,
	IN	ULONG					Length,
	IN	ULONG					LockKey,
	IN	PVOID					Buffer,
	OUT	PMDL					*MdlChain,
	OUT	PIO_STATUS_BLOCK		IoStatus,
	IN	PCOMPRESSED_DATA_INFO	CompressedDataInfo,
	IN	ULONG					CompressedDataInfoLength,
	IN	PDEVICE_OBJECT			DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_MDL_READ_COMPLETE_COMPRESSED) (
	IN	PFILE_OBJECT			FileObject,
	IN	PMDL					MdlChain,
	IN	PDEVICE_OBJECT			DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED) (
	IN	PFILE_OBJECT			FileObject,
	IN	PLARGE_INTEGER			FileOffset,
	IN	PMDL					MdlChain,
	IN	PDEVICE_OBJECT			DeviceObject);

typedef BOOLEAN (NTAPI *PFAST_IO_QUERY_OPEN) (
	IN OUT	PIRP							Irp,
	OUT		PFILE_NETWORK_OPEN_INFORMATION	NetworkInformation,
	IN		PDEVICE_OBJECT					DeviceObject);

typedef struct _FAST_IO_DISPATCH {
	ULONG									SizeOfFastIoDispatch;
	PFAST_IO_CHECK_IF_POSSIBLE				FastIoCheckIfPossible;
	PFAST_IO_READ							FastIoRead;
	PFAST_IO_WRITE							FastIoWrite;
	PFAST_IO_QUERY_BASIC_INFO				FastIoQueryBasicInfo;
	PFAST_IO_QUERY_STANDARD_INFO			FastIoQueryStandardInfo;
	PFAST_IO_LOCK							FastIoLock;
	PFAST_IO_UNLOCK_SINGLE					FastIoUnlockSingle;
	PFAST_IO_UNLOCK_ALL						FastIoUnlockAll;
	PFAST_IO_UNLOCK_ALL_BY_KEY				FastIoUnlockAllByKey;
	PFAST_IO_DEVICE_CONTROL					FastIoDeviceControl;
	PFAST_IO_ACQUIRE_FILE					AcquireFileForNtCreateSection;
	PFAST_IO_RELEASE_FILE					ReleaseFileForNtCreateSection;
	PFAST_IO_DETACH_DEVICE					FastIoDetachDevice;
	PFAST_IO_QUERY_NETWORK_OPEN_INFO		FastIoQueryNetworkOpenInfo;
	PFAST_IO_ACQUIRE_FOR_MOD_WRITE			AcquireForModWrite;
	PFAST_IO_MDL_READ						MdlRead;
	PFAST_IO_MDL_READ_COMPLETE				MdlReadComplete;
	PFAST_IO_PREPARE_MDL_WRITE				PrepareMdlWrite;
	PFAST_IO_MDL_WRITE_COMPLETE				MdlWriteComplete;
	PFAST_IO_READ_COMPRESSED				FastIoReadCompressed;
	PFAST_IO_WRITE_COMPRESSED				FastIoWriteCompressed;
	PFAST_IO_MDL_READ_COMPLETE_COMPRESSED	MdlReadCompleteCompressed;
	PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED	MdlWriteCompleteCompressed;
	PFAST_IO_QUERY_OPEN						FastIoQueryOpen;
	PFAST_IO_RELEASE_FOR_MOD_WRITE			ReleaseForModWrite;
	PFAST_IO_ACQUIRE_FOR_CCFLUSH			AcquireForCcFlush;
	PFAST_IO_RELEASE_FOR_CCFLUSH			ReleaseForCcFlush;
} TYPEDEF_TYPE_NAME(FAST_IO_DISPATCH);

typedef NTSTATUS (NTAPI *PDRIVER_INITIALIZE) (
	IN	PDRIVER_OBJECT	DriverObject,
	IN	PUNICODE_STRING	RegistryPath);

typedef VOID (NTAPI *PDRIVER_STARTIO) (
	IN OUT	PDEVICE_OBJECT	DeviceObject,
	IN OUT	PIRP			Irp);

typedef VOID (NTAPI *PDRIVER_UNLOAD) (
	IN	PDRIVER_OBJECT	DriverObject);

typedef NTSTATUS (NTAPI *PDRIVER_DISPATCH) (
	IN		PDEVICE_OBJECT	DeviceObject,
	IN OUT	PIRP			Irp);

#define IRP_MJ_CREATE                   0x00
#define IRP_MJ_CREATE_NAMED_PIPE        0x01
#define IRP_MJ_CLOSE                    0x02
#define IRP_MJ_READ                     0x03
#define IRP_MJ_WRITE                    0x04
#define IRP_MJ_QUERY_INFORMATION        0x05
#define IRP_MJ_SET_INFORMATION          0x06
#define IRP_MJ_QUERY_EA                 0x07
#define IRP_MJ_SET_EA                   0x08
#define IRP_MJ_FLUSH_BUFFERS            0x09
#define IRP_MJ_QUERY_VOLUME_INFORMATION 0x0a
#define IRP_MJ_SET_VOLUME_INFORMATION   0x0b
#define IRP_MJ_DIRECTORY_CONTROL        0x0c
#define IRP_MJ_FILE_SYSTEM_CONTROL      0x0d
#define IRP_MJ_DEVICE_CONTROL           0x0e
#define IRP_MJ_INTERNAL_DEVICE_CONTROL  0x0f
#define IRP_MJ_SHUTDOWN                 0x10
#define IRP_MJ_LOCK_CONTROL             0x11
#define IRP_MJ_CLEANUP                  0x12
#define IRP_MJ_CREATE_MAILSLOT          0x13
#define IRP_MJ_QUERY_SECURITY           0x14
#define IRP_MJ_SET_SECURITY             0x15
#define IRP_MJ_POWER                    0x16
#define IRP_MJ_SYSTEM_CONTROL           0x17
#define IRP_MJ_DEVICE_CHANGE            0x18
#define IRP_MJ_QUERY_QUOTA              0x19
#define IRP_MJ_SET_QUOTA                0x1a
#define IRP_MJ_PNP                      0x1b
#define IRP_MJ_PNP_POWER                IRP_MJ_PNP      // Obsolete....
#define IRP_MJ_MAXIMUM_FUNCTION         0x1b

#define DRVO_UNLOAD_INVOKED		0x00000001
#define DRVO_LEGACY_DRIVER		0x00000002
#define DRVO_BUILTIN_DRIVER		0x00000004

typedef struct _DRIVER_OBJECT {
	CSHORT				Type;
	CSHORT				Size;

	PDEVICE_OBJECT		DeviceObject;
	ULONG				Flags; // DRVO_*

	PVOID				DriverStart;
	ULONG				DriverSize;
	PVOID				DriverSection;
	PDRIVER_EXTENSION	DriverExtension;

	UNICODE_STRING		DriverName;

	PUNICODE_STRING		HardwareDatabase;

	PFAST_IO_DISPATCH	FastIoDispatch;

	PDRIVER_INITIALIZE	DriverInit;
	PDRIVER_STARTIO		DriverStartIo;
	PDRIVER_UNLOAD		DriverUnload;
	PDRIVER_DISPATCH	MajorFunction[IRP_MJ_MAXIMUM_FUNCTION + 1];
} TYPEDEF_TYPE_NAME(DRIVER_OBJECT);

#define LOW_LEVEL				0
#define PASSIVE_LEVEL			0
#define APC_LEVEL				1
#define DISPATCH_LEVEL			2
#define CMCI_LEVEL				5

#ifdef _M_X64
#  define CLOCK_LEVEL			13		// Interval clock level
#  define IPI_LEVEL				14		// Interprocessor interrupt level
#  define DRS_LEVEL				14		// Deferred Recovery Service level
#  define POWER_LEVEL			14		// Power failure level
#  define PROFILE_LEVEL			15		// timer used for profiling.
#  define HIGH_LEVEL			15		// Highest interrupt level
#else
#  define PROFILE_LEVEL			27		// timer used for profiling.
#  define CLOCK1_LEVEL			28		// Interval clock 1 level - Not used on x86
#  define CLOCK2_LEVEL			28		// Interval clock 2 level
#  define IPI_LEVEL				29		// Interprocessor interrupt level
#  define POWER_LEVEL			30		// Power failure level
#  define HIGH_LEVEL			31		// Highest interrupt level

#  define CLOCK_LEVEL			(CLOCK2_LEVEL)
#endif

typedef UCHAR TYPEDEF_TYPE_NAME(KIRQL);

//
// NTOSKRNL Functions
//

KIRQL NTAPI KeGetCurrentIrql(
	VOID);

//
// Function-Like Macros
//

#if KexIsDebugBuild
#  define KdPrint(...) DbgPrint(__VA_ARGS__)
#else
#  define KdPrint(...)
#endif

#define PAGED_CODE() ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL)