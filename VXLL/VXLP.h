#pragma once
#include "VXLL.h"

// Header for private functions of VXLL.

VXLSTATUS VxlpBuildIndex(
	IN	VXLHANDLE	LogHandle);

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