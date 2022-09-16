#include <KexComm.h>
#include <NtDll.h>
#include "VxKexMon.h"
#include "resource.h"

//
// Contains functions for reporting an invocation of NtRaiseHardError in the
// monitored process.
//

STATIC INT_PTR CALLBACK VkmHeDlgProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam);

// Call this function only when the thread specified in ThreadHandle has just hit
// a breakpoint at the address of NtRaiseHardError. If you do not follow this
// instruction, the behavior will be undefined.
VOID VkmHardError(
	IN	PCWSTR	ExeBaseName,
	IN	HANDLE	ProcessHandle,
	IN	HANDLE	ThreadHandle)
{
	CONTEXT ThreadContext;
	VKM_HARD_ERROR_INFO ErrorInfo;
	ULONG_PTR pParameters;
	ULONG_PTR pParameter;

	// Get context of the faulting thread so we can extract the parameters of
	// NtRaiseHardError.
	ThreadContext.ContextFlags = CONTEXT_INTEGER;

	ErrorInfo.ExeBaseName = ExeBaseName;

	if (!GetThreadContext(ThreadHandle, &ThreadContext)) {
		ErrorInfo.Status = 0;
		DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_HARD_ERROR), NULL, VkmHeDlgProc, (LPARAM) &ErrorInfo);
	}

	// We need to read the parameters of the Hard Error.
	// Here is a quick reference for the information we need to code this.
	// We don't need to bother with ValidResponseOptions and Response.
	//
	// NTSTATUS NtRaiseHardError (						64 bit			32 bit
	//		IN NTSTATUS ErrorStatus,					RCX				[ESP]
	//		IN ULONG NumberOfParameters,				RDX				[ESP+4]
	//		IN ULONG UnicodeStringParameterMask,		R8				[ESP+8]
	//		IN PULONG_PTR Parameters,					R9				[ESP+12]
	//		IN ULONG ValidResponseOptions,
	//		OUT PULONG Response);
	//
	// The "Parameters" parameter is a pointer to an array of pointers to
	// either UNICODE_STRING structures or ULONGs.

#ifdef _M_X64
	ErrorInfo.Status	= (NTSTATUS) ThreadContext.Rcx;
	pParameters			= ThreadContext.R9;
#else
	// TODO
#endif

	// The possible NTSTATUS codes are:
	//   STATUS_IMAGE_MACHINE_TYPE_MISMATCH		- someone tried running an ARM/other architecture binary or dll (%s)
	//   STATUS_INVALID_IMAGE_FORMAT			- PE is corrupt (%s)
	//   STATUS_DLL_NOT_FOUND					- a DLL is missing (%s)
	//   STATUS_DLL_INIT_FAILED					- a DllMain function returned FALSE (%s)
	//   STATUS_ORDINAL_NOT_FOUND				- DLL ordinal function is missing (%d, %s)
	//   STATUS_ENTRYPOINT_NOT_FOUND			- DLL function is missing (%s, %s)

	VaRead(ProcessHandle, pParameters, &pParameter, sizeof(pParameter));

	if (ErrorInfo.Status == STATUS_ORDINAL_NOT_FOUND) {
		// first parameter is ULONG
		VaRead(ProcessHandle, pParameter, &ErrorInfo.Num1, sizeof(ErrorInfo.Num1));
	} else {
		// first parameter is PWSTR
		ULONG_PTR VaStringBuffer;
		USHORT StringLength;
		
		VaRead(ProcessHandle, pParameter + FIELD_OFFSET(UNICODE_STRING, Length), &StringLength, sizeof(StringLength));
		VaRead(ProcessHandle, pParameter + FIELD_OFFSET(UNICODE_STRING, Buffer), &VaStringBuffer, sizeof(VaStringBuffer));
		StringLength = min(StringLength, ARRAYSIZE(ErrorInfo.String1));
		VaRead(ProcessHandle, VaStringBuffer, ErrorInfo.String1, StringLength * sizeof(WCHAR));
	}

	if (ErrorInfo.Status == STATUS_ORDINAL_NOT_FOUND || ErrorInfo.Status == STATUS_ENTRYPOINT_NOT_FOUND) {
		// second parameter is PWSTR
		ULONG_PTR VaStringBuffer;
		USHORT StringLength;
		
		VaRead(ProcessHandle, pParameters + sizeof(PVOID), &pParameter, sizeof(pParameter));
		VaRead(ProcessHandle, pParameter + FIELD_OFFSET(UNICODE_STRING, Length), &StringLength, sizeof(StringLength));
		VaRead(ProcessHandle, pParameter + FIELD_OFFSET(UNICODE_STRING, Buffer), &VaStringBuffer, sizeof(VaStringBuffer));
		StringLength = min(StringLength, ARRAYSIZE(ErrorInfo.String2));
		VaRead(ProcessHandle, VaStringBuffer, ErrorInfo.String2, StringLength * sizeof(WCHAR));
	}

	DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_HARD_ERROR), NULL, VkmHeDlgProc, (LPARAM) &ErrorInfo);
}

