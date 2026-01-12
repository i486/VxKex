///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     mls.c
//
// Abstract:
//
//     This file contains the code used to implement multi-language support in
//     VxKex.
//
//     This static library must operate under the following conditions:
//
//       1. Inside KexSetup, when VxKex has not yet been installed.
//       2. Inside KexDll, where the Win32 API is not yet available.
//       3. Inside ordinary Win32 applications such as VxlView.
//
//     Therefore, the MLS library must only depend on NTDLL.
//
// Author:
//
//     vxiiduu (17-May-2025)
//
// Environment:
//
//     Native mode
//
// Revision History:
//
//     vxiiduu              17-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexDll.h>
#include <KexMls.h>
#include <KexSmp.h>

STATIC BOOLEAN MlsInitialized = FALSE;
STATIC LANGID MlsLangId = LANG_ENGLISH;
STATIC LANGID MlsOverrideLangId = LANG_NEUTRAL;
STATIC MLSP_DICTIONARY MlsDictionary = {0};
STATIC PKEX_SMP_STRING_MAPPER MlsStringMapper = NULL;

//
// Get the language ID of the language we will try to translate all strings into.
// The MLS language ID is determined as follows:
//
//   1. See if the user has specified an override LangID through the registry.
//   2. If not, try to get the user's default language ID.
//   3. If that fails, use English as the default.
//
// Note: MlspGetIdealLangId returning some LANGID does not mean that we will
// actually translate any text into that language. For example, we may find that
// a particular language is not supported and either map it to another language
// or just fall back to the English default. This fallback mapping is the
// responsibility of MlsInitialize.
//
STATIC LANGID NTAPI MlspGetIdealLangId(
	VOID)
{
	NTSTATUS Status;
	HANDLE CurrentUserKeyHandle;
	HANDLE VxKexUserKeyHandle;
	UNICODE_STRING VxKexUserKeyPath;
	OBJECT_ATTRIBUTES ObjectAttributes;
	LANGID LangId;

	//
	// First of all, see if the user has specified an override LangID in the
	// registry. This value is located at HKCU\Software\VXsoft\VxKex\LanguageId
	// and is a DWORD value containing a LANGID code.
	//
	// This could fail for many reasons. For example, VxKex may not be installed
	// yet. Another example is if the user has simply not specified an override
	// LangID.
	//

	CurrentUserKeyHandle = NULL;
	VxKexUserKeyHandle = NULL;

	try {
		UNICODE_STRING ValueName;
		PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
		PVOID KeyValueBuffer;
		ULONG KeyValueBufferCb;
		ULONG KeyValue;
		ULONG ValueDataCb;

		//
		// Open HKCU key.
		//

		Status = RtlOpenCurrentUser(KEY_ENUMERATE_SUB_KEYS, &CurrentUserKeyHandle);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			leave;
		}

		//
		// Open VxKex user key.
		//

		RtlInitConstantUnicodeString(&VxKexUserKeyPath, L"SOFTWARE\\VXsoft\\VxKex");

		InitializeObjectAttributes(
			&ObjectAttributes,
			&VxKexUserKeyPath,
			OBJ_CASE_INSENSITIVE,
			CurrentUserKeyHandle,
			NULL);

		Status = NtOpenKey(
			&VxKexUserKeyHandle,
			KEY_READ,
			&ObjectAttributes);

		if (!NT_SUCCESS(Status)) {
			ASSERT (Status == STATUS_OBJECT_NAME_NOT_FOUND);
			leave;
		}

		//
		// Query "LanguageId" value.
		//

		KeyValueBufferCb = sizeof(ULONG) + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
		KeyValueBuffer = StackAlloc(BYTE, KeyValueBufferCb);

		RtlInitConstantUnicodeString(&ValueName, L"LanguageId");

		Status = NtQueryValueKey(
			VxKexUserKeyHandle,
			&ValueName,
			KeyValuePartialInformation,
			KeyValueBuffer,
			KeyValueBufferCb,
			&ValueDataCb);

		if (!NT_SUCCESS(Status)) {
			ASSERT (Status == STATUS_OBJECT_NAME_NOT_FOUND);
			leave;
		}

		//
		// Check the data type to make sure it is REG_DWORD. If not, bail out.
		//

		ASSUME (ValueDataCb >= sizeof(KEY_VALUE_PARTIAL_INFORMATION));
		ValueDataCb -= sizeof(KEY_VALUE_PARTIAL_INFORMATION);
		KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION) KeyValueBuffer;

		if (KeyValueInformation->Type != REG_DWORD) {
			leave;
		}

		if (KeyValueInformation->DataLength != sizeof(ULONG)) {
			leave;
		}

		//
		// We have our REG_DWORD value. Make sure it looks like a valid LANGID
		// (fits within 16-bits).
		//

		RtlCopyMemory(&KeyValue, KeyValueInformation->Data, sizeof(ULONG));

		if (KeyValue > USHRT_MAX) {
			// DWORD value is too large to be a LANGID
			leave;
		}

		//
		// We have our LANGID from the registry. Return that to the caller.
		//

		LangId = (LANGID) KeyValue;
		goto Finished;
	} finally {
		SafeClose(CurrentUserKeyHandle);
		SafeClose(VxKexUserKeyHandle);
	}

	//
	// The registry didn't give us a LANGID. Try to get the user's default
	// LANGID.
	//

	Status = NtQueryDefaultUILanguage(&LangId);
	ASSERT (NT_SUCCESS(Status));

	if (NT_SUCCESS(Status)) {
		goto Finished;
	}

	//
	// We can't get the user's default LANGID. Just use English (default).
	//
	
	LangId = LANG_ENGLISH;

