#pragma once
#include <KexComm.h>
#include <dxgi.h>
#include <dxgitype.h>
#include <d3d9types.h>

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
// GUIDs
//

DEFINE_GUID(IID_IDXGIDisplayControl,0xea9dbf1a,0xc88e,0x4486,0x85,0x4a,0x98,0xaa,0x01,0x38,0xf3,0x0c);
DEFINE_GUID(IID_IDXGIOutputDuplication,0x191cfac3,0xa341,0x470d,0xb2,0x6e,0xa8,0x64,0xf4,0x28,0x31,0x9c);
DEFINE_GUID(IID_IDXGISurface2,0xaba496dd,0xb617,0x4cb8,0xa8,0x66,0xbc,0x44,0xd7,0xeb,0x1f,0xa2);
DEFINE_GUID(IID_IDXGIResource1,0x30961379,0x4609,0x4a41,0x99,0x8e,0x54,0xfe,0x56,0x7e,0xe0,0xc1);
DEFINE_GUID(IID_IDXGIDevice2,0x05008617,0xfbfd,0x4051,0xa7,0x90,0x14,0x48,0x84,0xb4,0xf6,0xa9);
DEFINE_GUID(IID_IDXGISwapChain1,0x790a45f7,0x0d42,0x4876,0x98,0x3a,0x0a,0x55,0xcf,0xe6,0xf4,0xaa);
DEFINE_GUID(IID_IDXGIFactory2,0x50c83a1c,0xe072,0x4c48,0x87,0xb0,0x36,0x30,0xfa,0x36,0xa6,0xd0);
DEFINE_GUID(IID_IDXGIAdapter2,0x0AA1AE0A,0xFA0E,0x4B84,0x86,0x44,0xE0,0x5F,0xF8,0xE5,0xAC,0xB5);
DEFINE_GUID(IID_IDXGIOutput1,0x00cddea8,0x939b,0x4b83,0xa3,0x40,0xa6,0x85,0x22,0x66,0x66,0xcc);