STATIC INT_PTR CALLBACK VkmHeDlgProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	wParam,
	IN	LPARAM	lParam)
{
	PVKM_HARD_ERROR_INFO ErrorInfo;
	WCHAR ErrorStatement[MAX_PATH * 2 + 78];
	HRESULT Result;

	ErrorInfo = (PVKM_HARD_ERROR_INFO) lParam;

	switch (Message) {
	case WM_INITDIALOG:
		// play the error sound
		PlaySound((LPCWSTR) SND_ALIAS_SYSTEMHAND, NULL, SND_ALIAS_ID | SND_ASYNC | SND_SYSTEM);

		switch (ErrorInfo->Status) {
		case STATUS_IMAGE_MACHINE_TYPE_MISMATCH:
			SetWindowText(Window, L"Processor Architecture Mismatch");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"%s is designed for a different processor architecture. %s cannot start.",
				ErrorInfo->String1, ErrorInfo->ExeBaseName);
			break;
		case STATUS_INVALID_IMAGE_FORMAT:
			SetWindowText(Window, L"Invalid or Corrupt Executable or DLL");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"%s is invalid or corrupt. %s cannot start.",
				ErrorInfo->String1, ErrorInfo->ExeBaseName);
			break;
		case STATUS_DLL_NOT_FOUND:
			SetWindowText(Window, L"DLL Not Found");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"The required %s was not found while attempting to launch %s.",
				ErrorInfo->String1, ErrorInfo->ExeBaseName);
			break;
		case STATUS_DLL_INIT_FAILED:
			SetWindowText(Window, L"DLL Initialization Failed");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"%s failed to initialize. %s cannot start.",
				ErrorInfo->String1, ErrorInfo->ExeBaseName);
			break;
		case STATUS_ORDINAL_NOT_FOUND:
			SetWindowText(Window, L"Missing Function In DLL");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"The required ordinal function #%ld was not found in %s while attempting to launch %s.",
				ErrorInfo->Num1, ErrorInfo->String2, ErrorInfo->ExeBaseName);
			break;
		case STATUS_ENTRYPOINT_NOT_FOUND:
			SetWindowText(Window, L"Missing Function In DLL");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"The required function %s was not found in %s while attempting to launch %s.",
				ErrorInfo->String1, ErrorInfo->String2, ErrorInfo->ExeBaseName);
			break;
		default:
			SetWindowText(Window, L"Unknown Initialization Error");
			Result = StringCchPrintf(ErrorStatement, ARRAYSIZE(ErrorStatement),
				L"An unrecoverable fatal error has occurred during the initialization of %s.\r\n"
				L"The error code is 0x%08lx: %s",
				ErrorInfo->ExeBaseName, ErrorInfo->Status, NtStatusAsString(ErrorInfo->Status));
			break;
		}

		if (FAILED(Result)) {
			// an error while processing an error... WHAT FUN!
			WarningBoxF(L"Failed to format error message: 0x%08lx", Result);
			ErrorStatement[0] = '\0';
		}

		return TRUE;
	case WM_COMMAND:
		break;
	}

	return FALSE;
}