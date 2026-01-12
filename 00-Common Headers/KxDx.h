#pragma once

// This is a bit messy but basically we want to override the definition
// of DXGI_FORMAT (in dxgiformat.h) with our own one, so we'll force
// dxgiformat.h to not define the enum.
#define __dxgiformat_h__
#define DXGI_FORMAT_DEFINED 1
typedef enum DXGI_FORMAT DXGI_FORMAT;

#include <KexComm.h>
#include <dxgi.h>
#include <dxgitype.h>
#include <d3d9types.h>
#include <d2d1.h>

//
// Structure & enum
//

typedef enum _DXGI_SCALING {
	DXGI_SCALING_STRETCH				= 0,
	DXGI_SCALING_NONE					= 1,
	DXGI_SCALING_ASPECT_RATIO_STRETCH	= 2
} TYPEDEF_TYPE_NAME(DXGI_SCALING);

typedef enum _DXGI_ALPHA_MODE {
	DXGI_ALPHA_MODE_UNSPECIFIED			= 0,
	DXGI_ALPHA_MODE_PREMULTIPLIED		= 1,
	DXGI_ALPHA_MODE_STRAIGHT			= 2,
	DXGI_ALPHA_MODE_IGNORE				= 3,
	DXGI_ALPHA_MODE_FORCE_DWORD			= 0xffffffff
} TYPEDEF_TYPE_NAME(DXGI_ALPHA_MODE);

typedef struct _DXGI_SWAP_CHAIN_DESC1 {
	UINT Width;
	UINT Height;
	DXGI_FORMAT Format;
	BOOL Stereo;
	DXGI_SAMPLE_DESC SampleDesc;
	DXGI_USAGE BufferUsage;
	UINT BufferCount;
	DXGI_SCALING Scaling;
	DXGI_SWAP_EFFECT SwapEffect;
	DXGI_ALPHA_MODE AlphaMode;
	UINT Flags;
} TYPEDEF_TYPE_NAME(DXGI_SWAP_CHAIN_DESC1);

typedef struct _DXGI_SWAP_CHAIN_FULLSCREEN_DESC {
	DXGI_RATIONAL RefreshRate;
	DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
	DXGI_MODE_SCALING Scaling;
	BOOL Windowed;
} TYPEDEF_TYPE_NAME(DXGI_SWAP_CHAIN_FULLSCREEN_DESC);

typedef struct _DXGI_PRESENT_PARAMETERS {
	UINT DirtyRectsCount;
	RECT *pDirtyRects;
	RECT *pScrollRect;
	POINT *pScrollOffset;
} TYPEDEF_TYPE_NAME(DXGI_PRESENT_PARAMETERS);

