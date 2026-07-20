///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     mls.c
//
// Abstract:
//
//     User interface multi-language support functions.
//
// Author:
//
//     vxiiduu (20-May-2026)
//
// Environment:
//
//     Win32 graphical incl. KexSetup
//
// Revision History:
//
//     vxiiduu               20-May-2026  Initial creation.
//     vxiiduu               30-Jun-2026  Add override font support.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <KexMls.h>

STATIC BOOLEAN MlsgpUserLanguageIsEnglish(
	VOID)
{
	STATIC BOOLEAN AlreadyInitialized = FALSE;
	STATIC BOOLEAN UserLanguageIsEnglish = FALSE;
	NTSTATUS Status;
	LANGID LangId;

	if (AlreadyInitialized) {
		return UserLanguageIsEnglish;
	}

	Status = MlsInitialize();

	if (NT_SUCCESS(Status)) {
		Status = MlsGetCurrentLangId(&LangId);

		if (NT_SUCCESS(Status) && LangId == LANG_ENGLISH) {
			UserLanguageIsEnglish = TRUE;
		}
	}

	AlreadyInitialized = TRUE;
	return UserLanguageIsEnglish;
}

//
// Some languages such as Chinese require a different font to be set. This
// is because Chinese (Simplified), Chinese (Traditional), and Japanese all
// use the same Unicode code points but some code points are rendered
// differently in different languages.
//
// Returns NULL if an override font could not be created or if one is not
// required.
// Do not call DeleteFont on the return value of this function. It is a
// cached font handle which is valid for the process lifetime.
//
STATIC HFONT MlsgpGetOverrideFont(
	VOID)
{
	NTSTATUS Status;
	LANGID LangId;
	STATIC HFONT OverrideFont = NULL;
	PCWSTR FontFace;
	ULONG CharSet;
	HDC DeviceContext;
	ULONG DpiY;
	HFONT TempFont;
	HFONT OldValue;

	if (OverrideFont) {
		return OverrideFont;
	}

	Status = MlsGetCurrentLangId(&LangId);
	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return NULL;
	}

	switch (LangId) {
	case LANG_CHINESE_TRADITIONAL:
		FontFace = L"Microsoft JhengHei";
		CharSet = CHINESEBIG5_CHARSET;
		break;
	case LANG_CHINESE_SIMPLIFIED:
		FontFace = L"Microsoft YaHei";
		CharSet = GB2312_CHARSET;
		break;
	case LANG_JAPANESE:
		FontFace = L"MS Mincho";
		CharSet = SHIFTJIS_CHARSET;
		break;
	default:
		// Non-CJK languages don't require an override font.
		return NULL;
	}

	DeviceContext = GetDC(NULL);
	DpiY = GetDeviceCaps(DeviceContext, LOGPIXELSY);
	ReleaseDC(NULL, DeviceContext);
	DeviceContext = NULL;

	TempFont = CreateFont(
		-MulDiv(8, DpiY, 72),
		0,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		CharSet,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		FontFace);

	OldValue = (HFONT) InterlockedCompareExchangePointer(
		&OverrideFont,
		TempFont,
		NULL);

	if (OldValue != NULL) {
		DeleteFont(TempFont);
	}

	return OverrideFont;
}

//
// Translate all static user-interface text inside a window or control to
// the current MLS language.
// Return TRUE if everything is OK, and FALSE if an error occurred.
//

STATIC BOOLEAN MlsgpTranslateWindowNoRecursion(
	IN	HWND	Window,
	IN	BOOLEAN	Force)
{
	BOOLEAN Success;
	WCHAR ClassName[64];
	WCHAR WindowText[512];
	ULONG WindowTextCch;
	HFONT OverrideFont;

	ASSERT (IsWindow(Window));

	Success = GetClassName(Window, ClassName, ARRAYSIZE(ClassName));
	if (!Success) {
		return FALSE;
	}

	if (Force ||
		StringEqual(ClassName, WC_BUTTON) ||
		StringEqual(ClassName, WC_STATIC)) {

		//
		// Buttons and statics - just translate the window text.
		// The Button window class includes pushbuttons, checkboxes, radio
		// buttons, and group boxes.
		// Isn't it weird that group boxes are a type of button?
		//

		WindowTextCch = GetWindowText(Window, WindowText, ARRAYSIZE(WindowText));

		if (WindowTextCch == 0) {
			ULONG LastError;

			//
			// No text in window. Cannot translate it.
			//

			LastError = GetLastError();
			ASSERT (LastError != ERROR_INSUFFICIENT_BUFFER);
			ASSERT (LastError != ERROR_BUFFER_OVERFLOW);

			if (LastError != ERROR_SUCCESS) {
				return FALSE;
			}

			return TRUE;
		}
	} else if (StringEqual(ClassName, WC_LISTVIEW) ||
			   StringEqual(ClassName, WC_COMBOBOX) ||
			   StringEqual(ClassName, WC_COMBOBOXEX)) {

		// The actual text in these controls is generally set in the source code
		// rather than in a dialog resource, but we still need to set the control's
		// font.
		goto SetFontOnly;
	} else {
		// Nothing needs to be done, because we don't want to translate
		// this kind of window. It might be e.g. an edit control.
		return TRUE;
	}

	Success = SetWindowText(Window, MlsMapString(WindowText));
	ASSERT (Success);

	//
	// Change window font if needed for the MLS language.
	//

SetFontOnly:
	OverrideFont = MlsgpGetOverrideFont();

	if (OverrideFont) {
		SetWindowFont(Window, OverrideFont, FALSE);
	}

	return Success;
}

//
// Translate a single window without recursing into its children.
//

KEXGDECLSPEC BOOLEAN KEXGAPI MlsgTranslateWindowNoRecursion(
	IN	HWND	Window)
{
	ASSERT (IsWindow(Window));

	if (MlsgpUserLanguageIsEnglish()) {
		// No need to translate English
		return TRUE;
	}

	return MlsgpTranslateWindowNoRecursion(Window, TRUE);
}

STATIC BOOL CALLBACK MlsgpEnumChildWindowsProc(
	IN	HWND	Window,
	IN	LPARAM	LParam)
{
	MlsgpTranslateWindowNoRecursion(Window, FALSE);
	return TRUE;
}

//
// Translate a top-level window and all of its children.
//

KEXGDECLSPEC BOOLEAN KEXGAPI MlsgTranslateWindow(
	IN	HWND	Window)
{
	ASSERT (IsWindow(Window));

	if (MlsgpUserLanguageIsEnglish()) {
		// No need to translate English
		return TRUE;
	}

	// Forcibly translate the application window/dialog title.
	MlsgpTranslateWindowNoRecursion(Window, TRUE);

	// Translate the child windows/controls recursively.
	EnumChildWindows(Window, MlsgpEnumChildWindowsProc, 0);
	return TRUE;
}