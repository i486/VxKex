#include "buildcfg.h"
#include "kxcomp.h"
#include <ShellAPI.h>

HRESULT STDMETHODCALLTYPE CLauncherStatics_QueryInterface(
	IN	ILauncherStatics	*This,
	IN	REFIID				RefIID,
	OUT	PPVOID				Interface)
{
	ASSERT (This != NULL);
	ASSERT (RefIID != NULL);
	ASSERT (Interface != NULL);

	*Interface = NULL;

	if (IsEqualIID(RefIID, &IID_IUnknown) ||
		IsEqualIID(RefIID, &IID_IAgileObject) ||
		IsEqualIID(RefIID, &IID_IInspectable) ||
		IsEqualIID(RefIID, &IID_ILauncherStatics)) {

		*Interface = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CLauncherStatics_AddRef(
	IN	ILauncherStatics	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CLauncherStatics_Release(
	IN	ILauncherStatics	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_GetIids(
	IN	ILauncherStatics	*This,
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
	Array[0] = IID_ILauncherStatics;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_GetRuntimeClassName(
	IN	ILauncherStatics	*This,
	OUT	HSTRING				*ClassName)
{
	PCWSTR Name = L"Windows.System.Launcher";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_GetTrustLevel(
	IN	ILauncherStatics	*This,
	OUT	TrustLevel			*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_LaunchFileAsync(
	IN	ILauncherStatics	*This,
	IN	IUnknown			*StorageFile,
	OUT	IAsyncOperation		**AsyncOperation)
{
	KexDebugCheckpoint(); // not properly implemented
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_LaunchFileWithOptionsAsync(
	IN	ILauncherStatics	*This,
	IN	IUnknown			*StorageFile,
	IN	IUnknown			*LauncherOptions,
	OUT	IAsyncOperation		**AsyncOperation)
{
	KexDebugCheckpoint(); // not properly implemented
	return CLauncherStatics_LaunchFileAsync(This, StorageFile, AsyncOperation);
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_LaunchUriAsync(
	IN	ILauncherStatics	*This,
	IN	IUriRuntimeClass	*Uri,
	OUT	IAsyncOperation		**AsyncOperation)
{
	HRESULT Result;
	HSTRING RawUri;
	HINSTANCE ShellExecuteResult;

	Result = Uri->lpVtbl->get_AbsoluteUri(Uri, &RawUri);
	if (FAILED(Result)) {
		return Result;
	}

	ShellExecuteResult = ShellExecute(
		NULL,
		L"open",
		WindowsGetStringRawBuffer(RawUri, NULL),
		NULL,
		NULL,
		SW_SHOW);

	WindowsDeleteString(RawUri);

	if (((ULONG) ShellExecuteResult) <= 32) {
		return E_FAIL;
	}

	// HACK: Some apps (e.g. Spotify) require a valid pointer to be passed out.
	// However it seems to do fine if we just pass a pointer to some random
	// interface such as ourselves.
	*AsyncOperation = (IAsyncOperation *) This;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CLauncherStatics_LaunchUriWithOptionsAsync(
	IN	ILauncherStatics	*This,
	IN	IUriRuntimeClass	*Uri,
	IN	IUnknown			*LauncherOptions,
	OUT	IAsyncOperation		**AsyncOperation)
{
	KexDebugCheckpoint(); // not properly implemented
	return CLauncherStatics_LaunchUriAsync(This, Uri, AsyncOperation);
}

ILauncherStaticsVtbl CLauncherStaticsVtbl = {
	CLauncherStatics_QueryInterface,
	CLauncherStatics_AddRef,
	CLauncherStatics_Release,
	CLauncherStatics_GetIids,
	CLauncherStatics_GetRuntimeClassName,
	CLauncherStatics_GetTrustLevel,
	CLauncherStatics_LaunchFileAsync,
	CLauncherStatics_LaunchFileWithOptionsAsync,
	CLauncherStatics_LaunchUriAsync,
	CLauncherStatics_LaunchUriWithOptionsAsync
};

ILauncherStatics CLauncherStatics = {
	&CLauncherStaticsVtbl
};