#include <Windows.h>
#define DLLAPI __declspec(dllexport)
DECLARE_HANDLE(HSTRING);

typedef LPVOID *PPVOID;

typedef enum RO_INIT_TYPE {
	RO_INIT_SINGLETHREADED,
	RO_INIT_MULTITHREADED
} RO_INIT_TYPE;

DLLAPI HRESULT RoGetActivationFactory(
	IN	HSTRING	activatableClassId,
	IN	REFIID	riid,
	IN	PPVOID	factory)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT RoInitialize(
	IN	RO_INIT_TYPE	initType)
{
	if (initType == RO_INIT_SINGLETHREADED) {
		return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	} else {
		return CoInitializeEx(NULL, COINIT_MULTITHREADED);
	}
}

DLLAPI VOID RoUninitialize(
	VOID)
{
	CoUninitialize();
	return;
}

DLLAPI HRESULT RoGetMatchingRestrictedErrorInfo(
	HRESULT	hrIn,
	PPVOID	ppRestrictedErrorInfo)
{
	return E_NOTIMPL;
}

DLLAPI BOOL RoOriginateLanguageException(
	HRESULT error,
	HSTRING message,
	LPVOID languageException)
{
	return TRUE;
}

typedef struct HSTRING_HEADER {
	union {
		LPVOID	Reserved1;
		CHAR	Reserved2[24];
	} Reserved;
} HSTRING_HEADER;

DLLAPI HRESULT WindowsCreateString(
	PCNZWCH sourceString,
	UINT32 length,
	HSTRING *string)
{
	return E_NOTIMPL;
}

DLLAPI PCWSTR WindowsGetStringRawBuffer(
	HSTRING string,
	UINT32 *length)
{
	return L"Windows String";
}

DLLAPI HRESULT WindowsDeleteString(
	HSTRING string)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT WindowsCreateStringReference(
	PCWSTR sourceString,
	UINT32 length,
	HSTRING_HEADER *hstringHeader,
	HSTRING *string)
{
	return E_NOTIMPL;
}

DLLAPI BOOL WindowsIsStringEmpty(
	IN	HSTRING	string)
{
	return TRUE;
}

DLLAPI HRESULT WindowsStringHasEmbeddedNull(
	IN	HSTRING	string,
	OUT	LPBOOL	hasEmbedNull)
{
	*hasEmbedNull = FALSE;
	return S_OK;
}

DLLAPI HRESULT SetRestrictedErrorInfo(
	PVOID pRestrictedErrorInfo)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT GetRestrictedErrorInfo(
	PVOID pRestrictedErrorInfo)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT RoOriginateError(
	IN	HRESULT	error,
	IN	HSTRING	message)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT RoOriginateErrorW(
	IN	HRESULT	error,
	IN	UINT	cchMax,
	IN	PCWSTR	message)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT RoRegisterActivationFactories(
	IN	HSTRING	*activatableClassIds,
	IN	PPVOID	*activationFactoryCallbacks,
	IN	UINT32	count,
	OUT	PPVOID	cookie)
{
	return E_NOTIMPL;
}

DLLAPI HRESULT RoRevokeActivationFactories(
	IN	LPVOID	cookie)
{
	return E_NOTIMPL;
}