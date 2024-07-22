///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ckxshlex.c
//
// Abstract:
//
//     Implements the IKexShlEx (i.e. IShellExtInit and IShellPropSheetExt)
//     interfaces.
//
//     Ignore the standard COM crap at the top of the file, the functions of
//     interest are:
//
//       CKexShlEx_Initialize
//       CKexShlEx_AddPages
//
// Author:
//
//     vxiiduu (08-Feb-2024)
//
// Environment:
//
//     Inside explorer.exe
//
// Revision History:
//
//     vxiiduu              08-Feb-2024  Initial creation.
//     vxiiduu              14-Mar-2024  Fix a multiselect-related bug.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "KexShlEx.h"
#include "resource.h"

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface(
	IN	IKexShlEx		*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (ObjectOut != NULL);

	if (IsEqualIID(RefIID, &IID_IUnknown)) {
		*ObjectOut = This;
	} else if (IsEqualIID(RefIID, &IID_IShellExtInit)) {
		*ObjectOut = &This->ShellExtInitVtbl;
	} else if (IsEqualIID(RefIID, &IID_IShellPropSheetExt)) {
		*ObjectOut = &This->ShellPropSheetExtVtbl;
	} else {
		*ObjectOut = NULL;
		return E_NOINTERFACE;
	}

	InterlockedIncrement(&This->RefCount);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface_thunk0(
	IN	IShellExtInit	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut)
{
	return CKexShlEx_QueryInterface((IKexShlEx *) This, RefIID, ObjectOut);
}

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface_thunk1(
	IN	IShellPropSheetExt	*This,
	IN	REFIID				RefIID,
	OUT	PPVOID				ObjectOut)
{
	return CKexShlEx_QueryInterface((IKexShlEx *) (((ULONG_PTR) This) - sizeof(PVOID)), RefIID, ObjectOut);
}

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef(
	IN	IKexShlEx	*This)
{
	return InterlockedIncrement(&This->RefCount);
}

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef_thunk0(
	IN	IShellExtInit	*This)
{
	return CKexShlEx_AddRef((IKexShlEx *) This);
}

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef_thunk1(
	IN	IShellPropSheetExt	*This)
{
	return CKexShlEx_AddRef((IKexShlEx *) (((ULONG_PTR) This) - sizeof(PVOID)));
}

ULONG STDMETHODCALLTYPE CKexShlEx_Release(
	IN	IKexShlEx		*This)
{
	LONG NewRefCount;

	NewRefCount = InterlockedDecrement(&NewRefCount);

	if (NewRefCount == 0) {
		InterlockedDecrement(&DllReferenceCount);
		SafeFree(This);
		return 0;
	}

	return This->RefCount;
}

ULONG STDMETHODCALLTYPE CKexShlEx_Release_thunk0(
	IN	IShellExtInit	*This)
{
	return CKexShlEx_Release((IKexShlEx *) This);
}

ULONG STDMETHODCALLTYPE CKexShlEx_Release_thunk1(
	IN	IShellPropSheetExt	*This)
{
	return CKexShlEx_Release((IKexShlEx *) (((ULONG_PTR) This) - sizeof(PVOID)));
}

HRESULT STDMETHODCALLTYPE CKexShlEx_Initialize(
	IN	IKexShlEx		*This,
	IN	LPCITEMIDLIST	FolderIdList OPTIONAL,
	IN	LPDATAOBJECT	DataObject,
	IN	HKEY			ProgIdKey)
{
	HRESULT Result;
	ULONG NumberOfFilesSelected;
	FORMATETC Format;
	STGMEDIUM StorageMedium;
	HDROP Drop;
	PCWSTR FileExtension;

	ASSERT (This != NULL);
	ASSERT (FolderIdList == NULL);
	ASSERT (DataObject != NULL);

	Format.cfFormat			= CF_HDROP;
	Format.ptd				= NULL;
	Format.dwAspect			= DVASPECT_CONTENT;
	Format.lindex			= -1;
	Format.tymed			= TYMED_HGLOBAL;

	Result = DataObject->lpVtbl->GetData(
		DataObject,
		&Format,
		&StorageMedium);

	if (FAILED(Result)) {
		return Result;
	}

	Drop = (HDROP) StorageMedium.hGlobal;
	NumberOfFilesSelected = DragQueryFile(Drop, -1, NULL, 0);

	if (NumberOfFilesSelected != 1) {
		// If the user selects multiple files before opening properties,
		// we don't want to display any property sheet page.
		ReleaseStgMedium(&StorageMedium);
		return E_FAIL;
	}

	DragQueryFile(Drop, 0, This->ExeFullPath, ARRAYSIZE(This->ExeFullPath));
	ReleaseStgMedium(&StorageMedium);

	//
	// The full path to the target file is now stored in This->ExeFullPath.
	//

	Result = PathCchFindExtension(
		This->ExeFullPath,
		ARRAYSIZE(This->ExeFullPath),
		&FileExtension);

	if (FAILED(Result)) {
		return Result;
	}

	if (StringEqualI(FileExtension, L".LNK")) {
		//
		// Get the path to the file that the .LNK points to.
		//

		Result = GetTargetFromLnkfile(This->ExeFullPath);
		if (FAILED(Result)) {
			return Result;
		}

		//
		// Determine the file extension for the new target file.
		//

		Result = PathCchFindExtension(
			This->ExeFullPath,
			ARRAYSIZE(This->ExeFullPath),
			&FileExtension);

		if (FAILED(Result)) {
			return Result;
		}

		//
		// If it's not .EXE or .MSI, we won't display a property page.
		//

		if (!StringEqualI(FileExtension, L".EXE") &&
			!StringEqualI(FileExtension, L".MSI")) {

			return E_NOTIMPL;
		}
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CKexShlEx_AddPages(
	IN	IKexShlEx				*This,
	IN	LPFNADDPROPSHEETPAGE	AddPage,
	IN	LPARAM					LParam)
{
	BOOLEAN Success;
	PROPSHEETPAGE Page;
	HPROPSHEETPAGE PageHandle;
	WCHAR WinDir[MAX_PATH];
	WCHAR KexDir[MAX_PATH];

	ASSERT (This != NULL);
	ASSERT (AddPage != NULL);

	// adjustor
	This = (IKexShlEx *) (((ULONG_PTR) This) - sizeof(PVOID));

	ASSERT (This != NULL);
	ASSERT (This->ExeFullPath[0] != '\0');

	GetWindowsDirectory(WinDir, ARRAYSIZE(WinDir));
	Success = KxCfgGetKexDir(KexDir, ARRAYSIZE(KexDir));
	ASSERT (Success);

	if (!Success) {
		return E_FAIL;
	}
		
	//
	// If the executable is inside the Windows directory, or if it is inside
	// KexDir, then we will not allow the user to change any VxKex settings
	// for it. Windows executables or VxKex executables do not benefit from
	// enabling VxKex and doing so could cause serious problems.
	//

	if (PathIsPrefix(WinDir, This->ExeFullPath) || PathIsPrefix(KexDir, This->ExeFullPath)) {
		return E_FAIL;
	}

	ZeroMemory(&Page, sizeof(Page));
	Page.dwSize				= sizeof(Page);
	Page.dwFlags			= PSP_USEREFPARENT | PSP_USETITLE | PSP_USECALLBACK;
	Page.hInstance			= DllHandle;
	Page.pszTemplate		= MAKEINTRESOURCE(IDD_VXKEXPROPSHEETPAGE);
	Page.pszTitle			= L"VxKex";
	Page.pfnDlgProc			= DialogProc;
	Page.pfnCallback		= PropSheetCallbackProc;
	Page.pcRefParent		= (PUINT) &DllReferenceCount;
	Page.lParam				= (LPARAM) This;

	PageHandle = CreatePropertySheetPage(&Page);
	if (!PageHandle) {
		return E_OUTOFMEMORY;
	}

	Success = AddPage(PageHandle, LParam);
	if (!Success) {
		DestroyPropertySheetPage(PageHandle);
		return E_FAIL;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CKexShlEx_ReplacePage(
	IN	IKexShlEx				*This,
	IN	UINT					PageId,
	IN	LPFNADDPROPSHEETPAGE	ReplacePage,
	IN	LPARAM					LParam)
{
	return E_NOTIMPL;
}