///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     uiview.c
//
// Abstract:
//
//     IUIViewSettingsInterop and IUIViewSettings.
//
// Author:
//
//     vxiiduu (06-Mar-2024)
//
// Environment:
//
//     inside some bloatware c++ junk
//
// Revision History:
//
//     vxiiduu              06-Mar-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxcomp.h"

HRESULT STDMETHODCALLTYPE CUIViewSettingsInterop_QueryInterface(
	IN	IUIViewSettingsInterop	*This,
	IN	REFIID					RefIID,
	OUT	PPVOID					Interface)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Interface != NULL);

	*Interface = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown) ||
		IsEqualIID(RefIID, &IID_IAgileObject) ||
		IsEqualIID(RefIID, &IID_IInspectable) ||
		IsEqualIID(RefIID, &IID_IUIViewSettingsInterop)) {

		*Interface = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CUIViewSettingsInterop_AddRef(
	IN	IUIViewSettingsInterop	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CUIViewSettingsInterop_Release(
	IN	IUIViewSettingsInterop	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CUIViewSettingsInterop_GetIids(
	IN	IUIViewSettingsInterop	*This,
	OUT	PULONG					NumberOfIids,
	OUT	IID						**IidArray)
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
	Array[0] = IID_IUIViewSettingsInterop;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIViewSettingsInterop_GetRuntimeClassName(
	IN	IUIViewSettingsInterop	*This,
	OUT	HSTRING					*ClassName)
{
	PCWSTR Name = L"Windows.UI.ViewManagement.UISettings";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

HRESULT STDMETHODCALLTYPE CUIViewSettingsInterop_GetTrustLevel(
	IN	IUIViewSettingsInterop	*This,
	OUT	TrustLevel				*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIViewSettingsInterop_GetForWindow(
	IN	IUIViewSettingsInterop	*This,
	IN	HWND					Window,
	IN	REFIID					RefIID,
	OUT	PPVOID					Interface)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Interface != NULL);

	*Interface = NULL;

	if (!IsWindow(Window)) {
		return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
	}

	if (!IsEqualIID(RefIID, &IID_IUIViewSettings)) {
		return E_NOINTERFACE;
	}

	*Interface = &CUIViewSettings;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIViewSettings_QueryInterface(
	IN	IUIViewSettings	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Interface)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Interface != NULL);

	*Interface = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown) ||
		IsEqualIID(RefIID, &IID_IAgileObject) ||
		IsEqualIID(RefIID, &IID_IInspectable) ||
		IsEqualIID(RefIID, &IID_IUIViewSettings)) {

		*Interface = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CUIViewSettings_AddRef(
	IN	IUIViewSettings	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CUIViewSettings_Release(
	IN	IUIViewSettings	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CUIViewSettings_GetIids(
	IN	IUIViewSettings	*This,
	OUT	PULONG			NumberOfIids,
	OUT	IID				**IidArray)
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
	Array[0] = IID_IUIViewSettings;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUIViewSettings_GetRuntimeClassName(
	IN	IUIViewSettings	*This,
	OUT	HSTRING			*ClassName)
{
	PCWSTR Name = L"Windows.UI.ViewManagement.UISettings";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

HRESULT STDMETHODCALLTYPE CUIViewSettings_GetTrustLevel(
	IN	IUIViewSettings	*This,
	OUT	TrustLevel		*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

// this, ladies and gentlemen, is what this entire 200+ LINE FILE boils
// down to. this is the only function in the whole file that actually
// "does something" (and even that's a bit of a stretch). the rest is
// literally just copypasted boilerplate COM junk.
HRESULT STDMETHODCALLTYPE CUIViewSettings_get_UserInteractionMode(
	IN	IUIViewSettings		*This,
	OUT	UserInteractionMode	*InteractionMode)
{
	ASSERT (InteractionMode != NULL);
	*InteractionMode = UserInteractionMode_Mouse;
	return S_OK;
}

IUIViewSettingsInteropVtbl CUIViewSettingsInteropVtbl = {
	CUIViewSettingsInterop_QueryInterface,
	CUIViewSettingsInterop_AddRef,
	CUIViewSettingsInterop_Release,

	CUIViewSettingsInterop_GetIids,
	CUIViewSettingsInterop_GetRuntimeClassName,
	CUIViewSettingsInterop_GetTrustLevel,

	CUIViewSettingsInterop_GetForWindow
};

IUIViewSettingsInterop CUIViewSettingsInterop = {
	&CUIViewSettingsInteropVtbl
};

IUIViewSettingsVtbl CUIViewSettingsVtbl = {
	CUIViewSettings_QueryInterface,
	CUIViewSettings_AddRef,
	CUIViewSettings_Release,

	CUIViewSettings_GetIids,
	CUIViewSettings_GetRuntimeClassName,
	CUIViewSettings_GetTrustLevel,

	CUIViewSettings_get_UserInteractionMode
};

IUIViewSettings CUIViewSettings = {
	&CUIViewSettingsVtbl
};