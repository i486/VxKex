#include "buildcfg.h"
#include "kxdxp.h"
#include <d3d11.h>

KXDXAPI HRESULT WINAPI D3D11On12CreateDevice(
	IN		IUnknown					*pDevice,
	IN		UINT						Flags,
	IN		CONST D3D_FEATURE_LEVEL		*pFeatureLevels OPTIONAL,
	IN		UINT						FeatureLevels,
	IN		IUnknown					*CONST *ppCommandQueues OPTIONAL,
	IN		UINT						NumQueues,
	IN		UINT						NodeMask,
	OUT		ID3D11Device				**ppDevice OPTIONAL,
	OUT		ID3D11DeviceContext			**ppImmediateContext OPTIONAL,
	OUT		D3D_FEATURE_LEVEL			*pChosenFeatureLevel OPTIONAL)
{
	KexLogWarningEvent(L"Unsupported function D3D11On12CreateDevice called");
	KexDebugCheckpoint();

	return E_NOTIMPL;
}