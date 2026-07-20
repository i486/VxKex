///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllrewrt.c
//
// Abstract:
//
//     Contains routines related to DLL rewriting - more precisely, rewriting
//     the names of DLLs imported from a PE image (which itself may be an EXE
//     or a DLL).
//
// Author:
//
//     vxiiduu (18-Oct-2022)
//
// Revision History:
//
//     vxiiduu              18-Oct-2022  Initial creation.
//     vxiiduu              22-Oct-2022  Bound imports are now erased
//     vxiiduu              03-Nov-2022  Optimize KexRewriteImageImportDirectory
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//     vxiiduu              11-Feb-2024  Refactor DLL rewrite lookup code.
//     vxiiduu              13-Mar-2024  Move DLL redirects into a static table
//                                       instead of reading them from registry.
//     vxiiduu              03-Jul-2025  Update KexpRewriteImportTableDllNameInPlace
//                                       to fix rare issue triggered by specific
//                                       layout of import names in a PE image.
//     vxiiduu              15-Dec-2025  Replace KexLdrProtectImageImportSection
//                                       with KexLdrGetImageImportSection in order
//                                       to fix a rare bug where memory protections
//                                       of executable pages can get clobbered and
//                                       cause a crash.
//     vxiiduu              02-May-2026  Tidy up DLL classification functions.
//                                       Remove coreclr and mono-2.0-bdwgc from
//                                       the list of non-rewrite DLLs since we
//                                       have implemented proper compatibility.
//     vxiiduu              02-Jul-2026  Fix bug in KexApplyUserDllRewrite where
//                                       rewrite entries could be ignored if the
//                                       value is empty at the end of the string.
//     vxiiduu              02-Jul-2026  Replace the heap allocation with using the
//                                       TEB in KexpRewriteImportTableDllNameInPlace.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC PKEX_SMP_STRING_MAPPER DllRewriteStringMapper = NULL;

// This header file contains the definition of the DLL rewrite table.
#include "redirects.h"

NTSTATUS KexAddDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName)
{
	ASSUME (DllRewriteStringMapper != NULL);
	ASSUME (VALID_UNICODE_STRING(DllName));
	ASSUME (VALID_UNICODE_STRING(RewrittenDllName));

	return SmpInsertEntryStringMapper(
		DllRewriteStringMapper,
		DllName,
		RewrittenDllName);
}

NTSTATUS KexRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName)
{
	ASSUME (DllRewriteStringMapper != NULL);
	ASSUME (VALID_UNICODE_STRING(DllName));

	return SmpRemoveEntryStringMapper(
		DllRewriteStringMapper,
		DllName);
}

NTSTATUS KexAddUpdateRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName OPTIONAL)
{
	NTSTATUS Status;

	ASSUME (VALID_UNICODE_STRING(DllName));
	ASSUME (RewrittenDllName == NULL || WELL_FORMED_UNICODE_STRING(RewrittenDllName));

	Status = KexRemoveDllRewriteEntry(DllName);
	ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

	if (!NT_SUCCESS(Status) && Status != STATUS_STRING_MAPPER_ENTRY_NOT_FOUND) {
		return Status;
	}

	if (RewrittenDllName &&
		RewrittenDllName->Length != 0 &&
		RewrittenDllName->Buffer != NULL) {

		Status = KexAddDllRewriteEntry(DllName, RewrittenDllName);
	} else {
		Status = STATUS_SUCCESS;
	}

	ASSERT (NT_SUCCESS(Status));
	return Status;
}

//
// Initialize the DLL rewrite subsystem.
//

NTSTATUS KexInitializeDllRewrite(
	VOID)
{
	NTSTATUS Status;
	ULONG Index;

	ASSERT (DllRewriteStringMapper == NULL);

	//
	// Create the DLL rewrite string mapper.
	//

	Status = SmpCreateStringMapper(
		&DllRewriteStringMapper, 
		KEX_SMP_STRING_MAPPER_CASE_INSENSITIVE_KEYS);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	ASSERT (DllRewriteStringMapper != NULL);

	//
	// Populate the DLL rewrite string mapper.
	//

	ForEachArrayItem (DllRedirects, Index) {
		ASSERT (VALID_UNICODE_STRING(&DllRedirects[Index][0]));
		ASSERT (VALID_UNICODE_STRING(&DllRedirects[Index][1]));

		Status = KexAddDllRewriteEntry(
			&DllRedirects[Index][0],
			&DllRedirects[Index][1]);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}
	}

	//
	// Add the Kex32 or Kex64 directory to the default loader search path.
	//
	
	Status = KexpAddKex3264ToDllPath();
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		//
		// This can happen if, for example, the user chooses to install VxKex into a
		// directory with a long path name.
		//
		
		KexLogErrorEvent(
			L"Failed to append Kex32/64 to the DLL search path.\r\n\r\n"
			L"NTSTATUS error code: %s",
			KexRtlNtStatusToString(Status));
	} else {
		KexLogInformationEvent(
			L"Successfully initialized DLL rewrite subsystem.\r\n\r\n"
			L"The new default DLL search path is:\r\n\r\n"
			L"\"%wZ\"",
			&NtCurrentPeb()->ProcessParameters->DllPath);
	}

	return Status;
}

