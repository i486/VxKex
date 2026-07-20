///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     initapc.c
//
// Abstract:
//
//     Hack to work around shitty programming in old MacType versions.
//     Basically, old MacType would set the thread context of child processes
//     to its own crappy shellcode which parses Peb->Ldr and incorrectly
//     assumes that kernel32 is always the third item or something. When
//     Application Verifier is enabled, that assumption is wrong, since
//     verifier.dll and KexDll.dll get loaded before kernel32 and kernelbase.
//
//     In order to work around this we queue an APC to the initial thread, which
//     gets run (through NtTestAlert) at the end of LdrpInitialize. That prevents
//     the MacType shellcode from being run for VxKex-enabled applications.
//
//     In the APC procedure we simply call RtlUserThreadStart with the first
//     parameter being the EXE's entry point and the second parameter being a
//     pointer to the PEB, which is the same as what Windows would do normally.
//
// Author:
//
//     vxiiduu (27-Jun-2026)
//
// Environment:
//
//     Native mode.
//     This DLL is loaded before kernel32, and it can only import from NTDLL.
//
// Revision History:
//
//     vxiiduu              27-Jun-2026  Initial creation.
//     vxiiduu              29-Jun-2026  Fix EXE entry point still getting called
//                                       if loader initialization failed
//     vxiiduu              30-Jun-2026  Remove assertion failure for .NET AnyCPU
//                                       programs - it's expected
//     vxiiduu              30-Jun-2026  Remove assertion failure for InitialThread
//     vxiiduu              01-Jul-2026  Properly handle .NET AnyCPU applications
//                                       and/or other processes creating threads in
//                                       our process (CreateRemoteThread etc.)
//                                       prior to ldr initialization complete.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

//
// This APC gets run by NtTestAlert near the end of LdrpInitialize.
//
VOID NTAPI KexPostSuccessfulInitializationApcRoutine(
	IN	PVOID	NormalContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2)
{
	NTSTATUS Status;
	PTHREAD_START_ROUTINE InitialThreadRoutine;
	PTEB Teb;
	PPEB Peb;
	
	Teb = NtCurrentTeb();
	Peb = Teb->ProcessEnvironmentBlock;

	//
	// Patch CreateProcessInternalW subsystem version check.
	// Done here because kernel32 is guaranteed to be loaded at this point.
	// This is ok to call multiple times.
	//

	Status = KexPatchCpiwSubsystemVersionCheck();
	ASSERT (NT_SUCCESS(Status));

	//
	// If this APC is queued to the initial thread, force the EXE entry point
	// or the .NET entry point if the EXE entry point could not be found.
	//

	if (Teb->InitialThread) {
		Status = KexLdrFindImageEntryPoint(
			Peb->ImageBaseAddress,
			(PPVOID) &InitialThreadRoutine);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_ENTRYPOINT_NOT_FOUND);

		if (!NT_SUCCESS(Status)) {
			UNICODE_STRING MscoreeDllName;
			ANSI_STRING CorExeMainName;
			PVOID Mscoree;

			//
			// This can happen for e.g. .NET applications that were compiled for AnyCPU.
			// In this case, we need to find _CorExeMain in mscoree.dll.
			//

			RtlInitConstantUnicodeString(&MscoreeDllName, L"MSCOREE.DLL");

			Status = LdrGetDllHandleByName(
				&MscoreeDllName,
				NULL,
				&Mscoree);

			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				return;
			}

			RtlInitConstantAnsiString(&CorExeMainName, "_CorExeMain");

			Status = LdrGetProcedureAddress(
				Mscoree,
				&CorExeMainName,
				0,
				(PPVOID) &InitialThreadRoutine);

			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				return;
			}
		}

#ifdef _M_X64
		RtlUserThreadStart(InitialThreadRoutine, Peb);
#else
		// x86 and WOW64 RtlUserThreadStart uses a custom calling convention where
		// the parameters are passed in EAX and EBX.
		asm {
			mov eax, InitialThreadRoutine
			mov ebx, Peb
			jmp dword ptr [RtlUserThreadStart]
		}
#endif
	}
}

#ifndef _M_X64
#pragma warning(disable:4414)

//
// The NTSTATUS code of loader initialization is stored in [EBP-0x1C] for all
// known 32-bit builds of Windows 7 NTDLL, including RTM, WOW64, ESU and other
// variants. (the compiler probably chooses based on the order of variables in
// the source code)
//
__declspec(naked) VOID NTAPI KexPostInitializationApcRoutine(
	IN	PVOID	NormalContext,
	IN	PVOID	SystemArgument1,
	IN	PVOID	SystemArgument2)
{
	asm {
		asm cmp		dword ptr [ebp-1Ch], 0
		asm jge		KexPostSuccessfulInitializationApcRoutine
		asm ret
	}
}
#endif