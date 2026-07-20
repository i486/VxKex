#include "buildcfg.h"
#include "mlsbdic.h"

//
// ProcessDirectory
//
// Enumerate all .dic files in the input directory and convert each to a .bdi
// file in the output directory.
//

BOOLEAN ProcessDirectory(
	IN	PCWSTR	InputDir,
	IN	PCWSTR	OutputDir)
{
	WCHAR SearchPath[MAX_PATH];
	WCHAR InputPath[MAX_PATH];
	WCHAR OutputPath[MAX_PATH];
	WIN32_FIND_DATA FindData;
	HANDLE FindHandle;
	BOOLEAN OverallSuccess;
	HRESULT Result;
	PCWSTR Dot;
	PWCHAR Current;
	SIZE_T BaseLength;
	SIZE_T CurrentLength;
	BOOLEAN Success;
	DWORD LastError;
	
	OverallSuccess = TRUE;
	
	Result = StringCchCopyW(SearchPath, ARRAYSIZE(SearchPath), InputDir);
	if (FAILED(Result)) {
		ErrorBoxF(L"Input directory path is too long.");
		return FALSE;
	}
	
	Result = StringCchCatW(SearchPath, ARRAYSIZE(SearchPath), L"\\*.dic");
	if (FAILED(Result)) {
		ErrorBoxF(L"Input directory path is too long.");
		return FALSE;
	}
	
	FindHandle = FindFirstFileW(SearchPath, &FindData);
	if (FindHandle == INVALID_HANDLE_VALUE) {
		LastError = GetLastError();
		
		if (LastError == ERROR_FILE_NOT_FOUND) {
			ErrorBoxF(L"No .dic files found in input directory.");
			return FALSE;
		}
		
		ErrorBoxF(L"Failed to enumerate input directory.");
		return FALSE;
	}
	
	do {
		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		
		Result = StringCchCopyW(InputPath, ARRAYSIZE(InputPath), InputDir);
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		Result = StringCchCatW(InputPath, ARRAYSIZE(InputPath), L"\\");
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		Result = StringCchCatW(InputPath, ARRAYSIZE(InputPath), FindData.cFileName);
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		Dot = NULL;
		Current = FindData.cFileName;
		
		while (*Current != L'\0') {
			if (*Current == L'.') {
				Dot = Current;
			}
			++Current;
		}
		
		if (Dot && StringEqualI(Dot, L".dic")) {
			BaseLength = Dot - FindData.cFileName;
		} else {
			BaseLength = wcslen(FindData.cFileName);
		}
		
		Result = StringCchCopyW(OutputPath, ARRAYSIZE(OutputPath), OutputDir);
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		Result = StringCchCatW(OutputPath, ARRAYSIZE(OutputPath), L"\\");
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		CurrentLength = wcslen(OutputPath);
		
		if (CurrentLength + BaseLength >= ARRAYSIZE(OutputPath)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		KexRtlCopyMemory(
			&OutputPath[CurrentLength],
			FindData.cFileName,
			BaseLength * sizeof(WCHAR));
		OutputPath[CurrentLength + BaseLength] = L'\0';
		
		Result = StringCchCatW(OutputPath, ARRAYSIZE(OutputPath), L".bdi");
		if (FAILED(Result)) {
			ErrorBoxF(L"Path too long.");
			OverallSuccess = FALSE;
			continue;
		}
		
		Success = ProcessFile(InputPath, OutputPath);
		if (!Success) {
			OverallSuccess = FALSE;
		}
	} while (FindNextFileW(FindHandle, &FindData));
	
	FindClose(FindHandle);
	return OverallSuccess;
}