//
// This function accepts DLL base names only. They may or may not have a .dll
// extension.
//
// Callers are forbidden from editing the contents of RewrittenDllName->Buffer.
// If you do that, then you will modify the entry inside the string mapper
// itself, and cause a lot of problems.
//
STATIC NTSTATUS KexpLookupDllRewriteEntry(
	IN	PCUNICODE_STRING		DllName,
	OUT	PUNICODE_STRING			RewrittenDllName OPTIONAL)
{
	NTSTATUS Status;
	UNICODE_STRING CleanDllName;
	UNICODE_STRING DotDll;
	UNICODE_STRING ApiPrefix;
	UNICODE_STRING ExtPrefix;
	USHORT MaximumRewrittenLength;

	ASSERT (DllRewriteStringMapper != NULL);
	ASSERT (VALID_UNICODE_STRING(DllName));

	CleanDllName = *DllName;

	RtlInitConstantUnicodeString(&DotDll, L".dll");
	RtlInitConstantUnicodeString(&ApiPrefix, L"api-");
	RtlInitConstantUnicodeString(&ExtPrefix, L"ext-");

	//
	// If the name of the DLL has a .dll extension, shorten the length of it
	// so that it doesn't have a .dll extension anymore.
	// This allows image files to import from DLLs with no extension without
	// choking up the dll rewrite.
	//

	MaximumRewrittenLength = CleanDllName.Length;

	if (KexRtlUnicodeStringEndsWith(&CleanDllName, &DotDll, TRUE)) {
		CleanDllName.Length -= KexRtlUnicodeStringCch(&DotDll) * sizeof(WCHAR);
	}

	//
	// If the name of the DLL starts with "api-" or "ext-" (i.e. it's an API set DLL),
	// then remove the -lX-Y-Z suffix as well.
	// This code will have to be revised when API sets start appearing with
	// individual X-Y-Z numbers greater than 9.
	//

	if (RtlPrefixUnicodeString(&ApiPrefix, &CleanDllName, TRUE) ||
		RtlPrefixUnicodeString(&ExtPrefix, &CleanDllName, TRUE)) {

		CleanDllName.Length -= 7 * sizeof(WCHAR);
	}

	//
	// Now look up the DLL in the DLL rewrite string mapper to see whether we
	// have a rewrite entry for it.
	//

	Status = SmpLookupEntryStringMapper(
		DllRewriteStringMapper,
		&CleanDllName,
		RewrittenDllName);

	ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Check to see that the cch of the rewritten DLL is not greater than the
	// cch of the original DLL. This shouldn't happen unless someone has made
	// a mistake with the DLL rewrite table.
	//

	if (RewrittenDllName != NULL) {
		ASSERT (RewrittenDllName->Length <= MaximumRewrittenLength);
		ASSERT (VALID_UNICODE_STRING(RewrittenDllName));
	}

	return STATUS_SUCCESS;
}

//
// Returns TRUE if a DLL rewrite lookup for a particular DLL name would succeed.
// Returns FALSE otherwise.
//
BOOLEAN KexDoesDllRewriteEntryExist(
	IN	PCUNICODE_STRING		DllName)
{
	NTSTATUS Status;

	Status = KexpLookupDllRewriteEntry(DllName, NULL);

	return NT_SUCCESS(Status);
}

