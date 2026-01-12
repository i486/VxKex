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
//     vxiiduu              07-Nov-2022  Increase resilience of KexApplyVersionSpoof
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

PCWSTR HumanReadableWinVerSpoof[] = {
	L"None",
	L"Windows 7 SP1",
	L"Windows 8",
	L"Windows 8.1",
	L"Windows 10",
	L"Windows 11"
};

UNICODE_STRING CSDVersionUnicodeString;

STATIC NTSTATUS NTAPI KexpRtlGetVersionHookForDotNet(
	OUT	PRTL_OSVERSIONINFOEXW	Version)
{
	PPEB Peb;

	Peb = NtCurrentPeb();

	Version->dwMajorVersion	= Peb->OSMajorVersion;
	Version->dwMinorVersion	= Peb->OSMinorVersion;
	Version->dwBuildNumber	= Peb->OSBuildNumber;
	Version->dwPlatformId	= Peb->OSPlatformId;

	if (Peb->CSDVersion.Buffer && Peb->CSDVersion.Buffer[0] != '\0') {
		StringCchCopy(
			Version->szCSDVersion,
			ARRAYSIZE(Version->szCSDVersion),
			Peb->CSDVersion.Buffer);
	} else {
		Version->szCSDVersion[0] = '\0';
	}

	if (Version->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW)) {
		NT_PRODUCT_TYPE ProductType;

		Version->wServicePackMajor	= HIBYTE(Peb->OSCSDVersion);
		Version->wServicePackMinor	= LOBYTE(Peb->OSCSDVersion);
		Version->wSuiteMask			= (WORD) SharedUserData->SuiteMask;
		Version->wProductType		= 0;

		if (RtlGetNtProductType(&ProductType)) {
			Version->wProductType = ProductType;
		}

		if (ProductType == NtProductWinNt) {
			Version->wSuiteMask &= ~VER_SUITE_TERMINAL;
		}
	}

	//
	// As far as I can tell, System.Private.CoreLib is what tells "managed code" the
	// Windows version, so that's all we need to spoof.
	// This method might be usable for other .NET applications as well, but I haven't
	// tested enough .NET applications that require version spoofing to know for
	// sure, which is why this remains an app-specific hack and not just a general
	// version spoof for .NET applications.
	//

	if (AshModuleBaseNameIs(ReturnAddress(), L"System.Private.CoreLib")) {
		Version->szCSDVersion[0] = '\0';

		if (Version->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW)) {
			Version->wServicePackMajor = 0;
			Version->wServicePackMinor = 0;
		}

		switch (KexData->IfeoParameters.WinVerSpoof) {
		case WinVerSpoofWin7: // SP1
			Version->dwMajorVersion		= 6;
			Version->dwMinorVersion		= 1;
			Version->dwBuildNumber		= 7601;

			if (Version->dwOSVersionInfoSize == sizeof(RTL_OSVERSIONINFOEXW)) {
				Version->wServicePackMajor	= 1;
				Version->wServicePackMinor	= 0;
			}
			
			StringCchCopy(
				Version->szCSDVersion,
				ARRAYSIZE(Version->szCSDVersion),
				L"Service Pack 1");

			break;
		case WinVerSpoofWin8:
			Version->dwMajorVersion		= 6;
			Version->dwMinorVersion		= 2;
			Version->dwBuildNumber		= 9200;
			break;
		case WinVerSpoofWin8Point1:
			Version->dwMajorVersion		= 6;
			Version->dwMinorVersion		= 3;
			Version->dwBuildNumber		= 9600;
			break;
		case WinVerSpoofWin10:
			Version->dwMajorVersion		= 10;
			Version->dwMinorVersion		= 0;
			Version->dwBuildNumber		= 19044;
			break;
		case WinVerSpoofWin11:
		default:
			Version->dwMajorVersion		= 10;
			Version->dwMinorVersion		= 0;
			Version->dwBuildNumber		= 22000;
			break;
		}
	}

	return STATUS_SUCCESS;
}

STATIC VOID NTAPI KexpRtlGetNtVersionNumbersHook(
	OUT	PULONG	MajorVersion OPTIONAL,
	OUT	PULONG	MinorVersion OPTIONAL,
	OUT	PULONG	BuildNumber OPTIONAL)
{
	NTSTATUS Status;
	PPEB Peb;
	UNICODE_STRING CallerDll;
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

	RtlInitEmptyUnicodeStringFromTeb(&CallerDll);
	Status = KexLdrGetDllFullNameFromAddress(ReturnAddress(), &CallerDll);
	if (!NT_SUCCESS(Status)) {
		goto Exit;
	}

	if (NT_SUCCESS(Status)) {
		if (RtlPrefixUnicodeString(&KexData->WinDir, &CallerDll, TRUE)) {
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
}

VOID KexApplyVersionSpoof(
	VOID)
{
	PPEB Peb;
	ULONG MajorVersion;
	ULONG MinorVersion;
	USHORT BuildNumber;
	USHORT CSDVersion;

	if (KexData->IfeoParameters.WinVerSpoof == WinVerSpoofNone) {
		KexLogDebugEvent(L"Not spoofing Windows version since it is not requested.");
		return;
	}

	KexLogInformationEvent(
		L"Applying Windows version spoof: %s",
		ARRAY_LOOKUP_BOUNDS_CHECKED(HumanReadableWinVerSpoof, KexData->IfeoParameters.WinVerSpoof));

	//
	// APPSPECIFICHACK: Spoof version of HandBrake without breaking .NET
	//

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (AshExeBaseNameIs(L"HandBrake.exe") || AshExeBaseNameIs(L"HandBrake.Worker.exe")) {
			KexHkInstallBasicHook(RtlGetVersion, KexpRtlGetVersionHookForDotNet, NULL);
			return;
		}
	}

	Peb = NtCurrentPeb();

	//
	// CSDVersion is the service pack number, and therefore should be 0
	// for anything higher than Windows 7 since those OSes don't have any
	// service packs.
	//

	CSDVersion = 0;
	RtlInitConstantUnicodeString(&CSDVersionUnicodeString, L"");

	switch (KexData->IfeoParameters.WinVerSpoof) {
	case WinVerSpoofWin7:
		// Spoofing Windows 7 on Windows 7 is mainly meant for users of Win7 RTM,
		// since there are some programs which require SP1.
		MajorVersion = 6;
		MinorVersion = 1;
		BuildNumber = 7601;
		CSDVersion = 0x100;
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
	default: // default case should always be at the highest win version
		MajorVersion = 10;
		MinorVersion = 0;
		BuildNumber = 22000; // Win11 21H2
		break;
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
				// win10+ only field, reserved in win7
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
			KexLogWarningEvent(
				L"Failed to make SharedUserData read-write.\r\n\r\n"
				L"NTSTATUS error code: %s",
				KexRtlNtStatusToString(Status));
		}
	}
	
	//
	// TODO: Implement registry strong spoofing.
	//
}