typedef enum DXGI_FORMAT {
	DXGI_FORMAT_UNKNOWN									= 0,
	DXGI_FORMAT_R32G32B32A32_TYPELESS					= 1,
	DXGI_FORMAT_R32G32B32A32_FLOAT						= 2,
	DXGI_FORMAT_R32G32B32A32_UINT						= 3,
	DXGI_FORMAT_R32G32B32A32_SINT						= 4,
	DXGI_FORMAT_R32G32B32_TYPELESS						= 5,
	DXGI_FORMAT_R32G32B32_FLOAT							= 6,
	DXGI_FORMAT_R32G32B32_UINT							= 7,
	DXGI_FORMAT_R32G32B32_SINT							= 8,
	DXGI_FORMAT_R16G16B16A16_TYPELESS					= 9,
	DXGI_FORMAT_R16G16B16A16_FLOAT						= 10,
	DXGI_FORMAT_R16G16B16A16_UNORM						= 11,
	DXGI_FORMAT_R16G16B16A16_UINT						= 12,
	DXGI_FORMAT_R16G16B16A16_SNORM						= 13,
	DXGI_FORMAT_R16G16B16A16_SINT						= 14,
	DXGI_FORMAT_R32G32_TYPELESS							= 15,
	DXGI_FORMAT_R32G32_FLOAT							= 16,
	DXGI_FORMAT_R32G32_UINT								= 17,
	DXGI_FORMAT_R32G32_SINT								= 18,
	DXGI_FORMAT_R32G8X24_TYPELESS						= 19,
	DXGI_FORMAT_D32_FLOAT_S8X24_UINT					= 20,
	DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS				= 21,
	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT					= 22,
	DXGI_FORMAT_R10G10B10A2_TYPELESS					= 23,
	DXGI_FORMAT_R10G10B10A2_UNORM						= 24,
	DXGI_FORMAT_R10G10B10A2_UINT						= 25,
	DXGI_FORMAT_R11G11B10_FLOAT							= 26,
	DXGI_FORMAT_R8G8B8A8_TYPELESS						= 27,
	DXGI_FORMAT_R8G8B8A8_UNORM							= 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB						= 29,
	DXGI_FORMAT_R8G8B8A8_UINT							= 30,
	DXGI_FORMAT_R8G8B8A8_SNORM							= 31,
	DXGI_FORMAT_R8G8B8A8_SINT							= 32,
	DXGI_FORMAT_R16G16_TYPELESS							= 33,
	DXGI_FORMAT_R16G16_FLOAT							= 34,
	DXGI_FORMAT_R16G16_UNORM							= 35,
	DXGI_FORMAT_R16G16_UINT								= 36,
	DXGI_FORMAT_R16G16_SNORM							= 37,
	DXGI_FORMAT_R16G16_SINT								= 38,
	DXGI_FORMAT_R32_TYPELESS							= 39,
	DXGI_FORMAT_D32_FLOAT								= 40,
	DXGI_FORMAT_R32_FLOAT								= 41,
	DXGI_FORMAT_R32_UINT								= 42,
	DXGI_FORMAT_R32_SINT								= 43,
	DXGI_FORMAT_R24G8_TYPELESS							= 44,
	DXGI_FORMAT_D24_UNORM_S8_UINT						= 45,
	DXGI_FORMAT_R24_UNORM_X8_TYPELESS					= 46,
	DXGI_FORMAT_X24_TYPELESS_G8_UINT					= 47,
	DXGI_FORMAT_R8G8_TYPELESS							= 48,
	DXGI_FORMAT_R8G8_UNORM								= 49,
	DXGI_FORMAT_R8G8_UINT								= 50,
	DXGI_FORMAT_R8G8_SNORM								= 51,
	DXGI_FORMAT_R8G8_SINT								= 52,
	DXGI_FORMAT_R16_TYPELESS							= 53,
	DXGI_FORMAT_R16_FLOAT								= 54,
	DXGI_FORMAT_D16_UNORM								= 55,
	DXGI_FORMAT_R16_UNORM								= 56,
	DXGI_FORMAT_R16_UINT								= 57,
	DXGI_FORMAT_R16_SNORM								= 58,
	DXGI_FORMAT_R16_SINT								= 59,
	DXGI_FORMAT_R8_TYPELESS								= 60,
	DXGI_FORMAT_R8_UNORM								= 61,
	DXGI_FORMAT_R8_UINT									= 62,
	DXGI_FORMAT_R8_SNORM								= 63,
	DXGI_FORMAT_R8_SINT									= 64,
	DXGI_FORMAT_A8_UNORM								= 65,
	DXGI_FORMAT_R1_UNORM								= 66,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP						= 67,
	DXGI_FORMAT_R8G8_B8G8_UNORM							= 68,
	DXGI_FORMAT_G8R8_G8B8_UNORM							= 69,
	DXGI_FORMAT_BC1_TYPELESS							= 70,
	DXGI_FORMAT_BC1_UNORM								= 71,
	DXGI_FORMAT_BC1_UNORM_SRGB							= 72,
	DXGI_FORMAT_BC2_TYPELESS							= 73,
	DXGI_FORMAT_BC2_UNORM								= 74,
	DXGI_FORMAT_BC2_UNORM_SRGB							= 75,
	DXGI_FORMAT_BC3_TYPELESS							= 76,
	DXGI_FORMAT_BC3_UNORM								= 77,
	DXGI_FORMAT_BC3_UNORM_SRGB							= 78,
	DXGI_FORMAT_BC4_TYPELESS							= 79,
	DXGI_FORMAT_BC4_UNORM								= 80,
	DXGI_FORMAT_BC4_SNORM								= 81,
	DXGI_FORMAT_BC5_TYPELESS							= 82,
	DXGI_FORMAT_BC5_UNORM								= 83,
	DXGI_FORMAT_BC5_SNORM								= 84,
	DXGI_FORMAT_B5G6R5_UNORM							= 85,
	DXGI_FORMAT_B5G5R5A1_UNORM							= 86,
	DXGI_FORMAT_B8G8R8A8_UNORM							= 87,
	DXGI_FORMAT_B8G8R8X8_UNORM							= 88,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM				= 89,
	DXGI_FORMAT_B8G8R8A8_TYPELESS						= 90,
	DXGI_FORMAT_B8G8R8A8_UNORM_SRGB						= 91,
	DXGI_FORMAT_B8G8R8X8_TYPELESS						= 92,
	DXGI_FORMAT_B8G8R8X8_UNORM_SRGB						= 93,
	DXGI_FORMAT_BC6H_TYPELESS							= 94,
	DXGI_FORMAT_BC6H_UF16								= 95,
	DXGI_FORMAT_BC6H_SF16								= 96,
	DXGI_FORMAT_BC7_TYPELESS							= 97,
	DXGI_FORMAT_BC7_UNORM								= 98,
	DXGI_FORMAT_BC7_UNORM_SRGB							= 99,
	DXGI_FORMAT_AYUV									= 100,
	DXGI_FORMAT_Y410									= 101,
	DXGI_FORMAT_Y416									= 102,
	DXGI_FORMAT_NV12									= 103,
	DXGI_FORMAT_P010									= 104,
	DXGI_FORMAT_P016									= 105,
	DXGI_FORMAT_420_OPAQUE								= 106,
	DXGI_FORMAT_YUY2									= 107,
	DXGI_FORMAT_Y210									= 108,
	DXGI_FORMAT_Y216									= 109,
	DXGI_FORMAT_NV11									= 110,
	DXGI_FORMAT_AI44									= 111,
	DXGI_FORMAT_IA44									= 112,
	DXGI_FORMAT_P8										= 113,
	DXGI_FORMAT_A8P8									= 114,
	DXGI_FORMAT_B4G4R4A4_UNORM							= 115,

	DXGI_FORMAT_P208									= 130,
	DXGI_FORMAT_V208									= 131,
	DXGI_FORMAT_V408									= 132,

	DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE			= 189,
	DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE	= 190,

	DXGI_FORMAT_FORCE_UINT								= 0xffffffff
} TYPEDEF_TYPE_NAME(DXGI_FORMAT);

typedef D3DCOLORVALUE DXGI_RGBA;

//
// IDXGISwapChain1
//

