#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <InitGuid.h>
#include <OleCtl.h>
#include <WindowsX.h>
#include <stdio.h>
#include <KexComm.h>

#include "resource.h"

// {9AACA888-A5F5-4C01-852E-8A2005C1D45F}
DEFINE_GUID(CLSID_KexShlEx, 0x9aaca888, 0xa5f5, 0x4c01, 0x85, 0x2e, 0x8a, 0x20, 0x5, 0xc1, 0xd4, 0x5f);

#define CLSID_STRING_KEXSHLEX L"{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
#define APPNAME L"KexShlEx"
#define FRIENDLYAPPNAME L"VxKex Configuration Properties"
#define REPORT_BUG_URL L"https://github.com/vxiiduu/VxKex/issues/new"
#define DLLAPI EXTERN_C DECLSPEC_EXPORT

#ifdef _WIN64
#  define X64 TRUE
#  define X86 FALSE
#else
#  define X86 TRUE
#  define X64 FALSE
#endif

//
// GLOBAL VARIABLES
//

UINT g_cRefDll = 0;
HINSTANCE g_hInst = NULL;

// Misc function declarations

UINT WINAPI CallbackProc(
	IN		HWND				hWnd,
	IN		UINT				uMsg,
	IN OUT	LPPROPSHEETPAGE		lpPsp);

INT_PTR DialogProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam);

//
// COM INTERFACES
//

