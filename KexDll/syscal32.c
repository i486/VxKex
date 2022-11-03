///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     syscal32.c
//
// Abstract:
//
//     32-bit Unhooked System Call Stubs (native & WOW64).
//     TLDR: Tons of macros
//
// Author:
//
//     vxiiduu (23-Oct-2022)
//
// Environment:
//
//   Early process creation ONLY. For reasons of simplicity these functions
//   are not thread safe at all.
//
// Revision History:
//
//     vxiiduu              23-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

#ifdef KEX_ARCH_X86

#define KEXNTSYSCALLAPI __declspec(naked)
#pragma warning(disable:4414) // shut the fuck up i dont care

#define GENERATE_SYSCALL(SyscallName, SyscallNumber32, SyscallNumber64, EcxValue, Retn, ...) \
KEXNTSYSCALLAPI NTSTATUS NTAPI Kex##SyscallName##(__VA_ARGS__) { asm { \
	asm mov eax, SyscallNumber32 \
	asm mov edx, 0x7FFE0300 \
	asm cmp dword ptr [edx], 0 /* If [0x7ffe0300] is zero, that means we are running as a Wow64 program on a 64 bit OS. */ \
	asm je Kex##SyscallName##_Wow64 \
	asm call [edx] /* Native 32 bit call */ \
	asm ret Retn \
	asm Kex##SyscallName##_Wow64: /* Wow64 call */ \
	asm mov eax, SyscallNumber64 \
	asm mov ecx, EcxValue \
	asm lea edx, [esp+4] \
	asm call fs:0xC0 \
	asm add esp, 4 \
	asm ret Retn \
}}

GENERATE_SYSCALL(NtQuerySystemTime,					0x0107, 0x0057, 0x18, 0x04,
	OUT		PULONGLONG	CurrentTime);

GENERATE_SYSCALL(NtCreateUserProcess,				0x005D, 0x00AA, 0x00, 0x2C,
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

GENERATE_SYSCALL(NtProtectVirtualMemory,			0x00D7, 0x004D, 0x00, 0x14,
	IN		HANDLE		ProcessHandle,
	IN OUT	PPVOID		BaseAddress,
	IN OUT	PSIZE_T		RegionSize,
	IN		ULONG		NewProtect,
	OUT		PULONG		OldProtect);

#endif