typedef interface IDXGISwapChain1 IDXGISwapChain1;

typedef struct IDXGISwapChain1Vtbl
{
	BEGIN_INTERFACE
        
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
		IDXGISwapChain1 * This,
		/* [in] */ REFIID riid,
		/* [annotation][iid_is][out] */ 
		_Out_ void **ppvObject);
        
	ULONG ( STDMETHODCALLTYPE *AddRef )( 
		IDXGISwapChain1 * This);
        
	ULONG ( STDMETHODCALLTYPE *Release )( 
		IDXGISwapChain1 * This);
        
	HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [in] */ UINT DataSize,
		/* [annotation][in] */ 
		_In_ const void *pData);
        
	HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [annotation][in] */ 
		_In_opt_  const IUnknown *pUnknown);
        
	HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [annotation][out][in] */ 
		_Inout_  UINT *pDataSize,
		/* [annotation][out] */ 
		_Out_ void *pData);
        
	HRESULT ( STDMETHODCALLTYPE *GetParent )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFIID riid,
		/* [annotation][retval][out] */ 
		_Out_  void **ppParent);
        
	HRESULT ( STDMETHODCALLTYPE *GetDevice )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFIID riid,
		/* [annotation][retval][out] */ 
		_Out_  void **ppDevice);
        
	HRESULT ( STDMETHODCALLTYPE *Present )( 
		IDXGISwapChain1 * This,
		/* [in] */ UINT SyncInterval,
		/* [in] */ UINT Flags);
        
	HRESULT ( STDMETHODCALLTYPE *GetBuffer )( 
		IDXGISwapChain1 * This,
		/* [in] */ UINT Buffer,
		/* [annotation][in] */ 
		_In_  REFIID riid,
		/* [annotation][out][in] */ 
		_Out_  void **ppSurface);
        
	HRESULT ( STDMETHODCALLTYPE *SetFullscreenState )( 
		IDXGISwapChain1 * This,
		/* [in] */ BOOL Fullscreen,
		/* [annotation][in] */ 
		_In_opt_  IDXGIOutput *pTarget);
        
	HRESULT ( STDMETHODCALLTYPE *GetFullscreenState )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_opt_  BOOL *pFullscreen,
		/* [annotation][out] */ 
		_Out_opt_  IDXGIOutput **ppTarget);
        
	HRESULT ( STDMETHODCALLTYPE *GetDesc )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_SWAP_CHAIN_DESC *pDesc);
        
	HRESULT ( STDMETHODCALLTYPE *ResizeBuffers )( 
		IDXGISwapChain1 * This,
		/* [in] */ UINT BufferCount,
		/* [in] */ UINT Width,
		/* [in] */ UINT Height,
		/* [in] */ DXGI_FORMAT NewFormat,
		/* [in] */ UINT SwapChainFlags);
        
	HRESULT ( STDMETHODCALLTYPE *ResizeTarget )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  const DXGI_MODE_DESC *pNewTargetParameters);
        
	HRESULT ( STDMETHODCALLTYPE *GetContainingOutput )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  IDXGIOutput **ppOutput);
        
	HRESULT ( STDMETHODCALLTYPE *GetFrameStatistics )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_FRAME_STATISTICS *pStats);
        
	HRESULT ( STDMETHODCALLTYPE *GetLastPresentCount )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  UINT *pLastPresentCount);
        
	HRESULT ( STDMETHODCALLTYPE *GetDesc1 )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_SWAP_CHAIN_DESC1 *pDesc);
        
	HRESULT ( STDMETHODCALLTYPE *GetFullscreenDesc )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pDesc);
        
	HRESULT ( STDMETHODCALLTYPE *GetHwnd )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  HWND *pHwnd);
        
	HRESULT ( STDMETHODCALLTYPE *GetCoreWindow )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  REFIID refiid,
		/* [annotation][out] */ 
		_Out_  void **ppUnk);
        
	HRESULT ( STDMETHODCALLTYPE *Present1 )( 
		IDXGISwapChain1 * This,
		/* [in] */ UINT SyncInterval,
		/* [in] */ UINT PresentFlags,
		/* [annotation][in] */ 
		_In_  const DXGI_PRESENT_PARAMETERS *pPresentParameters);
        
	BOOL ( STDMETHODCALLTYPE *IsTemporaryMonoSupported )( 
		IDXGISwapChain1 * This);
        
	HRESULT ( STDMETHODCALLTYPE *GetRestrictToOutput )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  IDXGIOutput **ppRestrictToOutput);
        
	HRESULT ( STDMETHODCALLTYPE *SetBackgroundColor )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  const DXGI_RGBA *pColor);
        
	HRESULT ( STDMETHODCALLTYPE *GetBackgroundColor )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_RGBA *pColor);
        
	HRESULT ( STDMETHODCALLTYPE *SetRotation )( 
		IDXGISwapChain1 * This,
		/* [annotation][in] */ 
		_In_  DXGI_MODE_ROTATION Rotation);
        
	HRESULT ( STDMETHODCALLTYPE *GetRotation )( 
		IDXGISwapChain1 * This,
		/* [annotation][out] */ 
		_Out_  DXGI_MODE_ROTATION *pRotation);
        
	END_INTERFACE
} IDXGISwapChain1Vtbl;

interface IDXGISwapChain1
{
	CONST_VTBL struct IDXGISwapChain1Vtbl *lpVtbl;
};

//
// IDXGIFactory2
//

HRESULT WINAPI CreateDXGIFactory2(
	IN	ULONG	Flags,
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory);

