#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxDx.h>

EXTERN PKEX_PROCESS_DATA KexData;

typedef interface IMFMediaBuffer IMFMediaBuffer;
typedef interface IMFDXGIDeviceManager IMFDXGIDeviceManager;

#define D3D_FEATURE_LEVEL_11_1	((D3D_FEATURE_LEVEL) 0xb100)
#define D3D_FEATURE_LEVEL_12_0	((D3D_FEATURE_LEVEL) 0xc000)
#define D3D_FEATURE_LEVEL_12_1	((D3D_FEATURE_LEVEL) 0xc100)
#define D3D_FEATURE_LEVEL_12_2	((D3D_FEATURE_LEVEL) 0xc200)

//
// dcmpstub.c
//

HRESULT WINAPI CreateIDXGIFactoryMedia(
	OUT	PPVOID	FactoryMedia);

//
// direct2d.c
//

VOID WrapDirect2DDevice(
	IN OUT	ID2D1Device *Device);

VOID WrapDirect2DFactory(
	IN OUT	ID2D1Factory1 *Factory);

//
// dxgi.c
//

VOID WrapDXGIFactory(
	IN OUT	IDXGIFactory2 *Factory);

//
// rptnoint.c
//

VOID ReportNoInterfaceError(
	IN	REFIID	RefIID,
	IN	HRESULT	Result,
	IN	PCWSTR	ErrorMessage);