#include "buildcfg.h"
#include "kxdxp.h"
#include <dxgi.h>

HRESULT (STDMETHODCALLTYPE *IDXGIFactory2_OriginalQueryInterface)(IDXGIFactory2 *, REFIID, PPVOID);
HRESULT (STDMETHODCALLTYPE *IDXGIAdapter1_OriginalQueryInterface)(IDXGIAdapter1 *, REFIID, PPVOID);

HRESULT STDMETHODCALLTYPE IDXGIAdapter4_QueryInterface(
	IN	IDXGIAdapter4	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Object)
{
	HRESULT Result;

	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Object != NULL);

	if (IsEqualIID(RefIID, &IID_IDXGIAdapter2) ||
		IsEqualIID(RefIID, &IID_IDXGIAdapter3) ||
		IsEqualIID(RefIID, &IID_IDXGIAdapter4)) {

		*Object = This;
		This->lpVtbl->Base.Base.Base.AddRef((IDXGIAdapter1 *) This);
		return S_OK;
	}

	Result = IDXGIAdapter1_OriginalQueryInterface(
		(IDXGIAdapter1 *) This,
		RefIID,
		Object);

	if (FAILED(Result)) {
		ReportNoInterfaceError(RefIID, Result, L"QueryInterface failed");
	}

	return Result;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter2_GetDesc2(
	IN	IDXGIAdapter2		*This,
	OUT	PDXGI_ADAPTER_DESC2	Desc)
{
	ASSERT (This != NULL);
	ASSERT (Desc != NULL);

	RtlZeroMemory(Desc, sizeof(*Desc));

	return This->lpVtbl->Base.GetDesc1(
		(IDXGIAdapter1 *) This,
		(DXGI_ADAPTER_DESC1 *) Desc);
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_RegisterHardwareContentProtectionTeardownStatusEvent(
	IN	IDXGIAdapter3	*This,
	IN	HANDLE			Event,
	OUT	PULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_UnregisterHardwareContentProtectionTeardownStatus(
	IN	IDXGIAdapter3	*This,
	IN	ULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_QueryVideoMemoryInfo(
	IN	IDXGIAdapter3					*This,
	IN	ULONG							NodeIndex,
	IN	DXGI_MEMORY_SEGMENT_GROUP		MemorySegmentGroup,
	OUT	PDXGI_QUERY_VIDEO_MEMORY_INFO	VideoMemoryInfo)
{
	ASSERT (VideoMemoryInfo != NULL);

	KexLogUnimplementedFunctionEvent();
	
	RtlZeroMemory(VideoMemoryInfo, sizeof(*VideoMemoryInfo));
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_SetVideoMemoryReservation(
	IN	IDXGIAdapter3					*This,
	IN	ULONG							NodeIndex,
	IN	DXGI_MEMORY_SEGMENT_GROUP		MemorySegmentGroup,
	IN	ULONGLONG						Reservation)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent(
	IN	IDXGIAdapter3	*This,
	IN	HANDLE			Event,
	OUT	PULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter3_UnregisterVideoMemoryBudgetChangeNotification(
	IN	IDXGIAdapter3	*This,
	IN	ULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIAdapter4_GetDesc3(
	IN	IDXGIAdapter4		*This,
	OUT	PDXGI_ADAPTER_DESC3	Desc)
{
	// DXGI_ADAPTER_DESC2 and DXGI_ADAPTER_DESC3 are basically the same shit
	// It only differs for "feature level 9 graphics hardware", which the Desc2
	// version of the structure pretends is a "Software Adapter".
	return IDXGIAdapter2_GetDesc2(
		(IDXGIAdapter2 *) This,
		(PDXGI_ADAPTER_DESC2) Desc);
}

VOID WrapDXGIAdapter(
	IN OUT	IDXGIAdapter1	*Adapter)
{
	STATIC IDXGIAdapter4Vtbl Vtbl;

	ASSERT (Adapter != NULL);

	Vtbl.Base.Base.Base = *Adapter->lpVtbl;

	IDXGIAdapter1_OriginalQueryInterface = Vtbl.Base.Base.Base.QueryInterface;
	Vtbl.Base.Base.Base.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(IDXGIAdapter1 *, REFIID, PPVOID)) IDXGIAdapter4_QueryInterface;
	Vtbl.Base.Base.GetDesc2 = IDXGIAdapter2_GetDesc2;
	Vtbl.Base.RegisterHardwareContentProtectionTeardownStatusEvent = IDXGIAdapter3_RegisterHardwareContentProtectionTeardownStatusEvent;
	Vtbl.Base.UnregisterHardwareContentProtectionTeardownStatus = IDXGIAdapter3_UnregisterHardwareContentProtectionTeardownStatus;
	Vtbl.Base.QueryVideoMemoryInfo = IDXGIAdapter3_QueryVideoMemoryInfo;
	Vtbl.Base.SetVideoMemoryReservation = IDXGIAdapter3_SetVideoMemoryReservation;
	Vtbl.Base.RegisterVideoMemoryBudgetChangeNotificationEvent = IDXGIAdapter3_RegisterVideoMemoryBudgetChangeNotificationEvent;
	Vtbl.Base.UnregisterVideoMemoryBudgetChangeNotification = IDXGIAdapter3_UnregisterVideoMemoryBudgetChangeNotification;
	Vtbl.GetDesc3 = IDXGIAdapter4_GetDesc3;

	Adapter->lpVtbl = (IDXGIAdapter1Vtbl *) &Vtbl;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory7_QueryInterface(
	IN	IDXGIFactory7	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Object)
{
	HRESULT Result;

	if (IsEqualIID(RefIID, &IID_IDXGIFactory3) ||
		IsEqualIID(RefIID, &IID_IDXGIFactory4) ||
		IsEqualIID(RefIID, &IID_IDXGIFactory5) ||
		IsEqualIID(RefIID, &IID_IDXGIFactory6) ||
		IsEqualIID(RefIID, &IID_IDXGIFactory7)) {

		*Object = This;
		This->lpVtbl->Base.Base.Base.Base.Base.AddRef((IDXGIFactory2 *) This);
		return S_OK;
	} else if (IsEqualIID(RefIID, &IID_IDXGIFactoryMedia)) {
		return CreateIDXGIFactoryMedia(Object);
	}

	Result = IDXGIFactory2_OriginalQueryInterface(
		(IDXGIFactory2 *) This,
		RefIID,
		Object);

	if (FAILED(Result)) {
		ReportNoInterfaceError(RefIID, Result, L"QueryInterface failed");
	}

	return Result;
}

ULONG STDMETHODCALLTYPE IDXGIFactory3_GetCreationFlags(
	IN	IDXGIFactory3	*This)
{
	// We're supposed to return the flags passed to CreateDXGIFactory2.
	// Most of the time this is going to be 0 anyway so who cares
	return 0;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory4_EnumAdapterByLuid(
	IN	IDXGIFactory4	*This,
	IN	LUID			AdapterLuid,
	IN	REFIID			RefIID,
	OUT	PPVOID			Adapter)
{
	ASSERT (Adapter != NULL);

	KexLogUnimplementedFunctionEvent();

	*Adapter = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory4_EnumWarpAdapter(
	IN	IDXGIFactory4	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			Adapter)
{
	ASSERT (Adapter != NULL);

	KexLogUnimplementedFunctionEvent();

	*Adapter = NULL;
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory5_CheckFeatureSupport(
	IN	IDXGIFactory5	*This,
	IN	DXGI_FEATURE	Feature,
	OUT	PVOID			FeatureSupportData,
	IN	ULONG			FeatureSupportDataSize)
{
	RtlZeroMemory(FeatureSupportData, FeatureSupportDataSize);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory6_EnumAdapterByGpuPreference(
	IN	IDXGIFactory6		*This,
	IN	ULONG				AdapterNumber,
	IN	DXGI_GPU_PREFERENCE	Preference,
	IN	REFIID				RefIID,
	OUT	PPVOID				Adapter)
{
	if (IsEqualIID(RefIID, &IID_IDXGIAdapter)) {
		return This->lpVtbl->Base.Base.Base.Base.EnumAdapters(
			(IDXGIFactory2 *) This,
			AdapterNumber,
			(IDXGIAdapter **) Adapter);
	} else if (IsEqualIID(RefIID, &IID_IDXGIAdapter1)) {
		return This->lpVtbl->Base.Base.Base.Base.EnumAdapters1(
			(IDXGIFactory2 *) This,
			AdapterNumber,
			(IDXGIAdapter1 **) Adapter);
	} else if (IsEqualIID(RefIID, &IID_IDXGIAdapter2) ||
			   IsEqualIID(RefIID, &IID_IDXGIAdapter3) ||
			   IsEqualIID(RefIID, &IID_IDXGIAdapter4)) {

		HRESULT Result;
		PVOID DXGIAdapter;

		Result = This->lpVtbl->Base.Base.Base.Base.EnumAdapters1(
			(IDXGIFactory2 *) This,
			AdapterNumber,
			(IDXGIAdapter1 **) Adapter);

		if (FAILED(Result)) {
			return Result;
		}

		if (IsEqualIID(RefIID, &IID_IDXGIAdapter2)) {
			// Pointer soup... amazing
			// Win7+Platform Update should be able to handle this.
			Result = (*((IDXGIAdapter1 **) Adapter))->lpVtbl->QueryInterface(
				*((IDXGIAdapter1 **) Adapter),
				RefIID,
				&DXGIAdapter);

			ASSERT (SUCCEEDED(Result));

			if (FAILED(Result)) {
				return Result;
			}
		}

		WrapDXGIAdapter((IDXGIAdapter1 *) *Adapter);
		return Result;
	}

	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory7_RegisterAdaptersChangedEvent(
	IN	IDXGIFactory7	*This,
	IN	HANDLE			Event,
	OUT	PULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE IDXGIFactory7_UnregisterAdaptersChangedEvent(
	IN	IDXGIFactory7	*This,
	IN	ULONG			Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return S_OK;
}

VOID WrapDXGIFactory(
	IN OUT	IDXGIFactory2	*Factory)
{
	STATIC IDXGIFactory7Vtbl Vtbl;

	ASSERT (Factory != NULL);

	Vtbl.Base.Base.Base.Base.Base = *Factory->lpVtbl;
	IDXGIFactory2_OriginalQueryInterface = Vtbl.Base.Base.Base.Base.Base.QueryInterface;
	Vtbl.Base.Base.Base.Base.Base.QueryInterface = (HRESULT (STDMETHODCALLTYPE *)(IDXGIFactory2 *, REFIID, PPVOID)) IDXGIFactory7_QueryInterface;
	Vtbl.Base.Base.Base.Base.GetCreationFlags = IDXGIFactory3_GetCreationFlags;
	Vtbl.Base.Base.Base.EnumAdapterByLuid = IDXGIFactory4_EnumAdapterByLuid;
	Vtbl.Base.Base.Base.EnumWarpAdapter = IDXGIFactory4_EnumWarpAdapter;
	Vtbl.Base.Base.CheckFeatureSupport = IDXGIFactory5_CheckFeatureSupport;
	Vtbl.Base.EnumAdapterByGpuPreference = IDXGIFactory6_EnumAdapterByGpuPreference;
	Vtbl.RegisterAdaptersChangedEvent = IDXGIFactory7_RegisterAdaptersChangedEvent;
	Vtbl.UnregisterAdaptersChangedEvent = IDXGIFactory7_UnregisterAdaptersChangedEvent;

	Factory->lpVtbl = (IDXGIFactory2Vtbl *) &Vtbl;
}

KXDXAPI HRESULT WINAPI DXGIGetDebugInterface1(
	IN	UINT	Flags,
	IN	REFIID	RefIID,
	OUT	PPVOID	Interface)
{
	return E_NOINTERFACE;
}