typedef interface IDXGIFactory2 IDXGIFactory2;

typedef struct IDXGIFactory2Vtbl
{
	BEGIN_INTERFACE
        
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
		IDXGIFactory2 * This,
		/* [in] */ REFIID riid,
		/* [annotation][iid_is][out] */ 
		_Out_ void **ppvObject);
        
	ULONG ( STDMETHODCALLTYPE *AddRef )( 
		IDXGIFactory2 * This);
        
	ULONG ( STDMETHODCALLTYPE *Release )( 
		IDXGIFactory2 * This);
        
	HRESULT ( STDMETHODCALLTYPE *SetPrivateData )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [in] */ UINT DataSize,
		/* [annotation][in] */ 
		_In_ const void *pData);
        
	HRESULT ( STDMETHODCALLTYPE *SetPrivateDataInterface )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [annotation][in] */ 
		_In_opt_  const IUnknown *pUnknown);
        
	HRESULT ( STDMETHODCALLTYPE *GetPrivateData )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  REFGUID Name,
		/* [annotation][out][in] */ 
		_Inout_  UINT *pDataSize,
		/* [annotation][out] */ 
		_Out_ void *pData);
        
	HRESULT ( STDMETHODCALLTYPE *GetParent )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  REFIID riid,
		/* [annotation][retval][out] */ 
		_Out_ void **ppParent);
        
	HRESULT ( STDMETHODCALLTYPE *EnumAdapters )( 
		IDXGIFactory2 * This,
		/* [in] */ UINT Adapter,
		/* [annotation][out] */ 
		_Out_ IDXGIAdapter **ppAdapter);
        
	HRESULT ( STDMETHODCALLTYPE *MakeWindowAssociation )( 
		IDXGIFactory2 * This,
		HWND WindowHandle,
		UINT Flags);
        
	HRESULT ( STDMETHODCALLTYPE *GetWindowAssociation )( 
		IDXGIFactory2 * This,
		/* [annotation][out] */ 
		_Out_  HWND *pWindowHandle);
        
	HRESULT ( STDMETHODCALLTYPE *CreateSwapChain )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  IUnknown *pDevice,
		/* [annotation][in] */ 
		_In_  DXGI_SWAP_CHAIN_DESC *pDesc,
		/* [annotation][out] */ 
		_Out_ IDXGISwapChain **ppSwapChain);
        
	HRESULT ( STDMETHODCALLTYPE *CreateSoftwareAdapter )( 
		IDXGIFactory2 * This,
		/* [in] */ HMODULE Module,
		/* [annotation][out] */ 
		_Out_ IDXGIAdapter **ppAdapter);
        
	HRESULT ( STDMETHODCALLTYPE *EnumAdapters1 )( 
		IDXGIFactory2 * This,
		/* [in] */ UINT Adapter,
		/* [annotation][out] */ 
		_Out_ IDXGIAdapter1 **ppAdapter);
        
	BOOL ( STDMETHODCALLTYPE *IsCurrent )( 
		IDXGIFactory2 * This);
        
	BOOL ( STDMETHODCALLTYPE *IsWindowedStereoEnabled )( 
		IDXGIFactory2 * This);
        
	HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForHwnd )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  IUnknown *pDevice,
		/* [annotation][in] */ 
		_In_  HWND hWnd,
		/* [annotation][in] */ 
		_In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
		/* [annotation][in] */ 
		_In_opt_  const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc,
		/* [annotation][in] */ 
		_In_opt_  IDXGIOutput *pRestrictToOutput,
		/* [annotation][out] */ 
		_Out_ IDXGISwapChain1 **ppSwapChain);
        
	HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForCoreWindow )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  IUnknown *pDevice,
		/* [annotation][in] */ 
		_In_  IUnknown *pWindow,
		/* [annotation][in] */ 
		_In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
		/* [annotation][in] */ 
		_In_opt_  IDXGIOutput *pRestrictToOutput,
		/* [annotation][out] */ 
		_Out_ IDXGISwapChain1 **ppSwapChain);
        
	HRESULT ( STDMETHODCALLTYPE *GetSharedResourceAdapterLuid )( 
		IDXGIFactory2 * This,
		/* [annotation] */ 
		_In_  HANDLE hResource,
		/* [annotation] */ 
		_Out_  LUID *pLuid);
        
	HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusWindow )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  HWND WindowHandle,
		/* [annotation][in] */ 
		_In_  UINT wMsg,
		/* [annotation][out] */ 
		_Out_  DWORD *pdwCookie);
        
	HRESULT ( STDMETHODCALLTYPE *RegisterStereoStatusEvent )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  HANDLE hEvent,
		/* [annotation][out] */ 
		_Out_  DWORD *pdwCookie);
        
	void ( STDMETHODCALLTYPE *UnregisterStereoStatus )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  DWORD dwCookie);
        
	HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusWindow )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  HWND WindowHandle,
		/* [annotation][in] */ 
		_In_  UINT wMsg,
		/* [annotation][out] */ 
		_Out_  DWORD *pdwCookie);
        
	HRESULT ( STDMETHODCALLTYPE *RegisterOcclusionStatusEvent )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  HANDLE hEvent,
		/* [annotation][out] */ 
		_Out_  DWORD *pdwCookie);
        
	void ( STDMETHODCALLTYPE *UnregisterOcclusionStatus )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  DWORD dwCookie);
        
	HRESULT ( STDMETHODCALLTYPE *CreateSwapChainForComposition )( 
		IDXGIFactory2 * This,
		/* [annotation][in] */ 
		_In_  IUnknown *pDevice,
		/* [annotation][in] */ 
		_In_  const DXGI_SWAP_CHAIN_DESC1 *pDesc,
		/* [annotation][in] */ 
		_In_opt_  IDXGIOutput *pRestrictToOutput,
		/* [annotation][out] */ 
		_Out_ IDXGISwapChain1 **ppSwapChain);
        
	END_INTERFACE
} IDXGIFactory2Vtbl;

