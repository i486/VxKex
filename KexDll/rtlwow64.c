#include "buildcfg.h"
#include "kexdllp.h"

KEXAPI NTSTATUS NTAPI RtlWow64GetProcessMachines(
	IN	HANDLE	ProcessHandle,
	OUT	PUSHORT	ProcessMachine,
	OUT	PUSHORT	NativeMachine OPTIONAL)
{
	NTSTATUS Status;

	//
	// The KexRtlOperatingSystemBitness macro checks a field in SharedUserData in order
	// to determine whether the operating system is 64-bit or 32-bit (independently of
	// whether this is a WOW64 process or not).
	//

	if (KexRtlOperatingSystemBitness() == 64) {
		//
		// The OS is 64 bit. Processes can be either 32 bit or 64 bit.
		//

		if (ProcessHandle == NtCurrentProcess()) {
			//
			// We know the bitness of the current process because we know at
			// compile time whether we are running as 32-bit code or 64-bit code.
			// There is no need to ask the kernel.
			//

			if (KexRtlCurrentProcessBitness() == 64) {
				*ProcessMachine = IMAGE_FILE_MACHINE_AMD64;
			} else {
				*ProcessMachine = IMAGE_FILE_MACHINE_I386;
			}
		} else {
			ULONG_PTR AddressOf32BitPeb;

			//
			// We know the OS is 64-bit, but we want to see if another process is 32-bit.
			// We need to query the kernel to see whether that process is WOW64.
			//

			Status = NtQueryInformationProcess(
				ProcessHandle,
				ProcessWow64Information,
				&AddressOf32BitPeb,
				sizeof(AddressOf32BitPeb),
				NULL);

			if (!NT_SUCCESS(Status)) {
				return Status;
			}

			if (ProcessWow64Information) {
				*ProcessMachine = IMAGE_FILE_MACHINE_I386;
			} else {
				*ProcessMachine = IMAGE_FILE_MACHINE_AMD64;
			}
		}

		if (NativeMachine != NULL) {
			*NativeMachine = IMAGE_FILE_MACHINE_AMD64;
		}
	} else {
		//
		// The OS is 32 bit, so everything must be 32 bit. No need to check anything.
		//

		*ProcessMachine = IMAGE_FILE_MACHINE_I386;

		if (NativeMachine != NULL) {
			*NativeMachine = IMAGE_FILE_MACHINE_I386;
		}
	}

	return STATUS_SUCCESS;
}

//
// I'm not sure of exactly how this function is supposed to behave on
// 32-bit operating systems. The real function on Win10/11 calls
// NtQuerySystemInformationEx with SystemSupportedProcessorArchitectures2.
// TODO: write a test program on Win10 to see what it does.
//
KEXAPI NTSTATUS NTAPI RtlWow64IsWowGuestMachineSupported(
	IN	USHORT		WowGuestMachine,
	OUT	PBOOLEAN	MachineIsSupported)
{
	*MachineIsSupported = FALSE;

	if (KexRtlOperatingSystemBitness() == 64) {
		if (WowGuestMachine == IMAGE_FILE_MACHINE_I386) {
			*MachineIsSupported = TRUE;
		}
	}
	
	return STATUS_SUCCESS;
}