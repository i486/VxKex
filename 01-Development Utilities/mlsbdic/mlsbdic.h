///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//	 mlsbdic.h
//
// Abstract:
//
//	 Internal header file for the BDI compiler
//
// Author:
//
//	 vxiiduu (21-May-2025)
//
// Environment:
//
//	 N/A
//
// Revision History:
//
//	 vxiiduu			  21-May-2025  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "buildcfg.h"
#include <KexComm.h>
#include <KexW32ML.h>
#include <KexGui.h>
#include <KexMls.h>

//
// cmdline.c
//

BOOLEAN ParseCommandLine(
	IN	PWSTR		CommandLine,
	OUT	PWCHAR		InPath,
	IN	SIZE_T		InPathCch,
	OUT	PWCHAR		OutPath,
	IN	SIZE_T		OutPathCch,
	OUT	PBOOLEAN	OutPresent);

//
// multifile.c
//

BOOLEAN ProcessDirectory(
	IN	PCWSTR	InputDir,
	IN	PCWSTR	OutputDir);

//
// onefile.c
//

BOOLEAN ProcessFile(
	IN	PCWSTR	InputPath,
	IN	PCWSTR	OutputPath);

//
// parseline.c
//

BOOLEAN ParseDictionaryLine(
	IN	PWCHAR	Line,
	IN	ULONG	LineCch,
	OUT	PWCHAR	EnglishBuffer,
	IN	ULONG	EnglishBufferCch,
	OUT	PULONG	EnglishCch,
	OUT	PWCHAR	ForeignBuffer,
	IN	ULONG	ForeignBufferCch,
	OUT	PULONG	ForeignCch);

//
// util.c
//

VOID ShowUsage(
	VOID);

PCWSTR FindLastChar(
	IN	PCWSTR	String,
	IN	WCHAR	Char);

VOID GenerateOutputPath(
	IN	PCWSTR	InputPath,
	OUT	PWCHAR	OutputPath,
	IN	SIZE_T	OutputPathCch);