interface IDXGIFactory2
{
	CONST_VTBL struct IDXGIFactory2Vtbl *lpVtbl;
};

#define IDXGIFactory2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDXGIFactory2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDXGIFactory2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDXGIFactory2_SetPrivateData(This,Name,DataSize,pData)	\
    ( (This)->lpVtbl -> SetPrivateData(This,Name,DataSize,pData) ) 

#define IDXGIFactory2_SetPrivateDataInterface(This,Name,pUnknown)	\
    ( (This)->lpVtbl -> SetPrivateDataInterface(This,Name,pUnknown) ) 

#define IDXGIFactory2_GetPrivateData(This,Name,pDataSize,pData)	\
    ( (This)->lpVtbl -> GetPrivateData(This,Name,pDataSize,pData) ) 

#define IDXGIFactory2_GetParent(This,riid,ppParent)	\
    ( (This)->lpVtbl -> GetParent(This,riid,ppParent) ) 


#define IDXGIFactory2_EnumAdapters(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_MakeWindowAssociation(This,WindowHandle,Flags)	\
    ( (This)->lpVtbl -> MakeWindowAssociation(This,WindowHandle,Flags) ) 

#define IDXGIFactory2_GetWindowAssociation(This,pWindowHandle)	\
    ( (This)->lpVtbl -> GetWindowAssociation(This,pWindowHandle) ) 

#define IDXGIFactory2_CreateSwapChain(This,pDevice,pDesc,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChain(This,pDevice,pDesc,ppSwapChain) ) 

#define IDXGIFactory2_CreateSoftwareAdapter(This,Module,ppAdapter)	\
    ( (This)->lpVtbl -> CreateSoftwareAdapter(This,Module,ppAdapter) ) 


#define IDXGIFactory2_EnumAdapters1(This,Adapter,ppAdapter)	\
    ( (This)->lpVtbl -> EnumAdapters1(This,Adapter,ppAdapter) ) 

#define IDXGIFactory2_IsCurrent(This)	\
    ( (This)->lpVtbl -> IsCurrent(This) ) 


#define IDXGIFactory2_IsWindowedStereoEnabled(This)	\
    ( (This)->lpVtbl -> IsWindowedStereoEnabled(This) ) 

#define IDXGIFactory2_CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForHwnd(This,pDevice,hWnd,pDesc,pFullscreenDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForCoreWindow(This,pDevice,pWindow,pDesc,pRestrictToOutput,ppSwapChain) ) 

#define IDXGIFactory2_GetSharedResourceAdapterLuid(This,hResource,pLuid)	\
    ( (This)->lpVtbl -> GetSharedResourceAdapterLuid(This,hResource,pLuid) ) 

#define IDXGIFactory2_RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterStereoStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterStereoStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterStereoStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterStereoStatus(This,dwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusWindow(This,WindowHandle,wMsg,pdwCookie) ) 

#define IDXGIFactory2_RegisterOcclusionStatusEvent(This,hEvent,pdwCookie)	\
    ( (This)->lpVtbl -> RegisterOcclusionStatusEvent(This,hEvent,pdwCookie) ) 

#define IDXGIFactory2_UnregisterOcclusionStatus(This,dwCookie)	\
    ( (This)->lpVtbl -> UnregisterOcclusionStatus(This,dwCookie) ) 

#define IDXGIFactory2_CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain)	\
    ( (This)->lpVtbl -> CreateSwapChainForComposition(This,pDevice,pDesc,pRestrictToOutput,ppSwapChain) )

//
// IDXGIFactory3 (Win8.1+)
//

typedef interface IDXGIFactory3 IDXGIFactory3;

typedef struct {
	IDXGIFactory2Vtbl Base;

	ULONG (STDMETHODCALLTYPE *GetCreationFlags)(IDXGIFactory3 *This);
} IDXGIFactory3Vtbl;

interface IDXGIFactory3 {
	IDXGIFactory3Vtbl *lpVtbl;
};

//
// IDXGIFactory4 (Win10+?)
//

typedef interface IDXGIFactory4 IDXGIFactory4;

typedef struct {
	IDXGIFactory3Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *EnumAdapterByLuid)(IDXGIFactory4 *This, LUID AdapterLuid, REFIID RefIID, PPVOID Adapter);
	HRESULT (STDMETHODCALLTYPE *EnumWarpAdapter)(IDXGIFactory4 *This, REFIID RefIID, PPVOID Adapter);
} IDXGIFactory4Vtbl;

interface IDXGIFactory4 {
	IDXGIFactory4Vtbl *lpVtbl;
};

//
// IDXGIFactory5 (Win10+)
//

typedef enum {
	DXGI_FEATURE_PRESENT_ALLOW_TEARING = 0
} DXGI_FEATURE;

typedef interface IDXGIFactory5 IDXGIFactory5;

