#include "buildcfg.h"
#include "kxcomp.h"

KXCOMAPI HRESULT WINAPI RoGetActivationFactory(
	IN	HSTRING	ActivatableClassId,
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory)
{
	LPOLESTR RefIIDAsString;

	StringFromIID(RefIID, &RefIIDAsString);

	KexLogDetailEvent(
		L"RoGetActivationFactory called\r\n\r\n"
		L"ActivatableClassId: %s\r\n"
		L"IID:                %s",
		WindowsGetStringRawBuffer(ActivatableClassId, NULL),
		RefIIDAsString);

	CoTaskMemFree(RefIIDAsString);

	return CActivationFactory_QueryInterface(&CActivationFactory, RefIID, Factory);
}

// Added for Cavalry
KXCOMAPI HRESULT WINAPI RoRegisterActivationFactories(
	IN	HSTRING					*ActivatableClassIds,
	IN	PPVOID					ActivationFactoryCallbacks,
	IN	ULONG					Count,
	OUT	PRO_REGISTRATION_COOKIE	Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return E_NOTIMPL;
}

// Added for Cavalry
KXCOMAPI VOID WINAPI RoRevokeActivationFactories(
	IN	RO_REGISTRATION_COOKIE	Cookie)
{
	KexLogUnimplementedFunctionEvent();
	return;
}

KXCOMAPI HRESULT WINAPI RoActivateInstance(
	IN	HSTRING			ActivatableClassId,
	OUT	IInspectable	**Instance)
{
	ASSERT (Instance != NULL);

	KexLogDetailEvent(
		L"RoActivateInstance called: %s",
		WindowsGetStringRawBuffer(ActivatableClassId, NULL));

	*Instance = (IInspectable *) &CActivationFactory;
	return S_OK;
}