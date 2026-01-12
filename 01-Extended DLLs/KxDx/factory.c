#include "buildcfg.h"
#include "kxdxp.h"
#include <KexW32ML.h>
#include <dxgi.h>

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
				L"HRESULT error code: 0x%08lx (%s)\r\n"
				L"This interface is only supported by Windows 7 with Platform Update. "
				L"In order to solve this error, install Platform Update.",
				Result, Win32ErrorAsString(Result));
		}
	}

	return Result;
}