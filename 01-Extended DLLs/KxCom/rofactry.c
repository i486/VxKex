#include "buildcfg.h"
#include "kxcomp.h"
#include <InitGuid.h>

// {01BF4326-ED37-4E96-B0E9-C1340D1EA158}
DEFINE_GUID(IID_IGlobalizationPreferencesStatics, 0x01BF4326, 0xED37, 0x4E96, 0xB0, 0xE9, 0xC1, 0x34, 0x0D, 0x1E, 0xA1, 0x58);

HRESULT WINAPI RoGetActivationFactory(
	IN	HSTRING	ActivatableClassId,
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory)
{
	LPOLESTR RefIIDAsString;

	StringFromIID(RefIID, &RefIIDAsString);

	KexLogDetailEvent(
		L"RoGetActivationFactory called\r\n\r\n"
		L"ActivatableClassId: %s\r\n"
		L"RefIID:             %s",
		WindowsGetStringRawBuffer(ActivatableClassId, NULL),
		RefIIDAsString);

	CoTaskMemFree(RefIIDAsString);

	// APPSPECIFICHACK: Make spectralayers FILE PICKER work without crashing.
	// The problem lies within Qt6CoreDF.dll, and there is a fallback to use normal
	// Win32 APIs. However it seems to be a debug build with ASSERTIONS enabled
	// which means that if we just return E_NOTIMPL it will throw an exception and
	// crash the program...
	//
	// Note: This ASH is not required unless the QT_QPA_PLATFORMTHEME ASH is also
	// applied to SpectraLayers. Thankfully we don't need it yet, perhaps the version
	// of Qt6 isn't high enough to need it.
	// Commenting out the code for now.

#if 0
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (AshExeBaseNameIs(L"SpectraLayers.exe")) {
			if (IsEqualIID(RefIID, &IID_IGlobalizationPreferencesStatics)) {
				*Factory = &CGlobalizationPreferencesStatics;
				return S_OK;
			}
		}
	}
#endif

	return E_NOTIMPL;
}