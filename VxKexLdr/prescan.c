#include <NtDll.h>
#include "VxKexLdr.h"

//
// We need some information about the EXE file we are going to load.
// The best way to do this is just to map the EXE into memory and read some data
// out of its PE header.
//

// 1. Add the appropriate Kex(32/64) to PATH
// 2. Patch kernel32 CreateProcess if the subsystem version is incorrect
// 3. Set g_bExe64 correctly
VOID PreScanExeFile(
	VOID)
{
	WCHAR szPathAppend[MAX_PATH + 7];
	LPWSTR szPath;
	DWORD dwcchPath;
	HMODULE hModExe;
	PIMAGE_DOS_HEADER lpDosHdr;
	PIMAGE_NT_HEADERS lpNtHdr;
	PIMAGE_FILE_HEADER lpFileHdr;
	WORD MajorSubsystemVersion;
	WORD MinorSubsystemVersion;
	
	union {
		PIMAGE_OPTIONAL_HEADER Hdr;
		PIMAGE_OPTIONAL_HEADER32 Hdr32;
		PIMAGE_OPTIONAL_HEADER64 Hdr64;
	} lpOptHdr;

	hModExe = LoadLibraryEx(g_szExeFullPath, NULL, LOAD_LIBRARY_AS_DATAFILE);

	if (!hModExe) {
		DWORD dwError = GetLastError();

		// ERROR_FILE_INVALID
		// "The volume for a file has been externally altered so that the
		//  opened file is no longer valid".
		//
		// This error is given when the PE file we passed to LoadLibraryEx is an
		// invalid PE file. However, the error message provided by the system is
		// extremely cryptic, so we will have to correct it.

		if (dwError == ERROR_FILE_INVALID) {
			CriticalErrorBoxF(L"Failed to scan the executable file \"%s\": %#010I32x: "
							  L"The executable file is invalid or corrupt.",
							  g_lpszExeBaseName, dwError);
		}

		CriticalErrorBoxF(L"Failed to scan the executable file \"%s\": %#010I32x: %s",
						  g_lpszExeBaseName, dwError, GetLastErrorAsString());
	}

	lpDosHdr = (PIMAGE_DOS_HEADER) ((LPBYTE) hModExe);

	if (lpDosHdr->e_magic != IMAGE_DOS_SIGNATURE) {
		// When you specify LOAD_LIBRARY_AS_DATAFILE and the DLL/EXE is not already
		// loaded, the returned HMODULE is offset by +1 from the real beginning of
		// the DOS header.
		// But when the EXE is already loaded (which can only happen when you use
		// VxKexLdr to load VxKexLdr), the returned HMODULE is not offset.
		lpDosHdr = (PIMAGE_DOS_HEADER) (((LPBYTE) hModExe) - 1);
	}

	lpNtHdr = (PIMAGE_NT_HEADERS) (((LPBYTE) lpDosHdr) + lpDosHdr->e_lfanew);
	lpFileHdr = &lpNtHdr->FileHeader;
	lpOptHdr.Hdr = &lpNtHdr->OptionalHeader;
	g_bExe64 = (lpFileHdr->Machine == 0x8664);

	if (g_bExe64) {
		MajorSubsystemVersion = lpOptHdr.Hdr64->MajorSubsystemVersion;
		MinorSubsystemVersion = lpOptHdr.Hdr64->MinorSubsystemVersion;
	} else {
		MajorSubsystemVersion = lpOptHdr.Hdr32->MajorSubsystemVersion;
		MinorSubsystemVersion = lpOptHdr.Hdr32->MinorSubsystemVersion;
	}

	FreeLibrary(hModExe);

	// If we would fail BasepIsImageVersionOk (also called BasepCheckImageVersion in some versions)
	// due to the image subsystem versions being too high, then patch it to succeed. It will
	// be unpatched after the CreateProcess call.
	if ((MajorSubsystemVersion > SharedUserData->NtMajorVersion) ||
		((MajorSubsystemVersion == SharedUserData->NtMajorVersion) && (MinorSubsystemVersion > SharedUserData->NtMinorVersion))) {
		PatchKernel32ImageVersionCheck(TRUE);
	}

	// Append correct KexDLL directory to our PATH environment variable.
	// This PATH entry will be inherited by our child process, which can then
	// proceed to load DLLs from the correct KexDLL folder.
	swprintf_s(szPathAppend, ARRAYSIZE(szPathAppend), L";%s\\Kex%d", g_KexData.szKexDir, g_bExe64 ? 64 : 32);
	dwcchPath = GetEnvironmentVariable(L"Path", NULL, 0) + (DWORD) wcslen(szPathAppend);
	szPath = (LPWSTR) StackAlloc(dwcchPath * sizeof(WCHAR));
	GetEnvironmentVariable(L"Path", szPath, dwcchPath);
	wcscat_s(szPath, dwcchPath, szPathAppend);
	SetEnvironmentVariable(L"Path", szPath);
}