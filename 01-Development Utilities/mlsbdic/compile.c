///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     compile.c
//
// Abstract:
//
//     Code which implements the actual dictionary compilation/decompilation.
//
// Author:
//
//     vxiiduu (21-May-2025)
//
// Environment:
//
//     Win32 GUI
//
// Revision History:
//
//     vxiiduu              21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "mlsbdic.h"

//
// .dic (Text Dictionary) File Format Specification
//
// A .dic file is a text file composed of multiple key-value pairs.
// The file must be encoded with Unicode and start with a byte order mark (0xFEFF).
// The file must contain only Windows (CRLF) line endings.
// Lack of BOM, other character encodings, and other line endings will result in
// an error or garbled output.
//
// Each key-value pair exists on its own line and follows the following template:
//
//   "<Key>"="<Value>"
//
// Within each quoted Key and Value, the following escape sequences may be used:
//
//   - \n				(Newline, 0x000A)
//   - \r				(Carriage return, 0x000D)
//   - \t				(Tab, 0x0009)
//   - \\				(Backslash, 0x005C)
//   - \"				(Double quote, 0x0022)
//   - \0               (Null, 0x0000)
//   - \xHH				(Arbitrary hex, 0x00HH)
//   - \xHHHH, \uHHHH	(Arbitrary hex, 0xHHHH)
//
// A backslash combined with any other character(s) will result in an error.
// Any line beginning with a semicolon ';' will be ignored. This can be used to
// write comments.
//

//
// .bdi (Binary Dictionary) File Format Specification
//
// A .bdi file is a binary file, made up of a header and a data block.
// The structure of the header is defined in KexMls.h as the MLSP_DICTIONARY_HEADER
// structure.
// Immediately following the header is the data block. The data block is made up of
// alternating keys and values and always starts with a key. Keys and values follow
// a structure defined in KexMls.h as MLSP_DICTIONARY_KEY_OR_VALUE.
//

//
// Helper functions for BdiToDic and DicToBdi
//

STATIC PVOID MapInputFile(
	IN	PCWSTR	InputFilePath,
	OUT	PULONG	FileSizeOut OPTIONAL)
{
	HANDLE FileHandle;
	HANDLE SectionHandle;
	PVOID SectionView;
	ULONG FileSize;

	if (FileSizeOut) {
		*FileSizeOut = 0;
	}

	FileHandle = CreateFile(
		InputFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(L"Failed to open input file: %s", GetLastErrorAsString());
		return NULL;
	}

	FileSize = GetFileSize(FileHandle, NULL);

	SectionHandle = CreateFileMapping(
		FileHandle,
		NULL,
		PAGE_READONLY,
		0,
		0,
		NULL);

	SafeClose(FileHandle);

	if (SectionHandle == NULL) {
		ErrorBoxF(L"Failed to create section object: %s", GetLastErrorAsString());
		return NULL;
	}

	SectionView = MapViewOfFile(
		SectionHandle,
		FILE_MAP_READ,
		0,
		0,
		0);

	SafeClose(SectionHandle);

	if (!SectionView) {
		ErrorBoxF(L"Failed to map view of section: %s", GetLastErrorAsString());
		return NULL;
	}

	if (FileSizeOut) {
		*FileSizeOut = FileSize;
	}

	return SectionView;
}

