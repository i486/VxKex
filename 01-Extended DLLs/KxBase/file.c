#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI HANDLE WINAPI CreateFile2(
	IN	PCWSTR								FileName,
	IN	ULONG								DesiredAccess,
	IN	ULONG								ShareMode,
	IN	ULONG								CreationDisposition,
	IN	PCREATEFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL)
{
	if (ExtendedParameters) {
		ULONG FlagsAndAttributes;

		if (ExtendedParameters->dwSize < sizeof(CREATEFILE2_EXTENDED_PARAMETERS)) {
			BaseSetLastNTError(STATUS_INVALID_PARAMETER);
			return INVALID_HANDLE_VALUE;
		}

		FlagsAndAttributes = ExtendedParameters->dwFileFlags |
							 ExtendedParameters->dwSecurityQosFlags |
							 ExtendedParameters->dwFileAttributes;

		return CreateFile(
			FileName,
			DesiredAccess,
			ShareMode,
			ExtendedParameters->lpSecurityAttributes,
			CreationDisposition,
			FlagsAndAttributes,
			ExtendedParameters->hTemplateFile);
	} else {
		return CreateFile(
			FileName,
			DesiredAccess,
			ShareMode,
			NULL,
			CreationDisposition,
			0,
			NULL);
	}
}

//
// Technically, these functions are supposed to behave differently when the
// current thread is running under the local SYSTEM account. However, we'll
// cross that bridge when we get there, since very very few programs actually
// run under those conditions.
//
// See Windows 11 kernelbase.dll for more information.
//
// For future reference:
// https://devblogs.microsoft.com/oldnewthing/20210106-00/?p=104669
//

KXBASEAPI ULONG WINAPI GetTempPath2A(
	IN	ULONG	BufferCch,
	OUT	PSTR	Buffer)
{
	return GetTempPathA(BufferCch, Buffer);
}

KXBASEAPI ULONG WINAPI GetTempPath2W(
	IN	ULONG	BufferCch,
	OUT	PWSTR	Buffer)
{
	return GetTempPathW(BufferCch, Buffer);
}

STATIC DWORD CALLBACK KxBasepCopyFile2ProgressRoutine(
	IN	LARGE_INTEGER	TotalFileSize,
	IN	LARGE_INTEGER	TotalBytesTransferred,
	IN	LARGE_INTEGER	StreamSize,
	IN	LARGE_INTEGER	StreamBytesTransferred,
	IN	DWORD			StreamNumber,
	IN	DWORD			CallbackReason,
	IN	HANDLE			SourceFile,
	IN	HANDLE			DestinationFile,
	IN	PVOID			Context OPTIONAL)
{
	COPYFILE2_MESSAGE Message;
	PCOPYFILE2_EXTENDED_PARAMETERS Copyfile2Parameters;

	ASSERT (Context != NULL);

	RtlZeroMemory(&Message, sizeof(Message));

	switch (CallbackReason) {
	case CALLBACK_CHUNK_FINISHED:
		Message.Type = COPYFILE2_CALLBACK_CHUNK_FINISHED;
		Message.Info.ChunkFinished.dwStreamNumber = StreamNumber;
		Message.Info.ChunkFinished.uliTotalFileSize.QuadPart = TotalFileSize.QuadPart;
		Message.Info.ChunkFinished.uliTotalBytesTransferred.QuadPart = TotalBytesTransferred.QuadPart;
		Message.Info.ChunkFinished.uliStreamSize.QuadPart = StreamSize.QuadPart;
		Message.Info.ChunkFinished.uliStreamBytesTransferred.QuadPart = StreamBytesTransferred.QuadPart;
		break;
	case CALLBACK_STREAM_SWITCH:
		Message.Type = COPYFILE2_CALLBACK_STREAM_STARTED;
		Message.Info.StreamStarted.dwStreamNumber = StreamNumber;
		Message.Info.StreamStarted.hDestinationFile = DestinationFile;
		Message.Info.StreamStarted.hSourceFile = SourceFile;
		Message.Info.StreamStarted.uliStreamSize.QuadPart = StreamSize.QuadPart;
		Message.Info.StreamStarted.uliTotalFileSize.QuadPart = TotalFileSize.QuadPart;
		break;
	default:
		ASSUME (FALSE);
	}

	Copyfile2Parameters = (PCOPYFILE2_EXTENDED_PARAMETERS) Context;

	return Copyfile2Parameters->pProgressRoutine(
		&Message,
		Copyfile2Parameters->pvCallbackContext);
}

