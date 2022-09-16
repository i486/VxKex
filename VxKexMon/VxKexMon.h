#pragma once
#include <KexComm.h>
#include <NtDll.h>

// harderr.c

typedef struct _VKM_HARD_ERROR_INFO {
	PCWSTR		ExeBaseName;
	NTSTATUS	Status;
	WCHAR		String1[MAX_PATH];
	WCHAR		String2[MAX_PATH];
	ULONG		Num1;
} VKM_HARD_ERROR_INFO, *PVKM_HARD_ERROR_INFO;

VOID VkmHardError(
	IN	PCWSTR	ExeBaseName,
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	ThreadHandle);

// image.c

ULONG_PTR GetRemoteImageEntryPoint(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	ImageBase);

// kexdata.c

BOOLEAN AdjustRemoteKexDllDir(
	IN	HANDLE	ProcessHandle,
	IN	ULONG	BitnessFrom,
	IN	ULONG	BitnessTo);

BOOLEAN InitializeRemoteKexData(
	IN	HANDLE				ProcessHandle,
	IN	PCWSTR				ProcessImageFullName,
	OUT	PKEX_PROCESS_DATA	KexDataOut OPTIONAL);

BOOLEAN UpdateRemoteKexData(
	IN	HANDLE				ProcessHandle,
	IN	PKEX_PROCESS_DATA	NewKexData);

// procmem.c

#ifdef _M_X64
#  define VaWriteP VaWrite8
#else
#  define VaWriteP VaWrite4
#endif

BOOLEAN VaRead(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PVOID		Buffer,
	IN	SIZE_T		Size);

BOOLEAN VaReadSzA(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PSTR		Buffer,
	IN	SIZE_T		MaxCch,
	OUT	PSIZE_T		Cch OPTIONAL);

BOOLEAN VaReadSzW(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PWSTR		Buffer,
	IN	SIZE_T		MaxCch,
	OUT	PSIZE_T		Cch OPTIONAL);

BOOLEAN VaWrite(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	OUT	PVOID		Buffer,
	IN	SIZE_T		Size);

BOOLEAN VaWrite1(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	BYTE		Data);

BOOLEAN VaWrite2(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	WORD		Data);

BOOLEAN VaWrite4(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	DWORD		Data);

BOOLEAN VaWrite8(
	IN	HANDLE		ProcessHandle,
	IN	ULONG_PTR	VirtualAddress,
	IN	QWORD		Data);

// psinfo.c

BOOLEAN GetRemotePebVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	PebVirtualAddress);

BOOLEAN GetRemotePeb(
	IN	HANDLE	ProcessHandle,
	OUT	PPEB	Buffer,
	IN	ULONG	BufferSize);

BOOLEAN GetRemoteProcessParametersVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	ProcessParamsVirtualAddress);

BOOLEAN GetRemoteProcessParameters(
	IN	HANDLE							ProcessHandle,
	OUT	PRTL_USER_PROCESS_PARAMETERS	Buffer,
	IN	ULONG							BufferSize);

BOOLEAN GetRemoteEnvironmentBlockVa(
	IN	HANDLE		ProcessHandle,
	OUT	PULONG_PTR	EnvironmentBlockVirtualAddress);

BOOLEAN CloneRemoteEnvironmentBlock(
	IN	HANDLE	ProcessHandle,
	OUT	PVOID	*Destination);

// psutil.c

BOOLEAN UpdateRemoteEnvironmentBlock(
	IN	HANDLE	ProcessHandle,
	IN	PVOID	Environment);

// utility.c

NORETURN VOID IncorrectUsageError(
	VOID);

BOOLEAN ReadValidateCommandLine(
	IN	PCWSTR		String,
	OUT	PULONG		ProcessId,
	OUT	PBOOLEAN	KexDllDirIs64Bit);

// vxkexmon.c

BOOLEAN RewriteImageImportDirectory(
	IN	HANDLE		Process,
	IN	ULONG_PTR	ImageBase,
	IN	PCWSTR		ImageName);

BOOLEAN ShouldRewriteImportsOfDll(
	IN	PCWSTR		DllFullName);

BOOLEAN MonitorRewriteImageImports(
	IN	ULONG		TargetProcessId,
	IN	BOOLEAN		KexDllDirIs64Bit);