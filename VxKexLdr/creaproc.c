#include "buildcfg.h"
#include "vxkexldr.h"

BOOLEAN VklCreateProcess(
	IN	PCWSTR	Path,
	IN	PCWSTR	Arguments OPTIONAL)
{
	BOOLEAN Success;
	HINSTANCE ShellExecuteError;

	ASSERT (Path != NULL);

	if (Arguments != NULL && Arguments[0] == '\0') {
		Arguments = NULL;
	}

	if (!PathIsRelative(Path) && StringEqualI(PathFindExtension(Path), L".exe")) {
		NTSTATUS Status;
		ULONG ErrorCode;
		ULONG ProcessCreationFlags;
		STARTUPINFOEX StartupInfo;
		PROCESS_INFORMATION ProcessInformation;
		SIZE_T ProcThreadAttributeListCb;
		PROCESS_BASIC_INFORMATION BasicInformation;
		OBJECT_ATTRIBUTES ObjectAttributes;
		CLIENT_ID ParentProcessClientId;
		HANDLE ParentProcessHandle;
		PWSTR ArgumentsWithExeName;

		//
		// We are opening an EXE. Use CreateProcess to do this.
		//

		ParentProcessHandle = NULL;
		ProcessCreationFlags = 0;

		//
		// Deal with the StartupInfo and proc/thread attribute stuff.
		// First of all we need to get a handle to our parent process (which is
		// very likely to be Explorer).
		//

		Status = NtQueryInformationProcess(
			NtCurrentProcess(),
			ProcessBasicInformation,
			&BasicInformation,
			sizeof(BasicInformation),
			NULL);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			goto NoExtendedStartupInfo;
		}

		ParentProcessClientId.UniqueProcess = (HANDLE) BasicInformation.InheritedFromUniqueProcessId;
		ParentProcessClientId.UniqueThread = NULL;

		InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

		Status = NtOpenProcess(
			&ParentProcessHandle,
			PROCESS_CREATE_PROCESS,
			&ObjectAttributes,
			&ParentProcessClientId);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			//
			// The NtOpenProcess call can fail if the parent process has exited.
			//

			goto NoExtendedStartupInfo;
		}

		//
		// Now we need to find the size, allocate, and initialize a buffer for
		// the process/thread attribute list.
		//
		// Note: We don't bother calling DeleteProcThreadAttributeList. That
		// function does absolutely nothing on Windows 7.
		//
		// Note: InitializeProcThreadAttributeList and UpdateProcThreadAttribute
		// cannot fail unless passed invalid parameters. That is why we don't bother
		// checking their return values.
		//
		// The magic number 48 is the number of bytes required for one proc/thread
		// attribute. The formula for calculating the number of bytes required
		// for any particular number of proc/thread attributes is:
		//
		//   (n + 1) * 24
		//

		ProcThreadAttributeListCb = 48;

		StartupInfo.lpAttributeList =
			(PPROC_THREAD_ATTRIBUTE_LIST) StackAlloc(BYTE, ProcThreadAttributeListCb);

		InitializeProcThreadAttributeList(
			StartupInfo.lpAttributeList,
			1,
			0,
			&ProcThreadAttributeListCb);

		UpdateProcThreadAttribute(
			StartupInfo.lpAttributeList,
			0,
			PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
			&ParentProcessHandle,
			sizeof(ParentProcessHandle),
			NULL,
			NULL);

		ProcessCreationFlags |= EXTENDED_STARTUPINFO_PRESENT;

NoExtendedStartupInfo:

		GetStartupInfo(&StartupInfo.StartupInfo);
		StartupInfo.StartupInfo.wShowWindow = SW_SHOW;

		if (ProcessCreationFlags & EXTENDED_STARTUPINFO_PRESENT) {
			StartupInfo.StartupInfo.cb = sizeof(StartupInfo);
		}

		//
		// If we have a command line specified, we need to prepend the executable
		// name to it (quoted).
		//

		if (Arguments) {
			HRESULT Result;
			SIZE_T Cch;

			Cch = 1 +					// open quote
				  wcslen(Path) +
				  1 +					// close quote
				  1 +					// space
				  wcslen(Arguments) +
				  1;					// null terminator

			ArgumentsWithExeName = StackAlloc(WCHAR, Cch);

			Result = StringCchPrintf(
				ArgumentsWithExeName,
				Cch,
				L"\"%s\" %s",
				Path,
				Arguments);

			ASSERT (SUCCEEDED(Result));
		} else {
			ArgumentsWithExeName = NULL;
		}

		//
		// Actually create our child process.
		//

		Success = CreateProcess(
			Path,
			ArgumentsWithExeName,
			NULL,
			NULL,
			FALSE,
			ProcessCreationFlags,
			NULL,
			NULL,
			&StartupInfo.StartupInfo,
			&ProcessInformation);

		ErrorCode = GetLastError();
		SafeClose(ParentProcessHandle);

		if (Success) {
			SafeClose(ProcessInformation.hProcess);
			SafeClose(ProcessInformation.hThread);
			return TRUE;
		} else {
			if (ErrorCode == ERROR_INVALID_PARAMETER) {
				// Misleading error code. Change it so the user can get
				// a better error message.
				ErrorCode = ERROR_FILE_NOT_FOUND;
			}

			if (ErrorCode == ERROR_ELEVATION_REQUIRED) {
				// Fall through to the ShellExecute code.
				NOTHING;
			} else {
				ErrorBoxF(L"CreateProcess failed: %s", Win32ErrorAsString(ErrorCode));
				return FALSE;
			}
		}
	}

	//
	// We are either opening a non-EXE file (such as a MSI), or we are opening
	// an EXE file that requires UAC elevation, or the user did not specify
	// a full path to an EXE file (including extension). In this case we will
	// fall back to ShellExecute, which is slower and less flexible but will
	// deal with any conditions you can think of.
	//

	ShellExecuteError = ShellExecute(
		KexgApplicationMainWindow,
		L"open",
		Path,
		Arguments,
		NULL,
		SW_SHOW);

	if ((ULONG) ShellExecuteError <= 32) {
		PCWSTR ErrorMessage;

		switch ((ULONG) ShellExecuteError) {
		case 0:
		case SE_ERR_OOM:
			ErrorMessage = L"Insufficient resources.";
			break;
		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
		case ERROR_BAD_FORMAT:
			ErrorMessage = Win32ErrorAsString((ULONG) ShellExecuteError);
			break;
		case SE_ERR_ACCESSDENIED:
			ErrorMessage = L"Access was denied.";
			break;
		case SE_ERR_DLLNOTFOUND:
			ErrorMessage = L"A DLL was not found.";
			break;
		case SE_ERR_SHARE:
			ErrorMessage = L"A sharing violation occurred.";
			break;
		case SE_ERR_ASSOCINCOMPLETE:
			ErrorMessage = L"SE_ERR_ASSOCINCOMPLETE.";
			break;
		case SE_ERR_DDEBUSY:
			ErrorMessage = L"SE_ERR_DDEBUSY.";
			break;
		case SE_ERR_DDEFAIL:
			ErrorMessage = L"SE_ERR_DDEFAIL.";
			break;
		case SE_ERR_DDETIMEOUT:
			ErrorMessage = L"SE_ERR_DDETIMEOUT.";
			break;
		case SE_ERR_NOASSOC:
			ErrorMessage = L"SE_ERR_NOASSOC.";
			break;
		default:
			ASSERT (FALSE);
			ErrorMessage = L"Unknown error.";
			break;
		}

		ErrorBoxF(L"ShellExecute failed: %s", ErrorMessage);
		return FALSE;
	}

	return TRUE;
}