#pragma once
#include "VXLL.h"

// Header for private functions of VXLL.
// Do not call any of these outside VXLL.

VXLSTATUS VxlpTranslateWin32Error(
	IN	ULONG		Win32Error,
	IN	VXLSTATUS	Default OPTIONAL);

VXLSTATUS VxlpValidateFileOpenFlags(
	IN	ULONG		Flags);

VXLSTATUS VxlpTranslateFileOpenFlags(
	IN	ULONG		VxlFlags,
	OUT	PULONG		DesiredAccess,
	OUT	PULONG		FlagsAndAttributes,
	OUT	PULONG		CreationDisposition);

VXLSTATUS VxlpReadLogFileHeader(
	IN	VXLHANDLE			LogHandle);

VXLSTATUS VxlpInitializeLogFileHeader(
	OUT	PVXLLOGFILEHEADER	LogHeader,
	IN	PCWSTR				SourceApplication);

VXLSTATUS VxlpWriteLogFileHeader(
	IN	VXLHANDLE			LogHandle);

VXLSTATUS VxlpFindSourceComponentIndex(
	IN	VXLHANDLE	LogHandle,
	IN	PCWSTR		SourceComponent,
	OUT	PUSHORT		SourceComponentIndex OPTIONAL);

VXLSTATUS VxlpFindOrCreateSourceComponentIndex(
	IN OUT	VXLHANDLE	LogHandle,
	IN		PCWSTR		SourceComponent,
	OUT		PUSHORT		SourceComponentIndex OPTIONAL);

VXLSTATUS VxlpAcquireFileLock(
	IN	VXLHANDLE			LogHandle);

VXLSTATUS VxlpReleaseFileLock(
	IN	VXLHANDLE			LogHandle);

BOOLEAN VxlpFileIsEmpty(
	IN	HANDLE		FileHandle);

VXLSTATUS VxlpLogFileEntryToLogEntry(
	IN	VXLHANDLE			LogHandle,
	IN	PVXLLOGFILEENTRY	FileEntry,
	OUT	PPVXLLOGENTRY		Entry);

BOOLEAN VxlpSeverityFiltersMatchSeverity(
	IN	ULONG				SeverityFilters,
	IN	VXLSEVERITY			Severity);