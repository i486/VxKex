#include "buildcfg.h"
#include "kxdxp.h"
#include <KexW32ML.h>
#include <dxgi.h>
#include <d2d1.h>

VOID ReportNoInterfaceError(
	IN	REFIID	RefIID,
	IN	HRESULT	Result,
	IN	PCWSTR	ErrorMessage)
{
	HRESULT Result2;
	BSTR IidAsString;

	Result2 = StringFromIID(RefIID, &IidAsString);
	ASSERT (SUCCEEDED(Result2));

	if (FAILED(Result2)) {
		IidAsString = L"{unknown}";
	}

	KexLogErrorEvent(
		L"%s: %s\r\n\r\n"
		L"HRESULT error code: 0x%08lx: %s",
		ErrorMessage, IidAsString,
		Result, Win32ErrorAsString(Result));

	KexDebugCheckpoint();

	if (SUCCEEDED(Result2)) {
		CoTaskMemFree(IidAsString);
	}
}