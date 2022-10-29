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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC PKEX_RTL_STRING_MAPPER DllRewriteStringMapper = NULL;

//
// Add the Kex DLLs directory to the Path environment variable so that DLLs will
// get loaded from there when NTDLL processes imports.
//
STATIC NTSTATUS KexpAddKex3264ToPath(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING PathName;
	UNICODE_STRING PathValue;
	WCHAR PathAppendBuffer[MAX_PATH];
	UNICODE_STRING PathAppend;

	RtlInitUnicodeString(&PathName, L"Path");
	RtlInitEmptyUnicodeString(&PathValue, NULL, 0);

	//
	// form the string we need to append onto Path
	//

	RtlInitEmptyUnicodeString(&PathAppend, PathAppendBuffer, sizeof(PathAppendBuffer));
	RtlAppendUnicodeToString(&PathAppend, L";");
	RtlAppendUnicodeStringToString(&PathAppend, &KexData->KexDir);

	if (KexIs64BitBuild) {
		RtlAppendUnicodeToString(&PathAppend, L"\\Kex64");
	} else {
		RtlAppendUnicodeToString(&PathAppend, L"\\Kex32");
	}

	//
	// First find the length of the current %Path% - RtlQueryEnvironmentVariable_U
	// is expected to return STATUS_BUFFER_TOO_SMALL, setting PathValue.Length to
	// the correct buffer size.
	//
	Status = RtlQueryEnvironmentVariable_U(
		NULL,
		&PathName,
		&PathValue);

	if (Status == STATUS_BUFFER_TOO_SMALL) {
		// Allocate correct sized buffer and retry
		PathValue.MaximumLength = PathValue.Length + PathAppend.Length;
		PathValue.Buffer = (PWCHAR) StackAlloc(BYTE, PathValue.MaximumLength);
		
		Status = RtlQueryEnvironmentVariable_U(
			NULL,
			&PathName,
			&PathValue);
	} else if (Status == STATUS_VARIABLE_NOT_FOUND) {
		//
		// When a process is created with CreateProcess, the parent could
		// theoretically supply an empty environment block. If this happens,
		// skip past the ';' at the beginning, and just set the whole %Path%
		// env var to KexDir.
		//

		++PathAppend.Buffer;
		PathAppend.Length -= sizeof(WCHAR);
		PathAppend.MaximumLength -= sizeof(WCHAR);
		return RtlSetEnvironmentVariable(NULL, &PathName, &PathAppend);
	}
	
	if (NT_SUCCESS(Status)) {
		//
		// Successfully queried %Path% env var to PathValue - now append our
		// entry and write back to %Path%
		//

		Status = RtlAppendUnicodeStringToString(&PathValue, &PathAppend);
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		return RtlSetEnvironmentVariable(NULL, &PathName, &PathValue);
	}

	return Status;
}