Finished:
	//
	// Strip the sublanguage out of the LangId in order to simplify
	// downstream code.
	// Chinese requires special handling because it has a simplified and
	// traditional variation.
	//

	if (PRIMARYLANGID(LangId) == LANG_CHINESE) {
		switch (SUBLANGID(LangId)) {
		case SUBLANG_CHINESE_TRADITIONAL:
		case SUBLANG_CHINESE_HONGKONG:
		case SUBLANG_CHINESE_MACAU:
			LangId = LANG_CHINESE_TRADITIONAL;
			break;
		default:
			LangId = LANG_CHINESE_SIMPLIFIED;
			break;
		}
	} else {
		LangId = PRIMARYLANGID(LangId);
	}

	return LangId;
}

//
// Returns NULL for unsupported language.
//
STATIC PCWSTR NTAPI MlspGetLanguageName(
	IN	LANGID	LangId)
{
	switch (LangId) {
	case LANG_ARABIC:				return L"ar";
	case LANG_BELARUSIAN:			return L"be";
	case LANG_BULGARIAN:			return L"bg";
	case LANG_CHINESE_SIMPLIFIED:	return L"zh-CN";
	case LANG_CHINESE_TRADITIONAL:	return L"zh-TW";
	case LANG_CZECH:				return L"cs";
	case LANG_DANISH:				return L"da";
	case LANG_DUTCH:				return L"nl";
	case LANG_ESTONIAN:				return L"et";
	case LANG_FINNISH:				return L"fi";
	case LANG_FRENCH:				return L"fr";
	case LANG_GEORGIAN:				return L"ka";
	case LANG_GERMAN:				return L"de";
	case LANG_HUNGARIAN:			return L"hu";
	case LANG_ITALIAN:				return L"it";
	case LANG_JAPANESE:				return L"ja";
	case LANG_KAZAK:				return L"kk";
	case LANG_KYRGYZ:				return L"ky";
	case LANG_KOREAN:				return L"ko";
	case LANG_LAO:					return L"lo";
	case LANG_LATVIAN:				return L"lv";
	case LANG_LITHUANIAN:			return L"lt";
	case LANG_MALAY:				return L"ms";
	case LANG_NORWEGIAN:			return L"no";
	case LANG_PERSIAN:				return L"fa";
	case LANG_POLISH:				return L"pl";
	case LANG_PORTUGUESE:			return L"pt";
	case LANG_ROMANIAN:				return L"ro";
	case LANG_RUSSIAN:				return L"ru";
	case LANG_SERBIAN:				return L"sr";
	case LANG_SLOVAK:				return L"sk";
	case LANG_SPANISH:				return L"es";
	case LANG_SWEDISH:				return L"sv";
	case LANG_TAJIK:				return L"tg";
	case LANG_THAI:					return L"th";
	case LANG_TURKISH:				return L"tr";
	case LANG_TURKMEN:				return L"tk";
	case LANG_UKRAINIAN:			return L"uk";
	case LANG_UZBEK:				return L"uz";
	case LANG_VIETNAMESE:			return L"vi";
	default:						return NULL;
	}
}

