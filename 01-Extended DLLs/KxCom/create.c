#include "buildcfg.h"
#include "kxcomp.h"
#include <ShObjIdl.h>

STATIC VOID EnsureCOMInitialized(
	VOID)
{
	HRESULT Result;
	SOleTlsData *OleTlsData;

	//
	// Workaround buggy applications which don't call CoInitialize(Ex) before
	// calling functions which return COM objects. That causes a crash, which
	// sometimes does not appear on Win10/11/etc. for unknown reasons and
	// therefore does not get fixed by application developers.
	//
	// Hall of Shame for apps that need this "fix":
	//  - Firefox and forks
	//  - Thunderbird and forks
	//

	OleTlsData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;

	if (!OleTlsData) {
		//
		// When TEB->ReservedForOle == NULL, it means that COM has not been
		// initialized. This condition is a clear indication of a BUG within
		// the target application, since Microsoft documentation clearly states
		// that CoInitialize(Ex) must be called before any other COM-related
		// functions are called.
		//

		Result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		OleTlsData = (SOleTlsData *) NtCurrentTeb()->ReservedForOle;

		ASSERT (SUCCEEDED(Result));
		ASSERT (OleTlsData != NULL);

		KexLogInformationEvent(
			L"Called CoInitializeEx to work around buggy application\r\n\r\n"
			L"CoInitializeEx returned HRESULT: 0x%08lx\r\n"
			L"TEB->ReservedForOle = 0x%p",
			Result, OleTlsData);
	}

	if (OleTlsData && !OleTlsData->pCallCtrl) {
		//
		// Sometimes, CCliModalLoop::CCliModalLoop will crash trying to access
		// a field of OleTlsData->pCallCtrl, because pCallCtrl == NULL. Because it's
		// a simple structure, we can initialize it the exact same way the real code
		// in ole32.dll does it (the real code in ole32.dll that initializes the
		// pCallCtrl structure is called CAptCallCtrl::CAptCallCtrl).
		//
		// This is undoubtedly not the "correct" way to get this structure initialized.
		// I don't know what the correct way is, neither do I know what exactly the
		// applications are doing which triggers this issue with pCallCtrl being NULL.
		//
		// Additional Information: On Windows 10, there's a function exported from
		// combase.dll called InternalSetAptCallCtrlOnTlsIfRequired, which simply
		// initializes OleTlsData->pCallCtrl in much the same way we're doing here.
		//
		// This internal function is imported by ole32.dll. However, it does not seem
		// to get called in any different scenarios other than in the same places
		// CAptCallCtrl::CAptCallCtrl gets called on Windows 7, so I don't know why
		// this pCallCtrl == NULL issue doesn't appear on Windows 10.
		//

		CAptCallCtrl *CallCtrl;

		CallCtrl = SafeAlloc(CAptCallCtrl, 1);
		ASSERT (CallCtrl != NULL);

		if (CallCtrl) {
			IMessageFilter *MessageFilter;

			RtlZeroMemory(CallCtrl, sizeof(*CallCtrl));

			CallCtrl->WindowData[0].Window = HWND_TOPMOST;
			CallCtrl->WindowData[1].FirstMessage = WM_DDE_FIRST;
			CallCtrl->WindowData[1].LastMessage = WM_DDE_LAST;
				
			MessageFilter = OleTlsData->pMsgFilter;
			OleTlsData->pCallCtrl = CallCtrl;
			CallCtrl->MessageFilter = MessageFilter;
			OleTlsData->pMsgFilter = NULL;

			KexLogDebugEvent(
				L"Created missing CAptCallCtrl structure at 0x%p",
				CallCtrl);
		}
	}
}

KXCOMAPI HRESULT WINAPI Ext_CoCreateInstance(
	IN	REFCLSID	RefCLSID,
	IN	LPUNKNOWN	OuterUnknown,
	IN	ULONG		ClassContext,
	IN	REFIID		RefIID,
	OUT	PPVOID		Instance)
{
	HRESULT Result;

	EnsureCOMInitialized();

	Result = CoCreateInstance(
		RefCLSID,
		OuterUnknown,
		ClassContext,
		RefIID,
		Instance);

	if (KexIsDebugBuild) {
		LPOLESTR RefCLSIDAsString;
		LPOLESTR RefIIDAsString;

		StringFromCLSID(RefCLSID, &RefCLSIDAsString);
		StringFromIID(RefIID, &RefIIDAsString);

		KexLogEvent(
			SUCCEEDED(Result) ? LogSeverityDebug : LogSeverityWarning,
			L"%s (%s, %s)\r\n\r\n"
			L"HRESULT error code: 0x%08lx\r\n"
			L"OuterUnknown: %p\r\n"
			L"ClassContext: %lu\r\n"
			L"Instance: %p",
			SUCCEEDED(Result) ? L"Created COM object" : L"Failure to create COM object",
			RefCLSIDAsString, RefIIDAsString,
			Result,
			OuterUnknown,
			ClassContext,
			Instance);

		CoTaskMemFree(RefCLSIDAsString);
		CoTaskMemFree(RefIIDAsString);
	}

	return Result;
}

KXCOMAPI HRESULT WINAPI Ext_CoCreateInstanceEx(
	IN		REFCLSID		RefCLSID,
	IN		LPUNKNOWN		OuterUnknown,
	IN		ULONG			ClassContext,
	IN		COSERVERINFO	*ServerInfo,
	IN		ULONG			NumberOfInterfaces,
	IN OUT	MULTI_QI		*Interfaces)
{
	HRESULT Result;

	EnsureCOMInitialized();

	Result = CoCreateInstanceEx(
		RefCLSID,
		OuterUnknown,
		ClassContext,
		ServerInfo,
		NumberOfInterfaces,
		Interfaces);

	return Result;
}

KXCOMAPI HRESULT WINAPI Ext_CoGetClassObject(
	IN	REFCLSID		RefCLSID,
	IN	ULONG			ClassContext,
	IN	COSERVERINFO	*ServerInfo OPTIONAL,
	IN	REFIID			RefIID,
	OUT	PPVOID			ClassObject)
{
	HRESULT Result;

	EnsureCOMInitialized();

	Result = CoGetClassObject(
		RefCLSID,
		ClassContext,
		ServerInfo,
		RefIID,
		ClassObject);

	if (KexIsDebugBuild) {
		LPOLESTR RefCLSIDAsString;
		LPOLESTR RefIIDAsString;

		StringFromCLSID(RefCLSID, &RefCLSIDAsString);
		StringFromIID(RefIID, &RefIIDAsString);

		KexLogEvent(
			SUCCEEDED(Result) ? LogSeverityDebug : LogSeverityWarning,
			L"%s (%s, %s)\r\n\r\n"
			L"HRESULT error code: 0x%08lx\r\n"
			L"ClassContext: %lu\r\n"
			L"ClassObject: %p",
			SUCCEEDED(Result) ? L"Created class object" : L"Failure to create class object",
			RefCLSIDAsString, RefIIDAsString,
			Result,
			ClassContext,
			ClassObject);

		CoTaskMemFree(RefCLSIDAsString);
		CoTaskMemFree(RefIIDAsString);
	}

	return Result;
}