//
// Rewrite a DLL name based on the string mapper entries.
// This function is meant to operate directly on the import directories of
// loaded images, not for general use.
//
STATIC NTSTATUS KexpRewriteImportTableDllNameInPlace(
	IN OUT	PANSI_STRING			AnsiDllName)
{
	NTSTATUS Status;
	UNICODE_STRING DllName;
	UNICODE_STRING RewrittenDllName;
	BOOLEAN HaveModifiedPageProtection;
	PVOID BaseAddress;
	SIZE_T RegionSize;
	ULONG OldProtect;

	HaveModifiedPageProtection = FALSE;

	ASSERT (AnsiDllName != NULL);
	ASSERT (AnsiDllName->Length != 0);
	ASSERT (AnsiDllName->MaximumLength >= AnsiDllName->Length);
	ASSERT (AnsiDllName->Buffer != NULL);

	//
	// Convert the ANSI DLL name to Unicode since the string mapper API requires
	// a Unicode string.
	//

	RtlInitEmptyUnicodeStringFromTeb(&DllName);
	Status = RtlAnsiStringToUnicodeString(&DllName, AnsiDllName, FALSE);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Lookup the name of the DLL to see whether we should rewrite it.
	// If no entry was found in the string mapper, or another error occurred,
	// just return and leave the original DLL name un-modified.
	//

	Status = KexpLookupDllRewriteEntry(
		&DllName,
		&RewrittenDllName);

	ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

Retry:

	try {
		//
		// Convert to ANSI. This will result in RtlUnicodeStringToAnsiString writing
		// directly into the import table of an image file, so that's why we put
		// this code in a try-except block (in case we screwed up with memory access
		// flags earlier, or some weirdly formatted DLL file lays things out in an
		// unexpected way).
		//

		Status = RtlUnicodeStringToAnsiString(
			AnsiDllName,
			&RewrittenDllName,
			FALSE);

		ASSERT (NT_SUCCESS(Status));

	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) {
		Status = GetExceptionCode();

		if (HaveModifiedPageProtection) {
			//
			// This shouldn't happen. We will check for it anyway to prevent an infinite
			// loop if NtProtectVirtualMemory somehow doesn't work.
			//

			ASSERT (FALSE);

			KexLogErrorEvent(
				L"Failed to rewrite DLL import (%wZ -> %wZ)\r\n\r\n"
				L"Encountered STATUS_ACCESS_VIOLATION twice even after changing page protections.",
				&DllName,
				&RewrittenDllName);

			return Status;
		}

		//
		// We have an access violation. This can occur with uncommonly formatted
		// executables or DLLs. In this case we can fix the issue by simply calling
		// NtProtectVirtualMemory to make the ANSI DLL name read-write.
		//

		BaseAddress = AnsiDllName->Buffer;
		RegionSize = AnsiDllName->Length;

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			PAGE_READWRITE,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			KexLogErrorEvent(
				L"Failed to rewrite DLL import (%wZ -> %wZ)\r\n\r\n"
				L"Encountered STATUS_ACCESS_VIOLATION upon write to ANSI DLL name."
				L"While attempting to change memory protections, encountered %s.",
				&DllName,
				&RewrittenDllName,
				KexRtlNtStatusToString(Status));
		}

		// Record the fact that we've changed the page protections so we can undo
		// the changes later.
		HaveModifiedPageProtection = TRUE;
		goto Retry;
	}

	if (HaveModifiedPageProtection) {
		NTSTATUS Status; // overshadow the outer Status variable

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&BaseAddress,
			&RegionSize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));
	}

	if (NT_SUCCESS(Status)) {
		KexLogDetailEvent(L"Rewrote DLL import: %wZ -> %wZ", &DllName, &RewrittenDllName);
	}

	return Status;
}

//
// Returns TRUE if a given DLL is a part of Windows.
// Items in %SystemRoot% are considered Windows DLLs unless they are specifically
// exempted.
//
KEXAPI BOOLEAN NTAPI KexIsWindowsDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	if (RtlPrefixUnicodeString(&KexData->WinDir, FullDllName, TRUE)) {
		UNICODE_STRING Msvcp140;
		UNICODE_STRING SlashTemp;
		UNICODE_STRING DllNameAfterWinDir;

		//
		// The DLL is in the Windows directory.
		//

		DllNameAfterWinDir = *FullDllName;
		KexRtlAdvanceUnicodeString(&DllNameAfterWinDir, KexData->WinDir.Length);
		RtlInitConstantUnicodeString(&SlashTemp, L"\\Temp");

		if (RtlPrefixUnicodeString(&SlashTemp, &DllNameAfterWinDir, TRUE)) {
			//
			// DLL path starts with %SystemRoot%\Temp. In this case, we won't consider
			// it a Windows module.
			//
			// Some installers, such as the newest versions of the Microsoft C++ v14
			// Redistributable, copy themselves to %SystemRoot%\Temp and then run from
			// there and check the Windows version.
			//
			// We don't want such installers to be considered Windows components.
			//

			return FALSE;
		}

		RtlInitConstantUnicodeString(&Msvcp140, L"msvcp140");

		if (RtlPrefixUnicodeString(&Msvcp140, BaseDllName, TRUE)) {
			//
			// New versions of the Microsoft Visual C++ 2015-2022 runtime
			// are no longer compatible with Windows 7. These DLLs are installed
			// into system32, so we need to add such an exception here.
			//

			return FALSE;
		}

		//
		// Otherwise, it's a Windows DLL.
		//

		return TRUE;
	}

	return FALSE;
}

