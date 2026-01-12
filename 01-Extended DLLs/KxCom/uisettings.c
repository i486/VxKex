#include "buildcfg.h"
#include "kxcomp.h"

//
// Implements IUISettings and IUISettings3.
// Most of this crap is just a wrapper around basic win32 functions like
// SystemParametersInfo, GetSystemMetrics, GetSysColor, etc.
//

int _fltused;

STATIC HRESULT InternalGetSystemMetricHeightAndWidth(
	IN	INT				WidthIndex,
	IN	INT				HeightIndex,
	OUT	UISettingsSize	*Size)
{
	INT Width;
	INT Height;

	Size->Width = 0.0f;
	Size->Height = 0.0f;

	Width = GetSystemMetrics(WidthIndex);
	Height = GetSystemMetrics(HeightIndex);

	if (!Width || !Height) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	Size->Width = (FLOAT) Width;
	Size->Height = (FLOAT) Height;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_QueryInterface(
	IN	IUISettings	*This,
	IN	REFIID		RefIID,
	OUT	PPVOID		Object)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Object != NULL);

	*Object = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown) ||
		IsEqualIID(RefIID, &IID_IAgileObject) ||
		IsEqualIID(RefIID, &IID_IInspectable) ||
		IsEqualIID(RefIID, &IID_IUISettings)) {

		*Object = This;
	} else if (IsEqualIID(RefIID, &IID_IUISettings3)) {
		*Object = &CUISettings3;
	} else {
		LPOLESTR IidAsString;

		StringFromIID(RefIID, &IidAsString);

		KexLogWarningEvent(
			L"QueryInterface called with unsupported IID: %s",
			IidAsString);

		CoTaskMemFree(IidAsString);
		return E_NOINTERFACE;
	}

	return S_OK;
}

KXCOMAPI ULONG STDMETHODCALLTYPE CUISettings_AddRef(
	IN	IUISettings	*This)
{
	return 1;
}

