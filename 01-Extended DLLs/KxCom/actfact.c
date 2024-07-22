#include "buildcfg.h"
#include "kxcomp.h"

//
// Implements the IActivationFactory interface that is returned by
// RoGetActivationFactory.
//

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_QueryInterface(
	IN	IActivationFactory	*This,
	IN	REFIID				RefIID,
	OUT	PPVOID				Object)
{
	LPOLESTR RefIIDAsString;

	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Object != NULL);

	if (KexIsDebugBuild) {
		StringFromIID(RefIID, &RefIIDAsString);
	}

	*Object = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown) ||
		IsEqualIID(RefIID, &IID_IAgileObject) ||
		IsEqualIID(RefIID, &IID_IInspectable) ||
		IsEqualIID(RefIID, &IID_IActivationFactory)) {

		*Object = This;
	} else if (IsEqualIID(RefIID, &IID_IGlobalizationPreferencesStatics)) {
		*Object = &CGlobalizationPreferencesStatics;
	} else if (IsEqualIID(RefIID, &IID_IUIViewSettingsInterop)) {
		*Object = &CUIViewSettingsInterop;
	} else if (IsEqualIID(RefIID, &IID_IUIViewSettings)) {
		*Object = &CUIViewSettings;
	} else if (IsEqualIID(RefIID, &IID_IUISettings)) {
		*Object = &CUISettings;
	} else if (IsEqualIID(RefIID, &IID_IUISettings3)) {
		*Object = &CUISettings3;
	} else if (IsEqualIID(RefIID, &IID_IUriRuntimeClassFactory)) {
		*Object = &CUriRuntimeClassFactory;
	} else if (IsEqualIID(RefIID, &IID_ILauncherStatics)) {
		*Object = &CLauncherStatics;
	} else {
		if (!KexIsDebugBuild) {
			StringFromIID(RefIID, &RefIIDAsString);
		}

		KexLogWarningEvent(
			L"The Windows Runtime activation factory was queried for an unsupported interface.\r\n\r\n"
			L"IID: %s",
			RefIIDAsString);

		CoTaskMemFree(RefIIDAsString);
		return E_NOINTERFACE;
	}

	if (KexIsDebugBuild) {
		KexLogDebugEvent(L"WinRT activation factory queried: %s", RefIIDAsString);
		CoTaskMemFree(RefIIDAsString);
	}

	return S_OK;
}

KXCOMAPI ULONG STDMETHODCALLTYPE CActivationFactory_AddRef(
	IN	IActivationFactory	*This)
{
	return 1;
}

KXCOMAPI ULONG STDMETHODCALLTYPE CActivationFactory_Release(
	IN	IActivationFactory	*This)
{
	return 1;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_GetIids(
	IN	IActivationFactory	*This,
	OUT	PULONG				NumberOfIids,
	OUT	IID					**IidArray)
{
	IID *Array;
	ULONG Count;

	ASSERT (NumberOfIids != NULL);
	ASSERT (IidArray != NULL);

	Count = 1;

	Array = (IID *) CoTaskMemAlloc(Count * sizeof(IID));
	if (!Array) {
		return E_OUTOFMEMORY;
	}

	*NumberOfIids = Count;
	Array[0] = IID_IActivationFactory;

	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_GetRuntimeClassName(
	IN	IActivationFactory	*This,
	OUT	HSTRING				*ClassName)
{
	PCWSTR Name = L"IActivationFactory";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_GetTrustLevel(
	IN	IActivationFactory	*This,
	OUT	TrustLevel			*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_ActivateInstance(
	IN	IActivationFactory	*This,
	OUT	IInspectable		**Object)
{
	ASSERT (This != NULL);
	ASSERT (Object != NULL);

	*Object = (IInspectable *) This;
	return S_OK;
}

IActivationFactoryVtbl CActivationFactoryVtbl = {
	CActivationFactory_QueryInterface,
	CActivationFactory_AddRef,
	CActivationFactory_Release,
	CActivationFactory_GetIids,
	CActivationFactory_GetRuntimeClassName,
	CActivationFactory_GetTrustLevel,
	CActivationFactory_ActivateInstance
};

IActivationFactory CActivationFactory = {
	&CActivationFactoryVtbl
};