//
// Returns TRUE if a given DLL is a VxKex extended DLL.
// Items in KexDir are considered VxKex components only if their base names start
// with "Kx" i.e. KxBase, KxUser, etc.
//
KEXAPI BOOLEAN NTAPI KexIsVxKexExtendedDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	if (RtlPrefixUnicodeString(&KexData->KexDir, FullDllName, TRUE) &&
		KexRtlUnicodeStringCch(BaseDllName) >= 2 &&
		ToUpper(BaseDllName->Buffer[0]) == 'K' &&
		ToUpper(BaseDllName->Buffer[1]) == 'X') {

		// This is a VxKex API extension DLL.
		return TRUE;
	}

	return FALSE;
}

//
// Returns TRUE if a given DLL is exempted from DLL rewrite for compatibility
// reasons (i.e. rewriting its imports or dynamic loads will cause problems).
//
KEXAPI BOOLEAN NTAPI KexIsRewriteExemptedDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		//
		// APPSPECIFICHACK: Although Mirillis Action supports Windows 7, it installs
		// global hooks which cause crashes when dxgi is rewritten to kxdx.
		//
		// APPSPECIFICHACK: Do not interfere with RTSS/MSI Afterburner. It is
		// compatible with Win7 and rewriting imports causes the OSD to not appear.
		//
		// APPSPECIFICHACK: Rewriting MacType imports causes a crash. MacType is
		// compatible with Win7 so there is no need to rewrite it anyway.
		//

		STATIC CONST UNICODE_STRING RewriteExemptedDlls[] = {
#ifdef _M_X64
			RTL_CONSTANT_STRING(L"action_x64.dll"),
			RTL_CONSTANT_STRING(L"RTSSHooks64.dll"),
			RTL_CONSTANT_STRING(L"MacType64.dll"),
			RTL_CONSTANT_STRING(L"MacType64.Core.dll"),
#else
			RTL_CONSTANT_STRING(L"action_x86.dll"),
			RTL_CONSTANT_STRING(L"RTSSHooks.dll"),
			RTL_CONSTANT_STRING(L"MacType.dll"),
			RTL_CONSTANT_STRING(L"MacType.Core.dll"),
#endif
		};

		ULONG Index;

		for (Index = 0; Index < ARRAYSIZE(RewriteExemptedDlls); ++Index) {
			if (RtlEqualUnicodeString(BaseDllName, &RewriteExemptedDlls[Index], TRUE)) {
				return TRUE;
			}
		}
	}

	//
	// Allow the user to specify his own rewrite-exempted DLLs.
	// This is the support for the KEX_DllRewriteExemptions IFEO option.
	//

	if (KexData->IfeoParameters.DllRewriteExemptions[0] != '\0') {
		STATIC UNICODE_STRING UserSpecifiedRewriteExemptedDlls[256];
		STATIC ULONG NumberOfUserSpecifiedRewriteExemptedDlls = 0;
		STATIC BOOLEAN UserSpecifiedRewriteExemptedDllsInitialized = FALSE;
		ULONG Index;

		//
		// We won't worry about thread safety for this one-time initialization
		// because it's idempotent (i.e. running multiple times doesn't hurt).
		//

		if (!UserSpecifiedRewriteExemptedDllsInitialized) {
			ULONG StartIndex;
			PWSTR Exemptions;
			ULONG ExemptionCount;

			Index = 0;
			ExemptionCount = 0;
			Exemptions = KexData->IfeoParameters.DllRewriteExemptions;

			//
			// Parse a bar-delimited list of DLL names.
			//

			until (Exemptions[Index] == '\0') {
				PUNICODE_STRING Exemption;

				StartIndex = Index;

				// Find the next delimiter (which can either be a bar or the end of
				// the string).
				until (Exemptions[Index] == '|' || Exemptions[Index] == '\0') {
					++Index;
				}

				if (Index <= StartIndex) {
					// Zero-length entry - ignore it and move on.
					ASSERT (Exemptions[Index] != '\0');
					++Index;
					continue;
				}

				if (ExemptionCount >= ARRAYSIZE(UserSpecifiedRewriteExemptedDlls)) {
					ASSERT (("Too many user-specified rewrite exempted DLLs", FALSE));
					break;
				}

				ASSERT ((Index - StartIndex) * sizeof(WCHAR) <= USHRT_MAX);

				Exemption = &UserSpecifiedRewriteExemptedDlls[ExemptionCount];
				Exemption->Length = (USHORT) ((Index - StartIndex) * sizeof(WCHAR));
				Exemption->MaximumLength = Exemption->Length;
				Exemption->Buffer = &Exemptions[StartIndex];
				ASSERT (VALID_UNICODE_STRING(Exemption));

				if (Exemptions[Index] == '|') {
					// skip past the delimiter
					++Index;
				}

				++ExemptionCount;
			}

			ASSERT (Index < ARRAYSIZE(KexData->IfeoParameters.DllRewriteExemptions));

			NumberOfUserSpecifiedRewriteExemptedDlls = ExemptionCount;
			UserSpecifiedRewriteExemptedDllsInitialized = TRUE;
		}

		//
		// Check if the specified DLL is in the user-specified exemption list.
		//

		for (Index = 0; Index < NumberOfUserSpecifiedRewriteExemptedDlls; ++Index) {
			if (RtlEqualUnicodeString(BaseDllName, &UserSpecifiedRewriteExemptedDlls[Index], TRUE)) {
				return TRUE;
			}
		}
	}

	return FALSE;
}

