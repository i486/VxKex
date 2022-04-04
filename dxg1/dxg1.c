#include <Windows.h>
#include <KexDll.h>
#include <dxgi.h>

DLLAPI HRESULT CreateDXGIFactory2(
	IN	UINT	Flags,
	IN	REFIID	riid,
	OUT	PPVOID	ppFactory)
{
	return CreateDXGIFactory1(riid, ppFactory);
}