STATIC HANDLE OpenOutputFile(
	IN	PCWSTR	OutputFilePath)
{
	HANDLE FileHandle;

	FileHandle = CreateFile(
		OutputFilePath,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (FileHandle == INVALID_HANDLE_VALUE) {
		ErrorBoxF(L"Failed to open output file: %s", GetLastErrorAsString());
		return NULL;
	}

	return FileHandle;
}

STATIC BOOLEAN WriteOutputBinary(
	IN	HANDLE	FileHandle,
	IN	PCVOID	Data,
	IN	ULONG	DataCb)
{
	BOOLEAN Success;
	ULONG BytesWritten;

	Success = WriteFile(
		FileHandle,
		Data,
		DataCb,
		&BytesWritten,
		NULL);

	if (!Success || BytesWritten != DataCb) {
		ErrorBoxF(L"Failed to write %lu bytes to output file: %s", DataCb, GetLastErrorAsString());
	}
}

STATIC INLINE BOOLEAN WriteOutputString(
	IN	HANDLE	FileHandle,
	IN	PCWSTR	Data)
{
	return WriteOutputBinary(FileHandle, Data, wcslen(Data) * sizeof(WCHAR));
}

STATIC FORCEINLINE BOOLEAN BdiToDic(
	IN	PCWSTR	InputFilePath,
	IN	PCWSTR	OutputFilePath)
{
	BOOLEAN Success;
	HANDLE OutputFileHandle;
	PVOID InputMapping;
	ULONG FileSize;
	PCMLSP_DICTIONARY_HEADER Header;

	Success = TRUE;
	InputMapping = NULL;
	OutputFileHandle = NULL;

	InputMapping = MapInputFile(InputFilePath, &FileSize);
	OutputFileHandle = OpenOutputFile(OutputFilePath);

	Header = (PCMLSP_DICTIONARY_HEADER) InputMapping;

	if (!InputMapping || !OutputFileHandle) {
		Success = FALSE;
		goto Exit;
	}

Exit:
	UnmapViewOfFile(InputMapping);
	SafeClose(OutputFileHandle);
	return Success;
}

STATIC WCHAR DicParseHexEscapeCode(
	IN	ULONG	LineNumber,
	IN	PCWSTR	HexString)
{
}

typedef enum _DIC_PROCESS_LINE_STATE {
	BeforeKeyQuote,
	InKey,
	BetweenKeyAndValue,
	InValue,
	AfterValue
} TYPEDEF_TYPE_NAME(DIC_PROCESS_LINE_STATE);

STATIC BOOLEAN DicProcessLine(
	IN	ULONG	LineNumber,
	IN	PCWSTR	Line,
	OUT	PWSTR	Key,
	OUT	PWSTR	Value)
{
	ULONG KeyValueIndex;
	ULONG LineIndex;
	DIC_PROCESS_LINE_STATE State;

	State = BeforeKeyQuote;

	for (LineIndex = 0; Line[LineIndex] != '\0'; ++LineIndex) {
		WCHAR Character;

		Character = Line[LineIndex];

		if (!iswprint(Character)) {
			ErrorBoxF(L"Non-printable character found in input file.");
			return FALSE;
		}

		if (State == BeforeKeyQuote && Character == ';') {
			// This line is a comment. Skip it.
			break;
		}

		if (State == InKey || State == InValue) {
			if (Character == '\\') {
				WCHAR EscapeCode;

				EscapeCode = Line[LineIndex + 1];

				switch (EscapeCode) {
				case 'n':	Character = '\n';	break;
				case 'r':	Character = '\r';	break;
				case 't':	Character = '\t';	break;
				case '\\':	Character = '\\';	break;
				case '"':	Character = '"';	break;

				case 'x':
				case 'u':
					Character = DicParseHexEscapeCode(LineNumber, &Line[LineIndex + 2]);
					break;
				default:
					ErrorBoxF(L"Invalid backslash escape code at line %lu: '\\%c'.", LineNumber, EscapeCode);
					return FALSE;
				}
			}

			switch (State) {
			case InKey:
				Key[KeyValueIndex++] = Character;
				break;
			case InValue:
				Value[KeyValueIndex++] = Character;
				break;
			default:
				ASSUME (FALSE);
			}
		}

		if (Character == '"') {
			switch (State) {
			case BeforeKeyQuote:
				KeyValueIndex = 0;
				State = InKey;
				break;
			case InKey:
				Key[KeyValueIndex] = '\0';
				State = BetweenKeyAndValue;
				break;
			case BetweenKeyAndValue:
				KeyValueIndex = 0;
				State = InValue;
				break;
			case InValue:
				Value[KeyValueIndex] = '\0';
				State = AfterValue;
				break;
			default:
				ASSUME (FALSE);
			}
		}
	}

	if (State != AfterValue) {
		ErrorBoxF(L"Incomplete key or value found at line %lu.", LineNumber);
		return FALSE;
	}
}

STATIC FORCEINLINE BOOLEAN DicToBdi(
	IN	PCWSTR	InputFilePath,
	IN	PCWSTR	OutputFilePath)
{
	BOOLEAN Success;
	HANDLE OutputFileHandle;
	PVOID InputMapping;
	ULONG FileSize;
	ULONG FileCch;
	ULONG FileIndex;
	ULONG LineNumber;
	PWCHAR FileData;
	MLSP_DICTIONARY_HEADER Header;

	BOOLEAN InKey;
	BOOLEAN InValue;

	Success = TRUE;

	InputMapping = MapInputFile(InputFilePath, &FileSize);
	OutputFileHandle = OpenOutputFile(OutputFilePath);

	if (!InputMapping || !OutputFileHandle) {
		Success = FALSE;
		goto Exit;
	}

	if (FileSize & 1) {
		// If the file size is an odd number of bytes we know it's already invalid
		// since the DIC file format requires Unicode encoding.
		ErrorBoxF(L"Input .dic file must be encoded with Unicode.");
		Success = FALSE;
		goto Exit;
	}

	if (FileSize <= 12 * sizeof(WCHAR)) {
		// No data in file.
		ErrorBoxF(L"The input .dic file contains no valid data.");
		Success = FALSE;
		goto Exit;
	}

	FileCch = FileSize / sizeof(WCHAR);
	FileData = (PWCHAR) InputMapping;

	//
	// Verify that the first character in the file is a Unicode byte order mark
	// (0xFEFF).
	//

	unless (FileData[0] == 0xFEFF) {
		ErrorBoxF(L"The input .dic file must start with a byte order mark.\r\n"
				  L"Text editors such as Notepad save text files with a byte order mark "
				  L"when the Unicode encoding is specified.");

		Success = FALSE;
		goto Exit;
	}

	++FileData;
	--FileCch;

	//
	// Write header to output file.
	// The header is incomplete because we don't yet know the number of key-value pairs.
	// We will overwrite it again later - this is just a placeholder.
	//

	RtlZeroMemory(&Header, sizeof(Header));

	Header.Magic[0] = 'B';
	Header.Magic[1] = 'D';
	Header.Magic[2] = 'I';
	Header.Magic[3] = 'C';

	Header.Version = MLSP_VERSION;

	Success = WriteOutputBinary(OutputFileHandle, &Header, sizeof(Header));

	if (!Success) {
		goto Exit;
	}

	//
	// Main loop. Process file line by line.
	//

	LineNumber = 0;
	FileIndex = 0;
	InKey = FALSE;
	InValue = FALSE;

	while (TRUE) {
		WCHAR Line[2048];
		WCHAR Key[ARRAYSIZE(Line)];
		WCHAR Value[ARRAYSIZE(Line)];
		ULONG LineIndex;

		//
		// Get a line into the Line buffer.
		//

		if (FileData[FileIndex] == '\r' && FileData[FileIndex + 1] == '\n') {
			FileIndex += 2;
		}

		LineIndex = 0;

		until (FileData[FileIndex] == '\r' && FileData[FileIndex + 1] == '\n') {
			if (LineIndex < ARRAYSIZE(Line)) {
				Line[LineIndex++] = FileData[FileIndex];
			}

			++FileIndex;

			if (FileIndex >= FileCch) {
				// no more data in file
				break;
			}
		}

		Line[LineIndex] = '\0';
		LineIndex = 0;
		++LineNumber;

		//
		// Parse the line and isolate the key and value.
		//

		Success = DicProcessLine(LineNumber, Line, Key, Value);

		if (!Success) {
			goto Exit;
		}

		//
		// The key and the value are in the Key and Value buffers, respectively.
		// All escape sequences have already been handled and they are ready to
		// be placed in the output file.
		//
	}

Exit:
	UnmapViewOfFile(InputMapping);
	SafeClose(OutputFileHandle);
	return Success;
}

BOOLEAN BdicCompileBdiDic(
	IN	PCWSTR	InputFilePath,
	IN	PCWSTR	OutputFilePath)
{
	BOOLEAN Success;
	PCWSTR InputExtension;
	PCWSTR OutputExtension;

	if (InputFilePath[0] == '\0') {
		ErrorBoxF(L"The path to the input file was not specified.");
		return FALSE;
	}

	if (OutputFilePath[0] == '\0') {
		ErrorBoxF(L"The path to the output file was not specified.");
		return FALSE;
	}

	InputExtension = PathFindExtension(InputFilePath);
	OutputExtension = PathFindExtension(OutputFilePath);

	if (!StringEqualI(InputExtension, L".bdi") && !StringEqualI(InputExtension, L".dic")) {
		ErrorBoxF(L"The input file must have a .bdi or .dic extension.");
		return FALSE;
	}

	if (!StringEqualI(OutputExtension, L".bdi") && !StringEqualI(OutputExtension, L".dic")) {
		ErrorBoxF(L"The output file must have a .bdi or .dic extension.");
		return FALSE;
	}

	if (StringEqualI(InputExtension, OutputExtension)) {
		ErrorBoxF(
			L"If the input file is a .dic, the output file must be a .bdi.\r\n"
			L"If the input file is a .bdi, the output file must be a .dic.");
		return FALSE;
	}

	if (StringEqualI(InputExtension, L".bdi")) {
		Success = BdiToDic(InputFilePath, OutputFilePath);
	} else {
		Success = DicToBdi(InputFilePath, OutputFilePath);
	}

	return Success;
}