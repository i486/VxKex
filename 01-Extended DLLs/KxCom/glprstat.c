#include "buildcfg.h"
#include "kxcomp.h"

//
// VERY minimal barebones implementation of IGlobalizationPreferencesStatics.
// This can be fleshed out easily if more applications require it.
//

ULONG STDMETHODCALLTYPE CVectorView_HSTRING_Release(
	IN	IVectorView_HSTRING	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CVectorView_HSTRING_get_Size(
	IN	IVectorView_HSTRING	*This,
	OUT	PULONG				NumberOfElements)
{
	*NumberOfElements = 0;
	return S_OK;
}

IVectorView_HSTRINGVtbl CVectorView_HSTRINGVtbl = {
	NULL,
	NULL,
	CVectorView_HSTRING_Release,
	NULL,
	NULL,
	NULL,
	NULL,
	CVectorView_HSTRING_get_Size,
	NULL,
	NULL
};

IVectorView_HSTRING CVectorView_HSTRING = {
	&CVectorView_HSTRINGVtbl
};

typedef struct _IGlobalizationPreferencesStatics IGlobalizationPreferencesStatics;

HRESULT STDMETHODCALLTYPE CGlobalizationPreferencesStatics_QueryInterface(
	IN	IGlobalizationPreferencesStatics	*This,
	IN	REFIID								RefIID,
	OUT	PPVOID								Interface)
{
	*Interface = NULL;
	return E_NOTIMPL;
}

ULONG STDMETHODCALLTYPE CGlobalizationPreferencesStatics_AddRef(
	IN	IGlobalizationPreferencesStatics	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CGlobalizationPreferencesStatics_Release(
	IN	IGlobalizationPreferencesStatics	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING(
	IN	IGlobalizationPreferencesStatics	*This,
	OUT	IVectorView_HSTRING					**VectorView)
{
	*VectorView = &CVectorView_HSTRING;
	return S_OK;
}

IGlobalizationPreferencesStaticsVtbl CGlobalizationPreferencesStaticsVtbl = {
	CGlobalizationPreferencesStatics_QueryInterface,
	CGlobalizationPreferencesStatics_AddRef,
	CGlobalizationPreferencesStatics_Release,
	NULL,
	NULL,
	NULL,
	CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING,
	CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING,
	CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING,
	CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING,
	CGlobalizationPreferencesStatics_GetDummyIVectorView_HSTRING,
	NULL
};

IGlobalizationPreferencesStatics CGlobalizationPreferencesStatics = {
	&CGlobalizationPreferencesStaticsVtbl
};