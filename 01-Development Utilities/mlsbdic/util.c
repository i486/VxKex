#include "buildcfg.h"
#include "mlsbdic.h"

//
// Print usage information to the standard error handle.
//

VOID ShowUsage(
	VOID)
{
	InfoBoxF(
		L"Compiles textual .DIC dictionaries into binary .BDI dictionaries.\r\n"
		L"/IN:<file.dic|directory> Input .dic file, or directory containing .dic files\r\n"
		L"/OUT:<file.bdi|directory> Output .bdi file or directory (required when /IN is a directory)");
}

//
// Find the last occurrence of a character in a string.
//

PCWSTR FindLastChar(
	IN	PCWSTR	String,
	IN	WCHAR	Char)
{
	PCWSTR Last;
	
	Last = NULL;
	
	while (*String != L'\0') {
		if (*String == Char) {
			Last = String;
		}
		++String;
	}
	
	return Last;
}

//
// Generate a default output path by replacing the input file extension with
// .bdi. If no extension exists, .bdi is appended.
//

VOID GenerateOutputPath(
	IN	PCWSTR	InputPath,
	OUT	PWCHAR	OutputPath,
	IN	SIZE_T	OutputPathCch)
{
	HRESULT Result;

	Result = StringCchCopy(OutputPath, OutputPathCch, InputPath);
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		OutputPath[0] = '\0';
		return;
	}

	Result = PathCchRenameExtension(OutputPath, OutputPathCch, L".bdi");
	ASSERT (SUCCEEDED(Result));

	if (FAILED(Result)) {
		OutputPath[0] = '\0';
		return;
	}
}