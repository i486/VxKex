///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     main.c
//
// Abstract:
//
//     Dictionary compiler main file
//
// Author:
//
//     I vibe coded it because I couldn't be bothered to do more string parsing
//     Kimi 2.6
//
// Environment:
//
//     Win32 mode.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "mlsbdic.h"

VOID EntryPoint(
	VOID)
{
	PWSTR CommandLine;
	WCHAR InPath[MAX_PATH];
	WCHAR OutPath[MAX_PATH];
	BOOLEAN OutPresent;
	DWORD Attributes;
	BOOLEAN Success;
	INT ReturnValue;

	KexgApplicationFriendlyName = FRIENDLYAPPNAME;

	ReturnValue = 0;
	CommandLine = GetCommandLineWithoutImageName();
	
	Success = ParseCommandLine(
		CommandLine,
		InPath,
		ARRAYSIZE(InPath),
		OutPath,
		ARRAYSIZE(OutPath),
		&OutPresent);
	
	if (!Success) {
		ShowUsage();
		ExitProcess(0);
	}
	
	Attributes = GetFileAttributesW(InPath);
	
	if (Attributes == INVALID_FILE_ATTRIBUTES) {
		ErrorBoxF(L"Input path does not exist: %s", InPath);
		ExitProcess(1);
	}
	
	if (Attributes & FILE_ATTRIBUTE_DIRECTORY) {
		if (!OutPresent) {
			ErrorBoxF(L"/OUT must be specified when /IN is a directory.");
		}
		
		Attributes = GetFileAttributesW(OutPath);
		
		if (Attributes == INVALID_FILE_ATTRIBUTES) {
			if (!CreateDirectoryW(OutPath, NULL)) {
				ErrorBoxF(L"Failed to create output directory: %s", OutPath);
				ExitProcess(1);
			}
		} else if (!(Attributes & FILE_ATTRIBUTE_DIRECTORY)) {
			ErrorBoxF(L"/OUT must be a directory when /IN is a directory.");
			ExitProcess(1);
		}
		
		Success = ProcessDirectory(InPath, OutPath);
		if (!Success) {
			ReturnValue = 1;
		}
	} else {
		if (!OutPresent) {
			GenerateOutputPath(InPath, OutPath, ARRAYSIZE(OutPath));
			
			if (OutPath[0] == L'\0') {
				ErrorBoxF(L"Failed to generate output path.");
				ExitProcess(1);
			}
		}
		
		Success = ProcessFile(InPath, OutPath);
		if (!Success) {
			ReturnValue = 1;
		}
	}
	
	ExitProcess(ReturnValue);
}