KEXAPI BOOLEAN NTAPI KexIsRewriteForcedWindowsDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	//
	// Rewrite WebIO (used by WinHTTP) and WinInet, so that schannel.dll
	// and secur32.dll get caught. This is to enable support for KxSChanl
	// and TLS 1.3.
	//
	// Rewrite clr.dll since that allows .NET Framework applications to use
	// KxSChanl. (e.g. Fiddler Classic)
	//
	// Rewrite SSPICLI so that we can redirect the SecurityProviders
	// registry value to our own (see Ext_RegQueryValueExW in KxAdvapi).
	//

	UNICODE_STRING RewriteForcedDlls[] = {
		RTL_CONSTANT_STRING(L"webio.dll"),
		RTL_CONSTANT_STRING(L"wininet.dll"),
		RTL_CONSTANT_STRING(L"clr.dll"),
		RTL_CONSTANT_STRING(L"sspicli.dll"),
	};

	ULONG Index;

	for (Index = 0; Index < ARRAYSIZE(RewriteForcedDlls); ++Index) {
		if (RtlEqualUnicodeString(BaseDllName, &RewriteForcedDlls[Index], TRUE)) {
			return TRUE;
		}
	}

	return FALSE;
}

//
// Determine whether a particular DLL should have its static imports rewritten.
//
BOOLEAN KexShouldRewriteStaticImportsOfDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	if (KexIsWindowsDll(FullDllName, BaseDllName)) {
		UNICODE_STRING Kernel;

		if (KexIsRewriteForcedWindowsDll(FullDllName, BaseDllName)) {
			return TRUE;
		}

		RtlInitConstantUnicodeString(&Kernel, L"kernel");

		if (RtlPrefixUnicodeString(&Kernel, BaseDllName, TRUE)) {
			//
			// Rewrite the imports of kernelbase and kernel32. We want to do this
			// so that certain functions such as LoadLibrary and CreateFileMapping
			// end up going through KxNt (LdrLoadDll/NtCreateSection).
			//

			return TRUE;
		}

		if (KexData->IfeoParameters.WinVerSpoof > WinVerSpoofWin7) {
			UNICODE_STRING Iertutil;

			RtlInitConstantUnicodeString(&Iertutil, L"iertutil.dll");

			if (RtlEqualUnicodeString(BaseDllName, &Iertutil, TRUE)) {
				//
				// iertutil.dll checks versions and can shit itself if the
				// version number is too high. So we need to rewrite its
				// imports so our KXBASE version functions get applied.
				//

				return TRUE;
			}
		}

		//
		// Otherwise, do not rewrite the static imports of Windows DLLs.
		//

		return FALSE;
	}

	//
	// If this DLL is a part of VxKex, do not rewrite its imports.
	// Note: Only the DLLs which start with "Kx" are considered part of VxKex.
	// Other DLLs within Kex32/Kex64 are just prebuilt DLLs from later versions
	// of Windows.
	//

	if (KexIsVxKexExtendedDll(FullDllName, BaseDllName)) {
		return FALSE;
	}

	//
	// Check if a DLL is exempted from rewrite for compatibility reasons.
	//

	if (KexIsRewriteExemptedDll(FullDllName, BaseDllName)) {
		return FALSE;
	}

	//
	// If there's no other rules that apply to this DLL, then we will rewrite
	// its imports.
	//

	return TRUE;
}