typedef interface CKexShlEx : public IShellExtInit, IShellPropSheetExt
{
private:
	INT m_cRef;

public:
	WCHAR m_szExe[MAX_PATH];

	CKexShlEx()
	{
		++g_cRefDll;
		m_cRef = 1;
		m_szExe[0] = '\0';
	}

	~CKexShlEx()
	{
		--g_cRefDll;
	}

	// IUnknown
	STDMETHODIMP QueryInterface(
		IN	REFIID	riid,
		OUT	PPVOID	ppv)
	{
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppv = this;
		} else if (IsEqualIID(riid, IID_IShellExtInit)) {
			*ppv = (LPSHELLEXTINIT) this;
		} else if (IsEqualIID(riid, IID_IShellPropSheetExt)) {
			*ppv = (LPSHELLPROPSHEETEXT) this;
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef(
		VOID)
	{
		return ++m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(
		VOID)
	{
		if (--m_cRef == 0) {
			delete this;
		}

		return m_cRef;
	}

	// IShellExtInit
	STDMETHODIMP Initialize(
		IN	LPCITEMIDLIST	lpidlFolder OPTIONAL,
		IN	LPDATAOBJECT	lpDataObj,
		IN	HKEY			hKeyProgID)
	{
		UINT uCount; // number of files that were selected
		FORMATETC fmt;
		STGMEDIUM med;

		if (lpDataObj == NULL) {
			return E_INVALIDARG;
		}

		// get filename of the .exe
		fmt.cfFormat	= CF_HDROP;
		fmt.ptd			= NULL;
		fmt.dwAspect	= DVASPECT_CONTENT;
		fmt.lindex		= -1;
		fmt.tymed		= TYMED_HGLOBAL;

		if (SUCCEEDED(lpDataObj->GetData(&fmt, &med))) {
			HDROP hDrop = (HDROP) med.hGlobal;
			uCount = DragQueryFile(hDrop, (UINT) -1, NULL, 0);

			// if the user selected more than one exe, don't display the property sheet
			if (uCount != 1) {
				ReleaseStgMedium(&med);
				return E_NOTIMPL;
			}

			DragQueryFile(hDrop, 0, m_szExe, ARRAYSIZE(m_szExe));
			ReleaseStgMedium(&med);
		} else {
			return E_FAIL;
		}

		// full path of the .exe file is now stored in m_szExe
		return S_OK;
	}

	// IShellPropSheetExt
	STDMETHODIMP AddPages(
		IN	LPFNADDPROPSHEETPAGE	lpfnAddPage,
		IN	LPARAM					lParam)
	{
		PROPSHEETPAGE psp;
		HPROPSHEETPAGE hPsp;

		if (m_szExe[0] == '\0') {
			return E_UNEXPECTED;
		}

		ZeroMemory(&psp, sizeof(psp));
		psp.dwSize		= sizeof(psp);
		psp.dwFlags		= PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
		psp.hInstance	= g_hInst;
		psp.pszTemplate	= MAKEINTRESOURCE(IDD_DIALOG1);
		psp.hIcon		= NULL;
		psp.pszTitle	= L"VxKex";
		psp.pfnDlgProc	= (DLGPROC) DialogProc;
		psp.pcRefParent	= &g_cRefDll;
		psp.pfnCallback	= CallbackProc;
		psp.lParam		= (LPARAM) this;

		hPsp = CreatePropertySheetPage(&psp);

		if (hPsp) {
			if (lpfnAddPage(hPsp, lParam)) {
				AddRef();
				return S_OK;
			} else {
				DestroyPropertySheetPage(hPsp);
				return E_FAIL;
			}
		} else {
			return E_OUTOFMEMORY;
		}
	}

	STDMETHODIMP ReplacePage(
		IN	UINT					uPageID,
		IN	LPFNADDPROPSHEETPAGE	lpfnReplaceWith,
		IN	LPARAM					lParam)
	{
		return E_NOTIMPL;
	}
} CKEXSHLEX, *PCKEXSHLEX, *LPCKEXSHLEX;

typedef interface CClassFactory : public IClassFactory
{
protected:
	INT m_cRef;

public:
	CClassFactory()
	{
		++g_cRefDll;
		m_cRef = 1;
	}

	~CClassFactory()
	{
		--g_cRefDll;
	}

	// IUnknown
	STDMETHODIMP QueryInterface(
		IN	REFIID	riid,
		OUT	PPVOID	ppv)
	{
		if (IsEqualIID(riid, IID_IUnknown)) {
			*ppv = this;
		} else if (IsEqualIID(riid, IID_IClassFactory)) {
			*ppv = (LPCLASSFACTORY) this;
		} else {
			*ppv = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef(
		VOID)
	{
		return ++m_cRef;
	}
	
	STDMETHODIMP_(ULONG) Release(
		VOID)
	{
		if (--m_cRef == 0) {
			delete this;
		}
		
		return m_cRef;
	}

	// IClassFactory
	STDMETHODIMP CreateInstance(
		IN	LPUNKNOWN	pUnk,
		IN	REFIID		riid,
		OUT	PPVOID		ppv)
	{
		HRESULT hr;
		LPCKEXSHLEX lpKexShlEx;

		*ppv = NULL;

		if (pUnk) {
			return CLASS_E_NOAGGREGATION;
		}

		if ((lpKexShlEx = new CKexShlEx) == NULL) {
			return E_OUTOFMEMORY;
		}

		hr = lpKexShlEx->QueryInterface(riid, ppv);
		lpKexShlEx->Release();
		return hr;
	}

	STDMETHODIMP LockServer(
		BOOL)
	{
		return E_NOTIMPL;
	}
} CCLASSFACTORY, *PCCLASSFACTORY, *LPCCLASSFACTORY;

//
// DIALOG FUNCTIONS
//

UINT WINAPI CallbackProc(
	IN		HWND				hWnd,
	IN		UINT				uMsg,
	IN OUT	LPPROPSHEETPAGE		lpPsp)
{
	if (uMsg == PSPCB_RELEASE) {
	}

	return TRUE;
}

INT_PTR DialogProc(
	IN	HWND	hWnd,
	IN	UINT	uMsg,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	static BOOL bSettingsChanged = FALSE;

	if (uMsg == WM_INITDIALOG) {
		DWORD dwEnableVxKex = FALSE;
		WCHAR szWinVerSpoof[6];
		DWORD dwAlwaysShowDebug = FALSE;
		DWORD dwDisableForChild = FALSE;
		DWORD dwDisableAppSpecific = FALSE;
		DWORD dwWaitForChild = FALSE;
		WCHAR szVxKexLdrPath[MAX_PATH];
		WCHAR szIfeoDebugger[MAX_PATH + 2];
		WCHAR szIfeoKey[74 + MAX_PATH] = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\";
		WCHAR szKexIfeoKey[54 + MAX_PATH] = L"SOFTWARE\\VXsoft\\VxKexLdr\\Image File Execution Options\\";
		LPCWSTR lpszExePath = ((LPCKEXSHLEX) (((LPPROPSHEETPAGE) lParam)->lParam))->m_szExe;
		WCHAR szExePath[MAX_PATH];
		WCHAR szExeName[MAX_PATH];
		CONST HWND hWndWinVer = GetDlgItem(hWnd, IDWINVERCOMBOBOX);

		// store the pointer to the name of the .EXE in the userdata of the dialog
		// for convenience purposes. These casts are confusing and horrible, but
		// it works so don't mess with it. :^)
		wcscpy_s(szExePath, ARRAYSIZE(szExePath), lpszExePath);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) lpszExePath);

		// populate and set up the Windows version selector
		ComboBox_AddString(hWndWinVer, L"Windows 8");
		ComboBox_AddString(hWndWinVer, L"Windows 8.1");
		ComboBox_AddString(hWndWinVer, L"Windows 10");
		ComboBox_AddString(hWndWinVer, L"Windows 11");
		ComboBox_SetItemData(hWndWinVer, 0, L"WIN8");
		ComboBox_SetItemData(hWndWinVer, 1, L"WIN81");
		ComboBox_SetItemData(hWndWinVer, 2, L"WIN10");
		ComboBox_SetItemData(hWndWinVer, 3, L"WIN11");
		ComboBox_SetCurSel(hWndWinVer, 2); // set default selection to Windows 10

		wcscat_s(szKexIfeoKey, ARRAYSIZE(szKexIfeoKey), szExePath);
		wcscpy_s(szExeName, ARRAYSIZE(szExeName), szExePath);
		PathStripPath(szExeName);
		wcscat_s(szIfeoKey, ARRAYSIZE(szIfeoKey), szExeName);
		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath)));
		wcscat_s(szVxKexLdrPath, ARRAYSIZE(szVxKexLdrPath), L"\\VxKexLdr.exe");

		if (RegReadSz(HKEY_LOCAL_MACHINE, szIfeoKey, L"Debugger", szIfeoDebugger, ARRAYSIZE(szIfeoDebugger)) &&
			(!wcsicmp(szIfeoDebugger, szVxKexLdrPath) || !wcsnicmp(szIfeoDebugger+1, szVxKexLdrPath, wcslen(szVxKexLdrPath))) &&
			RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, L"EnableVxKex", &dwEnableVxKex) && dwEnableVxKex) {
			CheckDlgButton(hWnd, IDUSEVXKEX, TRUE);
		}

		if (RegReadSz(HKEY_CURRENT_USER, szKexIfeoKey, L"WinVerSpoof", szWinVerSpoof, ARRAYSIZE(szWinVerSpoof))) {
			if (lstrcmpi(szWinVerSpoof, L"NONE")) {
				CheckDlgButton(hWnd, IDSPOOFVERSIONCHECK, TRUE);
				ComboBox_Enable(hWndWinVer, TRUE);
				
				if (!lstrcmpi(szWinVerSpoof, L"WIN8")) {
					ComboBox_SetCurSel(hWndWinVer, 0);
				} else if (!lstrcmpi(szWinVerSpoof, L"WIN81")) {
					ComboBox_SetCurSel(hWndWinVer, 1);
				} else if (!lstrcmpi(szWinVerSpoof, L"WIN10")) {
					ComboBox_SetCurSel(hWndWinVer, 2);
				} else if (!lstrcmpi(szWinVerSpoof, L"WIN11")) {
					ComboBox_SetCurSel(hWndWinVer, 3);
				}
			}
		}

		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, L"AlwaysShowDebug", &dwAlwaysShowDebug);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, L"DisableForChild", &dwDisableForChild);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, L"DisableAppSpecific", &dwDisableAppSpecific);
		RegReadDw(HKEY_CURRENT_USER, szKexIfeoKey, L"WaitForChild", &dwWaitForChild);

		CheckDlgButton(hWnd, IDALWAYSSHOWDEBUG, dwAlwaysShowDebug);
		CheckDlgButton(hWnd, IDDISABLEFORCHILD, dwDisableForChild);
		CheckDlgButton(hWnd, IDDISABLEAPPSPECIFIC, dwDisableAppSpecific);
		CheckDlgButton(hWnd, IDWAITONCHILD, dwWaitForChild);

		PathRemoveFileSpec(szVxKexLdrPath);

		if (PathIsPrefix(szVxKexLdrPath, szExePath)) {
			// Discourage the user from enabling VxKex for VxKex executables, since they certainly
			// don't need it and it could cause major problems (especially for VxKexLdr.exe itself)
			EnableWindow(GetDlgItem(hWnd, IDUSEVXKEX), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDSPOOFVERSIONCHECK), FALSE);
			EnableWindow(hWndWinVer, FALSE);
			EnableWindow(GetDlgItem(hWnd, IDALWAYSSHOWDEBUG), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDDISABLEFORCHILD), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDDISABLEAPPSPECIFIC), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDWAITONCHILD), FALSE);
		}

		CreateToolTip(hWnd, IDUSEVXKEX, L"Enable or disable the main VxKex compatibility layer.");
		CreateToolTip(hWnd, IDSPOOFVERSIONCHECK, L"Some applications check the Windows version and refuse to run if it is incorrect. "
												 L"This option can help these applications to run correctly.");
		CreateToolTip(hWnd, IDALWAYSSHOWDEBUG, L"Creates a console window and displays some additional "
											   L"information which may be useful for troubleshooting.");
		CreateToolTip(hWnd, IDDISABLEFORCHILD, L"By default, if this program launches another program, VxKex will be enabled "
											   L"for the second program and all subsequent programs which are launched. This option "
											   L"disables such behavior.");
		CreateToolTip(hWnd, IDDISABLEAPPSPECIFIC, L"Disable application-specific code. This is mainly useful for VxKex developers. "
												  L"Usually, enabling this option will degrade application compatibility.");
		CreateToolTip(hWnd, IDWAITONCHILD, L"Wait for the child process to exit before exiting the loader. This is useful in combination "
										   L"with the 'Always show debugging information' option which lets you view debug strings. It "
										   L"may also be useful to support programs that wait for child processes to exit.");
		CreateToolTip(hWnd, IDREPORTBUG, REPORT_BUG_URL);

		SetFocus(GetDlgItem(hWnd, IDUSEVXKEX));
		return TRUE;
	} else if (uMsg == WM_COMMAND) {
		if (LOWORD(wParam) == IDSPOOFVERSIONCHECK) {
			EnableWindow(GetDlgItem(hWnd, IDWINVERCOMBOBOX), !!IsDlgButtonChecked(hWnd, IDSPOOFVERSIONCHECK));
		}
		
		// enable the "Apply" button if it's not already enabled
		PropSheet_Changed(GetParent(hWnd), hWnd);
		bSettingsChanged = TRUE;

		return TRUE;
	} else if (uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == PSN_APPLY && bSettingsChanged) {
		// OK or Apply button was clicked and we need to apply new settings

		HWND hWndWinVer = GetDlgItem(hWnd, IDWINVERCOMBOBOX);
		STARTUPINFO startupInfo;
		PROCESS_INFORMATION procInfo;
		LPCWSTR lpszExeFullPath = (LPCWSTR) GetWindowLongPtr(hWnd, GWLP_USERDATA);
		WCHAR szKexCfgFullPath[MAX_PATH];
		WCHAR szKexCfgCmdLine[542];

		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexCfgFullPath, ARRAYSIZE(szKexCfgFullPath)));
		wcscat_s(szKexCfgFullPath, MAX_PATH, L"\\KexCfg.exe");

		swprintf_s(szKexCfgCmdLine, ARRAYSIZE(szKexCfgCmdLine), L"\"%s\" \"%s\" %d \"%s\" %d %d %d %d",
				  szKexCfgFullPath, lpszExeFullPath,
				  !!IsDlgButtonChecked(hWnd, IDUSEVXKEX),
				  !!IsDlgButtonChecked(hWnd, IDSPOOFVERSIONCHECK) ? (LPCWSTR) ComboBox_GetItemData(hWndWinVer, ComboBox_GetCurSel(hWndWinVer))
																  : L"NONE",
				  !!IsDlgButtonChecked(hWnd, IDALWAYSSHOWDEBUG),
				  !!IsDlgButtonChecked(hWnd, IDDISABLEFORCHILD),
				  !!IsDlgButtonChecked(hWnd, IDDISABLEAPPSPECIFIC),
				  !!IsDlgButtonChecked(hWnd, IDWAITONCHILD));
		GetStartupInfo(&startupInfo);

		if (CreateProcess(szKexCfgFullPath, szKexCfgCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &startupInfo, &procInfo) == FALSE) {
			ErrorBoxF(L"Failed to start KexCfg helper process. Error code: %#010I32x: %s",
							  GetLastError(), GetLastErrorAsString());
			goto Error;
		}

		CloseHandle(procInfo.hThread);
		CloseHandle(procInfo.hProcess);

		bSettingsChanged = FALSE;
		return TRUE;
	} else if ((uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == NM_CLICK ||
			    uMsg == WM_NOTIFY && ((LPNMHDR) lParam)->code == NM_RETURN) &&
			   wParam == IDREPORTBUG) {
		// user wants to report a bug on the github
		ShellExecute(hWnd, L"open", REPORT_BUG_URL, NULL, NULL, SW_NORMAL);

		return TRUE;
	}

