#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <KxCfgHlp.h>

KEXGDECLSPEC LANGID KEXGAPI GetVxKexUserInterfaceLanguage(
	VOID)
{
	HKEY VxKexUserKeyHandle;
	STATIC LANGID CachedLangId = LANG_NEUTRAL;
	LANGID LangId;

	if (CachedLangId != LANG_NEUTRAL) {
		// Already figured out the language.
		return CachedLangId;
	}

	//
	// First of all, check if user has overridden the language in the
	// HKCU registry settings. The language override exists for two purposes:
	//
	//   1. User's language setting on the computer is equivalent to neither
	//      of the VxKex supported languages, but the user understands one
	//      of the supported languages better than English.
	//
	//   2. For developers to test in other languages without changing the
	//      global system language.
	//

	VxKexUserKeyHandle = KxCfgOpenVxKexRegistryKey(TRUE, KEY_READ, NULL);
	if (VxKexUserKeyHandle) {
		ULONG ErrorCode;
		ULONG RegistryLangId;

		ErrorCode = RegReadI32(VxKexUserKeyHandle, NULL, L"LanguageId", &RegistryLangId);
		ASSERT (ErrorCode == ERROR_SUCCESS || ErrorCode == ERROR_FILE_NOT_FOUND);

		SafeClose(VxKexUserKeyHandle);

		if (ErrorCode != ERROR_SUCCESS || RegistryLangId == LANG_NEUTRAL) {
			goto NoRegistryLanguageFound;
		}

		if (RegistryLangId & ~0x3FF) {
			// invalid
			goto NoRegistryLanguageFound;
		}

		LangId = PRIMARYLANGID((WORD) RegistryLangId);
	} else {
NoRegistryLanguageFound:

		//
		// No language override. Use user or system default.
		//

		LangId = PRIMARYLANGID(GetUserDefaultUILanguage());

		if (LangId == LANG_NEUTRAL) {
			LangId = PRIMARYLANGID(GetSystemDefaultUILanguage());
		}
	}

	ASSERT (LangId != LANG_NEUTRAL);
	ASSERT ((LangId & ~0x3FF) == 0);

	switch (LangId) {
	case LANG_ENGLISH:
	case LANG_RUSSIAN:
		// languages with primary support
		return LangId;
	case LANG_UKRAINIAN:
	case LANG_BELARUSIAN:
		// mutually intelligible - most ukrainians and belarusians understand
		// russian better than english
		return LANG_RUSSIAN;
	default:
		return LANG_ENGLISH;
	}
}