//
// Read DLL rewrite information from a registry key and place it in a
// string mapper for efficient querying.
//
// This function also adds a PATH entry which contains Kex DLLs. (e.g.
// C:\Program Files\VxKex\Kex64 or Kex32).
//
NTSTATUS KexInitializeDllRewrite(
	VOID)
{
	NTSTATUS Status;
	HANDLE DllRewriteKeyHandle;
	UNICODE_STRING DllRewriteKeyName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	ULONG Index;

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

	RtlInitUnicodeString(&DllRewriteKeyName, L"\\Registry\\Machine\\Software\\VXsoft\\VxKex\\DllRewrite");
	InitializeObjectAttributes(&ObjectAttributes, &DllRewriteKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = NtOpenKey(
		&DllRewriteKeyHandle,
		KEY_READ,
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
		UNICODE_STRING StringMapperKey;
		UNICODE_STRING StringMapperValue;

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

		if (!KeyInformationBuffer) {
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = NtEnumerateValueKey(
			DllRewriteKeyHandle,
			Index,
			KeyValueFullInformation,
			KeyInformationBuffer,
			KeyInformationBufferCb,
			&KeyInformationBufferCb);

		if (!NT_SUCCESS(Status)) {
			SafeFree(KeyInformationBuffer);

			if (Status == STATUS_NO_MORE_ENTRIES) {
				Status = STATUS_SUCCESS;
				break;
			} else {
				KexSrvLogWarningEvent(
					L"Failed to read a DLL rewrite value\r\n\r\n"
					L"NTSTATUS error code: 0x%08lx",
					Status);

				// In release builds we don't care if stuff fails. Too bad, we have
				// to make do with what we have.
				if (KexIsDebugBuild) {
					break;
				} else {
					continue;
				}
			}
		}

		//
		// The DLL rewrite value must be a string.
		//

		if (KeyInformationBuffer->Type != REG_SZ) {
			SafeFree(KeyInformationBuffer);
			Status = STATUS_REG_DATA_TYPE_MISMATCH;

			KexSrvLogWarningEvent(
				L"A registry DLL rewrite key has the wrong data type.\r\n\r\n"
				L"Check HKLM\\Software\\VXsoft\\VxKex\\DllRewrite and remove any non-string keys.");

			if (KexIsDebugBuild) {
				break;
			} else {
				continue;
			}
		}

		//
		// Resize the ~1KB heap allocation we made earlier down to only the
		// size we need.
		//

		KeyInformationBufferCb = sizeof(KEY_VALUE_FULL_INFORMATION) +
								 KeyInformationBuffer->NameLength +
								 KeyInformationBuffer->DataLength;
		KeyInformationBuffer = (PKEY_VALUE_FULL_INFORMATION) SafeReAlloc(
			KeyInformationBuffer,
			BYTE, KeyInformationBufferCb);

		if (!KeyInformationBuffer) {
			SafeFree(KeyInformationBuffer);
			Status = STATUS_NO_MEMORY;
			break;
		}

		StringMapperKey.Length = (USHORT) KeyInformationBuffer->NameLength;
		StringMapperKey.MaximumLength = StringMapperKey.Length;
		StringMapperKey.Buffer = KeyInformationBuffer->NameAndData;

		StringMapperValue.Length = (USHORT) KeyInformationBuffer->DataLength;
		StringMapperValue.MaximumLength = StringMapperKey.Length;
		StringMapperValue.Buffer = &KeyInformationBuffer->NameAndData[KeyInformationBuffer->NameLength / sizeof(WCHAR)];

		KexSrvLogDebugEvent(
			L"Processed a DLL rewrite registry entry: %wZ -> %wZ",
			&StringMapperKey,
			&StringMapperValue);

		Status = KexRtlInsertEntryStringMapper(
			DllRewriteStringMapper,
			&StringMapperKey,
			&StringMapperValue);

		if (KexIsDebugBuild && !NT_SUCCESS(Status)) {
			break;
		}
	}

	NtClose(DllRewriteKeyHandle);
	
	Status = KexpAddKex3264ToPath();
	if (!NT_SUCCESS(Status)) {
		KexSrvLogErrorEvent(
			L"Failed to append Kex32/64 to %%Path%%\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			Status);
	}

	return Status;
}

//
// Rewrite a DLL name based on the string mapper entries.
//
STATIC NTSTATUS KexpRewriteDllName(
	IN OUT	PANSI_STRING	AnsiDllName)
{
	NTSTATUS Status;
	UNICODE_STRING DllName;
	UNICODE_STRING RewrittenDllName;
	UNICODE_STRING DotDll;
	
	Status = RtlAnsiStringToUnicodeString(&DllName, AnsiDllName, TRUE);
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// If the original DLL name ends with ".dll", then we strip it out.
	// This allows image files to import from DLLs with no extension without
	// choking up the dll rewrite.
	//

	RtlInitUnicodeString(&DotDll, L".dll");

	if (KexRtlUnicodeStringEndsWith(&DllName, &DotDll, TRUE)) {
		DllName.Length -= DotDll.Length;
	}

	Status = KexRtlLookupEntryStringMapper(
		DllRewriteStringMapper,
		&DllName,
		&RewrittenDllName);

	RtlFreeUnicodeString(&DllName);

	//
	// If no entry was found in the string mapper, or another error occurred,
	// just return and leave the original DLL name un-modified.
	//

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Check to see that the cch of the rewritten DLL is not greater than the
	// cch of the original DLL. This shouldn't happen except if someone messed
	// with the DllRewrite registry entries or if there is a coding error.
	//

	if (KexRtlUnicodeStringCch(&RewrittenDllName) > KexRtlAnsiStringCch(AnsiDllName)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	Status = RtlUnicodeStringToAnsiString(
		AnsiDllName,
		&RewrittenDllName,
		FALSE);

	return Status;
}

//
// Determine whether the imports of a particular DLL (identified by name and
// path) should be rewritten.
//
BOOLEAN KexShouldRewriteImportsOfDll(
	IN	PCUNICODE_STRING	BaseDllName,
	IN	PCUNICODE_STRING	FullDllName)
{
	//
	// If this DLL is inside the Windows directory or is a part of VxKex, do
	// not rewrite its imports.
	//

	if (RtlPrefixUnicodeString(&KexData->WinDir, FullDllName, TRUE) ||
		RtlPrefixUnicodeString(&KexData->KexDir, FullDllName, TRUE)) {
		return FALSE;
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

	AtLeastOneImportWasRewritten = FALSE;

	Status = RtlImageNtHeaderEx(RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK, ImageBase, 0, &NtHeaders);
	if (!NT_SUCCESS(Status)) {
		KexSrvLogErrorEvent(
			L"Failed to retrieve the address of the image NT headers for %wZ\r\n\r\n"
			L"NTSTATUS error code: 0x%08lx",
			BaseImageName,
			Status);

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

		KexSrvLogInformationEvent(
			L"The bitness of %wZ does not match the host process\r\n\r\n"
			L"This may be caused by a resource-only DLL and "
			L"is not usually a cause for concern. Not rewriting this import.",
			BaseImageName);

		return STATUS_IMAGE_MACHINE_TYPE_MISMATCH;
	}

	if (OptionalHeader->NumberOfRvaAndSizes < (IMAGE_DIRECTORY_ENTRY_IMPORT + 1) ||
		ImportDirectory->VirtualAddress == 0) {
		//
		// There is no import directory in the image.
		//

		KexSrvLogInformationEvent(
			L"%wZ contains no import directory",
			BaseImageName);

		return STATUS_IMAGE_NO_IMPORT_DIRECTORY;
	}

	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) RVA_TO_VA(ImageBase, ImportDirectory->VirtualAddress);

	if (ImportDescriptor->Name == 0) {
		//
		// There shouldn't be an import directory if it has no entries.
		//

		KexSrvLogWarningEvent(
			L"%wZ contains an empty import directory",
			BaseImageName);

		return STATUS_INVALID_IMAGE_FORMAT;
	}

	//
	// Walk through imports and rewrite each one if necessary.
	//

	do {
		ULONG OldProtect;
#ifdef _DEBUG
		CHAR OriginalDllNameBuffer[MAX_PATH];
		ANSI_STRING ImportedDllNameOriginal;
#endif
		PSTR DllNameBuffer;
		SIZE_T DllNameBufferCb;
		ANSI_STRING ImportedDllNameAnsi;

		DllNameBuffer = (PSTR) RVA_TO_VA(ImageBase, ImportDescriptor->Name);
		RtlInitAnsiString(&ImportedDllNameAnsi, DllNameBuffer);
		ImportedDllNameAnsi.MaximumLength;
		DllNameBufferCb = ImportedDllNameAnsi.MaximumLength;

#ifdef _DEBUG
		ImportedDllNameOriginal.MaximumLength = sizeof(OriginalDllNameBuffer);
		ImportedDllNameOriginal.Buffer = OriginalDllNameBuffer;
		RtlCopyAnsiString(&ImportedDllNameOriginal, &ImportedDllNameAnsi);
#endif

		//
		// Before we actually modify and write the DLL name, we will
		// change memory protections to make sure we can do this.
		//
		// 1. We can't change memory protections on the entire import
		//    directory because there is no guarantee that the DLL names
		//    themselves actually reside here. So there is no potential
		//    for optimization.
		//
		// 2. If changing memory protections here fails, we will not
		//    rewrite the import, because it may lead to an access violation
		//    and crash the process.
		//

		Status = NtProtectVirtualMemory(
			NtCurrentProcess(),
			(PPVOID) &DllNameBuffer,
			&DllNameBufferCb,
			PAGE_READWRITE,
			&OldProtect);

		if (!NT_SUCCESS(Status)) {
			KexSrvLogErrorEvent(
				L"Failed to change page protection\r\n\r\n"
				L"on memory at base 0x%p (region size %hu)\r\n"
				L"NTSTATUS error code: 0x%08lx",
				ImportedDllNameAnsi.Buffer,
				ImportedDllNameAnsi.MaximumLength,
				Status);

			continue;
		}

		Status = KexpRewriteDllName(&ImportedDllNameAnsi);

		// restore old permissions
		NtProtectVirtualMemory(
			NtCurrentProcess(),
			(PPVOID) &DllNameBuffer,
			&DllNameBufferCb,
			OldProtect,
			&OldProtect);

		if (NT_SUCCESS(Status)) {
			AtLeastOneImportWasRewritten = TRUE;

			KexSrvLogDebugEvent(
				L"Rewrote DLL import (%hZ -> %hZ)",
				&ImportedDllNameOriginal,
				&ImportedDllNameAnsi);
		} else {
			KexSrvLogDebugEvent(
				L"Did not rewrite DLL import %hZ\r\n\r\n"
				L"NTSTATUS result code: 0x%08lx",
				&ImportedDllNameOriginal,
				Status);
		}
	} while ((++ImportDescriptor)->Name != 0);

	if (AtLeastOneImportWasRewritten) {
		PIMAGE_DATA_DIRECTORY BoundImportDirectory;
		PVOID DataDirectoryPtr;
		SIZE_T DataDirectorySize;
		ULONG OldProtect;

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

		if (NT_SUCCESS(Status)) {
			RtlZeroMemory(BoundImportDirectory, sizeof(*BoundImportDirectory));
		} else {
			KexSrvLogErrorEvent(
				L"Failed to change page protection\r\n\r\n"
				L"on memory at base 0x%p (region size %Iu)\r\n"
				L"NTSTATUS error code: 0x%08lx",
				DataDirectoryPtr,
				DataDirectorySize,
				Status);
		}

		NtProtectVirtualMemory(
			NtCurrentProcess(),
			&DataDirectoryPtr,
			&DataDirectorySize,
			OldProtect,
			&OldProtect);
	}

	return STATUS_SUCCESS;
}