Error:
	return FALSE;
}

//
// DLL EXPORTED FUNCTIONS
//

DLLAPI BOOL WINAPI DllMain(
	IN	HINSTANCE	hInstance,
	IN	DWORD		dwReason,
	IN	LPVOID		lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		g_hInst = hInstance;
		DisableThreadLibraryCalls(hInstance);
		SetFriendlyAppName(FRIENDLYAPPNAME);
	}

	return TRUE;
}

STDAPI DllCanUnloadNow(
	VOID)
{
	return ((g_cRefDll == 0) ? S_OK : S_FALSE);
}

STDAPI DllGetClassObject(
	IN	REFCLSID	rclsid,
	IN	REFIID		riid,
	OUT	PPVOID		ppv)
{
	*ppv = NULL;

	if (IsEqualIID(rclsid, CLSID_KexShlEx)) {
		HRESULT hr;
		LPCCLASSFACTORY lpCClassFactory = new CClassFactory;

		if (lpCClassFactory == NULL) {
			return E_OUTOFMEMORY;
		}

		hr = lpCClassFactory->QueryInterface(riid, ppv);
		lpCClassFactory->Release();
		return hr;
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

// NEVER REMOVE THE DELETION OF ANY REGISTRY ENTRIES HERE! Even if a newer version
// of the software stops creating a particular registry key, it may still be left over
// from a previous version.
STDAPI DllUnregisterServer(
	VOID)
{
	SHDeleteKey(HKEY_CLASSES_ROOT, L"CLSID\\" CLSID_STRING_KEXSHLEX);

	if (X64) {
		SHDeleteKey(HKEY_CLASSES_ROOT, L"Wow6432Node\\CLSID\\" CLSID_STRING_KEXSHLEX);
	}

	return S_OK;
}

// IF YOU ADD A REGISTRY ENTRY HERE, YOU MUST ALSO UPDATE DllUnregisterServer TO
// REMOVE THAT REGISTRY ENTRY! YOU MUST ALSO UPDATE THE FOLLOWING TREE DIAGRAM:
//
// HKEY_CLASSES_ROOT
//   CLSID
//     {9AACA888-A5F5-4C01-852E-8A2005C1D45F} (*)
//       InProcServer32
//         (Default)		= REG_SZ "<VxKexLdr installation directory>\KexShlEx.dll"
//         ThreadingModel	= REG_SZ "Apartment"
//
// A (*) next to a key indicates that this key is "owned" by VxKexLdr and may be
// safely deleted, along with all its subkeys, when the program is uninstalled.
//
// The above tree diagram must also be replicated into the Wow6432Node (but only
// if running on a 64-bits system). For 32-bits, the name of KexShlEx.dll changes
// to KexShl32.dll.
//
// The value of <VxKexLdr installation directory> can be found from the registry key
// HKEY_LOCAL_MACHINE\Software\VXsoft\VxKexLdr\KexDir (REG_SZ). This registry key is
// created by the VxKexLdr installer, and its absence indicates an incorrect usage of
// the DllRegisterServer function.
STDAPI DllRegisterServer(
	VOID)
{
	BOOL bSuccess;
	WCHAR szKexShlExDll32[MAX_PATH];
	WCHAR szKexShlExDll64[MAX_PATH];
	LPCWSTR szKexShlExDllNative = NULL;

	if (X64) {
		szKexShlExDllNative = szKexShlExDll64;
	} else if (X86) {
		szKexShlExDllNative = szKexShlExDll32;
	}

	bSuccess = RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szKexShlExDll32, ARRAYSIZE(szKexShlExDll32));
	if (bSuccess == FALSE) return E_UNEXPECTED;

	wcscpy_s(szKexShlExDll64, MAX_PATH, szKexShlExDll32);
	wcscat_s(szKexShlExDll32, MAX_PATH, L"\\KexShl32.dll");
	wcscat_s(szKexShlExDll64, MAX_PATH, L"\\KexShlEx.dll");

	CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32", NULL, szKexShlExDllNative));
	CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32", L"ThreadingModel", L"Apartment"));

	if (X64) {
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"Wow6432Node\\CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32", NULL, szKexShlExDll32));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"Wow6432Node\\CLSID\\" CLSID_STRING_KEXSHLEX L"\\InProcServer32", L"ThreadingModel", L"Apartment"));
	}

	return S_OK;

