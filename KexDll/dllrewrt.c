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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC PKEX_RTL_STRING_MAPPER DllRewriteStringMapper = NULL;

//
// Read DLL rewrite information from a registry key and place it in a
// string mapper for efficient querying.
//
NTSTATUS KexInitializeDllRewrite(
	VOID)
{
	NTSTATUS Status;
	HANDLE DllRewriteKeyHandle;
	UNICODE_STRING DotDll;
	UNICODE_STRING DllRewriteKeyName;
	UNICODE_STRING StringMapperKey;
	UNICODE_STRING StringMapperValue;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG Index;

	RtlInitConstantUnicodeString(&DotDll, L".DLL");

	Status = KexRtlCreateStringMapper(
		&DllRewriteStringMapper, 
		KEX_RTL_STRING_MAPPER_CASE_INSENSITIVE_KEYS);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Open the DllRewrite key. This key contains values that indicate which
	// DLLs to rewrite to what.
	//

	RtlInitConstantUnicodeString(&DllRewriteKeyName, L"\\Registry\\Machine\\Software\\VXsoft\\VxKex\\DllRewrite");
	InitializeObjectAttributes(&ObjectAttributes, &DllRewriteKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenKey(
		&DllRewriteKeyHandle,
		KEY_READ | KEY_WOW64_64KEY,
		&ObjectAttributes);

	if (!NT_SUCCESS(Status)) {
		KexRtlDeleteStringMapper(&DllRewriteStringMapper);
		return Status;
	}

	//
	// Enumerate the DllRewrite key and add entries to the string mapper.
	//

	for (Index = 0;; ++Index) {
		PKEY_VALUE_FULL_INFORMATION KeyInformationBuffer;
		ULONG KeyInformationBufferCb;

		//
		// Allocate space for the key info structure plus 2 path-sized
		// buffers (DLL from, and DLL to).
		//
		// This allocation is never freed, because we potentially need it for
		// the entire life of the process. But we will resize it down to
		// only what is needed, hence why we are allocating such a relatively
		// large buffer to begin with.
		//

		KeyInformationBufferCb = sizeof(KEY_VALUE_FULL_INFORMATION) +
								 (MAX_PATH * 2 * sizeof(WCHAR));
		KeyInformationBuffer = (PKEY_VALUE_FULL_INFORMATION) SafeAlloc(
			BYTE, KeyInformationBufferCb);

		ASSERT (KeyInformationBuffer != NULL);

		if (!KeyInformationBuffer) {
			break;
		}

		Status = NtEnumerateValueKey(
			DllRewriteKeyHandle,
			Index,
			KeyValueFullInformation,
			KeyInformationBuffer,
			KeyInformationBufferCb,
			&KeyInformationBufferCb);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_NO_MORE_ENTRIES);

		if (!NT_SUCCESS(Status)) {
			SafeFree(KeyInformationBuffer);

			if (Status == STATUS_NO_MORE_ENTRIES) {
				Status = STATUS_SUCCESS;
				break;
			} else {
				KexLogWarningEvent(
					L"Failed to read a DLL rewrite value\r\n\r\n"
					L"NTSTATUS error code: %s",
					KexRtlNtStatusToString(Status));

				continue;
			}
		}

		//
		// The DLL rewrite value must be a string.
		//

		ASSERT (KeyInformationBuffer->Type == REG_SZ);

		if (KeyInformationBuffer->Type != REG_SZ) {
			SafeFree(KeyInformationBuffer);

			KexLogWarningEvent(
				L"A registry DLL rewrite key has the wrong data type.\r\n\r\n"
				L"Check HKLM\\Software\\VXsoft\\VxKex\\DllRewrite and remove any non-string keys.");

			continue;
		}

		//
		// Resize the ~1KB heap allocation we made earlier down to only the
		// size we need.
		//

		KeyInformationBufferCb = sizeof(KEY_VALUE_FULL_INFORMATION) +
								 KeyInformationBuffer->DataOffset +
								 KeyInformationBuffer->DataLength;
		KeyInformationBuffer = (PKEY_VALUE_FULL_INFORMATION) SafeReAlloc(
			KeyInformationBuffer,
			BYTE, KeyInformationBufferCb);

		ASSERT (KeyInformationBuffer != NULL);

		if (!KeyInformationBuffer) {
			SafeFree(KeyInformationBuffer);
			break;
		}

		StringMapperKey.Length = (USHORT) KeyInformationBuffer->NameLength;
		StringMapperKey.MaximumLength = StringMapperKey.Length;
		StringMapperKey.Buffer = KeyInformationBuffer->NameAndData;

		StringMapperValue.Length = (USHORT) KeyInformationBuffer->DataLength;
		StringMapperValue.MaximumLength = StringMapperValue.Length;
		StringMapperValue.Buffer = (PWCHAR) (((PBYTE) KeyInformationBuffer) + KeyInformationBuffer->DataOffset);

		// Often, the registry returns DataLength values that are too large because
		// they include the null terminator.
		// We have to guard against that here, since it would cause problems for VXL.
		// (StringCchVPrintfBufferLength returns the wrong values in this situation...)

		if (StringMapperValue.Length >= sizeof(WCHAR) &&
			KexRtlEndOfUnicodeString(&StringMapperValue)[-1] == '\0') {

			StringMapperValue.Length -= sizeof(WCHAR);
		}

		//
		// The keys and values under the DllRewrite key shouldn't include the file extension
		// ".dll" anywhere under normal circumstances. We should emit a warning entry in the
		// log if this situation is detected, but take no further action.
		//

		if (KexRtlUnicodeStringEndsWith(&StringMapperKey, &DotDll, TRUE) ||
			KexRtlUnicodeStringEndsWith(&StringMapperValue, &DotDll, TRUE)) {

			ASSERT (FALSE);

			KexLogWarningEvent(
				L"The DLL rewrite registry entry for %wZ has a \".dll\" extension\r\n\r\n"
				L"DLL rewrite entries should not generally have file extensions. Unless you know "
				L"what you are doing, you should remove the file extension, or otherwise the rewrite "
				L"entry might not work.",
				&StringMapperKey);
		}

		KexLogDebugEvent(
			L"Processed a DLL rewrite registry entry: %wZ -> %wZ",
			&StringMapperKey,
			&StringMapperValue);

		Status = KexRtlInsertEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey,
			&StringMapperValue);

		ASSERT (NT_SUCCESS(Status));
		continue;
	}

	SafeClose(DllRewriteKeyHandle);

	//
	// Decide which D3D12.dll implementation to use.
	//

	RtlInitConstantUnicodeString(&StringMapperKey, L"d3d12");
	RtlInitEmptyUnicodeString(&StringMapperValue, NULL, 0);

	if (KexData->IfeoParameters.D3D12Implementation == D3D12AutomaticImplementation) {
		if (KexData->IfeoParameters.DisableAppSpecific) {
			// No app specific hacks means the automatic d3d12 implementation
			// always defaults to the stub implementation.
			KexData->IfeoParameters.D3D12Implementation = D3D12StubImplementation;
		} else {
			// Apply app specific D3D12 implementations when we have them.
			// Currently there are no apps known which require D3D12on7.
			KexData->IfeoParameters.D3D12Implementation = D3D12StubImplementation;
		}
	}

	switch (KexData->IfeoParameters.D3D12Implementation) {
	case D3D12NoImplementation:
		// Do nothing.
		// In most situations this means D3D12.dll is not redirected.
		// If the user has for some reason redirected D3D12 through the DllRewrite key,
		// then we will just use that.
		break;
	case D3D12MicrosoftImplementation:
		// The Microsoft D3D12on7 implementation is only available for the
		// x64 architecture. If we are not x64, then we will fall through to
		// the stub implementation.

		if (KexRtlCurrentProcessBitness() == 64) {
			RtlInitConstantUnicodeString(&StringMapperValue, L"d12ms");
			break;
		} else {
			KexLogWarningEvent(L"D3D12on7 is not available for 32-bit applications");
		}

		// fall through
	case D3D12StubImplementation:
		// Use the stubs in KxDx.dll.
		RtlInitConstantUnicodeString(&StringMapperValue, L"kxdx");
		break;
	default:
		ASSERT (FALSE);
		break;
	}

	unless (StringMapperValue.Buffer == NULL) {
		// remove any existing entry with the same name
		Status = KexRtlRemoveEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

		Status = KexRtlInsertEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey,
			&StringMapperValue);

		ASSERT (NT_SUCCESS(Status));
	}

	//
	// Decide which dnsapi.dll implementation to use.
	//

	RtlInitConstantUnicodeString(&StringMapperKey, L"dnsapi");
	RtlInitEmptyUnicodeString(&StringMapperValue, NULL, 0);

	if (KexData->IfeoParameters.DnsapiImplementation == DnsapiAutomaticImplementation) {
		if (KexData->IfeoParameters.DisableAppSpecific) {
			// Default to the Windows 8 implementation unconditionally.
			KexData->IfeoParameters.DnsapiImplementation = DnsapiWindows8Implementation;
		} else {
			// Apply app-specific DNSAPI. Currently there are no known apps that do not
			// work with the Windows 8 DNSAPI.dll.
			KexData->IfeoParameters.DnsapiImplementation = DnsapiWindows8Implementation;
		}
	}

	switch (KexData->IfeoParameters.DnsapiImplementation) {
	case DnsapiNoImplementation:
		break;
	case DnsapiWindows8Implementation:
		RtlInitConstantUnicodeString(&StringMapperValue, L"dnsw8");
		break;
	default:
		ASSERT (FALSE);
		break;
	}

	unless (StringMapperValue.Buffer == NULL) {
		// remove any existing entry with the same name
		Status = KexRtlRemoveEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey);

		ASSERT (NT_SUCCESS(Status) || Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);

		Status = KexRtlInsertEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey,
			&StringMapperValue);

		ASSERT (NT_SUCCESS(Status));
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
			L"The new DLL search path (used only for static imports of the main process image) is:\r\n\r\n"
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
// itself, and cause a lot of problems. (Probably a DLL load failure followed by
// a confused user wondering why Mojibake.dll was not found).
//
STATIC NTSTATUS KexpLookupDllRewriteEntry(
	IN	PCUNICODE_STRING	DllName,
	OUT	PUNICODE_STRING		RewrittenDllName)
{
	NTSTATUS Status;
	UNICODE_STRING CleanDllName;
	UNICODE_STRING DotDll;
	UNICODE_STRING ApiPrefix;
	USHORT MaximumRewrittenLength;

	CleanDllName = *DllName;

	RtlInitConstantUnicodeString(&DotDll, L".dll");
	RtlInitConstantUnicodeString(&ApiPrefix, L"api-");

	//
	// If the name of the DLL has a .dll extension, shorten the length of it
	// so that it doesn't have a .dll extension anymore.
	// This allows image files to import from DLLs with no extension without
	// choking up the dll rewrite.
	//

	if (KexRtlUnicodeStringEndsWith(&CleanDllName, &DotDll, TRUE)) {
		CleanDllName.Length -= KexRtlUnicodeStringCch(&DotDll) * sizeof(WCHAR);
	}

	MaximumRewrittenLength = CleanDllName.Length;

	//
	// If the name of the DLL starts with "api-" (i.e. it's an API set DLL),
	// then remove the -lX-Y-Z suffix as well.
	// This code will have to be revised when API sets start appearing with
	// individual X-Y-Z numbers greater than 9.
	//

	if (RtlPrefixUnicodeString(&ApiPrefix, &CleanDllName, TRUE)) {
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
	// cch of the original DLL. This shouldn't happen except if someone messed
	// with the DllRewrite registry entries or if there is a coding error.
	//

	ASSERT (RewrittenDllName->Length <= MaximumRewrittenLength);

	if (RewrittenDllName->Length > MaximumRewrittenLength) {
		Status = STATUS_BUFFER_TOO_SMALL;
	}

	return Status;
}

//
// Rewrite a DLL name based on the string mapper entries.
// This function is meant to operate directly on the import directories of
// loaded images, not for general use.
//
STATIC NTSTATUS KexpRewriteImportTableDllNameInPlace(
	IN OUT	PANSI_STRING	AnsiDllName)
{
	NTSTATUS Status;
	UNICODE_STRING DllName;
	UNICODE_STRING RewrittenDllName;

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
	} except (GetExceptionCode() == STATUS_ACCESS_VIOLATION) {
		Status = GetExceptionCode();

		KexLogErrorEvent(
			L"Failed to rewrite DLL import (%wZ -> %wZ): STATUS_ACCESS_VIOLATION",
			&DllName,
			&RewrittenDllName);
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
BOOLEAN KexShouldRewriteImportsOfDll(
	IN	PCUNICODE_STRING	BaseDllName OPTIONAL,
	IN	PCUNICODE_STRING	FullDllName)
{
	if (KexData->IfeoParameters.WinVerSpoof > WinVerSpoofWin7) {
		UNICODE_STRING Iertutil;

		//
		// iertutil.dll checks versions and can shit itself if the version number is
		// too high. So we need to rewrite its imports so our KXBASE version functions
		// get applied to it.
		//

		RtlInitConstantUnicodeString(&Iertutil, L"iertutil.dll");

		if (KexRtlUnicodeStringEndsWith(FullDllName, &Iertutil, TRUE)) {
			KexLogDebugEvent(L"Specially rewriting DLL imports of %wZ", FullDllName);
			return TRUE;
		}
	}

	//
	// If this DLL is inside the Windows directory or is a part of VxKex, do
	// not rewrite its imports.
	//

	if (RtlPrefixUnicodeString(&KexData->WinDir, FullDllName, TRUE) ||
		RtlPrefixUnicodeString(&KexData->KexDir, FullDllName, TRUE)) {
		return FALSE;
	}

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		UNICODE_STRING Wpfgfx;

		//
		// APPSPECIFICHACK: This is some sort of .NET DLL that will screw up if we
		// rewrite its imports. No idea why. What typically happens if you allow its
		// imports to be rewritten is you get a blank window that can be interacted
		// with (i.e. all the buttons and things are "working") but you just can't
		// see anything the app is drawing.
		//

		RtlInitConstantUnicodeString(&Wpfgfx, L"wpfgfx_cor3.dll");

		if (KexRtlUnicodeStringEndsWith(FullDllName, &Wpfgfx, TRUE)) {
			return FALSE;
		}
	}

	return TRUE;
}

NTSTATUS KexRewriteImageImportDirectory(
	IN	PVOID				ImageBase,
	IN	PCUNICODE_STRING	BaseImageName,
	IN	PCUNICODE_STRING	FullImageName)
{
	NTSTATUS Status;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_FILE_HEADER CoffHeader;
	PIMAGE_OPTIONAL_HEADER OptionalHeader;
	PIMAGE_DATA_DIRECTORY ImportDirectory;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor;
	BOOLEAN AtLeastOneImportWasRewritten;
	ULONG OldProtect;

	AtLeastOneImportWasRewritten = FALSE;

	Status = RtlImageNtHeaderEx(
		RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK,
		ImageBase,
		0,
		&NtHeaders);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to retrieve the address of the image NT headers for %wZ\r\n\r\n"
			L"NTSTATUS error code: %s",
			BaseImageName,
			KexRtlNtStatusToString(Status));

		return Status;
	}

	CoffHeader = &NtHeaders->FileHeader;
	OptionalHeader = &NtHeaders->OptionalHeader;
	ImportDirectory = &OptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

	if ((KexRtlCurrentProcessBitness() == 64) != (CoffHeader->Machine == 0x8664)) {
		//
		// 32-bit dll loaded in 64-bit process or vice versa
		// This can happen with resource-only DLLs, in which case there are no
		// imports to rewrite anyway.
		//

		KexLogInformationEvent(
			L"The bitness of %wZ does not match the host process\r\n\r\n"
			L"This may be caused by a resource-only DLL and "
			L"is not usually a cause for concern. Not rewriting this import.",
			BaseImageName);

		return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
	}

	if (OptionalHeader->NumberOfRvaAndSizes < (IMAGE_DIRECTORY_ENTRY_IMPORT + 1) ||
		ImportDirectory->VirtualAddress == 0) {
		//
		// There is no import directory in the image (e.g. resource-only DLL).
		//

		KexLogDebugEvent(
			L"%wZ contains no import directory",
			BaseImageName);

		return STATUS_IMAGE_NO_IMPORT_DIRECTORY;
	}

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) RVA_TO_VA(ImageBase, ImportDirectory->VirtualAddress);

	if (ImportDescriptor->Name == 0) {
		//
		// There shouldn't be an import directory if it has no entries.
		//

		KexLogWarningEvent(
			L"%wZ contains an empty import directory",
			BaseImageName);

		return STATUS_INVALID_IMAGE_FORMAT;
	}

	//
	// Set the entire section that contains the image import directory to read-write.
	//
	
	Status = KexLdrProtectImageImportSection(
		ImageBase,
		PAGE_READWRITE,
		&OldProtect);

	if (!NT_SUCCESS(Status)) {
		KexLogErrorEvent(
			L"Failed to change page protection on import section to read-write\r\n\r\n"
			L"Image base: 0x%p\r\n"
			L"NTSTATUS error code: %s",
			ImageBase,
			KexRtlNtStatusToString(Status));

		return Status;
	}

	//
	// Walk through imports and rewrite each one if necessary.
	//

	do {
		PSTR DllNameBuffer;
		ANSI_STRING ImportedDllNameAnsi;

		DllNameBuffer = (PSTR) RVA_TO_VA(ImageBase, ImportDescriptor->Name);
		RtlInitAnsiString(&ImportedDllNameAnsi, DllNameBuffer);

		Status = KexpRewriteImportTableDllNameInPlace(&ImportedDllNameAnsi);

		if (NT_SUCCESS(Status)) {
			AtLeastOneImportWasRewritten = TRUE;
		}
	} while ((++ImportDescriptor)->Name != 0);

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

		if (NT_SUCCESS(Status)) {
			RtlZeroMemory(BoundImportDirectory, sizeof(*BoundImportDirectory));
		} else {
			KexLogErrorEvent(
				L"Failed to change page protection\r\n\r\n"
				L"on memory at base 0x%p (region size %Iu)\r\n"
				L"NTSTATUS error code: %s",
				DataDirectoryPtr,
				DataDirectorySize,
				KexRtlNtStatusToString(Status));
		}

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
// This function expects Win32 paths and is intended to be used for outside
// modules, such as KXBASE.
//
// The path may be omitted from the DllPath argument, i.e. "kernel32.dll".
// The rewritten path will always be a DLL base name without extension, for
// example "kxbase".
//
// The RewrittenDllPath argument must have the Buffer element set to an
// appropriate Unicode buffer, and the MaximumLength element set to the size
// in bytes of that buffer.
//
// It's OK to have DllPath and RewrittenDllNameOut point to the same
// UNICODE_STRING structure.
//
KEXAPI NTSTATUS NTAPI KexRewriteDllPath(
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

	if (!DllPath || !DllPath->Buffer || !DllPath->Length) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!RewrittenDllNameOut || !RewrittenDllNameOut->Buffer || !RewrittenDllNameOut->MaximumLength) {
		return STATUS_INVALID_PARAMETER_2;
	}

	if (RewrittenDllNameOut->MaximumLength < DllPath->Length) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Set this length to zero. We will append to it later.
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
	return Status;
}