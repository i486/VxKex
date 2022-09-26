#include <Windows.h>
#include <KexComm.h>
#include "VXLL.h"

PCWSTR TEST_LOG_FILE_NAME = L"C:\\Users\\vxiiduu\\Desktop\\Test.vxl";
PCWSTR TEST_EXPORT_FILE_NAME = L"C:\\Users\\vxiiduu\\Desktop\\Test.log";
VXLHANDLE LogHandle;

ULONG WINAPI ThreadProc(
	IN	PVOID	Parameter)
{
	VXLSTATUS Status;
	ULONG Index;
	PCWSTR Object1 = L"monster condom";
	PCWSTR Object2 = L"magnum dong";
	PCWSTR SevString = VxlSeverityLookup((VXLSEVERITY) (ULONG) Parameter, FALSE);

	for (Index = 1; Index <= 20000; Index++) {
		Status = VxlLog(LogHandle, (VXLSEVERITY) (ULONG) Parameter,
						L"WHOOPS I dropped my %lu'th %s %s that I use for my %s",
						Index, SevString, Object1, Object2);

		if (VXL_FAILED(Status)) {
			CriticalErrorBoxF(L"Failed to write log entry (threaded): %s.", VxlErrorLookup(Status));
		}
	}

	return 0;
}

VOID EntryPoint(
	VOID)
{
	PVXLLOGENTRY Entry;
	VXLSTATUS Status;
	HANDLE ThreadHandles[6];
	ULONG Index;
	ULONG EventCount;
	ULONG EventCountSize;

	SetFriendlyAppName(L"VXLL Test Application");

	//
	// Write test
	//

	Status = VxlOpenLogFileEx(
		TEST_LOG_FILE_NAME,
		&LogHandle,
		L"VxKex",
		L"loggingtest",
		VXL_OPEN_WRITE_ONLY | VXL_OPEN_OVERWRITE_IF_EXISTS | VXL_OPEN_CREATE_IF_NOT_EXISTS);

	if (VXL_FAILED(Status)) {
		CriticalErrorBoxF(L"Failed to open log file: %s.", VxlErrorLookup(Status));
	}

	Status = VxlLog(LogHandle, LogSeverityInformation, L"Application Started!");
	if (VXL_FAILED(Status)) {
		CriticalErrorBoxF(L"Failed to write log entry: %s.", VxlErrorLookup(Status));
	}

	Status = VxlLog(LogHandle, LogSeverityDetail, L"Your mother is homosexual, by the way.\r\nThis is a very very very very very very very long long long line of text intended to test the edit control's word break abilities and also to let you know how fucking stupid you are. What's wrong with you? Were you dropped on your head as a baby?");
	if (VXL_FAILED(Status)) {
		CriticalErrorBoxF(L"Failed to write detail log entry: %s.", VxlErrorLookup(Status));
	}

	//
	// Write test (multi-threaded)
	//

	ThreadHandles[0] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityCritical, 0, NULL);
	ThreadHandles[1] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityError, 0, NULL);
	ThreadHandles[2] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityWarning, 0, NULL);
	ThreadHandles[3] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityInformation, 0, NULL);
	ThreadHandles[4] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityDetail, 0, NULL);
	ThreadHandles[5] = CreateThread(NULL, 0, ThreadProc, (PVOID) LogSeverityDebug, 0, NULL);

	WaitForMultipleObjects(ARRAYSIZE(ThreadHandles), ThreadHandles, TRUE, INFINITE);

	Status = VxlCloseLogFile(&LogHandle);
	if (VXL_FAILED(Status)) {
		CriticalErrorBoxF(L"Failed to close log file: %s.", VxlErrorLookup(Status));
	}

	ExitProcess(0);
}