KXCOMAPI ULONG STDMETHODCALLTYPE CUISettings_Release(
	IN	IUISettings	*This)
{
	return 1;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_GetIids(
	IN	IUISettings	*This,
	OUT	PULONG		NumberOfIids,
	OUT	IID			**IidArray)
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
	Array[0] = IID_IUISettings;

	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_GetRuntimeClassName(
	IN	IUISettings	*This,
	OUT	HSTRING		*ClassName)
{
	PCWSTR Name = L"Windows.UI.ViewManagement.UISettings";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_GetTrustLevel(
	IN	IUISettings	*This,
	OUT	TrustLevel	*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_HandPreference(
	IN	IUISettings			*This,
	OUT	UIHandPreference	*HandPreference)
{
	ULONG RightHanded;
	BOOLEAN Success;

	*HandPreference = HandPreference_RightHanded;

	Success = SystemParametersInfo(SPI_GETMENUDROPALIGNMENT, 0, &RightHanded, FALSE);
	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	*HandPreference = (RightHanded == TRUE) ? HandPreference_RightHanded : HandPreference_LeftHanded;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_CursorSize(
	IN	IUISettings		*This,
	OUT	UISettingsSize	*Size)
{
	return InternalGetSystemMetricHeightAndWidth(SM_CXCURSOR, SM_CYCURSOR, Size);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_ScrollBarSize(
	IN	IUISettings		*This,
	OUT	UISettingsSize	*Size)
{
	return InternalGetSystemMetricHeightAndWidth(SM_CYHSCROLL, SM_CXVSCROLL, Size);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_ScrollBarArrowSize(
	IN	IUISettings		*This,
	OUT	UISettingsSize	*Size)
{
	return InternalGetSystemMetricHeightAndWidth(SM_CXHSCROLL, SM_CYVSCROLL, Size);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_ScrollBarThumbBoxSize(
	IN	IUISettings		*This,
	OUT	UISettingsSize	*Size)
{
	return InternalGetSystemMetricHeightAndWidth(SM_CXHTHUMB, SM_CYVTHUMB, Size);
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_MessageDuration(
	IN	IUISettings		*This,
	OUT	PULONG			Duration)
{
	BOOLEAN Success;

	*Duration = 0;

	Success = SystemParametersInfo(SPI_GETMESSAGEDURATION, 0, Duration, FALSE);
	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_AnimationsEnabled(
	IN	IUISettings	*This,
	OUT	PBOOLEAN	Enabled)
{
	BOOLEAN Success;
	BOOL EnabledI32;

	Success = SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &EnabledI32, FALSE);
	if (!Success) {
		*Enabled = FALSE;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	*Enabled = !!EnabledI32;
	return S_OK;
}

// NB: This value is not natively supported in Win7.
#define SPI_GETCARETBROWSING 0x104C

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_CaretBrowsingEnabled(
	IN	IUISettings	*This,
	OUT	PBOOLEAN	Enabled)
{
	BOOLEAN Success;
	BOOL EnabledI32;

	Success = SystemParametersInfo(SPI_GETCARETBROWSING, 0, &EnabledI32, FALSE);
	if (!Success) {
		*Enabled = FALSE;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	*Enabled = !!EnabledI32;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_CaretBlinkRate(
	IN	IUISettings	*This,
	OUT	PULONG		BlinkRate)
{
	*BlinkRate = GetCaretBlinkTime();
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_CaretWidth(
	IN	IUISettings	*This,
	OUT	PULONG		Width)
{
	BOOLEAN Success;

	Success = SystemParametersInfo(SPI_GETCARETWIDTH, 0, Width, FALSE);
	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_DoubleClickTime(
	IN	IUISettings	*This,
	OUT	PULONG		DoubleClickTime)
{
	*DoubleClickTime = GetDoubleClickTime();
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_get_MouseHoverTime(
	IN	IUISettings	*This,
	OUT	PULONG		MouseHoverTime)
{
	BOOLEAN Success;

	Success = SystemParametersInfo(SPI_GETMOUSEHOVERTIME, 0, MouseHoverTime, FALSE);
	if (!Success) {
		return HRESULT_FROM_WIN32(GetLastError());
	}
	
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings_UIElementColor(
	IN	IUISettings		*This,
	IN	UIElementType	Type,
	OUT	UIColor			*Color)
{
	ULONG ColorIndex;
	ULONG ColorI32;

	switch (Type) {
	case UIElementType_ActiveCaption:			ColorIndex = COLOR_ACTIVECAPTION;		break;
	case UIElementType_Background:				ColorIndex = COLOR_BACKGROUND;			break;
	case UIElementType_ButtonFace:				ColorIndex = COLOR_BTNFACE;				break;
	case UIElementType_ButtonText:				ColorIndex = COLOR_BTNTEXT;				break;
	case UIElementType_CaptionText:				ColorIndex = COLOR_CAPTIONTEXT;			break;
	case UIElementType_GrayText:				ColorIndex = COLOR_GRAYTEXT;			break;
	case UIElementType_Highlight:				ColorIndex = COLOR_HIGHLIGHT;			break;
	case UIElementType_HighlightText:			ColorIndex = COLOR_HIGHLIGHTTEXT;		break;
	case UIElementType_Hotlight:				ColorIndex = COLOR_HOTLIGHT;			break;
	case UIElementType_InactiveCaption:			ColorIndex = COLOR_INACTIVECAPTION;		break;
	case UIElementType_InactiveCaptionText:		ColorIndex = COLOR_INACTIVECAPTIONTEXT;	break;
	case UIElementType_Window:					ColorIndex = COLOR_WINDOW;				break;
	case UIElementType_WindowText:				ColorIndex = COLOR_WINDOWTEXT;			break;
	default:
		RtlZeroMemory(Color, sizeof(*Color));
		return S_OK;
	}

	ColorI32 = GetSysColor(ColorIndex);
	Color->A = 0xFF; // Opaque
	Color->R = GetRValue(ColorI32);
	Color->G = GetGValue(ColorI32);
	Color->B = GetBValue(ColorI32);

	return S_OK;
}

IUISettingsVtbl CUISettingsVtbl = {
	CUISettings_QueryInterface,
	CUISettings_AddRef,
	CUISettings_Release,

	CUISettings_GetIids,
	CUISettings_GetRuntimeClassName,
	CUISettings_GetTrustLevel,

	CUISettings_get_HandPreference,
	CUISettings_get_CursorSize,
	CUISettings_get_ScrollBarSize,
	CUISettings_get_ScrollBarArrowSize,
	CUISettings_get_ScrollBarThumbBoxSize,
	CUISettings_get_MessageDuration,
	CUISettings_get_AnimationsEnabled,
	CUISettings_get_CaretBrowsingEnabled,
	CUISettings_get_CaretBlinkRate,
	CUISettings_get_CaretWidth,
	CUISettings_get_DoubleClickTime,
	CUISettings_get_MouseHoverTime,
	CUISettings_UIElementColor
};

IUISettings CUISettings = {
	&CUISettingsVtbl
};

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings3_GetColorValue(
	IN	IUISettings3	*This,
	IN	UIColorType		DesiredColor,
	OUT	UIColor			*ColorValue)
{
	ULONG SysColor;
	ULONG ColorI32;

	switch (DesiredColor) {
	case UIColorType_Background:	SysColor = COLOR_WINDOW;		break;
	case UIColorType_Foreground:	SysColor = COLOR_WINDOWTEXT;	break;
	case UIColorType_AccentDark3:	SysColor = COLOR_3DDKSHADOW;	break;
	case UIColorType_AccentDark2:	SysColor = COLOR_3DDKSHADOW;	break;
	case UIColorType_AccentDark1:	SysColor = COLOR_3DDKSHADOW;	break;
	case UIColorType_Accent:		SysColor = COLOR_3DSHADOW;		break;
	case UIColorType_AccentLight1:	SysColor = COLOR_3DLIGHT;		break;
	case UIColorType_AccentLight2:	SysColor = COLOR_3DLIGHT;		break;
	case UIColorType_AccentLight3:	SysColor = COLOR_3DLIGHT;		break;
	default:
		return E_INVALIDARG;
	}

	ColorI32 = GetSysColor(SysColor);
	ColorValue->A = 0xFF;
	ColorValue->R = GetRValue(ColorI32);
	ColorValue->G = GetGValue(ColorI32);
	ColorValue->B = GetBValue(ColorI32);

	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings3_add_ColorValuesChanged(
	IN	IUISettings3	*This,
	IN	PVOID			Callback,
	OUT	PPVOID			Cookie)
{
	*Cookie = (PVOID) (ULONG_PTR) 0x12345678;
	return S_OK;
}

KXCOMAPI HRESULT STDMETHODCALLTYPE CUISettings3_remove_ColorValuesChanged(
	IN	IUISettings3	*This,
	IN	PVOID			Cookie)
{
	if (((ULONG_PTR) Cookie) != 0x12345678) {
		return E_INVALIDARG;
	}

	return S_OK;
}

#pragma warning(disable:4028)
IUISettings3Vtbl CUISettings3Vtbl = {
	CUISettings_QueryInterface,
	CUISettings_AddRef,
	CUISettings_Release,

	CUISettings_GetIids,
	CUISettings_GetRuntimeClassName,
	CUISettings_GetTrustLevel,

	CUISettings3_GetColorValue,
	CUISettings3_add_ColorValuesChanged,
	CUISettings3_remove_ColorValuesChanged
};
#pragma warning(default:4028)

IUISettings3 CUISettings3 = {
	&CUISettings3Vtbl
};