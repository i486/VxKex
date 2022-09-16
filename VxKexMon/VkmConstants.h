#pragma once

//
// The format string you should pass as the lpCommandLine parameter to
// CreateProcess when starting KexMonXX.exe.
//
//  %s			-> Should be full path to KexMonXX.exe
//  %08lx		-> Process ID of target process
//  %d			-> Whether or not the KexDllDir in the target process is 64 bit
//
#define VKM_CMDLINE_FORMAT_STRING (L"\"%s\" %08lx %d")

//
// The number of non-executable-name characters in the command line that
// should be passed to KexMonXX.exe.
// 8 for Process ID (hex), 1 space, 1 boolean
//
#define VKM_ARGUMENT_LENGTH (8+1+1)