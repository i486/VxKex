#include "buildcfg.h"
#include "kxcomp.h"

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_QueryInterface(
	IN	IUriRuntimeClass	*This,
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
		IsEqualIID(RefIID, &IID_IUriRuntimeClass)) {

		*Interface = This;
		InterlockedIncrement(&This->RefCount);
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CUriRuntimeClass_AddRef(
	IN	IUriRuntimeClass	*This)
{
	return InterlockedIncrement(&This->RefCount);
}

ULONG STDMETHODCALLTYPE CUriRuntimeClass_Release(
	IN	IUriRuntimeClass	*This)
{
	ULONG NewRefCount;

	NewRefCount = InterlockedDecrement(&This->RefCount);

	if (NewRefCount == 0) {
		SafeFree(This);
	}

	return NewRefCount;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_GetIids(
	IN	IUriRuntimeClass	*This,
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
	Array[0] = IID_IUriRuntimeClass;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_GetRuntimeClassName(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*ClassName)
{
	PCWSTR Name = L"Windows.Foundation.Uri";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_GetTrustLevel(
	IN	IUriRuntimeClass	*This,
	OUT	TrustLevel			*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_AbsoluteUri(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*AbsoluteUri)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetAbsoluteUri(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), AbsoluteUri);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_DisplayUri(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*DisplayUri)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetDisplayUri(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), DisplayUri);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Domain(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Domain)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetDomain(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Domain);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Extension(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Extension)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetExtension(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Extension);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Fragment(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Fragment)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetFragment(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Fragment);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Host(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Host)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetHost(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Host);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Password(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Password)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetPassword(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Password);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Path(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Path)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetPath(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Path);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Query(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*Query)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetQuery(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), Query);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_QueryParsed(
	IN	IUriRuntimeClass	*This,
	OUT	IUnknown			**QueryParsed)
{
	KexDebugCheckpoint();
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_RawUri(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*RawUri)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetRawUri(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), RawUri);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_SchemeName(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*SchemeName)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetSchemeName(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), SchemeName);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_UserName(
	IN	IUriRuntimeClass	*This,
	OUT	HSTRING				*UserName)
{
	HRESULT Result;
	BSTR Output;

	Result = This->Uri->lpVtbl->GetUserName(This->Uri, &Output);
	if (FAILED(Result)) {
		return Result;
	}

	Result = WindowsCreateString(Output, (ULONG) wcslen(Output), UserName);
	SysFreeString(Output);

	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Port(
	IN	IUriRuntimeClass	*This,
	OUT	PULONG				Port)
{
	return This->Uri->lpVtbl->GetPort(This->Uri, Port);
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_get_Suspicious(
	IN	IUriRuntimeClass	*This,
	OUT	PBOOLEAN			Suspicious)
{
	KexDebugCheckpoint();

	*Suspicious = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_Equals(
	IN	IUriRuntimeClass	*This,
	IN	IUriRuntimeClass	*OtherUri,
	OUT	PBOOLEAN			IsEqual)
{
	HRESULT Result;
	BOOL Equal;

	Result = This->Uri->lpVtbl->IsEqual(This->Uri, OtherUri->Uri, &Equal);

	*IsEqual = Equal;
	return Result;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClass_CombineUri(
	IN	IUriRuntimeClass	*This,
	IN	HSTRING				ExtraPart,
	OUT	IUriRuntimeClass	**NewUri)
{
	KexDebugCheckpoint();
	return E_NOTIMPL;
}

IUriRuntimeClassVtbl CUriRuntimeClassVtbl = {
	CUriRuntimeClass_QueryInterface,
	CUriRuntimeClass_AddRef,
	CUriRuntimeClass_Release,
	CUriRuntimeClass_GetIids,
	CUriRuntimeClass_GetRuntimeClassName,
	CUriRuntimeClass_GetTrustLevel,

	CUriRuntimeClass_get_AbsoluteUri,
	CUriRuntimeClass_get_DisplayUri,
	CUriRuntimeClass_get_Domain,
	CUriRuntimeClass_get_Extension,
	CUriRuntimeClass_get_Fragment,
	CUriRuntimeClass_get_Host,
	CUriRuntimeClass_get_Password,
	CUriRuntimeClass_get_Path,
	CUriRuntimeClass_get_Query,
	CUriRuntimeClass_get_QueryParsed,
	CUriRuntimeClass_get_RawUri,
	CUriRuntimeClass_get_SchemeName,
	CUriRuntimeClass_get_UserName,
	CUriRuntimeClass_get_Port,
	CUriRuntimeClass_get_Suspicious,
	CUriRuntimeClass_Equals,
	CUriRuntimeClass_CombineUri
};

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_QueryInterface(
	IN	IUriRuntimeClassFactory	*This,
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
		IsEqualIID(RefIID, &IID_IUriRuntimeClassFactory)) {

		*Interface = This;
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE CUriRuntimeClassFactory_AddRef(
	IN	IUriRuntimeClassFactory	*This)
{
	return 1;
}

ULONG STDMETHODCALLTYPE CUriRuntimeClassFactory_Release(
	IN	IUriRuntimeClassFactory	*This)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_GetIids(
	IN	IUriRuntimeClassFactory	*This,
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
	Array[0] = IID_IUriRuntimeClassFactory;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_GetRuntimeClassName(
	IN	IUriRuntimeClassFactory	*This,
	OUT	HSTRING					*ClassName)
{
	PCWSTR Name = L"Windows.Foundation.Uri";
	ASSERT (ClassName != NULL);
	return WindowsCreateString(Name, (ULONG) wcslen(Name), ClassName);
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_GetTrustLevel(
	IN	IUriRuntimeClassFactory	*This,
	OUT	TrustLevel				*Level)
{
	ASSERT (Level != NULL);
	*Level = BaseTrust;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_CreateUri(
	IN	IUriRuntimeClassFactory	*This,
	IN	HSTRING					Uri,
	OUT	IUriRuntimeClass		**UriRuntimeClass)
{
	HRESULT Result;
	IUriRuntimeClass *RuntimeClass;

	ASSERT (UriRuntimeClass != NULL);

	RuntimeClass = SafeAlloc(IUriRuntimeClass, 1);
	if (!RuntimeClass) {
		return E_OUTOFMEMORY;
	}

	RuntimeClass->lpVtbl = &CUriRuntimeClassVtbl;
	RuntimeClass->RefCount = 1;

	Result = CreateUri(
		WindowsGetStringRawBuffer(Uri, NULL),
		Uri_CREATE_NO_IE_SETTINGS | Uri_CREATE_PRE_PROCESS_HTML_URI |
		Uri_CREATE_CRACK_UNKNOWN_SCHEMES | Uri_CREATE_CANONICALIZE |
		Uri_CREATE_NO_DECODE_EXTRA_INFO | Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME,
		0,
		&RuntimeClass->Uri);

	if (FAILED(Result)) {
		KexDebugCheckpoint();
		SafeFree(RuntimeClass);
		return Result;
	}

	ASSERT (RuntimeClass != NULL);
	*UriRuntimeClass = RuntimeClass;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CUriRuntimeClassFactory_CreateWithRelativeUri(
	IN	IUriRuntimeClassFactory	*This,
	IN	HSTRING					BaseUri,
	IN	HSTRING					RelativeUri,
	OUT	IUriRuntimeClass		**UriRuntimeClass)
{
	KexDebugCheckpoint();
	return E_NOTIMPL;
}

IUriRuntimeClassFactoryVtbl CUriRuntimeClassFactoryVtbl = {
	CUriRuntimeClassFactory_QueryInterface,
	CUriRuntimeClassFactory_AddRef,
	CUriRuntimeClassFactory_Release,
	CUriRuntimeClassFactory_GetIids,
	CUriRuntimeClassFactory_GetRuntimeClassName,
	CUriRuntimeClassFactory_GetTrustLevel,
	CUriRuntimeClassFactory_CreateUri,
	CUriRuntimeClassFactory_CreateWithRelativeUri
};

IUriRuntimeClassFactory CUriRuntimeClassFactory = {
	&CUriRuntimeClassFactoryVtbl
};