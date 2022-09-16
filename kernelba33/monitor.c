#include <Windows.h>
#include <KexComm.h>
#include <KexDll.h>
#include <NtDll.h>
#include "module.h"

#include <Shlwapi.h>

//
// Functions related to DLL load monitoring and import rewriting
//

LPVOID lpCookie;

BOOLEAN ShouldRewriteImportsOfDll(
	IN	LPCWSTR	lpszDllFullName)
{
	WCHAR szDllDir[MAX_PATH];

	if (!lpszDllFullName || !wcslen(lpszDllFullName) || PathIsRelative(lpszDllFullName)) {
		// just be safe and don't rewrite it, since we don't even know what it is
		return FALSE;
	}

	wcscpy_s(szDllDir, ARRAYSIZE(szDllDir), lpszDllFullName);
	PathRemoveFileSpec(szDllDir);

	if (PathIsPrefix(SharedUserData->NtSystemRoot, szDllDir) || PathIsPrefix(KexData->KexDir, szDllDir)) {
		// don't rewrite any imports if the particular DLL is in the Windows folder
		// or the kernel extensions folder
		return FALSE;
	}

	return TRUE;
}

VOID RewriteImportsOfDll(
	IN	LPCWSTR	DllBaseName,
	IN	LPVOID	DllBase)
{
	PIMAGE_DOS_HEADER				DosHdr;
	PIMAGE_NT_HEADERS				NtHdrs;
	PIMAGE_FILE_HEADER				CoffHdr;
	PIMAGE_OPTIONAL_HEADER			OptHdr;
	PIMAGE_IMPORT_DESCRIPTOR		ImportDescriptor;
	BOOL							DllIs64Bit;
	BOOL							RewriteOccurred;
	DWORD							OldMemoryProtection;
	
	DosHdr = (PIMAGE_DOS_HEADER) DllBase;
	NtHdrs = (PIMAGE_NT_HEADERS) RVA_TO_VA(DllBase, DosHdr->e_lfanew);
	CoffHdr = &NtHdrs->FileHeader;
	OptHdr = &NtHdrs->OptionalHeader;
	ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR) RVA_TO_VA(DllBase, OptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	if (CoffHdr->Machine == 0x8664) {
		DllIs64Bit = TRUE;
	}

	if (DllIs64Bit != PROCESS_IS_64BIT) {
		KexRaiseHardError(L"Architecture Mismatch", NULL,
						  L"The %d-bit %s was loaded into a %d-bit application. "
						  L"The bitness of DLLs and applications must match for successful program operation.",
						  DllIs64Bit ? 64 : 32, DllBaseName, PROCESSBITS);
	}

	if (OptHdr->NumberOfRvaAndSizes < IMAGE_DIRECTORY_ENTRY_IMPORT || ImportDescriptor == DllBase) {
		// no import directory - we don't need to do anything
		return;
	}

	RewriteOccurred = FALSE;

	while (ImportDescriptor->Name != 0) {
		LPSTR NameOfImportedDll;
		WCHAR NameOfImportedDllU[MAX_PATH];
		CHAR NameOfRewrittenDll[MAX_PATH];

		NameOfImportedDll = (LPSTR) RVA_TO_VA(DllBase, ImportDescriptor->Name);
		
		if (MultiByteToWideChar(CP_ACP, 0, NameOfImportedDll, -1, NameOfImportedDllU,
								ARRAYSIZE(NameOfImportedDllU)) == 0) {
			ODS(L"MultiByteToWideChar failed: %s", GetLastErrorAsString());
			goto SoftFail;
		}

		// 99% of the time, DLL names already contain the .dll extension.
		// However, technically, it is legal to not have it, so we have to ensure it is there,
		// since all the DLL rewrite entries in the registry assume it.
		PathAddExtension(NameOfImportedDllU, L".dll");
		
		if (KexRewriteDllNameW(NameOfImportedDllU)) {
			UINT LengthOfOriginalDllName;
			UINT LengthOfRewrittenDllName;
			BOOL SubstitutionCharacterWasUsed;

			LengthOfOriginalDllName = lstrlenA(NameOfImportedDll);
			LengthOfRewrittenDllName = lstrlenW(NameOfImportedDllU);

			if (LengthOfRewrittenDllName > LengthOfOriginalDllName) {
				ODS(L"Rewritten DLL name of %hs has a greater length than the original", NameOfImportedDll);
				goto SoftFail;
			}

			if (WideCharToMultiByte(CP_ACP, 0, NameOfImportedDllU, LengthOfRewrittenDllName, NameOfRewrittenDll,
									LengthOfRewrittenDllName + sizeof(CHAR), NULL, &SubstitutionCharacterWasUsed) == 0) {
				ODS(L"WideCharToMultiByte failed: %s", GetLastErrorAsString());
				goto SoftFail;
			}

			if (SubstitutionCharacterWasUsed) {
				ODS(L"The rewritten DLL name of %hs contains one or more special characters", NameOfImportedDll);
				goto SoftFail;
			}

			ODS(L"Rewrote an import of %s: %hs -> %s", DllBaseName, NameOfImportedDll, NameOfImportedDllU);

			VirtualProtect(NameOfImportedDll, LengthOfRewrittenDllName + sizeof(CHAR), PAGE_READWRITE, &OldMemoryProtection);
			CopyMemory(NameOfImportedDll, NameOfRewrittenDll, LengthOfRewrittenDllName + sizeof(CHAR));
			VirtualProtect(NameOfImportedDll, LengthOfRewrittenDllName + sizeof(CHAR), OldMemoryProtection, &OldMemoryProtection);
			
			RewriteOccurred = TRUE;
		}

SoftFail:
		++ImportDescriptor;
	}

	if (RewriteOccurred) {
		// A Bound Import Directory will cause process initialization to fail if we have rewritten
		// anything. So we simply zero it out.
		// Bound imports are a performance optimization, but basically we can't use it because
		// the bound import addresses are dependent on the "real" function addresses within the
		// imported DLL - and since we have replaced one or more imported DLLs, these pre-calculated
		// function addresses are no longer valid, so we just have to delete it.
		PIMAGE_DATA_DIRECTORY BoundImportDirectory = &OptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT];

		VirtualProtect(BoundImportDirectory, sizeof(BoundImportDirectory), PAGE_READWRITE, &OldMemoryProtection);
		ZeroMemory(BoundImportDirectory, sizeof(BoundImportDirectory));
		VirtualProtect(BoundImportDirectory, sizeof(BoundImportDirectory), OldMemoryProtection, &OldMemoryProtection);
	}
}

