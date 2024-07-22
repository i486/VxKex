///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     msgbox.c
//
// Abstract:
//
//     Message box variadic functions.
//
// Author:
//
//     vxiiduu (01-Oct-2022)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               01-Oct-2022  Initial creation.
//     vxiiduu               03-Oct-2022  Convert message boxes to task dialogs
//     vxiiduu               22-Feb-2024  Delete stub ReportAssertionFailure
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>

KEXGDECLSPEC EXTERN_C INT KEXGAPI MessageBoxV(
	IN	ULONG	Buttons OPTIONAL,
	IN	PCWSTR	Icon OPTIONAL,
	IN	PCWSTR	WindowTitle OPTIONAL,
	IN	PCWSTR	MessageTitle OPTIONAL,
	IN	PCWSTR	Format,
	IN	va_list	ArgList)
{
	INT ButtonPressed;
	HRESULT Result;
	SIZE_T BufferCch;
	PWSTR Buffer;
	TASKDIALOGCONFIG TaskDialogConfig;

	ASSERT (Format != NULL);
	
	Result = StringCchVPrintfBufferLength(&BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return 0;
	}

	Buffer = StackAlloc(WCHAR, BufferCch);

	Result = StringCchVPrintf(Buffer, BufferCch, Format, ArgList);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		return 0;
	}

	RtlZeroMemory(&TaskDialogConfig, sizeof(TaskDialogConfig));

	TaskDialogConfig.cbSize = sizeof(TaskDialogConfig);
	TaskDialogConfig.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_ALLOW_DIALOG_CANCELLATION;
	TaskDialogConfig.hwndParent = KexgApplicationMainWindow;
	TaskDialogConfig.pszWindowTitle = WindowTitle;
	TaskDialogConfig.pszMainInstruction = MessageTitle;
	TaskDialogConfig.pszContent = Buffer;
	TaskDialogConfig.dwCommonButtons = Buttons;
	TaskDialogConfig.pszMainIcon = Icon;

	Result = TaskDialogIndirect(
		&TaskDialogConfig,
		&ButtonPressed,
		NULL,
		NULL);

	ASSERT (SUCCEEDED(Result));

	return ButtonPressed;
}

KEXGDECLSPEC EXTERN_C INT KEXGAPI MessageBoxF(
	IN	ULONG	Buttons OPTIONAL,
	IN	PCWSTR	Icon OPTIONAL,
	IN	PCWSTR	WindowTitle OPTIONAL,
	IN	PCWSTR	MessageTitle OPTIONAL,
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;

	va_start(ArgList, Format);
	return MessageBoxV(Buttons, Icon, WindowTitle, MessageTitle, Format, ArgList);
}

