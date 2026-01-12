///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     dllmain.c
//
// Abstract:
//
//     Main file for the shell extension.
//
//     To be honest, this file is mostly just full of COM boilerplate slop,
//     and you don't need to look in here unless there's a bug. The other files
//     in this project might contain more interesting code.
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
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "KexShlEx.h"
#include "resource.h"

#include <InitGuid.h>
// {9AACA888-A5F5-4C01-852E-8A2005C1D45F}
DEFINE_GUID(CLSID_KexShlEx, 0x9aaca888, 0xa5f5, 0x4c01, 0x85, 0x2e, 0x8a, 0x20, 0x5, 0xc1, 0xd4, 0x5f);

HMODULE DllHandle = NULL;
LONG DllReferenceCount = 0;

//
// CKexShlEx
//

IShellExtInitVtbl CShellExtInitVtbl = {
	CKexShlEx_QueryInterface_thunk0,
	CKexShlEx_AddRef_thunk0,
	CKexShlEx_Release_thunk0,
	(HRESULT (STDMETHODCALLTYPE *) (IShellExtInit *, LPCITEMIDLIST, LPDATAOBJECT, HKEY)) CKexShlEx_Initialize
};

IShellPropSheetExtVtbl CShellPropSheetExtVtbl = {
	CKexShlEx_QueryInterface_thunk1,
	CKexShlEx_AddRef_thunk1,
	CKexShlEx_Release_thunk1,
	(HRESULT (STDMETHODCALLTYPE *) (IShellPropSheetExt *, LPFNADDPROPSHEETPAGE, LPARAM)) CKexShlEx_AddPages,
	(HRESULT (STDMETHODCALLTYPE *) (IShellPropSheetExt *, UINT, LPFNADDPROPSHEETPAGE, LPARAM)) CKexShlEx_ReplacePage
};

CONST IKexShlEx IKexShlExTemplate = {
	&CShellExtInitVtbl,
	&CShellPropSheetExtVtbl,
	FALSE,
	0,
	{0}
};

//
// CClassFactory
//

IClassFactoryVtbl CClassFactoryVtbl = {
	CClassFactory_QueryInterface,
	CClassFactory_AddRef,
	CClassFactory_Release,
	CClassFactory_CreateInstance,
	CClassFactory_LockServer
};

IClassFactory CClassFactory = {
	&CClassFactoryVtbl
};

//
// DLL exports
//

BOOL WINAPI DllMain(
	IN	HMODULE	InstanceHandle,
	IN	ULONG	Reason,
	IN	PVOID	Reserved)
{
	if (Reason == DLL_PROCESS_ATTACH) {
		DllHandle = InstanceHandle;
		DisableThreadLibraryCalls(DllHandle);
		KexgApplicationFriendlyName = FRIENDLYAPPNAME;
	}

	return TRUE;
}

STDAPI DllGetClassObject(
	IN	REFCLSID	RefCLSID,
	IN	REFIID		RefIID,
	IN	PPVOID		ClassObjectOut)
{
	ASSERT (RefCLSID != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (ClassObjectOut != NULL);

	*ClassObjectOut = NULL;

	if (IsEqualIID(RefCLSID, &CLSID_KexShlEx)) {
		return CClassFactory.lpVtbl->QueryInterface(&CClassFactory, RefIID, ClassObjectOut);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow(
	VOID)
{
	if (DllReferenceCount == 0) {
		return S_OK;
	} else {
		return S_FALSE;
	}
}

//
// The functionality usually assigned to DllRegisterServer, DllUnregisterServer
// and DllInstall has been moved to KexSetup as part of the rewrite.
//

STDAPI DllUnregisterServer(
	VOID)
{
	return E_NOTIMPL;
}

STDAPI DllRegisterServer(
	VOID)
{
	return E_NOTIMPL;
}

STDAPI DllInstall(
	IN	BOOL	Install,
	IN	PCWSTR	CommandLine OPTIONAL)
{
	return E_NOTIMPL;
}