//
// Determine whether a DLL should have its dynamic imports rewritten.
//
KEXAPI BOOLEAN NTAPI KexShouldRewriteDynamicImportsOfDll(
	IN	PCUNICODE_STRING	FullDllName,
	IN	PCUNICODE_STRING	BaseDllName)
{
	if (KexIsWindowsDll(FullDllName, BaseDllName)) {
		if (KexIsRewriteForcedWindowsDll(FullDllName, BaseDllName)) {
			return TRUE;
		}

		return FALSE;
	}

	if (KexIsWindowsDll(FullDllName, BaseDllName) ||
		KexIsVxKexExtendedDll(FullDllName, BaseDllName) ||
		KexIsRewriteExemptedDll(FullDllName, BaseDllName)) {

		return FALSE;
	}

	return TRUE;
}

NTSTATUS KexRewriteImageImportDirectory(
	IN		PVOID						ImageBase,
	IN		PCUNICODE_STRING			BaseImageName,
	IN		PCUNICODE_STRING			FullImageName)
{
	NTSTATUS Status;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_FILE_HEADER CoffHeader;
	PIMAGE_OPTIONAL_HEADER OptionalHeader;
	PIMAGE_DATA_DIRECTORY ImportDirectory;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	PVOID ImportSectionBase;
	SIZE_T ImportSectionSize;
	UNICODE_STRING Kernel32;
	BOOLEAN AtLeastOneImportWasRewritten;
	ULONG OldProtect;

	AtLeastOneImportWasRewritten = FALSE;

	ASSERT (DllRewriteStringMapper != NULL);
	ASSERT (ImageBase != NULL);
	ASSERT (VALID_UNICODE_STRING(BaseImageName));
	ASSERT (VALID_UNICODE_STRING(FullImageName));

	Status = RtlImageNtHeaderEx(
		RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
		ImageBase,
		0,
		&NtHeaders);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	CoffHeader = &NtHeaders->FileHeader;
	OptionalHeader = &NtHeaders->OptionalHeader;
	ImportDirectory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	if ((KexRtlCurrentProcessBitness() == 64) != (CoffHeader->Machine == 0x8664)) {
		//
		// 32-bit dll loaded in 64-bit process or vice versa
		// This can happen with resource-only DLLs, in which case there are no
		// imports to rewrite anyway. Many .NET dlls have this characteristic.
		//

		return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
	}

	if (OptionalHeader->NumberOfRvaAndSizes < (IMAGE_DIRECTORY_ENTRY_IMPORT + 1) ||
		ImportDirectory->VirtualAddress == 0) {
		//
		// There is no import directory in the image (e.g. resource-only DLL).
		//

		return STATUS_IMAGE_NO_IMPORT_DIRECTORY;
	}

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) RVA_TO_VA(ImageBase, ImportDirectory->VirtualAddress);

	if (ImportDescriptor->Name == 0) {
		//
		// There shouldn't be an import directory if it has no entries.
		//

		return STATUS_INVALID_IMAGE_FORMAT;
	}

	//
	// Set the entire section that contains the image import directory to read-write.
	//

	Status = KexLdrGetImageImportSection(
		ImageBase,
		&ImportSectionBase,
		&ImportSectionSize);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = NtProtectVirtualMemory(
		NtCurrentProcess(),
		&ImportSectionBase,
		&ImportSectionSize,
		PAGE_READWRITE,
		&OldProtect);
	
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Check if this is kernel32. If so, only rewrite its NTDLL import
	// (there is also a kernelbase import which we do not want to rewrite).
	//

	RtlInitConstantUnicodeString(&Kernel32, L"kernel32.dll");

	if (RtlEqualUnicodeString(BaseImageName, &Kernel32, TRUE)) {
		PSTR NtdllImport;

		// On Windows 7, NTDLL is always the 2nd import of kernel32.
		NtdllImport = (PSTR) RVA_TO_VA(ImageBase, ImportDescriptor[1].Name);
		ASSERT (StringEqualA(NtdllImport, "ntdll.dll"));

		RtlCopyMemory(NtdllImport, "kxnt.dll", sizeof("kxnt.dll"));
		AtLeastOneImportWasRewritten = TRUE;
		goto SkipNormalImportRewrite;
	}

	//
	// Walk through imports and rewrite each one if necessary.
	//

	do {
		PSTR DllNameBuffer;
		ANSI_STRING ImportedDllNameAnsi;

		DllNameBuffer = (PSTR) RVA_TO_VA(ImageBase, ImportDescriptor->Name);
		RtlInitAnsiString(&ImportedDllNameAnsi, DllNameBuffer);

		Status = KexpRewriteImportTableDllNameInPlace(
			&ImportedDllNameAnsi);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

		if (NT_SUCCESS(Status)) {
			AtLeastOneImportWasRewritten = TRUE;
		}
	} while ((++ImportDescriptor)->Name != 0);

