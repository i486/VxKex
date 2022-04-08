#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>

// This function resolves an API set DLL (e.g. api-ms-win-core-synch-l1-2-0.dll)
// to a "real" DLL (e.g. kernel33.dll).
// This implementation of the function ignores everything that the "real" function
// in NTDLL does, and just uses the DllRewrite registry entry instead, since we are
// in a VxKex environment anyway and all this work is already done.
NTSTATUS NTAPI ApiSetResolveToHost(
	IN	PAPI_SET_NAMESPACE	ApiNamespace,
	IN	PUNICODE_STRING		ApiToResolve,
	IN	PUNICODE_STRING		ParentName OPTIONAL,
	OUT	PBOOLEAN			Resolved,
	OUT	PUNICODE_STRING		Output)
{
	LPCWSTR lpszApiSetDll = ApiToResolve->Buffer;

	ODS_ENTRY(L"(%p, (UNICODE_STRING) \"%ws\", %p, %p, %p)", ApiNamespace, ApiToResolve->Buffer, ParentName, Resolved, Output);

	if (wcsnicmp(ApiToResolve->Buffer, L"api-", 4) && wcsnicmp(ApiToResolve->Buffer, L"ext-", 4)) {
		// The name of the API set DLL must begin with api- on Windows 7
		// The ext- prefix is available starting from Windows 8
		*Resolved = FALSE;
	} else {
		WCHAR szRealDll[MAX_PATH];
		*Resolved = RegReadSz(HKEY_LOCAL_MACHINE, L"SOFTWARE\\VXsoft\\VxKexLdr\\DllRewrite", lpszApiSetDll, szRealDll, ARRAYSIZE(szRealDll));

		if (*Resolved) {
			RtlInitUnicodeString(Output, szRealDll);
		}
	}

	return STATUS_SUCCESS;
}

DLLAPI BOOL WINAPI ApiSetQueryApiSetPresence(
	IN	PUNICODE_STRING	Namespace,
	OUT	PBOOLEAN		Present)
{
	PPEB ppeb = NtCurrentPeb();
	BOOL bResult;
	UNICODE_STRING ApiSetHost;

	ODS_ENTRY(L"((UNICODE_STRING) \"%ws\", %p)", Namespace->Buffer, Present);

	if (Namespace->Length < 8) {
		*Present = FALSE;
		bResult = FALSE;
	} else {
		NTSTATUS st = ApiSetResolveToHost(ppeb->ApiSetMap, Namespace, NULL, Present, &ApiSetHost);

		if (NT_SUCCESS(st)) {
			if (*Present && ApiSetHost.Length == 0) {
				*Present = FALSE;
			}

			bResult = FALSE;
		} else {
			bResult = st;
		}
	}

	return bResult;
}