///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     opnclose.c
//
// Abstract:
//
//     Contains the public routines for opening and closing log files.
//
// Author:
//
//     vxiiduu (30-Sep-2022)
//
// Revision History:
//
//     vxiiduu	            30-Sep-2022  Initial creation.
//     vxiiduu              15-Oct-2022  Convert to v2 format.
//     vxiiduu              12-Nov-2022  Convert to v3 + native API
//     vxiiduu              08-Jan-2023  Set compressed attribute on vxl files
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

STATIC CONST CHAR VXLL_MAGIC[] = {'V','X','L','L'};

//
// Open a log file.
//
// LogHandle
//   Pointer to a VXLHANDLE that will receive the log handle.
//
// SourceApplication
//   String that indicates which application is opening the log file.
//   If this string is specified, and the log file is opened for append
//   access, then the source application already inside the log file
//   will be checked to see if it is the same. If not, the call fails.
//   This parameter must be specified if a new log file is created.
//
// ObjectAttributes
//   An OBJECT_ATTRIBUTES structure which specifies the file name, its
//   root directory, etc.
//
// DesiredAccess
//   Must be either GENERIC_READ or GENERIC_WRITE.
//   Any other values will cause a failure.
//
// CreateDisposition
//   One of the following values:
//
//   FILE_SUPERSEDE, FILE_CREATE, FILE_OPEN, FILE_OPEN_IF,
//   FILE_OVERWRITE, FILE_OVERWRITE_IF
//
//   See the documentation for NtCreateFile to understand what these
//   values mean.
//
NTSTATUS NTAPI VxlOpenLog(
	OUT		PVXLHANDLE			LogHandle,
	IN		PUNICODE_STRING		SourceApplication OPTIONAL,
	IN		POBJECT_ATTRIBUTES	ObjectAttributes,
	IN		ACCESS_MASK			DesiredAccess,
	IN		ULONG				CreateDisposition)
{
	NTSTATUS Status;
	HANDLE SectionHandle;
	SIZE_T ViewSize;
	IO_STATUS_BLOCK IoStatusBlock;
	ULONG ShareAccess;
	LONGLONG CreationInitialSize;
	PVXLCONTEXT Context;
	BOOLEAN NewLogFileCreated;
	ULONG SectionDesiredAccess;
	ULONG SectionPageProtection;

	if (LogHandle) {
		*LogHandle = NULL;
	}

	//
	// Validate input parameters.
	//

	if (!LogHandle || !ObjectAttributes || !DesiredAccess) {
		return STATUS_INVALID_PARAMETER;
	}

	if (DesiredAccess == GENERIC_READ) {
		if (CreateDisposition == FILE_SUPERSEDE ||
			CreateDisposition == FILE_CREATE ||
			CreateDisposition == FILE_OVERWRITE ||
			CreateDisposition == FILE_OVERWRITE_IF) {

			return STATUS_INVALID_PARAMETER;
		}
	} else if (DesiredAccess != GENERIC_WRITE) {
		return STATUS_INVALID_PARAMETER;
	}

	Context = NULL;
	SectionHandle = NULL;

	try {
		//
		// Allocate memory for the context structure.
		//

		Context = SafeAllocSeh(VXLCONTEXT, 1);

		RtlInitializeSRWLock(&Context->Lock);

		//
		// Open the log file itself.
		//

		Context->OpenMode = DesiredAccess;
		DesiredAccess |= SYNCHRONIZE;

		if (Context->OpenMode == GENERIC_READ) {
			ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		} else {
			// Add read permission since we still need to read file headers,
			// etc.
			DesiredAccess |= GENERIC_READ;

			// Do not allow other apps to write to the log file while we are
			// writing it.
			ShareAccess = FILE_SHARE_READ | FILE_SHARE_DELETE;
		}

		// If we are creating or overwriting a file, then we will allocate enough
		// space for the header up front.
		CreationInitialSize = sizeof(VXLLOGFILEHEADER);

		Status = NtCreateFile(
			&Context->FileHandle,
			DesiredAccess,
			ObjectAttributes,
			&IoStatusBlock,
			&CreationInitialSize,
			FILE_ATTRIBUTE_NORMAL,
			ShareAccess,
			CreateDisposition,
			FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
			NULL,
			0);

		if (!NT_SUCCESS(Status)) {
			leave;
		}

		if (IoStatusBlock.Information == FILE_CREATED ||
			IoStatusBlock.Information == FILE_SUPERSEDED ||
			IoStatusBlock.Information == FILE_OVERWRITTEN) {
			NewLogFileCreated = TRUE;
		} else {
			NewLogFileCreated = FALSE;
		}

		if (NewLogFileCreated) {
			ULONG CompressionType;

			//
			// If we created or emptied the file, set the valid data length
			// to the size of the log file header so we can map it. Otherwise
			// we will get STATUS_MAPPED_FILE_SIZE_ZERO from NtCreateSection.
			//

			Status = NtSetInformationFile(
				Context->FileHandle,
				&IoStatusBlock,
				&CreationInitialSize,
				sizeof(CreationInitialSize),
				FileEndOfFileInformation);

			if (!NT_SUCCESS(Status)) {
				leave;
			}

			//
			// Attempt to set the NTFS compression attribute on the log file.
			// Uncompressed log files can quickly accumulate and consume the
			// user's disk space, so we want to reduce this impact as much as
			// possible.
			//
			// Of course, this call may fail if the file is on a FAT volume or
			// other file system that does not support compression. In this
			// case we don't really care - we tried.
			//

			CompressionType = COMPRESSION_FORMAT_LZNT1;

			NtFsControlFile(
				Context->FileHandle,
				NULL,
				NULL,
				NULL,
				&IoStatusBlock,
				FSCTL_SET_COMPRESSION,
				&CompressionType,
				sizeof(CompressionType),
				NULL,
				0);
		}

		//
		// Create a section backed by the entire log file.
		// If we are opening for read only, map the entire log file into
		// memory. If we are opening read-write, only map the header.
		//

		if (Context->OpenMode == GENERIC_READ) {
			SectionDesiredAccess = SECTION_MAP_READ;
			SectionPageProtection = PAGE_READONLY;
		} else {
			SectionDesiredAccess = SECTION_MAP_READ | SECTION_MAP_WRITE;
			SectionPageProtection = PAGE_READWRITE;
		}

		Status = NtCreateSection(
			&SectionHandle,
			SectionDesiredAccess,
			NULL,
			NULL,
			SectionPageProtection,
			SEC_COMMIT,
			Context->FileHandle);

		if (!NT_SUCCESS(Status)) {
			leave;
		}

		if (Context->OpenMode == GENERIC_READ) {
			// map entire
			ViewSize = 0;
		} else {
			// only map header
			ViewSize = sizeof(VXLLOGFILEHEADER);
		}

		Status = NtMapViewOfSection(
			SectionHandle,
			NtCurrentProcess(),
			&Context->MappedSection,
			0,
			0,
			NULL,
			&ViewSize,
			ViewUnmap,
			0,
			Context->OpenMode == GENERIC_READ ? PAGE_READONLY : PAGE_READWRITE);

		if (!NT_SUCCESS(Status)) {
			leave;
		}

		//
		// If we are creating a new log file, we must create a new
		// log file header. Otherwise, we will validate the existing header.
		//

		if (NewLogFileCreated) {
			UNICODE_STRING DestinationSourceApplication;

			//
			// New empty log file has been created.
			// Populate it with a header.
			//

			if (!SourceApplication) {
				Status = STATUS_INVALID_PARAMETER;
				leave;
			}

			RtlZeroMemory(Context->Header, sizeof(*Context->Header));

			RtlInitEmptyUnicodeString(
				&DestinationSourceApplication,
				Context->Header->SourceApplication,
				sizeof(Context->Header->SourceApplication) - sizeof(WCHAR));

			RtlCopyUnicodeString(&DestinationSourceApplication, SourceApplication);

			Context->Header->Version = VXLL_VERSION;
			RtlCopyMemory(Context->Header->Magic, VXLL_MAGIC, sizeof(VXLL_MAGIC));
		} else {
			//
			// Opening existing log file. Validate the header.
			//

			if (!RtlEqualMemory(Context->Header->Magic, VXLL_MAGIC, sizeof(VXLL_MAGIC))) {
				Status = STATUS_FILE_INVALID;
				leave;
			}

			if (Context->Header->Version != VXLL_VERSION) {
				Status = STATUS_VERSION_MISMATCH;
				leave;
			}

			if (Context->OpenMode == GENERIC_WRITE) {
				//
				// If source application parameter was specified, make sure
				// that it is the same as what is in the log file.
				//

				if (SourceApplication) {
					UNICODE_STRING SourceApplicationFromFile;

					RtlInitUnicodeString(
						&SourceApplicationFromFile,
						Context->Header->SourceApplication);

					if (!RtlEqualUnicodeString(SourceApplication, &SourceApplicationFromFile, TRUE)) {
						Status = STATUS_SOURCE_APPLICATION_MISMATCH;
						leave;
					}
				}
			} else {
				//
				// Opened for reading - build the index.
				//

				Status = VxlpBuildIndex(Context);
				if (!NT_SUCCESS(Status)) {
					leave;
				}
			}
		}

		//
		// If opening the file for write access, mark the header as dirty.
		//

		if (Context->OpenMode == GENERIC_WRITE) {
			Context->Header->Dirty = TRUE;
			VxlpFlushLogFileHeader(Context);
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
		Status = GetExceptionCode();
	}

	SafeClose(SectionHandle);

	if (!NT_SUCCESS(Status)) {
		VxlCloseLog(&Context);
	}

	*LogHandle = Context;
	return Status;
}

NTSTATUS NTAPI VxlCloseLog(
	IN OUT	PVXLHANDLE		LogHandle)
{
	PVXLCONTEXT Context;

	if (!LogHandle) {
		return STATUS_INVALID_PARAMETER;
	}

	Context = *LogHandle;

	if (Context) {
		if (Context->OpenMode == GENERIC_WRITE && Context->Header != NULL) {
			Context->Header->Dirty = FALSE;
		}

		if (Context->MappedSection) {
			NtUnmapViewOfSection(NtCurrentProcess(), Context->MappedSection);
		}

		SafeClose(Context->FileHandle);
		SafeFree(Context->EntryIndexToFileOffset);
		SafeFree(*LogHandle);
	}

	return STATUS_SUCCESS;
}