typedef struct {
	IDXGIFactory4Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *CheckFeatureSupport)(IDXGIFactory5 *This, DXGI_FEATURE Feature, PVOID FeatureSupportData, ULONG FeatureSupportDataSize);
} IDXGIFactory5Vtbl;

interface IDXGIFactory5 {
	IDXGIFactory5Vtbl *lpVtbl;
};

//
// IDXGIFactory6 (Win10+)
//

typedef enum {
	DXGI_GPU_PREFERENCE_UNSPECIFIED = 0,
	DXGI_GPU_PREFERENCE_MINIMUM_POWER,
	DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE
} DXGI_GPU_PREFERENCE;

typedef interface IDXGIFactory6 IDXGIFactory6;

typedef struct {
	IDXGIFactory5Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *EnumAdapterByGpuPreference)(IDXGIFactory6 *This, ULONG AdapterNumber, DXGI_GPU_PREFERENCE Preference, REFIID RefIID, PPVOID Adapter);
} IDXGIFactory6Vtbl;

interface IDXGIFactory6 {
	IDXGIFactory6Vtbl *lpVtbl;
};

//
// IDXGIFactory7 (Win10+)
//

typedef interface IDXGIFactory7 IDXGIFactory7;

typedef struct {
	IDXGIFactory6Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *RegisterAdaptersChangedEvent)(IDXGIFactory7 *This, HANDLE Event, PULONG Cookie);
	HRESULT (STDMETHODCALLTYPE *UnregisterAdaptersChangedEvent)(IDXGIFactory7 *This, ULONG Cookie);
} IDXGIFactory7Vtbl;

interface IDXGIFactory7 {
	IDXGIFactory7Vtbl *lpVtbl;
};

//
// ID2D1Device (Win7 w/Platform Update)
//

typedef interface ID2D1Device ID2D1Device;

typedef struct {
	ID2D1ResourceVtbl Base;

	// WARNING: All of these methods are incorrect. They take extra parameters beyond what
	// is defined here. If you ever want to call them, you need to fill out the correct parameters.
	HRESULT (STDMETHODCALLTYPE *CreateDeviceContext)(ID2D1Device *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *CreatePrintControl)(ID2D1Device *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *SetMaximumTextureMemory)(ID2D1Device *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *GetMaximumTextureMemory)(ID2D1Device *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *ClearResources)(ID2D1Device *DoNotUse);
} ID2D1DeviceVtbl;

interface ID2D1Device {
	ID2D1DeviceVtbl *lpVtbl;
};

//
// ID2D1Factory1 (Win7 w/Platform Update)
//

typedef interface ID2D1Factory1 ID2D1Factory1;

typedef struct {
	ID2D1FactoryVtbl Base;

	HRESULT (STDMETHODCALLTYPE *CreateDevice)(ID2D1Factory1 *This, IDXGIDevice *DXGIDevice, ID2D1Device **D2D1Device);

	// WARNING: All of these methods are incorrect. They take extra parameters beyond what
	// is defined here. If you ever want to call them, you need to fill out the correct parameters.
	HRESULT (STDMETHODCALLTYPE *CreateStrokeStyle)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *CreatePathGeometry)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *CreateDrawingStateBlock)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *CreateGdiMetafile)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *RegisterEffectFromStream)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *RegisterEffectFromString)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *UnregisterEffect)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *GetRegisteredEffects)(ID2D1Factory1 *DoNotUse);
	HRESULT (STDMETHODCALLTYPE *GetEffectProperties)(ID2D1Factory1 *DoNotUse);
} ID2D1Factory1Vtbl;

interface ID2D1Factory1 {
	ID2D1Factory1Vtbl *lpVtbl;
};

//
// ID2D1Device1 (Win8.1+)
//

typedef interface ID2D1Device1 ID2D1Device1;

typedef enum {
	D2D1_RENDERING_PRIORITY_NORMAL,
	D2D1_RENDERING_PRIORITY_LOW
} D2D1_RENDERING_PRIORITY;

typedef struct {
	ID2D1DeviceVtbl Base;

	D2D1_RENDERING_PRIORITY (STDMETHODCALLTYPE *GetRenderingPriority)(ID2D1Device1 *This);
	VOID (STDMETHODCALLTYPE *SetRenderingPriority)(ID2D1Device1 *This, D2D1_RENDERING_PRIORITY RenderingPriority);

	// there is one undocumented method, CreateDeviceContext. we won't deal with that now.
} ID2D1Device1Vtbl;

interface ID2D1Device1 {
	ID2D1Device1Vtbl		*lpVtbl;
};

//
// ID2D1Factory2 (Win8.1+)
//

typedef interface ID2D1Factory2 ID2D1Factory2;

typedef struct {
	ID2D1Factory1Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *CreateDevice)(ID2D1Factory2 *This, IDXGIDevice *DXGIDevice, ID2D1Device1 **D2D1Device);
} ID2D1Factory2Vtbl;

interface ID2D1Factory2 {
	ID2D1Factory2Vtbl	*lpVtbl;
};

//
// IDXGIFactoryMedia (Win8.1+)
//

typedef interface IDXGIFactoryMedia IDXGIFactoryMedia;

typedef struct {
	IUnknownVtbl Base;

	HRESULT (STDMETHODCALLTYPE *CreateSwapChainForCompositionSurfaceHandle)(IDXGIFactoryMedia *This, IUnknown *Device, HANDLE Surface, CONST DXGI_SWAP_CHAIN_DESC1 *Desc, IDXGIOutput *RestrictToOutput, IDXGISwapChain1 **SwapChain);
	HRESULT (STDMETHODCALLTYPE *CreateDecodeSwapChainForCompositionSurfaceHandle)(IDXGIFactoryMedia *This, IUnknown *Device, HANDLE Surface, PVOID Desc, IDXGIResource *YuvDecodeBuffers, IDXGIOutput *RestrictToOutput, IUnknown **SwapChain);
} IDXGIFactoryMediaVtbl;