SkipNormalImportRewrite:

	// restore old permissions
	Status = NtProtectVirtualMemory(
		NtCurrentProcess(),
		&ImportSectionBase,
		&ImportSectionSize,
		OldProtect,
		&OldProtect);

	ASSERT (NT_SUCCESS(Status));

	if (AtLeastOneImportWasRewritten) {
		PIMAGE_DATA_DIRECTORY BoundImportDirectory;
		PVOID DataDirectoryPtr;
		SIZE_T DataDirectorySize;

		//
		// A Bound Import Directory will cause process initialization to fail if we have rewritten
		// anything. So we simply zero it out.
		// Bound imports are a performance optimization, but basically we can't use it because
		// the bound import addresses are dependent on the "real" function addresses within the
		// imported DLL - and since we have replaced one or more imported DLLs, these pre-calculated
		// function addresses are no longer valid, so we just have to delete it.
		//

		BoundImportDirectory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];

		DataDirectoryPtr = BoundImportDirectory;
		DataDirectorySize = sizeof(IMAGE_DATA_DIRECTORY);

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&DataDirectoryPtr,
			&DataDirectorySize,
			PAGE_READWRITE,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		// Zero it out.
		RtlZeroMemory(BoundImportDirectory, sizeof(*BoundImportDirectory));

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			&DataDirectoryPtr,
			&DataDirectorySize,
			OldProtect,
			&OldProtect);

		ASSERT (NT_SUCCESS(Status));
	}

	return STATUS_SUCCESS;
}

//
// This function expects Win32 paths.
//
// The path may be omitted from the DllPath argument, i.e. "kernel32.dll".
// The rewritten path will always be a DLL base name without extension, for
// example "kxbase".
//
// The RewrittenDllPath argument must have the Buffer element set to an
// appropriate Unicode buffer, and the MaximumLength element set to the size
// in bytes of that buffer.
//
// It's OK to have DllPath->Buffer and RewrittenDllNameOut->Buffer point to
// the same buffer. However, DllPath and RewrittenDllNameOut must not point
// to the same UNICODE_STRING structure.
//

NTSTATUS KexRewriteDllPath(
	IN	PCUNICODE_STRING	DllPath,
	OUT	PUNICODE_STRING		RewrittenDllNameOut)
{
	NTSTATUS Status;
	UNICODE_STRING DllFileName;
	UNICODE_STRING RewrittenDllName;

	//
	// Validate parameters.
	//

	ASSERT (DllRewriteStringMapper != NULL);

	ASSERT (VALID_UNICODE_STRING(DllPath));
	ASSERT (DllPath->Length != 0);
	ASSERT (DllPath->Buffer != NULL);

	ASSERT (VALID_UNICODE_STRING(RewrittenDllNameOut));
	ASSERT (RewrittenDllNameOut->MaximumLength != 0);
	ASSERT (RewrittenDllNameOut->Buffer != NULL);

	ASSERT (DllPath != RewrittenDllNameOut);
	ASSERT (RewrittenDllNameOut->MaximumLength >= DllPath->Length);

	//
	// Set the output length to zero. We will append to it later.
	//

	RewrittenDllNameOut->Length = 0;

	//
	// Get the file-name component of the path.
	// If the path consists of only a filename, that's ok too.
	//

	Status = KexRtlPathFindFileName(
		DllPath,
		&DllFileName);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// If the original DLL path (when specified) is *not* in the Windows directory,
	// we will not rewrite this DLL name. VxKex only involves itself in replacing
	// Windows DLLs, not other DLLs bundled with the application.
	//

	if (DllFileName.Buffer != DllPath->Buffer) {
		// A full path was specified, not just a DLL name.

		if (!RtlPrefixUnicodeString(&KexData->WinDir, DllPath, TRUE)) {
			// the requested DLL isn't inside the Windows directory
			return STATUS_DLL_NOT_IN_SYSTEM_ROOT;
		}
	}

	//
	// Lookup the DLL rewrite entry.
	// Keep in mind that the output (RewrittenDllName) of this function
	// is a DLL base name without .dll extension, e.g. "kxbase".
	//

	Status = KexpLookupDllRewriteEntry(
		&DllFileName,
		&RewrittenDllName);

	ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	KexLogDebugEvent(
		L"DLL name or path was successfully rewritten: %wZ -> %wZ\r\n\r\n"
		L"Original DLL name or path: %wZ",
		&DllFileName,
		&RewrittenDllName,
		DllPath);

	RtlCopyUnicodeString(RewrittenDllNameOut, &RewrittenDllName);

	ASSERT (VALID_UNICODE_STRING(RewrittenDllNameOut));
	return Status;
}