KXBASEAPI HRESULT WINAPI CopyFile2(
	IN	PCWSTR							ExistingFileName,
	IN	PCWSTR							NewFileName,
	IN	PCOPYFILE2_EXTENDED_PARAMETERS	ExtendedParameters OPTIONAL)
{
	BOOL Success;
	ULONG EffectiveCopyFlags;

	if (ExtendedParameters == NULL) {
		Success = CopyFileW(
			ExistingFileName,
			NewFileName,
			FALSE);

		if (Success) {
			return S_OK;
		} else {
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	if (ExtendedParameters->dwSize != sizeof(COPYFILE2_EXTENDED_PARAMETERS)) {
		//
		// Windows 11 defines a COPYFILE2_EXTENDED_PARAMETERS_V2 struture.
		// When apps start using it, we will support it too.
		//

		KexLogWarningEvent(
			L"Unrecognized dwSize member of ExtendedParameters: %lu",
			ExtendedParameters->dwSize);

		return E_INVALIDARG;
	}

	if (ExtendedParameters->dwCopyFlags & ~COPY_FILE_ALL_VALID_FLAGS) {
		return E_INVALIDARG;
	}

	EffectiveCopyFlags = ExtendedParameters->dwCopyFlags & COPY_FILE_WIN7_VALID_FLAGS;

	Success = CopyFileExW(
		ExistingFileName,
		NewFileName,
		ExtendedParameters->pProgressRoutine ? KxBasepCopyFile2ProgressRoutine : NULL,
		ExtendedParameters,
		ExtendedParameters->pfCancel,
		EffectiveCopyFlags);

	if (Success) {
		return S_OK;
	} else {
		return HRESULT_FROM_WIN32(GetLastError());
	}
}

//KXBASEAPI HANDLE WINAPI Ext_CreateFileMappingW(
//	IN	HANDLE					FileHandle,
//	IN	PSECURITY_ATTRIBUTES	SecurityAttributes OPTIONAL,
//	IN	ULONG					PageProtection,
//	IN	ULONG					MaximumSizeHigh,
//	IN	ULONG					MaximumSizeLow,
//	IN	PCWSTR					Name OPTIONAL)
//{
//	HANDLE SectionHandle;
//	WCHAR GeneratedName[64];
//
//	//
//	// Windows 7 ignores security attributes on unnamed sections. Chromium checks
//	// this and crashes. In order to solve this problem we will give a random name
//	// to any unnamed section that has a security descriptor.
//	//
//
//	if (SecurityAttributes && SecurityAttributes->lpSecurityDescriptor && !Name) {
//		ULONGLONG RandomIdentifier;
//		PTEB Teb;
//
//		//
//		// Create a random identifier.
//		//
//
//		Teb = NtCurrentTeb();
//		NtQuerySystemTime((PLONGLONG) &RandomIdentifier);
//		RandomIdentifier *= (ULONG_PTR) Teb->ClientId.UniqueThread;
//		RandomIdentifier *= RtlRandomEx((PULONG) &RandomIdentifier);
//
//		StringCchPrintf(
//			GeneratedName,
//			ARRAYSIZE(GeneratedName),
//			L"VxKexRandomSectionName_%016I64x",
//			RandomIdentifier);
//
//		Name = GeneratedName;
//	}
//
//	SectionHandle = CreateFileMappingW(
//		FileHandle,
//		SecurityAttributes,
//		PageProtection,
//		MaximumSizeHigh,
//		MaximumSizeLow,
//		Name);
//
//	if (SectionHandle == NULL && Name == GeneratedName) {
//		KexDebugCheckpoint();
//	}
//
//	return SectionHandle;
//}