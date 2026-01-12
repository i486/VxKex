#include "buildcfg.h"
#include "kxdxp.h"
#include <KexW32ML.h>
#include <dxgi.h>
#include <d2d1.h>

HRESULT WINAPI Ext_CreateDXGIFactory1(
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory)
{
	if (IsEqualIID(RefIID, &IID_IDXGIFactoryMedia)) {
		return CreateIDXGIFactoryMedia(Factory);
	}

	return CreateDXGIFactory2(0, RefIID, Factory);
}

//
// Function introduced in Windows 8.1.
// The IDXGIFactory2 interface is actually supported in Windows 7 but only with
// Platform Update.
//
HRESULT WINAPI CreateDXGIFactory2(
	IN	ULONG	Flags,
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory)
{
	HRESULT Result;

	Result = CreateDXGIFactory1(
		RefIID,
		Factory);

	if (FAILED(Result)) {
		if (IsEqualIID(RefIID, &IID_IDXGIFactory2)) {
			KexLogErrorEvent(
				L"Failed to create IDXGIFactory2\r\n\r\n"
				L"HRESULT error code: 0x%08lx: %s\r\n"
				L"This interface is only supported by Windows 7 with Platform Update. "
				L"In order to solve this error, install Platform Update.",
				Result, Win32ErrorAsString(Result));
		} else if (IsEqualIID(RefIID, &IID_IDXGIFactory3) ||
				   IsEqualIID(RefIID, &IID_IDXGIFactory4) ||
				   IsEqualIID(RefIID, &IID_IDXGIFactory5) ||
				   IsEqualIID(RefIID, &IID_IDXGIFactory6) ||
				   IsEqualIID(RefIID, &IID_IDXGIFactory7)) {

			KexLogInformationEvent(
				L"Redirected unsupported DXGI factory to IDXGIFactory2 + wrapper\r\n\r\n"
				L"HRESULT error code: 0x%08lx: %s",
				Result, Win32ErrorAsString(Result));

			Result = CreateDXGIFactory2(0, &IID_IDXGIFactory2, Factory);

			if (SUCCEEDED(Result)) {
				WrapDXGIFactory((IDXGIFactory2 *) *Factory);
			}
		} else {
			//
			// TODO: Paint.NET depends on IDXGIFactory7.
			// See if we can implement that in a satisfactory way.
			//

			ReportNoInterfaceError(RefIID, Result, L"Failed to create DXGI factory");
		}
	}

	return Result;
}

//
// Added in order to support ID2D1Factory2.
//
// ID2D1Factory (Windows 7 & Windows Vista with SP2 and Platform Update)
// ID2D1Factory1 (Windows 8 & Windows 7 with Platform Update)
// ID2D1Factory2 (Windows 8.1)
//
// Cinebench 2024 depends on ID2D1Factory2.
//

HRESULT WINAPI Ext_D2D1CreateFactory(
	IN	D2D1_FACTORY_TYPE			FactoryType,
	IN	REFIID						RefIID,
	IN	CONST D2D1_FACTORY_OPTIONS	*FactoryOptions,
	OUT	PPVOID						Factory)
{
	HRESULT Result;

	Result = D2D1CreateFactory(
		FactoryType,
		RefIID,
		FactoryOptions,
		Factory);

	if (FAILED(Result)) {
		if (IsEqualIID(RefIID, &IID_ID2D1Factory1)) {
			KexLogErrorEvent(
				L"Failed to create ID2D1Factory1\r\n\r\n"
				L"HRESULT error code: 0x%08lx: %s\r\n"
				L"This interface is only supported by Windows 7 with Platform Update. "
				L"In order to solve this error, install Platform Update.",
				Result, Win32ErrorAsString(Result));
		} else if (IsEqualIID(RefIID, &IID_ID2D1Factory2)) {
			KexLogInformationEvent(
				L"Failed to create ID2D1Factory2\r\n\r\n"
				L"HRESULT error code: 0x%08lx: %s\r\n"
				L"Redirecting to ID2D1Factory1",
				Result, Win32ErrorAsString(Result));

			Result = Ext_D2D1CreateFactory(
				FactoryType,
				&IID_ID2D1Factory1,
				FactoryOptions,
				Factory);

			//
			// Use our helper function to "wrap" the system-provided ID2D1Factory1
			// into our ID2D1Factory2 shim
			//

			if (SUCCEEDED(Result)) {
				WrapDirect2DFactory((ID2D1Factory1 *) *Factory);
			}
		} else {
			HRESULT Result2;
			BSTR IidAsString;

			Result2 = StringFromIID(RefIID, &IidAsString);
			ASSERT (SUCCEEDED(Result2));

			if (FAILED(Result2)) {
				IidAsString = L"{unknown}";
			}

			KexLogErrorEvent(
				L"Failed to create Direct2D factory: %s\r\n\r\n"
				L"HRESULT error code: 0x%08lx: %s",
				IidAsString,
				Result, Win32ErrorAsString(Result));

			KexDebugCheckpoint();

			if (SUCCEEDED(Result2)) {
				CoTaskMemFree(IidAsString);
			}
		}
	}

	return Result;
}