//
// Implements support for KEX_DllRewriteEntries IFEO parameter.
// RewriteSpec is a null-terminated string which contains bar-delimited key-value
// pairs. Keys and values are separated by a colon character.
//
// Example: "d3d12:dxvk|xinput1_5:xinput1_3|dxgi:" (DXGI.dll would be removed from
// the rewrite list in that case).
//

NTSTATUS KexApplyUserDllRewrite(
	IN	PWSTR	RewriteSpec)
{
	NTSTATUS Status;
	ULONG Index;
	UNICODE_STRING Key;
	UNICODE_STRING Value;

	enum {
		InKey,
		InValue
	} State;

	Index = 0;
	State = InKey;
	Status = STATUS_SUCCESS;

	if (RewriteSpec[0] == '\0') {
		return Status;
	}

	while (TRUE) {
		ULONG StartIndex;

		StartIndex = Index;

		until (RewriteSpec[Index] == '|' ||
			   RewriteSpec[Index] == ':' ||
			   RewriteSpec[Index] == '\0') {

			++Index;
		}

		if (State == InKey) {
			switch (RewriteSpec[Index]) {
			case ':':
				ASSERT ((Index - StartIndex) * sizeof(WCHAR) <= USHRT_MAX);
				Key.Buffer = &RewriteSpec[StartIndex];
				Key.Length = (USHORT) ((Index - StartIndex) * sizeof(WCHAR));
				Key.MaximumLength = Key.Length;
				ASSERT (VALID_UNICODE_STRING(&Key));

				if (Key.Length == 0) {
					// This is not reasonable.
					return STATUS_INVALID_PARAMETER;
				}

				State = InValue;
				break;
			case '|':
			case '\0':
				// Invalid for these characters to appear now.
				return STATUS_INVALID_PARAMETER;
			}
		} else if (State == InValue) {
			switch (RewriteSpec[Index]) {
			case '|':
			case '\0':
				ASSERT ((Index - StartIndex) * sizeof(WCHAR) <= USHRT_MAX);
				Value.Buffer = &RewriteSpec[StartIndex];
				Value.Length = (USHORT) ((Index - StartIndex) * sizeof(WCHAR));
				Value.MaximumLength = Value.Length;
				ASSERT (WELL_FORMED_UNICODE_STRING(&Value));

				if (Value.Length > Key.Length) {
					// DLL rewrite value must be less than or equal to the key length,
					// since we overwrite the DLL name in-place.
					return STATUS_INVALID_PARAMETER;
				}

				// If Value.Length is zero, this function will simply remove the DLL
				// rewrite entry from the mapper.
				Status = KexAddUpdateRemoveDllRewriteEntry(
					&Key,
					&Value);

				if (Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND) {
					// We don't consider this an error.
					Status = STATUS_SUCCESS;
				}

				ASSERT (NT_SUCCESS(Status));

				if (!NT_SUCCESS(Status)) {
					return Status;
				}

				if (RewriteSpec[Index] == '|') {
					State = InKey;
				}

				break;
			case ':':
				return STATUS_INVALID_PARAMETER;
			}
		} else {
			NOT_REACHED;
		}

		if (RewriteSpec[Index] == '\0') {
			break;
		}

		++Index;
	}

	if (State != InValue) {
		return STATUS_INVALID_PARAMETER;
	}

	return Status;
}