// This function will be called on every DLL load and unload.
// When a DLL is loaded, we must rewrite its imports.
VOID NTAPI DllNotificationFunction(
	IN	LDR_DLL_NOTIFICATION_REASON	NotificationReason,
	IN	PCLDR_DLL_NOTIFICATION_DATA	NotificationData,
	IN	PVOID						Context)
{
	LPCWSTR lpszDllFullName;
	LPCWSTR lpszDllBaseName;
	LPVOID lpDllBase;

	if (KexData->ProcessInitializationComplete == FALSE) {
		// Sometimes DLLs can load before the application entry point is reached.
		// We don't want to attempt rewriting anything because at this point
		// VxKexLdr is still debugging the process, and it will be responsible
		// for rewriting DLLs.
		return;
	}

	if (NotificationReason != LDR_DLL_NOTIFICATION_REASON_LOADED) {
		// we don't care about DLL unloads
		return;
	}

	lpszDllFullName = NotificationData->FullDllName->Buffer;
	lpszDllBaseName = NotificationData->BaseDllName->Buffer;
	lpDllBase = NotificationData->DllBase;

	if (ShouldRewriteImportsOfDll(lpszDllFullName)) {
		PVOID Cookie;

		LdrLockLoaderLock(0, NULL, &Cookie);
		RewriteImportsOfDll(lpszDllBaseName, lpDllBase);
		LdrUnlockLoaderLock(0, Cookie);
	}
}

VOID DllMain_RegisterDllNotification(
	VOID)
{
	NTSTATUS st;
	
	ODS_ENTRY();
	st = LdrRegisterDllNotification(0, DllNotificationFunction, NULL, &lpCookie);

	if (!NT_SUCCESS(st)) {
		ODS(L"Registration of DLL notification function failed. NTSTATUS=%#010I32x: %s", st, NtStatusAsString(st));
	}
}