///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     test.c
//
// Abstract:
//
//     Utility for testing VXLL and creating large log files to test the
//     performance of VxlView.
//
// Author:
//
//     vxiiduu (14-Oct-2022)
//
// Revision History:
//
//     vxiiduu              14-Oct-2022  Initial creation.
//     vxiiduu              05-Jan-2023  Convert to user friendly NTSTATUS.
//
///////////////////////////////////////////////////////////////////////////////

#define KEX_TARGET_TYPE_EXE
#define KEX_COMPONENT L"LoggingTest"
#include <KexComm.h>
#include <KexDll.h>

#define TEST_LOG_FILE_NAME L"\\??\\C:\\Users\\vxiiduu\\Desktop\\Test.vxl"
VXLHANDLE LogHandle;

NTSTATUS NTAPI ThreadProc(
	IN	PVOID	Parameter)
{
	NTSTATUS Status;
	ULONG Index;
	VXLSEVERITY Severity;

	Severity = (VXLSEVERITY) (ULONG) Parameter;

	for (Index = 0; Index < 100000; ++Index) {
		Status = VxlWriteLog(
			LogHandle,
			KEX_COMPONENT,
			Severity,
			L"This is the %ld'th %s log entry.",
			Index,
			VxlSeverityToText(Severity, FALSE));

		if (!NT_SUCCESS(Status)) {
			DbgPrint("Failed to write log entry %ld of severity %ws: 0x%08lx\r\n",
					 Index, VxlSeverityToText(Severity, FALSE), Status);
		}
	}

	return Status;
}

NTSTATUS NTAPI EntryPoint(
	IN	PVOID	Parameter)
{
	NTSTATUS Status;
	HRESULT Result;
	UNICODE_STRING SourceApplication;
	UNICODE_STRING LogFileName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE ThreadHandles[6];
	ULONG Index;

	RtlInitConstantUnicodeString(&SourceApplication, L"VxKex");
	RtlInitConstantUnicodeString(&LogFileName, TEST_LOG_FILE_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &LogFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);

	Status = VxlOpenLog(
		&LogHandle,
		&SourceApplication,
		&ObjectAttributes,
		GENERIC_WRITE,
		FILE_OPEN_IF);

	VxlWriteLog(
		LogHandle,
		KEX_COMPONENT,
		LogSeverityInformation,
		L"Logging Test application has been started\r\n\r\n"
		L"This is a demonstration of multi-line log entries");

	//
	// Test making a shitload of source files, source functions and source components
	//

	for (Index = 0; Index < 70; ++Index) {
		WCHAR SourceFileString[64];
		WCHAR SourceFunctionString[64];
		WCHAR SourceComponentString[64];

		Result = StringCchPrintf(
			SourceFileString,
			ARRAYSIZE(SourceFileString),
			L"File%d", Index);

		Result = StringCchPrintf(
			SourceFunctionString,
			ARRAYSIZE(SourceFunctionString),
			L"Function%d", Index);

		Result = StringCchPrintf(
			SourceComponentString,
			ARRAYSIZE(SourceComponentString),
			L"Component%d", Index);

		Status = VxlWriteLogEx(
			LogHandle,
			SourceComponentString,
			SourceFileString,
			69420,
			SourceFunctionString,
			LogSeverityDebug,
			L"Test source file/function/component exceeding limit");

		if (!NT_SUCCESS(Status)) {
			DbgPrint("VxlWriteLogEx failed at iteration %d NTSTATUS 0x%08lx\r\n", Index, Status);
			break;
		}
	}

	for (Index = 0; Index < 6; ++Index) {
		Status = RtlCreateUserThread(
			NtCurrentProcess(),
			NULL,
			FALSE,
			0,
			0,
			0,
			ThreadProc,
			(PVOID) Index,
			&ThreadHandles[Index],
			NULL);

		if (!NT_SUCCESS(Status)) {
			DbgPrint("Failed to create thread #%d. NTSTATUS error code: %ws\r\n",
				Index, KexRtlNtStatusToString(Status));
			NtTerminateProcess(NtCurrentProcess(), Status);
		}
	}

	Status = NtWaitForMultipleObjects(
		ARRAYSIZE(ThreadHandles),
		ThreadHandles,
		WaitAllObject,
		FALSE,
		NULL);

	if (!NT_SUCCESS(Status)) {
		DbgPrint("Failed to wait for threads to exit. NTSTATUS error code: %ws\r\n",
			KexRtlNtStatusToString(Status));
		NtTerminateProcess(NtCurrentProcess(), Status);
	}

	Status = VxlCloseLog(&LogHandle);

	// no need to bother closing thread handles
	LdrShutdownProcess();
	return NtTerminateProcess(NtCurrentProcess(), Status);
}