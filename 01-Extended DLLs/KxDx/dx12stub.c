#include "buildcfg.h"
#include "kxdxp.h"
#include <D3DCommon.h>

//
// These stubs are intended for applications that have d3d12.dll in the import table,
// but do not actually call them. (Or for applications that have a fallback in case
// D3D12 is not available.)
//

KXDXAPI HRESULT WINAPI D3D12CreateDevice(
	IN	IUnknown			*Adapter OPTIONAL,
	IN	D3D_FEATURE_LEVEL	MinimumFeatureLevel,
	IN	REFIID				RefIID,
	OUT	PPVOID				Device OPTIONAL)
{
	if (Device) {
		*Device = NULL;
	}

	return DXGI_ERROR_UNSUPPORTED;
}

KXDXAPI HRESULT WINAPI D3D12CreateRootSignatureDeserializer(
	IN	PCVOID	SourceData,
	IN	SIZE_T	SourceDataCb,
	IN	REFIID	RefIID,
	OUT	PPVOID	Deserializer)
{
	if (Deserializer) {
		*Deserializer = NULL;
	}

	return DXGI_ERROR_UNSUPPORTED;
}

KXDXAPI HRESULT WINAPI D3D12CreateVersionedRootSignatureDeserializer(
	IN	PCVOID	SourceData,
	IN	SIZE_T	SourceDataCb,
	IN	REFIID	RefIID,
	OUT	PPVOID	Deserializer)
{
	if (Deserializer) {
		*Deserializer = NULL;
	}

	return DXGI_ERROR_UNSUPPORTED;
}

KXDXAPI HRESULT WINAPI D3D12EnableExperimentalFeatures(
	IN	ULONG	NumberOfFeatures,
	IN	REFIID	*RefIIDs,
	IN	PVOID	ConfigurationStructs,
	IN	PULONG	ConfigurationStructSizes)
{
	return DXGI_ERROR_UNSUPPORTED;
}

KXDXAPI HRESULT WINAPI D3D12GetDebugInterface(
	IN	REFIID	RefIID,
	OUT	PPVOID	DebugInterface OPTIONAL)
{
	if (DebugInterface) {
		*DebugInterface = NULL;
	}

	return E_NOINTERFACE;
}

KXDXAPI HRESULT WINAPI D3D12GetInterface(
	IN	REFCLSID	RefCLSID,
	IN	REFIID		RefIID,
	OUT	PPVOID		Interface OPTIONAL)
{
	if (Interface) {
		*Interface = NULL;
	}

	return E_NOINTERFACE;
}

KXDXAPI HRESULT WINAPI D3D12SerializeRootSignature(
	IN	PVOID	RootSignature,
	IN	ULONG	Version,
	OUT	PPVOID	Blob,
	OUT	PPVOID	ErrorBlob OPTIONAL)
{
	*Blob = NULL;

	if (ErrorBlob) {
		*ErrorBlob = NULL;
	}

	return E_NOTIMPL;
}

KXDXAPI HRESULT WINAPI D3D12SerializeVersionedRootSignature(
	IN	PVOID	RootSignature,
	OUT	PPVOID	Blob,
	OUT	PPVOID	ErrorBlob OPTIONAL)
{
	*Blob = NULL;

	if (ErrorBlob) {
		*ErrorBlob = NULL;
	}

	return E_NOTIMPL;
}