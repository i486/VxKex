#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI BOOL WINAPI GetOsSafeBootMode(
	OUT	PBOOL	IsSafeBootMode)
{
	*IsSafeBootMode = FALSE;
	return TRUE;
}

KXBASEAPI BOOL WINAPI GetFirmwareType(
	OUT	PFIRMWARE_TYPE	FirmwareType)
{
	*FirmwareType = FirmwareTypeUnknown;
	return TRUE;
}

KXBASEAPI UINT WINAPI Ext_GetSystemWindowsDirectoryW(
	OUT	PWSTR	Buffer,
	IN	UINT	Cch)
{
	//
	// ICU.dll (which is a pre-built DLL from Windows 10) looks for a data
	// file, icudtl.dat, inside %WinDir%\Globalization\ICU. This data file
	// is necessary for many .NET applications to function and it is not
	// present on Windows 7.
	//
	// We would like to avoid unnecessarily installing things into %WinDir%
	// and we also don't want to modify the DLL (doing so will complicate
	// debugging because the PDB file from Microsoft will no longer match).
	//
	// ICU calls GetSystemWindowsDirectoryW in order to obtain the address
	// of the Windows directory. We will intercept that call and return
	// KexDir instead, so that we can install the necessary ICU data files
	// inside KexDir.
	//
	// ICU does not call GetSystemWindowsDirectoryW for any purpose other than
	// to find the directory which contains its data files.
	//

	if (Buffer != NULL && Cch == MAX_PATH) {
		if (AshModuleBaseNameIs(ReturnAddress(), L"icu.dll")) {
			NTSTATUS Status;
			UNICODE_STRING Destination;

			Destination.Buffer = Buffer;
			Destination.Length = 0;
			Destination.MaximumLength = (USHORT) (Cch * sizeof(WCHAR));

			Status = RtlAppendUnicodeStringToString(&Destination, &KexData->KexDir);
			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				BaseSetLastNTError(Status);
				return 0;
			}

			Status = KexRtlNullTerminateUnicodeString(&Destination);
			ASSERT (NT_SUCCESS(Status));

			if (!NT_SUCCESS(Status)) {
				BaseSetLastNTError(Status);
				return 0;
			}

			return KexRtlUnicodeStringCch(&Destination);
		}
	}

	return GetSystemWindowsDirectoryW(Buffer, Cch);
}

KXBASEAPI BOOL WINAPI GetIsEdpEnabled(
	VOID)
{
	return FALSE;
}

KXBASEAPI NTSTATUS WINAPI SubscribeEdpEnabledStateChange(
	IN	PVOID	Callback,
	IN	PVOID	Unknown1,
	IN	PVOID	Unknown2)
{
	return STATUS_INSUFFICIENT_RESOURCES;
}

KXBASEAPI NTSTATUS WINAPI UnsubscribeEdpEnabledStateChange(
	IN	PVOID	Parameter)
{
	return STATUS_UNSUCCESSFUL;
}

KXBASEAPI BOOL WINAPI GetIsWdagEnabled(
	VOID)
{
	return FALSE;
}

KXBASEAPI NTSTATUS WINAPI SubscribeWdagEnabledStateChange(
	IN	PVOID	Callback,
	IN	PVOID	Unknown1,
	IN	PVOID	Unknown2)
{
	return STATUS_INSUFFICIENT_RESOURCES;
}

KXBASEAPI NTSTATUS WINAPI UnsubscribeWdagEnabledStateChange(
	IN	PVOID	Parameter)
{
	return STATUS_UNSUCCESSFUL;
}