//
// Load a dictionary. We will search the "Languages" subdirectory in the
// current executable's directory for the correct .bdi file.
//
// This approach works for KexSetup and all other VxKex applications which are
// installed into KexDir.
//
STATIC NTSTATUS MlspLoadDictionary(
	IN	LANGID				LangId,
	OUT	PMLSP_DICTIONARY	Dictionary)
{
	NTSTATUS Status;
	HRESULT Result;
	ULONG Index;
	PCWSTR LanguageName;

	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	HANDLE FileHandle;
	HANDLE SectionHandle;
	UNICODE_STRING FileName;
	WCHAR FileNameBuffer[MAX_PATH + 4]; // +4 for \??\

	ASSERT (LangId != LANG_ENGLISH);
	ASSERT (Dictionary != NULL);
	ASSERT (Dictionary->Data == NULL);
	ASSERT (Dictionary->DataCb == 0);
	ASSERT (Dictionary->Flags == 0);

	//
	// Find the name of the current language. This can be either in the format such as
	// "ru", "es" etc. or can be a two-part name like "zh-CN" and "zh-TW".
	//

	LanguageName = MlspGetLanguageName(LangId);
	if (!LanguageName) {
		return STATUS_OBJECT_NAME_NOT_FOUND;
	}

	//
	// Build the path to the dictionary file.
	// We will look for the dictionary file at %EXE_DIR%\Languages\%LanguageName%.bdi
	//

	// We need \??\ because NtOpenFile expects a NT-style path.
	FileNameBuffer[0] = '\\';
	FileNameBuffer[1] = '?';
	FileNameBuffer[2] = '?';
	FileNameBuffer[3] = '\\';
	FileNameBuffer[4] = '\0';

	// Append the path to the EXE.
	Result = StringCchCat(
		FileNameBuffer,
		ARRAYSIZE(FileNameBuffer),
		NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);

	ASSERT (SUCCEEDED(Result));
	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Remove the EXE name.
	Index = (ULONG) wcslen(FileNameBuffer);

	until (FileNameBuffer[Index] == '\\' || Index == 0) {
		--Index;
	}

	if (FileNameBuffer[Index] != '\\') {
		return STATUS_OBJECT_NAME_INVALID;
	}

	FileNameBuffer[Index] = '\0';

	// FileNameBuffer now contains the directory which the EXE is in.
	Result = StringCchCat(FileNameBuffer, ARRAYSIZE(FileNameBuffer), L"\\Languages\\");
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Append the name of the language.
	Result = StringCchCat(FileNameBuffer, ARRAYSIZE(FileNameBuffer), LanguageName);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	// Append the .bdi extension.
	Result = StringCchCat(FileNameBuffer, ARRAYSIZE(FileNameBuffer), L".bdi");
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	//
	// FileNameBuffer now contains the full path to the binary dictionary (.bdi)
	// file, so we can try to map it into memory.
	//

	RtlInitUnicodeString(&FileName, FileNameBuffer);

	InitializeObjectAttributes(
		&ObjectAttributes,
		&FileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = NtOpenFile(
		&FileHandle,
		GENERIC_READ | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_READ,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if (!NT_SUCCESS(Status)) {
		ASSERT (Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND);
		return Status;
	}

	Status = NtCreateSection(
		&SectionHandle,
		SECTION_MAP_READ,
		NULL,
		NULL,
		PAGE_READONLY,
		SEC_COMMIT,
		FileHandle);

	ASSERT (NT_SUCCESS(Status));

	SafeClose(FileHandle);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	Status = NtMapViewOfSection(
		SectionHandle,
		NtCurrentProcess(),
		(PPVOID) &Dictionary->Data,
		0,
		0,
		NULL,
		&Dictionary->DataCb,
		ViewUnmap,
		0,
		PAGE_READONLY);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (Dictionary->Data != NULL);
	ASSERT (Dictionary->DataCb != 0);

	SafeClose(SectionHandle);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Read the header of the bdi file and verify that everything matches
	// what we expect.
	//

	Dictionary->Header = (PMLSP_DICTIONARY_HEADER) Dictionary->Data;

	if (Dictionary->Header->Magic[0] != 'B' ||
		Dictionary->Header->Magic[1] != 'D' ||
		Dictionary->Header->Magic[2] != 'I' ||
		Dictionary->Header->Magic[3] != 'C') {

		Status = STATUS_MLS_BDI_MAGIC_MISMATCH;
		goto Exit;
	}

	if (Dictionary->Header->Version != MLSP_VERSION) {
		Status = STATUS_VERSION_MISMATCH;
		goto Exit;
	}

	Dictionary->Data = RVA_TO_VA(Dictionary->Header, sizeof(MLSP_DICTIONARY_HEADER));
	Dictionary->DataCb -= sizeof(MLSP_DICTIONARY_HEADER);

	// Mark this dictionary as being a mapped file so we can unmap it later.
	Dictionary->Flags |= MLSP_DICTIONARY_FLAG_MAPPED_FILE;

	Status = STATUS_SUCCESS;

Exit:
	if (!NT_SUCCESS(Status)) {
		NTSTATUS Status2;
		
		Status2 = NtUnmapViewOfSection(
			NtCurrentProcess(),
			(PVOID) Dictionary->Header);

		ASSERT (NT_SUCCESS(Status2));

		RtlZeroMemory(Dictionary, sizeof(*Dictionary));
	}

	return Status;
}

STATIC NTSTATUS MlspCleanupDictionary(
	IN OUT	PMLSP_DICTIONARY	Dictionary)
{
	NTSTATUS Status;

	ASSERT (Dictionary != NULL);

	Status = STATUS_SUCCESS;

	if (Dictionary->Flags & MLSP_DICTIONARY_FLAG_MAPPED_FILE) {
		ASSERT (Dictionary->Data != NULL);
		ASSERT (Dictionary->DataCb != 0);

		Status = NtUnmapViewOfSection(
			NtCurrentProcess(),
			(PVOID) Dictionary->Data);

		ASSERT (NT_SUCCESS(Status));
	}

	RtlZeroMemory(Dictionary, sizeof(*Dictionary));
	return Status;
}

//
// Public API for getting the current language.
//
MLSAPI NTSTATUS NTAPI MlsGetCurrentLangId(
	OUT	PLANGID	LangId)
{
	if (!LangId) {
		return STATUS_INVALID_PARAMETER_1;
	}

	if (!MlsInitialized) {
		return STATUS_MLS_NOT_INITIALIZED;
	}

	*LangId = MlsLangId;
	return STATUS_SUCCESS;
}

//
// Go through a given Dictionary, and call the iterator function for every
// key-value pair.
// Note that the function signature for MLSP_DICTIONARY_ITERATOR is very not-
// coincidentally the same as the one for SmpInsertEntryStringMapper.
//
STATIC NTSTATUS MlspScanDictionary(
	IN	PMLSP_DICTIONARY			Dictionary,
	IN	PMLSP_DICTIONARY_ITERATOR	Iterator,
	IN	PVOID						IteratorContext OPTIONAL)
{
	NTSTATUS Status;
	PCWCHAR Bdi;
	ULONG BdiCch;
	UNICODE_STRING Key;
	UNICODE_STRING Value;
	ULONG Index;
	ULONG NumberOfKeyValuePairsProcessed;
	BOOLEAN InKey;
	BOOLEAN OversizedKeyOrValue;
	BOOLEAN KeyValueSwapped;

	ASSERT (Dictionary != NULL);
	ASSERT (Dictionary->Data != NULL);
	ASSERT (Dictionary->DataCb != 0);
	ASSERT (Iterator != NULL);

	if (Dictionary->DataCb & 1) {
		return STATUS_INVALID_BUFFER_SIZE;
	}

	if ((Dictionary->DataCb / sizeof(WCHAR)) > ULONG_MAX) {
		return STATUS_FILE_TOO_LARGE;
	}

	Status = STATUS_SUCCESS;

	Bdi = Dictionary->Data;
	BdiCch = (ULONG) (Dictionary->DataCb / sizeof(WCHAR));

	NumberOfKeyValuePairsProcessed = 0;
	InKey = TRUE;
	OversizedKeyOrValue = FALSE;
	KeyValueSwapped = FALSE;

	Key.Length = 0;
	Key.MaximumLength = USHRT_MAX - 1;
	Key.Buffer = (PWCHAR) Bdi;

	Value.Length = 0;
	Value.MaximumLength = USHRT_MAX - 1;
	Value.Buffer = NULL;

	for (Index = 0; Index < BdiCch; ++Index) {
		if (Bdi[Index] == '\0') {
			if (InKey && Key.Length == 0) {
				// Empty key (double null terminator) marks the end of data.
				break;
			}

			if (!InKey) {
				// Finished processing key-value pair.

				if (Value.Length > 0 && !OversizedKeyOrValue) {
					
					Status = Iterator(IteratorContext, &Key, &Value);
					++NumberOfKeyValuePairsProcessed;

					if (NumberOfKeyValuePairsProcessed >= Dictionary->Header->NumberOfKeyValuePairs) {
						break;
					}
					
					if (!NT_SUCCESS(Status)) {
						break;
					}
				} else {
					// Some kind of invalid entry. This isn't supposed to happen.
					// We'll raise an assertion in debug builds and just ignore it
					// in release builds.
					ASSERT (("Invalid key or value length", FALSE));
				}

				Key.Length = 0;
				Value.Length = 0;
				OversizedKeyOrValue = FALSE;
			}

			InKey = !InKey;
			KeyValueSwapped = TRUE;

			continue;
		}

		if (OversizedKeyOrValue) {
			// Don't add any more data because we'll ignore any key-value pairs
			// where one of them is oversized.
			continue;
		}

		if (KeyValueSwapped) {
			// We just swapped from a key to a value or vice versa, and we're now past
			// the null terminator so we can set the buffer to our current character.

			if (InKey) {
				Key.Buffer = (PWCHAR) &Bdi[Index];
			} else {
				Value.Buffer = (PWCHAR) &Bdi[Index];
			}

			KeyValueSwapped = FALSE;
		}

		if (InKey) {
			if (Key.Length >= (Key.MaximumLength - sizeof(WCHAR))) {
				OversizedKeyOrValue = TRUE;
			}

			Key.Length += sizeof(WCHAR);
		} else {
			if (Value.Length >= (Value.MaximumLength - sizeof(WCHAR))) {
				OversizedKeyOrValue = TRUE;
			}

			Value.Length += sizeof(WCHAR);
		}
	}

	return Status;
}

//
// This function initializes multi-language support.
//
//   1. Get the ideal LANGID using MlspGetIdealLangId.
//   2. Check if this language is supported.
//   3. If it is, load or find and then process the dictionary file.
//   4. If not, try to map the language to something else that is supported.
//   5. If the language cannot be mapped, or if the mapped language's dictionary
//      cannot be loaded, fall back to English.
//
MLSAPI NTSTATUS NTAPI MlsInitialize(
	VOID)
{
	NTSTATUS Status;
	LANGID IdealLangId;

	if (MlsInitialized) {
		return STATUS_ALREADY_INITIALIZED;
	}

	Status = STATUS_SUCCESS;

	//
	// Find the user's language.
	//

	if (MlsOverrideLangId != LANG_NEUTRAL) {
		IdealLangId = MlsOverrideLangId;
	} else {
		IdealLangId = MlspGetIdealLangId();
	}

	MlsLangId = IdealLangId;

	if (IdealLangId == LANG_ENGLISH) {
		//
		// Nothing needs to be done. No need to load dictionary files or
		// anything like that.
		//

		goto Finished;
	}

	//
	// Try to load the user language's dictionary.
	//

	Status = MlspLoadDictionary(IdealLangId, &MlsDictionary);

	if (!NT_SUCCESS(Status)) {
		BOOLEAN LanguageWasMappedToAnother;

		//
		// User's language is not supported or the dictionary cannot be found.
		// Try to map the language to another language which may be supported.
		// The purpose of this is to select a language which we think the user
		// may understand better than English.
		//

		LanguageWasMappedToAnother = TRUE;

		switch (MlsLangId) {
		case LANG_ARMENIAN:		// 65.3% native and non-native
		case LANG_BASHKIR:		// Bashkortostan is part of Russia
		case LANG_BELARUSIAN:	// Russian is official language
		case LANG_KAZAK:		// 83.7% native and non native
		case LANG_KYRGYZ:		// 49.5% native and non native
		case LANG_TAJIK:		// 25.9% native and non native
		case LANG_TURKMEN:		// 19.3% native and non native
		case LANG_UKRAINIAN:	// 68.0% native and non native
		case LANG_YAKUT:		// Yakutia is part of Russia
			MlsLangId = LANG_RUSSIAN;
			break;
		default:
			LanguageWasMappedToAnother = FALSE;
			break;
		}

		if (LanguageWasMappedToAnother) {
			Status = MlspLoadDictionary(MlsLangId, &MlsDictionary);

			if (!NT_SUCCESS(Status)) {
				//
				// Still could not load dictionary. Fall back to English.
				//

				MlsLangId = LANG_ENGLISH;
				goto Finished;
			}
		} else {
			// Didn't map anything.
			MlsLangId = LANG_ENGLISH;
			goto Finished;
		}
	}

	//
	// The dictionary is now mapped. We will scan the dictionary and put
	// all the key-value pairs into a string mapper.
	//

	Status = SmpCreateStringMapper(&MlsStringMapper, 0);

	if (!NT_SUCCESS(Status)) {
		MlspCleanupDictionary(&MlsDictionary);
		MlsLangId = LANG_ENGLISH;
		goto Finished;
	}

	Status = MlspScanDictionary(
		&MlsDictionary,
		SmpInsertEntryStringMapper,
		MlsStringMapper);

	if (!NT_SUCCESS(Status)) {
		MlspCleanupDictionary(&MlsDictionary);
		SmpDeleteStringMapper(&MlsStringMapper);
		MlsLangId = LANG_ENGLISH;
		goto Finished;
	}

Finished:
	MlsInitialized = TRUE;
	return Status;
}

MLSAPI NTSTATUS NTAPI MlsCleanup(
	VOID)
{
	NTSTATUS Status;

	if (!MlsInitialized) {
		return STATUS_MLS_NOT_INITIALIZED;
	}

	if (MlsStringMapper) {
		Status = SmpDeleteStringMapper(&MlsStringMapper);
		ASSERT (NT_SUCCESS(Status));
	}

	Status = MlspCleanupDictionary(&MlsDictionary);
	ASSERT (NT_SUCCESS(Status));

	MlsInitialized = FALSE;
	return STATUS_SUCCESS;
}

//
// Dynamically override the preferred language (e.g. based on user dropdown box
// selection).
//
// This function accepts only language identifiers without sublanguages.
//
// Pass LANG_NEUTRAL to disable the override and revert to the default
// behavior.
//
// This function can be called before MLS is initialized.
//
// Note that MlsGetCurrentLangId does not always return the same as what this
// was set to, so you must always call MlsGetCurrentLangId to get the actual
// language.
//
MLSAPI NTSTATUS NTAPI MlsSetCurrentLangId(
	IN	LANGID	LangId)
{
	NTSTATUS Status;

	if (!MlsInitialized) {
		MlsOverrideLangId = LangId;
		return STATUS_SUCCESS;
	}

	Status = MlsCleanup();
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	MlsOverrideLangId = LangId;

	Status = MlsInitialize();
	return Status;
}

//
// Take an English string, possibly translate it to the user's language,
// and return the translated string.
//
// If an error occurs, the original English string will be directly passed
// through as the return value.
//
MLSAPI PCWSTR NTAPI MlsMapString(
	IN	PCWSTR	EnglishString)
{
	NTSTATUS Status;
	UNICODE_STRING Key;
	UNICODE_STRING Value;

	ASSERT (EnglishString != NULL);

	//
	// Parameter validation
	//

	if (!EnglishString) {
		ASSERT (("EnglishString is NULL", FALSE));
		return NULL;
	}

	//
	// Check if MLS is initialized. If not, initialize it now.
	//

	if (!MlsInitialized) {
		// Don't check the return status. If failed, the string mapper lookup
		// will simply fail and we will return the original English string.
		MlsInitialize();
	}

	//
	// Special case: English does not need to be translated.
	//

	if (MlsLangId == LANG_ENGLISH) {
		return EnglishString;
	}

	//
	// Query the string mapper for our English string.
	//

	RtlInitUnicodeString(&Key, EnglishString);

	Status = SmpLookupEntryStringMapper(MlsStringMapper, &Key, &Value);

	if (NT_SUCCESS(Status)) {
		return Value.Buffer;
	} else {
		ASSERT (Status == STATUS_STRING_MAPPER_ENTRY_NOT_FOUND);
		return EnglishString;
	}
}