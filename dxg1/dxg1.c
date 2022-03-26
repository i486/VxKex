#include <Windows.h>
#include <dxgi.h>
#define DLLAPI __declspec(dllexport)

typedef LPVOID *PPVOID;

DLLAPI HRESULT CreateDXGIFactory2(
	IN	UINT	Flags,
	IN	REFIID	riid,
	OUT	PPVOID	ppFactory)
{
	return CreateDXGIFactory1(riid, ppFactory);
}