Error:
	DllUnregisterServer();
	return SELFREG_E_CLASS;
}

// HKEY_CLASSES_ROOT
//   exefile
//     shellex
//       PropertySheetHandlers
//         KexShlEx Property Page (*)
//           (Default)	= REG_SZ "{9AACA888-A5F5-4C01-852E-8A2005C1D45F}"
STDAPI DllInstall(
	IN	BOOL	bInstall,
	IN	LPCWSTR	lpszCmdLine OPTIONAL)
{
	if (bInstall) {
		WCHAR szVxKexLdr[MAX_PATH];
		WCHAR szOpenVxKexCommand[MAX_PATH + 17];

		CHECKED(DllRegisterServer() == S_OK);
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page", NULL, CLSID_STRING_KEXSHLEX));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"exefile\\shell\\open_vxkex", NULL, L"Run with VxKex enabled"));
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"exefile\\shell\\open_vxkex", L"Extended", L""));
		CHECKED(RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr", L"KexDir", szVxKexLdr, ARRAYSIZE(szVxKexLdr)));
		CHECKED(!wcscat_s(szVxKexLdr, ARRAYSIZE(szVxKexLdr), L"\\VxKexLdr.exe"));
		CHECKED(swprintf_s(szOpenVxKexCommand, ARRAYSIZE(szOpenVxKexCommand), L"\"%s\" /FORCE \"%%1\"", szVxKexLdr) != -1);
		CHECKED(RegWriteSz(HKEY_CLASSES_ROOT, L"exefile\\shell\\open_vxkex\\command", NULL, szOpenVxKexCommand));
	} else {
		// Important note: You cannot "goto Error" from this section, so no CHECKED(x)
		// is permitted here. Otherwise it will cause recursion in the case of an error.

		// We use SHDeleteKeyA in preference to RegDeleteTree because it basically does
		// the same thing but is available since Win2k (instead of only since Vista)
		SHDeleteKey(HKEY_CLASSES_ROOT, L"exefile\\shellex\\PropertySheetHandlers\\KexShlEx Property Page");
		SHDeleteKey(HKEY_CLASSES_ROOT, L"exefile\\shell\\open_vxkex");
		DllUnregisterServer();
	}

	return S_OK;

Error:
	DllInstall(FALSE, NULL);
	return E_FAIL;
}