interface IDXGIFactoryMedia {
	IDXGIFactoryMediaVtbl *lpVtbl;
};

//
// IDXGIAdapter2 (Win7 with Platform Update)
//

typedef enum {
	DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY = 0,
	DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY = 1,
	DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY = 2,
	DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY = 3,
	DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY = 4
} DXGI_GRAPHICS_PREEMPTION_GRANULARITY;

typedef enum {
	DXGI_COMPUTE_PREEMPTION_DMA_BUFFER_BOUNDARY = 0,
	DXGI_COMPUTE_PREEMPTION_DISPATCH_BOUNDARY = 1,
	DXGI_COMPUTE_PREEMPTION_THREAD_GROUP_BOUNDARY = 2,
	DXGI_COMPUTE_PREEMPTION_THREAD_BOUNDARY = 3,
	DXGI_COMPUTE_PREEMPTION_INSTRUCTION_BOUNDARY = 4
} DXGI_COMPUTE_PREEMPTION_GRANULARITY;

typedef struct {
	WCHAR									Description[128];
	UINT									VendorId;
	UINT									DeviceId;
	UINT									SubSysId;
	UINT									Revision;
	SIZE_T									DedicatedVideoMemory;
	SIZE_T									DedicatedSystemMemory;
	SIZE_T									SharedSystemMemory;
	LUID									AdapterLuid;
	UINT									Flags;
	DXGI_GRAPHICS_PREEMPTION_GRANULARITY	GraphicsPreemptionGranularity;
	DXGI_COMPUTE_PREEMPTION_GRANULARITY		ComputePreemptionGranularity;
} TYPEDEF_TYPE_NAME(DXGI_ADAPTER_DESC2);

typedef interface IDXGIAdapter2 IDXGIAdapter2;

typedef struct {
	IDXGIAdapter1Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *GetDesc2)(IDXGIAdapter2 *This, PDXGI_ADAPTER_DESC2 Desc);
} IDXGIAdapter2Vtbl;

interface IDXGIAdapter2 {
	IDXGIAdapter2Vtbl *lpVtbl;
};

//
// IDXGIAdapter3 (Win10+?)
//

typedef enum {
	DXGI_MEMORY_SEGMENT_GROUP_LOCAL = 0,
	DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL = 1
} DXGI_MEMORY_SEGMENT_GROUP;

typedef struct {
	UINT64 Budget;
	UINT64 CurrentUsage;
	UINT64 AvailableForReservation;
	UINT64 CurrentReservation;
} TYPEDEF_TYPE_NAME(DXGI_QUERY_VIDEO_MEMORY_INFO);

typedef interface IDXGIAdapter3 IDXGIAdapter3;

typedef struct {
	IDXGIAdapter2Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *RegisterHardwareContentProtectionTeardownStatusEvent)(IDXGIAdapter3 *This, HANDLE Event, PULONG Cookie);
	HRESULT (STDMETHODCALLTYPE *UnregisterHardwareContentProtectionTeardownStatus)(IDXGIAdapter3 *This, ULONG Cookie);
	HRESULT (STDMETHODCALLTYPE *QueryVideoMemoryInfo)(IDXGIAdapter3 *This, ULONG NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, PDXGI_QUERY_VIDEO_MEMORY_INFO VideoMemoryInfo);
	HRESULT (STDMETHODCALLTYPE *SetVideoMemoryReservation)(IDXGIAdapter3 *This, ULONG NodeIndex, DXGI_MEMORY_SEGMENT_GROUP MemorySegmentGroup, ULONGLONG Reservation);
	HRESULT (STDMETHODCALLTYPE *RegisterVideoMemoryBudgetChangeNotificationEvent)(IDXGIAdapter3 *This, HANDLE Event, PULONG Cookie);
	HRESULT (STDMETHODCALLTYPE *UnregisterVideoMemoryBudgetChangeNotification)(IDXGIAdapter3 *This, ULONG Cookie);
} IDXGIAdapter3Vtbl;

interface IDXGIAdapter3 {
	IDXGIAdapter3Vtbl *lpVtbl;
};

//
// IDXGIAdapter4 (Win10+)
//

typedef enum {
	DXGI_ADAPTER_FLAG3_NONE = 0,
	DXGI_ADAPTER_FLAG3_REMOTE = 1,
	DXGI_ADAPTER_FLAG3_SOFTWARE = 2,
	DXGI_ADAPTER_FLAG3_ACG_COMPATIBLE = 4,
	DXGI_ADAPTER_FLAG3_SUPPORT_MONITORED_FENCES = 8,
	DXGI_ADAPTER_FLAG3_SUPPORT_NON_MONITORED_FENCES = 0x10,
	DXGI_ADAPTER_FLAG3_KEYED_MUTEX_CONFORMANCE = 0x20,
	DXGI_ADAPTER_FLAG3_FORCE_DWORD = 0xffffffff
} DXGI_ADAPTER_FLAG3;

