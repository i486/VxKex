#include "buildcfg.h"
#include "mlsbdic.h"

//
// Convert a single .dic file to a .bdi file.
//

BOOLEAN ProcessFile(
	IN	PCWSTR	InputPath,
	IN	PCWSTR	OutputPath)
{
	HANDLE InputFileHandle;
	HANDLE MappingHandle;
	PVOID ViewBase;
	LARGE_INTEGER FileSize;
	PWCHAR Text;
	SIZE_T TextCch;
	HANDLE OutputFileHandle;
	MLSP_DICTIONARY_HEADER Header;
	DWORD BytesWritten;
	ULONG PairCount;
	ULONG Index;
	ULONG LineStart;
	ULONG LineEnd;
	ULONG LineLength;
	WCHAR Ch;
	BOOLEAN ParseSuccess;
	WCHAR EnglishBuffer[2048];
	WCHAR ForeignBuffer[2048];
	ULONG EnglishCch;
	ULONG ForeignCch;
	ULONG LineNumber;
	SIZE_T WriteSize;
	DWORD WriteSizeDw;
	DWORD LastError;

	InputFileHandle = CreateFileW(
		InputPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	
	if (InputFileHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(L"Failed to open input file: %s", InputPath);
		return FALSE;
	}
	
	if (!GetFileSizeEx(InputFileHandle, &FileSize)) {
		ErrorBoxF(L"Failed to get input file size.");
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	if (FileSize.QuadPart < 2) {
		ErrorBoxF(
			L"Input file is too small to contain a BOM.\r\n"
			L"Save the input file as Unicode format in Notepad.");
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	MappingHandle = CreateFileMappingW(
		InputFileHandle,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);
	
	if (!MappingHandle) {
		ErrorBoxF(L"Failed to create file mapping.");
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	ViewBase = MapViewOfFile(
		MappingHandle,
		FILE_MAP_READ,
		0,
		0,
		0);
	
	if (!ViewBase) {
		ErrorBoxF(L"Failed to map view of file.");
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	Text = (PWCHAR) ViewBase;
	
	if (Text[0] != 0xFEFF) {
		ErrorBoxF(
			L"Input file must have a little-endian UTF-16 BOM.\r\n"
			L"Save the input file as Unicode in Notepad.");
		UnmapViewOfFile(ViewBase);
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	Text = Text + 1;
	TextCch = (SIZE_T) (FileSize.QuadPart / sizeof(WCHAR)) - 1;
	
	OutputFileHandle = CreateFileW(
		OutputPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	
	if (OutputFileHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(L"Failed to create output file: %s", OutputPath);

		UnmapViewOfFile(ViewBase);
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	KexRtlZeroMemory(&Header, sizeof(Header));
	Header.Magic[0] = 'B';
	Header.Magic[1] = 'D';
	Header.Magic[2] = 'I';
	Header.Magic[3] = 'C';
	Header.Version = MLSP_VERSION;
	
	if (!WriteFile(OutputFileHandle, &Header, sizeof(Header), &BytesWritten, NULL)) {
		ErrorBoxF(L"Failed to write output header.");
		CloseHandle(OutputFileHandle);
		UnmapViewOfFile(ViewBase);
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	PairCount = 0;
	Index = 0;
	LineNumber = 1;
	
	while (Index < TextCch) {
		while (Index < TextCch) {
			Ch = Text[Index];
			
			if (Ch == L'\r') {
				++Index;
				if (Index < TextCch && Text[Index] == L'\n') {
					++Index;
				}
				++LineNumber;
			} else if (Ch == L'\n') {
				++Index;
				++LineNumber;
			} else if (Ch == L' ' || Ch == L'\t') {
				++Index;
			} else {
				break;
			}
		}
		
		if (Index >= TextCch) {
			break;
		}
		
		if (Text[Index] == ';') {
			while (Index < TextCch) {
				Ch = Text[Index];
				
				if (Ch == '\r') {
					++Index;
					if (Index < TextCch && Text[Index] == '\n') {
						++Index;
					}
					++LineNumber;
					break;
				} else if (Ch == '\n') {
					++Index;
					++LineNumber;
					break;
				} else {
					++Index;
				}
			}
			continue;
		}
		
		LineStart = Index;
		LineEnd = Index;
		
		while (LineEnd < TextCch) {
			Ch = Text[LineEnd];
			
			if (Ch == L'\r' || Ch == L'\n') {
				break;
			}
			
			++LineEnd;
		}
		
		LineLength = LineEnd - LineStart;
		
		if (LineLength > 0) {
			ParseSuccess = ParseDictionaryLine(
				&Text[LineStart],
				LineLength,
				EnglishBuffer,
				ARRAYSIZE(EnglishBuffer),
				&EnglishCch,
				ForeignBuffer,
				ARRAYSIZE(ForeignBuffer),
				&ForeignCch);
			
			if (!ParseSuccess) {
				ErrorBoxF(
					L"Failed to parse line %lu in %s (entry too long or bad syntax)",
					LineNumber,
					InputPath);

				CloseHandle(OutputFileHandle);
				UnmapViewOfFile(ViewBase);
				CloseHandle(MappingHandle);
				CloseHandle(InputFileHandle);
				return FALSE;
			}
			
			WriteSize = (EnglishCch + 1) * sizeof(WCHAR);
			WriteSizeDw = (DWORD) WriteSize;
			
			if (!WriteFile(OutputFileHandle, EnglishBuffer, WriteSizeDw, 
							&BytesWritten, NULL)) {

				ErrorBoxF(L"Failed to write to output file.");
				CloseHandle(OutputFileHandle);
				UnmapViewOfFile(ViewBase);
				CloseHandle(MappingHandle);
				CloseHandle(InputFileHandle);
				return FALSE;
			}
			
			WriteSize = (ForeignCch + 1) * sizeof(WCHAR);
			WriteSizeDw = (DWORD) WriteSize;
			
			if (!WriteFile(OutputFileHandle, ForeignBuffer, WriteSizeDw,
							&BytesWritten, NULL)) {

				ErrorBoxF(L"Failed to write to output file.");
				CloseHandle(OutputFileHandle);
				UnmapViewOfFile(ViewBase);
				CloseHandle(MappingHandle);
				CloseHandle(InputFileHandle);
				return FALSE;
			}
			
			++PairCount;
		}
		
		Index = LineEnd;
	}
	
	Ch = L'\0';
	if (!WriteFile(OutputFileHandle, &Ch, sizeof(WCHAR), &BytesWritten, NULL)) {
		ErrorBoxF(L"Failed to write terminator to output file.");
		CloseHandle(OutputFileHandle);
		UnmapViewOfFile(ViewBase);
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	if (SetFilePointer(OutputFileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
		LastError = GetLastError();
		
		if (LastError != NO_ERROR) {
			ErrorBoxF(L"Failed to seek in output file.");
			CloseHandle(OutputFileHandle);
			UnmapViewOfFile(ViewBase);
			CloseHandle(MappingHandle);
			CloseHandle(InputFileHandle);
			return FALSE;
		}
	}
	
	Header.NumberOfKeyValuePairs = PairCount;
	
	if (!WriteFile(OutputFileHandle, &Header, sizeof(Header), &BytesWritten, NULL)) {
		ErrorBoxF(L"Failed to write final header.");
		CloseHandle(OutputFileHandle);
		UnmapViewOfFile(ViewBase);
		CloseHandle(MappingHandle);
		CloseHandle(InputFileHandle);
		return FALSE;
	}
	
	CloseHandle(OutputFileHandle);
	UnmapViewOfFile(ViewBase);
	CloseHandle(MappingHandle);
	CloseHandle(InputFileHandle);
	
	return TRUE;
}