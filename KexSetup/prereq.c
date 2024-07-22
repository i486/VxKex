#include "buildcfg.h"
#include "kexsetup.h"
#include <InitGuid.h>

DEFINE_GUID(IID_IDXGIFactory2, 0x50C83A1C, 0xE072, 0x4C48, 0x87, 0xB0, 0x36, 0x30, 0xFA, 0x36, 0xA6, 0xD0);

//
// Check for the presence of the following three APIs inside kernel32.dll:
//
//   - AddDllDirectory
//   - RemoveDllDirectory
//   - SetDefaultDllDirectories
//
STATIC BOOLEAN IsDllDirectoriesUpdatePresent(
	VOID)
{
	HMODULE Kernel32;
	ULONG Index;
	PCSTR ApiNames[] = {
		"AddDllDirectory",
		"RemoveDllDirectory",
		"SetDefaultDllDirectories"
	};

	Kernel32 = GetModuleHandle(L"kernel32.dll");
	ASSERT (Kernel32 != NULL);

	if (!Kernel32) {
		return FALSE;
	}

	for (Index = 0; Index < ARRAYSIZE(ApiNames); ++Index) {
		if (GetProcAddress(Kernel32, ApiNames[Index]) == NULL) {
			return FALSE;
		}
	}

	return TRUE;
}

//
// Check for the ability of CreateDXGIFactory1 to support the IDXGIFactory2
// interface.
//
STATIC BOOLEAN IsPlatformUpdatePresent(
	VOID)
{
	HRESULT Result;
	HMODULE Dxgi;
	HRESULT (WINAPI *pCreateDXGIFactory1) (REFIID, IUnknown **);
	IUnknown *DXGIFactory2;

	DXGIFactory2 = NULL;

	Dxgi = LoadLibrary(L"dxgi.dll");
	ASSERT (Dxgi != NULL);

	if (!Dxgi) {
		return FALSE;
	}

	pCreateDXGIFactory1 = (HRESULT (WINAPI *) (REFIID, IUnknown **)) GetProcAddress(
		Dxgi,
		"CreateDXGIFactory1");

	ASSERT (pCreateDXGIFactory1 != NULL);

	if (!pCreateDXGIFactory1) {
		FreeLibrary(Dxgi);
		return FALSE;
	}

	Result = pCreateDXGIFactory1(&IID_IDXGIFactory2, &DXGIFactory2);
	if (FAILED(Result) || !DXGIFactory2) {
		FreeLibrary(Dxgi);
		return FALSE;
	}

	SafeRelease(DXGIFactory2);
	return TRUE;
}

BOOLEAN IsServicePack1Present(
	VOID)
{
	return (NtCurrentPeb()->OSBuildNumber > 7600);
}

VOID KexSetupCheckForPrerequisites(
	VOID)
{
	ULONG ErrorCode;
	BOOLEAN ServicePack1Present;
	BOOLEAN DllDirectoriesUpdatePresent;
	BOOLEAN PlatformUpdatePresent;
	BOOL DontShowAgain;
	ULONG DataCb;

	ServicePack1Present = FALSE;
	DllDirectoriesUpdatePresent = FALSE;
	PlatformUpdatePresent = FALSE;

	//
	// If the user has previously indicated that he doesn't want to see this
	// warning, then don't check anything.
	//

	DataCb = sizeof(DontShowAgain);

	ErrorCode = RegGetValue(
		HKEY_CURRENT_USER,
		L"Software\\VXsoft\\VxKex",
		L"SetupPrerequisitesDontShowAgain",
		RRF_RT_REG_DWORD,
		NULL,
		&DontShowAgain,
		&DataCb);

	if (ErrorCode == ERROR_SUCCESS && DontShowAgain) {
		return;
	}

	//
	// Check for Service Pack 1.
	//

	ServicePack1Present = IsServicePack1Present();

	//
	// Check for KB2533623 (DllDirectories update) and KB2670838
	// (Platform Update).
	//

	DllDirectoriesUpdatePresent = IsDllDirectoriesUpdatePresent();
	PlatformUpdatePresent = IsPlatformUpdatePresent();

	//
	// If any of the prerequisites were not found, inform the user and ask
	// him whether he wants to cancel setup.
	//

	unless (ServicePack1Present && DllDirectoriesUpdatePresent && PlatformUpdatePresent) {
		HRESULT Result;
		WCHAR MainText[1024];
		TASKDIALOGCONFIG TaskDialogConfig;
		TASKDIALOG_BUTTON Buttons[] = {
			{IDCANCEL, L"Cancel installation"},
			{IDOK, L"Continue installation anyway\n"
				   L"Without the prerequisites listed above, be aware that some "
				   L"applications will not work, even with VxKex."},
		};

		INT UserSelection;

		StringCchCopy(
			MainText,
			ARRAYSIZE(MainText),
			L"Setup has detected that the following prerequisites were not installed on "
			L"your computer:\r\n");

		if (!ServicePack1Present) {
			StringCchCat(
				MainText,
				ARRAYSIZE(MainText),
				L"\r\n    • Service Pack 1 (SP1) for Windows® 7");
		}

		if (!DllDirectoriesUpdatePresent) {
			StringCchCat(
				MainText,
				ARRAYSIZE(MainText),
				L"\r\n    • Update KB2533623 (DllDirectories update)");
		}

		if (!PlatformUpdatePresent) {
			StringCchCat(
				MainText,
				ARRAYSIZE(MainText),
				L"\r\n    • Update KB2670838 (Platform Update)");
		}

		RtlZeroMemory(&TaskDialogConfig, sizeof(TaskDialogConfig));
		TaskDialogConfig.cbSize				= sizeof(TaskDialogConfig);
		TaskDialogConfig.dwFlags			= TDF_ALLOW_DIALOG_CANCELLATION |
											  TDF_USE_COMMAND_LINKS |
											  TDF_POSITION_RELATIVE_TO_WINDOW;
		TaskDialogConfig.pszWindowTitle		= FRIENDLYAPPNAME;
		TaskDialogConfig.pszMainIcon		= TD_WARNING_ICON;
		TaskDialogConfig.pszMainInstruction	= L"System requirements not met";
		TaskDialogConfig.pszContent			= MainText;
		TaskDialogConfig.cButtons			= ARRAYSIZE(Buttons);
		TaskDialogConfig.pButtons			= Buttons;
		TaskDialogConfig.nDefaultButton		= IDOK;
		TaskDialogConfig.pszVerificationText= L"Don't show this warning again";

		Result = TaskDialogIndirect(
			&TaskDialogConfig,
			&UserSelection,
			NULL,
			&DontShowAgain);

		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			return;
		}

		if (UserSelection == IDCANCEL) {
			// User cancelled the installation.
			ExitProcess(0);
		}

		if (DontShowAgain) {
			ErrorCode = RegSetKeyValue(
				HKEY_CURRENT_USER,
				L"Software\\VXsoft\\VxKex",
				L"SetupPrerequisitesDontShowAgain",
				REG_DWORD,
				&DontShowAgain,
				sizeof(DontShowAgain));

			ASSERT (ErrorCode == ERROR_SUCCESS);
		}
	}
}