typedef struct {
	WCHAR									Description[128];
	UINT									VendorId;
	UINT									DeviceId;
	UINT									SubSysId;
	UINT									Revision;
	SIZE_T									DedicatedVideoMemory;
	SIZE_T									DedicatedSystemMemory;
	SIZE_T									SharedSystemMemory;
	LUID									AdapterLuid;
	DXGI_ADAPTER_FLAG3						Flags;
	DXGI_GRAPHICS_PREEMPTION_GRANULARITY	GraphicsPreemptionGranularity;
	DXGI_COMPUTE_PREEMPTION_GRANULARITY		ComputePreemptionGranularity;
} TYPEDEF_TYPE_NAME(DXGI_ADAPTER_DESC3);

typedef interface IDXGIAdapter4 IDXGIAdapter4;

typedef struct {
	IDXGIAdapter3Vtbl Base;

	HRESULT (STDMETHODCALLTYPE *GetDesc3)(IDXGIAdapter4 *This, PDXGI_ADAPTER_DESC3 Desc);
} IDXGIAdapter4Vtbl;

interface IDXGIAdapter4 {
	IDXGIAdapter4Vtbl *lpVtbl;
};

//
// GUIDs
//

DEFINE_GUID(IID_IDXGIDisplayControl,			0xea9dbf1a,0xc88e,0x4486,0x85,0x4a,0x98,0xaa,0x01,0x38,0xf3,0x0c);
DEFINE_GUID(IID_IDXGIOutputDuplication,			0x191cfac3,0xa341,0x470d,0xb2,0x6e,0xa8,0x64,0xf4,0x28,0x31,0x9c);
DEFINE_GUID(IID_IDXGISurface2,					0xaba496dd,0xb617,0x4cb8,0xa8,0x66,0xbc,0x44,0xd7,0xeb,0x1f,0xa2);
DEFINE_GUID(IID_IDXGIResource1,					0x30961379,0x4609,0x4a41,0x99,0x8e,0x54,0xfe,0x56,0x7e,0xe0,0xc1);
DEFINE_GUID(IID_IDXGIDevice2,					0x05008617,0xfbfd,0x4051,0xa7,0x90,0x14,0x48,0x84,0xb4,0xf6,0xa9);
DEFINE_GUID(IID_IDXGISwapChain1,				0x790a45f7,0x0d42,0x4876,0x98,0x3a,0x0a,0x55,0xcf,0xe6,0xf4,0xaa);
DEFINE_GUID(IID_IDXGIFactory2,					0x50c83a1c,0xe072,0x4c48,0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0);
DEFINE_GUID(IID_IDXGIFactory3,					0x25483823,0xcd46,0x4c7d,0x86,0xca,0x47,0xaa,0x95,0xb8,0x37,0xbd);
DEFINE_GUID(IID_IDXGIFactory4,					0x1bc6ea02,0xef36,0x464f,0xbf,0x0c,0x21,0xca,0x39,0xe5,0x16,0x8a);
DEFINE_GUID(IID_IDXGIFactory5,					0x7632e1f5,0xee65,0x4dca,0x87,0xfd,0x84,0xcd,0x75,0xf8,0x83,0x8d);
DEFINE_GUID(IID_IDXGIFactory6,					0xc1b6694f,0xff09,0x44a9,0xb0,0x3c,0x77,0x90,0x0a,0x0a,0x1d,0x17);
DEFINE_GUID(IID_IDXGIFactory7,					0xa4966eed,0x76db,0x44da,0x84,0xc1,0xee,0x9a,0x7a,0xfb,0x20,0xa8);
DEFINE_GUID(IID_IDXGIAdapter,					0x2411e7e1,0x12ac,0x4ccf,0xbd,0x14,0x97,0x98,0xe8,0x53,0x4d,0xc0);
DEFINE_GUID(IID_IDXGIAdapter1,					0x29038f61,0x3839,0x4626,0x91,0xfd,0x08,0x68,0x79,0x01,0x1a,0x05);
DEFINE_GUID(IID_IDXGIAdapter2,					0x0AA1AE0A,0xFA0E,0x4B84,0x86,0x44,0xE0,0x5F,0xF8,0xE5,0xAC,0xB5);
DEFINE_GUID(IID_IDXGIAdapter3,					0x645967A4,0x1392,0x4310,0xA7,0x98,0x80,0x53,0xCE,0x3E,0x93,0xFD);
DEFINE_GUID(IID_IDXGIAdapter4,					0x3c8d99d1,0x4fbf,0x4181,0xa8,0x2c,0xaf,0x66,0xbf,0x7b,0xd2,0x4e);
DEFINE_GUID(IID_IDXGIOutput1,					0x00cddea8,0x939b,0x4b83,0xa3,0x40,0xa6,0x85,0x22,0x66,0x66,0xcc);
DEFINE_GUID(IID_ID2D1Factory1,					0xbb12d362,0xdaee,0x4b9a,0xaa,0x1d,0x14,0xba,0x40,0x1c,0xfa,0x1f);
DEFINE_GUID(IID_ID2D1Factory2,					0x94f81a73,0x9212,0x4376,0x9c,0x58,0xb1,0x6a,0x3a,0x0d,0x39,0x92);
DEFINE_GUID(IID_ID2D1Device1,                   0xd21768e1,0x23a4,0x4823,0xa1,0x4b,0x7c,0x3e,0xba,0x85,0xd6,0x58);
DEFINE_GUID(IID_IDXGIFactoryMedia,				0x41e7d1f2,0xa591,0x4f7b,0xa2,0xe5,0xfa,0x9c,0x84,0x3e,0x1c,0x12);