KEXGDECLSPEC NORETURN EXTERN_C VOID KEXGAPI CriticalErrorBoxF(
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	va_start(ArgList, Format);
	MessageBoxV(0, TD_ERROR_ICON, KexgApplicationFriendlyName, NULL, Format, ArgList);
	ExitProcess(0);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI ErrorBoxF(
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	va_start(ArgList, Format);
	MessageBoxV(0, TD_ERROR_ICON, KexgApplicationFriendlyName, NULL, Format, ArgList);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI WarningBoxF(
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	va_start(ArgList, Format);
	MessageBoxV(0, TD_WARNING_ICON, KexgApplicationFriendlyName, NULL, Format, ArgList);
}

KEXGDECLSPEC EXTERN_C VOID KEXGAPI InfoBoxF(
	IN	PCWSTR	Format,
	IN	...)
{
	ARGLIST ArgList;
	va_start(ArgList, Format);
	MessageBoxV(0, TD_INFORMATION_ICON, KexgApplicationFriendlyName, NULL, Format, ArgList);
}

#ifdef ASSERTS_ENABLED
//
// Report TRUE if the caller should cause an exception,
// or FALSE otherwise.
//
KEXGDECLSPEC EXTERN_C BOOLEAN KEXGAPI ReportAssertionFailure(
	IN	PCWSTR	SourceFile,
	IN	ULONG	SourceLine,
	IN	PCWSTR	SourceFunction,
	IN	PCWSTR	AssertCondition)
{
	BOOLEAN Success;
	HRESULT Result;
	SIZE_T ExtraInfoBufferSize;
	PWSTR ExtraInfoText;
	HMODULE OriginatingModuleHandle;
	WCHAR ModuleNameBuffer[MAX_PATH];
	ULONG ModuleNameLength;
	TASKDIALOGCONFIG TaskDialogConfig;
	TASKDIALOG_BUTTON ButtonArray[3];
	INT UserSelectedButton;

	PCWSTR ExtraInfoFormat = L"The condition '%s' failed at the following location:\r\n\r\n"
							 L"%s!%s (%s, line %lu)";

	//
	// No assertions to check the input of this function.
	// It would be very ironic to enter an infinite loop with them.
	//

	//
	// Prepare the module base name to format.
	//

	Success = GetModuleHandleEx(
		GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(PCWSTR) _ReturnAddress(),
		&OriginatingModuleHandle);

	if (!Success) {
		goto CannotGetModuleName;
	}

	ModuleNameLength = GetModuleFileName(
		OriginatingModuleHandle,
		ModuleNameBuffer,
		ARRAYSIZE(ModuleNameBuffer));

	if (ModuleNameLength > 0) {
		PathStripPath(ModuleNameBuffer);
		PathRemoveExtension(ModuleNameBuffer);
		PathMakePretty(ModuleNameBuffer);
		goto GotModuleName;
	}

CannotGetModuleName:
	StringCchCopy(ModuleNameBuffer, ARRAYSIZE(ModuleNameBuffer), L"<unknown>");

GotModuleName:

	//
	// Format the text to be displayed to the user.
	// This is displayed in the footer.
	//

	Result = StringCchPrintfBufferLength(
		&ExtraInfoBufferSize,
		ExtraInfoFormat,
		AssertCondition,
		ModuleNameBuffer,
		SourceFunction,
		SourceFile,
		SourceLine);

	if (FAILED(Result)) {
		goto CannotFormatExtraInfoText;
	}

	ExtraInfoText = StackAlloc(WCHAR, ExtraInfoBufferSize);

	Result = StringCchPrintf(
		ExtraInfoText,
		ExtraInfoBufferSize,
		ExtraInfoFormat,
		AssertCondition,
		ModuleNameBuffer,
		SourceFunction,
		SourceFile,
		SourceLine);

	if (SUCCEEDED(Result)) {
		goto FormattedExtraInfoText;
	}

CannotFormatExtraInfoText:
	ExtraInfoText = L"A secondary error occurred while processing this error.";

FormattedExtraInfoText:

	//
	// Display task dialog to the user and gather his response.
	//

	ButtonArray[0].nButtonID = IDYES;
	ButtonArray[1].nButtonID = IDRETRY;
	ButtonArray[2].nButtonID = IDCLOSE;
	ButtonArray[0].pszButtonText = L"&Break";
	ButtonArray[1].pszButtonText = L"&Continue";
	ButtonArray[2].pszButtonText = L"&Quit";

	ZeroMemory(&TaskDialogConfig, sizeof(TaskDialogConfig));
	TaskDialogConfig.cbSize					= sizeof(TaskDialogConfig);
	TaskDialogConfig.hwndParent				= KexgApplicationMainWindow;
	TaskDialogConfig.dwFlags				= TDF_EXPAND_FOOTER_AREA |
											  TDF_POSITION_RELATIVE_TO_WINDOW;
	TaskDialogConfig.pszWindowTitle			= KexgApplicationFriendlyName;
	TaskDialogConfig.pszMainIcon			= TD_ERROR_ICON;
	TaskDialogConfig.pszMainInstruction		= L"Assertion failure";
	TaskDialogConfig.pszContent				= L"Select Break to enter the debugger, "
											  L"Continue to ignore the error, "
											  L"or Quit to exit the program.";
	TaskDialogConfig.cButtons				= ARRAYSIZE(ButtonArray);
	TaskDialogConfig.pButtons				= ButtonArray;
	TaskDialogConfig.nDefaultButton			= IDYES;
	TaskDialogConfig.pszExpandedInformation	= ExtraInfoText;

	Result = TaskDialogIndirect(
		&TaskDialogConfig,
		&UserSelectedButton,
		NULL,
		NULL);

	if (SUCCEEDED(Result)) {
		if (UserSelectedButton == IDCLOSE) {
			ExitProcess(0);
		} else if (UserSelectedButton == IDYES) {
			return TRUE;
		} else if (UserSelectedButton == IDRETRY) {
			return FALSE;
		}
	}

	// cause exception HERE if there was a problem
	__debugbreak();
	return FALSE;
}
#endif // ifdef ASSERTS_ENABLED