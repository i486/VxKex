///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     verspoof.c
//
// Abstract:
//
//     Contains routines for spoofing Windows version.
//
// Author:
//
//     vxiiduu (06-Nov-2022)
//
// Revision History:
//
//     vxiiduu              06-Nov-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

PCWSTR HumanReadableWinVerSpoof[] = {
	L"WinVerSpoofNone",
	L"WinVerSpoofWin7",
	L"WinVerSpoofWin8",
	L"WinVerSpoofWin8Point1",
	L"WinVerSpoofWin10",
	L"WinVerSpoofWin11"
};

UNICODE_STRING CSDVersionUnicodeString;

STATIC VOID NTAPI KexpRtlGetNtVersionNumbersHook(
	OUT	PULONG	MajorVersion OPTIONAL,
	OUT	PULONG	MinorVersion OPTIONAL,
	OUT	PULONG	BuildNumber OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	PPEB Peb;
	UNICODE_STRING CallerDll;
	WCHAR CallerDllBuffer[MAX_PATH];
	ULONG ReturnMajorVersion;
	ULONG ReturnMinorVersion;
	ULONG ReturnBuildNumber;

	Peb = NtCurrentPeb();
	ReturnMajorVersion = Peb->OSMajorVersion;
	ReturnMinorVersion = Peb->OSMinorVersion;
	ReturnBuildNumber = Peb->OSCSDVersion;

	//
	// If a call to this function comes from a native Windows DLL, we will
	// return the real version numbers.
	//
	// The C-runtime DLL checks to see if it's running on the intended OS
	// version and will fail in its DllMain if that isn't the case.
	//

	RtlInitEmptyUnicodeString(&CallerDll, CallerDllBuffer, sizeof(CallerDllBuffer));
	Status = KexLdrGetDllFullPathFromAddress(ReturnAddress(), &CallerDll);
	if (!NT_SUCCESS(Status)) {
		goto Exit;
	}

	if (NT_SUCCESS(Status)) {
		KexSrvLogDebugEvent(L"Called from %wZ (pfx %wZ)", &CallerDll, &KexData->WinDir);

		if (RtlPrefixUnicodeString(&KexData->WinDir, &CallerDll, TRUE)) {
			KexSrvLogDebugEvent(L"XXX");

			ReturnMajorVersion = 6;
			ReturnMinorVersion = 1;
			ReturnBuildNumber = 7601;
		}
	}

Exit:
	if (MajorVersion) {
		*MajorVersion = ReturnMajorVersion;
	}

	if (MinorVersion) {
		*MinorVersion = ReturnMinorVersion;
	}

	if (BuildNumber) {
		*BuildNumber = ReturnBuildNumber | 0xF0000000;
	}
} PROTECTED_FUNCTION_END_VOID

VOID KexApplyVersionSpoof(
	VOID) PROTECTED_FUNCTION
{
	PPEB Peb;
	ULONG MajorVersion;
	ULONG MinorVersion;
	USHORT BuildNumber;
	USHORT CSDVersion;

	ASSUME (KexData->IfeoParameters.WinVerSpoof < WinVerSpoofMax);

	if (KexData->IfeoParameters.WinVerSpoof == WinVerSpoofNone) {
		KexSrvLogDebugEvent(L"Not spoofing Windows version since it is not requested.");
		return;
	}

	Peb = NtCurrentPeb();

	KexSrvLogInformationEvent(
		L"Applying Windows version spoof: %s",
		ARRAY_LOOKUP_BOUNDS_CHECKED(HumanReadableWinVerSpoof, KexData->IfeoParameters.WinVerSpoof));

	//
	// CSDVersion is the service pack number, and therefore should be 0
	// for anything higher than Windows 7 since those OSes don't have any
	// service packs.
	//
	// Spoofing Windows 7 is not available through the standard configuration
	// user interface. It is only meant for debugging the version spoofer
	// mechanism itself.
	//

	CSDVersion = 0;
	RtlInitConstantUnicodeString(&CSDVersionUnicodeString, L"");

	switch (KexData->IfeoParameters.WinVerSpoof) {
	case WinVerSpoofWin7:
		MajorVersion = 6;
		MinorVersion = 1;
		BuildNumber = 7601;
		CSDVersion = 1;
		RtlInitConstantUnicodeString(&CSDVersionUnicodeString, L"Service Pack 1");
		break;
	case WinVerSpoofWin8:
		MajorVersion = 6;
		MinorVersion = 2;
		BuildNumber = 9200;
		break;
	case WinVerSpoofWin8Point1:
		MajorVersion = 6;
		MinorVersion = 3;
		BuildNumber = 9600;
		break;
	case WinVerSpoofWin10:
		MajorVersion = 10;
		MinorVersion = 0;
		BuildNumber = 19044; // Win10 21H2
		break;
	case WinVerSpoofWin11:
		MajorVersion = 10;
		MinorVersion = 0;
		BuildNumber = 22000; // Win11 21H2
		break;
	default:
		NOT_REACHED;
	}

	Peb->OSMajorVersion = MajorVersion;
	Peb->OSMinorVersion = MinorVersion;
	Peb->OSBuildNumber = BuildNumber;
	Peb->OSCSDVersion = CSDVersion;
	Peb->CSDVersion = CSDVersionUnicodeString;

	//
	// RtlGetNtVersionNumbers has hard coded numbers, so we have to hook and
	// replace it with a custom function.
	//

	KexHkInstallBasicHook(RtlGetNtVersionNumbers, KexpRtlGetNtVersionNumbersHook, NULL);

	//
	// Strong version spoofing is anything that involves runtime performance
	// penalty, or a potential application compatibility penalty to apply.
	// For example, spoofing the version number in the registry, in the file
	// system, or in SharedUserData.
	//
	// The StrongVersionSpoof IFEO parameter is a bit field.
	//
	
	if (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA) {
		NTSTATUS Status;
		PVOID SharedUserDataPageAddress;
		SIZE_T SharedUserDataSize;
		ULONG OldProtect;

		//
		// SharedUserData spoofing requires additional support from BASE dlls
		// and even then, may cause an app compat problem if anything reads
		// from it directly.
		//
		// We are lucky in that it can be made read-write, but any write to it
		// will cause the system time fields to stop updating, as the page is
		// copy on write.
		//

		SharedUserDataPageAddress = SharedUserData;
		SharedUserDataSize = sizeof(KUSER_SHARED_DATA);
		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&SharedUserDataPageAddress,
			&SharedUserDataSize,
			PAGE_READWRITE,
			&OldProtect);

		if (NT_SUCCESS(Status)) {
			SharedUserData->NtMajorVersion = MajorVersion;
			SharedUserData->NtMinorVersion = MinorVersion;

			if (MajorVersion >= 10) {
				// win10+ only field
				SharedUserData->NtBuildNumber = BuildNumber;
			}

			//
			// Performance counter is queried from SharedUserData directly
			// unless bit 1 of TscQpcData (i.e. TscQpcEnabled) is cleared.
			// See RtlQueryPerformanceCounter for more information.
			//
			SharedUserData->TscQpcEnabled = FALSE;

			//
			// The NtQuerySystemTime stub actually reads from SharedUserData,
			// which is why we need to redirect it to the custom syscall stub.
			//
			KexHkInstallBasicHook(NtQuerySystemTime, KexNtQuerySystemTime, NULL);
		} else {
			KexSrvLogWarningEvent(
				L"Failed to make SharedUserData read-write.\r\n\r\n"
				L"NTSTATUS error code: 0x%08lx",
				Status);
		}
	}
	
} PROTECTED_FUNCTION_END_VOID