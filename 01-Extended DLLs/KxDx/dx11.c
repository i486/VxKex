#include "buildcfg.h"
#include "kxdxp.h"
#include <d3d11.h>

//
// Some games call D3D11CreateDevice with DX12 feature levels. This will fail
// on Windows 7 but works on Windows 10.
//
KXDXAPI HRESULT WINAPI Ext_D3D11CreateDevice(
	IN	IDXGIAdapter			*pAdapter OPTIONAL,
	IN	D3D_DRIVER_TYPE			DriverType,
	IN	HMODULE					Software,
	IN	UINT					Flags,
	IN	const D3D_FEATURE_LEVEL	*pFeatureLevels OPTIONAL,
	IN	UINT					nFeatureLevels,
	IN	UINT					SDKVersion,
	OUT	ID3D11Device			**ppDevice OPTIONAL,
	OUT	D3D_FEATURE_LEVEL		*pFeatureLevel OPTIONAL,
	OUT	ID3D11DeviceContext		**ppImmediateContext)
{
	if (pFeatureLevels != NULL && nFeatureLevels > 0) {
		D3D_FEATURE_LEVEL *pNewFeatureLevels;
		UINT nNewFeatureLevels;
		ULONG Index;

		//
		// Remove any feature levels not supported on Windows 7
		//

		pNewFeatureLevels = StackAlloc(D3D_FEATURE_LEVEL, nFeatureLevels);
		nNewFeatureLevels = 0;

		for (Index = 0; Index < nFeatureLevels; ++Index) {
			if (pFeatureLevels[Index] <= D3D_FEATURE_LEVEL_11_1) {
				pNewFeatureLevels[nNewFeatureLevels++] = pFeatureLevels[Index];
			}
		}

		if (nNewFeatureLevels != nFeatureLevels) {
			pFeatureLevels = pNewFeatureLevels;
			nFeatureLevels = nNewFeatureLevels;
		}
	}

	return D3D11CreateDevice(
		pAdapter,
		DriverType,
		Software,
		Flags,
		pFeatureLevels,
		nFeatureLevels,
		SDKVersion,
		ppDevice,
		pFeatureLevel,
		ppImmediateContext);
}