#include "buildcfg.h"
#include "kxcomp.h"

KXCOMAPI HRESULT WINAPI RoInitialize(
	IN	RO_INIT_TYPE	InitType)
{
	if (InitType == RO_INIT_SINGLETHREADED) {
		return CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	} else if (InitType == RO_INIT_MULTITHREADED) {
		return CoInitializeEx(NULL, COINIT_MULTITHREADED);
	} else {
		return E_INVALIDARG;
	}
}

KXCOMAPI VOID WINAPI RoUninitialize(
	VOID)
{
	// yes, this is all it does on win8.
	CoUninitialize();
}