///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     clsfctry.c
//
// Abstract:
//
//     Implements the IClassFactory interface.
//     All just COM boilerplate crap, nothing to see here.
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

HRESULT STDMETHODCALLTYPE CClassFactory_QueryInterface(
	IN	IClassFactory	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (ObjectOut != NULL);

	if (IsEqualIID(RefIID, &IID_IUnknown)) {
		*ObjectOut = This;
	} else if (IsEqualIID(RefIID, &IID_IClassFactory)) {
		*ObjectOut = This;
	} else {
		*ObjectOut = NULL;
		return E_NOINTERFACE;
	}

	return S_OK;
}

ULONG STDMETHODCALLTYPE CClassFactory_AddRef(
	IN	IClassFactory	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CClassFactory_Release(
	IN	IClassFactory	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CClassFactory_CreateInstance(
	IN	IClassFactory	*This,
	IN	IUnknown		*OuterIUnknown OPTIONAL,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut)
{
	IKexShlEx *CKexShlEx;

	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (ObjectOut != NULL);

	if (OuterIUnknown) {
		return CLASS_E_NOAGGREGATION;
	}

	CKexShlEx = SafeAlloc(IKexShlEx, 1);
	if (!CKexShlEx) {
		return E_OUTOFMEMORY;
	}

	InterlockedIncrement(&DllReferenceCount);
	CopyMemory(CKexShlEx, &IKexShlExTemplate, sizeof(IKexShlExTemplate));
	return CKexShlEx_QueryInterface(CKexShlEx, RefIID, ObjectOut);
}

HRESULT STDMETHODCALLTYPE CClassFactory_LockServer(
	IN	IClassFactory	*This,
	IN	BOOL			Lock)
{
	return E_NOTIMPL;
}