#include "VxKexLdr.h"
#include <Psapi.h>

// The purpose of this function is to avoid the subsystem version check performed by kernel32.dll
// as a part of the CreateProcess() routine.
// There is a function called BasepCheckImageVersion (on XP, it's called BasepIsImageVersionOk) which
// queries two memory addresses:
//   0x7ffe026c: Subsystem Major Version (DWORD)
//   0x7ffe0270: Subsystem Minor Version (DWORD)
// These two values are part of the USER_SHARED_DATA structure, which is read-only. However we can
// patch the memory values themselves to point to our own fake subsystem image numbers.
// It turns out that we can simply scan the in-memory kernel32.dll for these two addresses (works
// on XP, Vista and 7 - x86, x64 and WOW64) and only one result will come up. What a stroke of luck.
// Then we rewrite those addresses to point at our fake numbers.
//
// If the bPatch parameter is TRUE, Kernel32 will be patched.
// If the bPatch parameter is FALSE, Kernel32 will be returned to its original state.
VOID PatchKernel32ImageVersionCheck(
	BOOL bPatch)
{
	static BOOL bAlreadyPatched = FALSE;
	static LPDWORD lpMajorAddress = NULL; // addresses within kernel32.dll that we are patching
	static LPDWORD lpMinorAddress = NULL; // ^
	static LPDWORD lpdwFakeMajorVersion = NULL;
	static LPDWORD lpdwFakeMinorVersion = NULL;
	DWORD dwOldProtect;

	if (bPatch) {
		// patch
		MODULEINFO k32ModInfo;
		LPBYTE lpPtr;
		LPBYTE lpEndPtr;

		if (bAlreadyPatched) {
			// this shouldn't happen
#ifdef _DEBUG
			CriticalErrorBoxF(L"Internal error: %s(bPatch=TRUE) called inappropriately", __FUNCTION__);
#else
			return;
#endif
		}

		if (!lpdwFakeMajorVersion) {
			lpdwFakeMajorVersion = (LPDWORD) VirtualAlloc(NULL, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			if (!lpdwFakeMajorVersion) {
#ifdef _DEBUG
				CriticalErrorBoxF(L"Failed to allocate memory: %s", GetLastErrorAsString());
#else
				return;
#endif
			}

			*lpdwFakeMajorVersion = 0xFFFFFFFF;
			VirtualProtect(lpdwFakeMajorVersion, sizeof(DWORD), PAGE_READONLY, &dwOldProtect);
		}

		if (!lpdwFakeMinorVersion) {
			lpdwFakeMinorVersion = (LPDWORD) VirtualAlloc(NULL, sizeof(DWORD), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

			if (!lpdwFakeMinorVersion) {
#ifdef _DEBUG
				CriticalErrorBoxF(L"Failed to allocate memory: %s", GetLastErrorAsString());
#else
				return;
#endif
			}

			*lpdwFakeMinorVersion = 0xFFFFFFFF;
			VirtualProtect(lpdwFakeMinorVersion, sizeof(DWORD), PAGE_READONLY, &dwOldProtect);
		}


		if ((DWORD_PTR) lpdwFakeMajorVersion > 0xFFFFFFFC || (DWORD_PTR) lpdwFakeMinorVersion > 0xFFFFFFFC) {
			// this shouldn't happen on win7 and earlier, they only started adding ASLR stuff
			// in windows 8 IIRC (and we don't care about those)
#ifdef _DEBUG
			CriticalErrorBoxF(L"Internal error: Pointer to faked subsystem version too large.");
#else
			return;
#endif
		}

		if (GetModuleInformation(GetCurrentProcess(), GetModuleHandle(L"kernel32.dll"), &k32ModInfo, sizeof(k32ModInfo)) == FALSE) {
#ifdef _DEBUG
			CriticalErrorBoxF(L"Failed to retrieve module information of KERNEL32.DLL: %s", GetLastErrorAsString());
#else
			return;
#endif
		}

		lpEndPtr = (LPBYTE) k32ModInfo.lpBaseOfDll + k32ModInfo.SizeOfImage;

		for (lpPtr = (LPBYTE) k32ModInfo.lpBaseOfDll; lpPtr < (lpEndPtr - sizeof(DWORD)); ++lpPtr) {
			LPDWORD lpDw = (LPDWORD) lpPtr;

			if (lpMajorAddress && lpMinorAddress) {
				// we have already patched both required values
				break;
			}

			if (*lpDw == 0x7FFE026C) {
				DWORD dwOldProtect;
				lpMajorAddress = lpDw;
				VirtualProtect(lpDw, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
				*lpDw = (DWORD) lpdwFakeMajorVersion;
				VirtualProtect(lpDw, sizeof(DWORD), dwOldProtect, &dwOldProtect);
			} else if (*lpDw == 0x7FFE0270) {
				DWORD dwOldProtect;
				lpMinorAddress = lpDw;
				VirtualProtect(lpDw, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
				*lpDw = (DWORD) lpdwFakeMinorVersion;
				VirtualProtect(lpDw, sizeof(DWORD), dwOldProtect, &dwOldProtect);
			}
		}

		bAlreadyPatched = TRUE;
	} else if (bAlreadyPatched) {
		// unpatch
		VirtualProtect(lpMajorAddress, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
		*lpMajorAddress = 0x7FFE026C;
		VirtualProtect(lpMajorAddress, sizeof(DWORD), dwOldProtect, &dwOldProtect);

		VirtualProtect(lpMinorAddress, sizeof(DWORD), PAGE_EXECUTE_WRITECOPY, &dwOldProtect);
		*lpMinorAddress = 0x7FFE0270;
		VirtualProtect(lpMinorAddress, sizeof(DWORD), dwOldProtect, &dwOldProtect);
		
		lpMajorAddress = NULL;
		lpMinorAddress = NULL;
		VirtualFree(lpdwFakeMajorVersion, 0, MEM_RELEASE); lpdwFakeMajorVersion = NULL;
		VirtualFree(lpdwFakeMinorVersion, 0, MEM_RELEASE); lpdwFakeMinorVersion = NULL;
		bAlreadyPatched = FALSE;
	}
}