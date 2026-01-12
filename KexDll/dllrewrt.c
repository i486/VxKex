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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC PKEX_RTL_STRING_MAPPER DllRewriteStringMapper = NULL;

// This header file contains the definition of the DLL rewrite table.
#include "redirects.h"

NTSTATUS KexAddDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	IN	PCUNICODE_STRING	RewrittenDllName)
{
	ASSUME (DllRewriteStringMapper != NULL);
	ASSUME (VALID_UNICODE_STRING(DllName));
	ASSUME (VALID_UNICODE_STRING(RewrittenDllName));

	return KexRtlInsertEntryStringMapper(
		DllRewriteStringMapper,
		DllName,
		RewrittenDllName);
}

NTSTATUS KexRemoveDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName)
{
	ASSUME (DllRewriteStringMapper != NULL);
	ASSUME (VALID_UNICODE_STRING(DllName));

	return KexRtlRemoveEntryStringMapper(
		DllRewriteStringMapper,
		DllName);
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

	Status = KexRtlCreateStringMapper(
		&DllRewriteStringMapper, 
		KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS);

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
	OUT	PUNICODE_STRING			RewrittenDllName)
{
	NTSTATUS Status;
	UNICODE_STRING CleanDllName;
	UNICODE_STRING DotDll;
	UNICODE_STRING ApiPrefix;
	UNICODE_STRING ExtPrefix;
	USHORT MaximumRewrittenLength;

	ASSERT (DllRewriteStringMapper != NULL);
	ASSERT (VALID_UNICODE_STRING(DllName));
	ASSERT (RewrittenDllName != NULL);

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

	Status = KexRtlLookupEntryStringMapper(
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

	ASSERT (RewrittenDllName->Length <= MaximumRewrittenLength);
	ASSERT (VALID_UNICODE_STRING(RewrittenDllName));

	return STATUS_SUCCESS;
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

	ASSERT (AnsiDllName != NULL);
	ASSERT (AnsiDllName->Length != 0);
	ASSERT (AnsiDllName->MaximumLength >= AnsiDllName->Length);
	ASSERT (AnsiDllName->Buffer != NULL);

	//
	// This Unicode DLL name gets allocated from the heap.
	// We have to remember to free it.
	//
	
	Status = RtlAnsiStringToUnicodeString(&DllName, AnsiDllName, TRUE);
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
		goto Exit;
	}

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

		KexLogErrorEvent(
			L"Failed to rewrite DLL import (%wZ -> %wZ): STATUS_ACCESS_VIOLATION",
			&DllName,
			&RewrittenDllName);

		ASSERT (FALSE);
	}

Exit:
	if (NT_SUCCESS(Status)) {
		KexLogDetailEvent(L"Rewrote DLL import: %wZ -> %wZ", &DllName, &RewrittenDllName);
	}

	RtlFreeUnicodeString(&DllName);
	return Status;
}

//
// Determine whether the imports of a particular DLL (identified by name and
// path) should be rewritten.
//
// Returns a pointer to the string mapper object which is to be used for
// rewriting the imports of the specified DLL. If this function returns NULL,
// it means the imports of the specified DLL must not be rewritten.
//
BOOLEAN KexShouldRewriteImportsOfDll(
	IN	PCUNICODE_STRING	FullDllName)
{
	NTSTATUS Status;
	UNICODE_STRING BaseDllName;

	//
	// Find the file name of the DLL.
	//

	Status = KexRtlPathFindFileName(FullDllName, &BaseDllName);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return FALSE;
	}

	if (RtlPrefixUnicodeString(&KexData->WinDir, FullDllName, TRUE)) {
		UNICODE_STRING Kernel;

		//
		// The DLL is in the Windows directory.
		//

		if (KexData->IfeoParameters.WinVerSpoof > WinVerSpoofWin7) {
			UNICODE_STRING Iertutil;

			RtlInitConstantUnicodeString(&Iertutil, L"iertutil.dll");

			if (RtlEqualUnicodeString(&BaseDllName, &Iertutil, TRUE)) {
				//
				// iertutil.dll checks versions and can shit itself if the
				// version number is too high. So we need to rewrite its
				// imports so our KXBASE version functions get applied.
				//

				return TRUE;
			}
		}

		RtlInitConstantUnicodeString(&Kernel, L"kernel");

		if (RtlPrefixUnicodeString(&Kernel, &BaseDllName, TRUE)) {
			//
			// Rewrite the imports of kernelbase and kernel32. We want to do this
			// so that certain functions such as LoadLibrary and CreateFileMapping
			// end up going through KxNt (LdrLoadDll/NtCreateSection).
			//

			return TRUE;
		}

		//
		// Otherwise, do not rewrite imports of Windows DLLs.
		//

		return FALSE;
	}

	//
	// If this DLL is a part of VxKex, do not rewrite its imports.
	//

	if (RtlPrefixUnicodeString(&KexData->KexDir, FullDllName, TRUE)) {
		return FALSE;
	}

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		UNICODE_STRING TargetDllName;

		//
		// APPSPECIFICHACK: This is some sort of .NET DLL that will screw up if we
		// rewrite its imports. No idea why. What typically happens if you allow its
		// imports to be rewritten is you get a blank window that can be interacted
		// with (i.e. all the buttons and things are "working") but you just can't
		// see anything the app is drawing.
		//

		RtlInitConstantUnicodeString(&TargetDllName, L"wpfgfx_cor3.dll");

		if (KexRtlUnicodeStringEndsWith(FullDllName, &TargetDllName, TRUE)) {
			return FALSE;
		}

		//
		// APPSPECIFICHACK: Although Mirillis Action supports Windows 7, it installs
		// global hooks which cause crashes when dxgi is rewritten to kxdx.
		//

		if (KexRtlCurrentProcessBitness() == 64) {
			RtlInitConstantUnicodeString(&TargetDllName, L"action_x64.dll");
		} else {
			RtlInitConstantUnicodeString(&TargetDllName, L"action_x86.dll");
		}

		if (KexRtlUnicodeStringEndsWith(FullDllName, &TargetDllName, TRUE)) {
			return FALSE;
		}
	}

	//
	// If there's no other rules that apply to this DLL, then we will rewrite
	// its imports.
	//

	return TRUE;
}

NTSTATUS KexRewriteImageImportDirectory(
	IN	PVOID					ImageBase,
	IN	PCUNICODE_STRING		BaseImageName,
	IN	PCUNICODE_STRING		FullImageName)
{
	NTSTATUS Status;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_FILE_HEADER CoffHeader;
	PIMAGE_OPTIONAL_HEADER OptionalHeader;
	PIMAGE_DATA_DIRECTORY ImportDirectory;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
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
	
	Status = KexLdrProtectImageImportSection(
		ImageBase,
		PAGE_READWRITE,
		&OldProtect);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Check if this is kernel32.
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
	Status = KexLdrProtectImageImportSection(
		ImageBase,
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