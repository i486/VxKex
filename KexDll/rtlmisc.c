///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     kexrtl.c
//
// Abstract:
//
//     RTL functions added from newer Windows versions
//
// Author:
//
//     vxiiduu (06-May-2026)
//
// Revision History:
//
//     vxiiduu              06-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kexdllp.h"

// Compatible with RtlSetBit from win8+.
KEXAPI VOID NTAPI RtlSetBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber)
{
	_bittestandset((PLONG) BitmapHeader->Buffer, BitNumber);
}

// Compatible with RtlClearBit from win8+.
KEXAPI VOID NTAPI RtlClearBit(
	IN	PRTL_BITMAP	BitmapHeader,
	IN	ULONG		BitNumber)
{
	_bittestandreset((PLONG) BitmapHeader->Buffer, BitNumber);
}

#ifndef _M_X64
typedef PVOID TYPEDEF_TYPE_NAME(RUNTIME_FUNCTION);
#endif

KEXAPI NTSTATUS NTAPI RtlAddGrowableFunctionTable(
	OUT	PPVOID				DynamicTable,
	IN	PRUNTIME_FUNCTION	FunctionTable,
	IN	ULONG				EntryCount,
	IN	ULONG				MaximumEntryCount,
	IN	ULONG_PTR			RangeBase,
	IN	ULONG_PTR			RangeEnd)
{
#ifdef _M_X64
	BOOLEAN Success;

	Success = RtlAddFunctionTable(FunctionTable, MaximumEntryCount, RangeBase);

	if (Success) {
		*DynamicTable = NULL;
		return STATUS_SUCCESS;
	} else {
		return STATUS_UNSUCCESSFUL;
	}
#else
	ASSERT (FALSE);
	return STATUS_NOT_IMPLEMENTED;
#endif
}

// Required by mono-2.0-bdwgc.dll which is a part of Unity.
KEXAPI VOID NTAPI RtlDeleteGrowableFunctionTable(
	IN	PVOID	DynamicTable)
{
#ifdef _M_X64
	RtlDeleteFunctionTable(DynamicTable);
#else
	ASSERT (FALSE);
#endif
}

//
// Similar to RtlFindUnicodeSubstring in Win10 NTDLL (but does not
// respect NLS).
// Returns the address of the character in Haystack where Needle starts,
// or NULL if Needle could not be found.
//
KEXAPI PWCHAR NTAPI RtlFindUnicodeSubstring(
	PCUNICODE_STRING	Haystack,
	PCUNICODE_STRING	Needle,
	BOOLEAN				CaseInsensitive)
{
	ULONG LengthOfNeedle;
	ULONG LengthOfHaystack;
	PWCHAR NeedleBuffer;
	PWCHAR NeedleBufferEnd;
	PWCHAR HaystackBuffer;
	PWCHAR HaystackBufferEnd;
	PWCHAR HaystackBufferRealEnd;
	PWCHAR StartOfNeedleInHaystack;
	WCHAR NeedleFirst;

	LengthOfNeedle = Needle->Length & ~1;
	LengthOfHaystack = Haystack->Length & ~1;

	if (LengthOfNeedle > LengthOfHaystack || !LengthOfHaystack || !LengthOfNeedle) {
		return NULL;
	}

	NeedleBuffer = Needle->Buffer;
	NeedleBufferEnd = (PWCHAR) (((PBYTE) NeedleBuffer) + LengthOfNeedle);
	HaystackBuffer = Haystack->Buffer;
	HaystackBufferEnd = (PWCHAR) (((PBYTE) HaystackBuffer) + LengthOfHaystack - LengthOfNeedle);
	HaystackBufferRealEnd = (PWCHAR) (((PBYTE) HaystackBufferEnd) + LengthOfNeedle);

	if (CaseInsensitive) {
		NeedleFirst = ToUpper(*NeedleBuffer);

		while (TRUE) {
			NeedleBuffer = Needle->Buffer + 1;

			while (ToUpper(*HaystackBuffer) != NeedleFirst) {
				++HaystackBuffer; // Multiple evaluation. Can't increment inside macro

				if (HaystackBuffer > HaystackBufferEnd) {
					return NULL;
				}
			}

			StartOfNeedleInHaystack = HaystackBuffer++;

			while (ToUpper(*HaystackBuffer) == ToUpper(*NeedleBuffer)) {
				++HaystackBuffer;
				++NeedleBuffer;

				if (HaystackBuffer > HaystackBufferRealEnd) {
					break;
				} else if (NeedleBuffer >= NeedleBufferEnd) {
					return StartOfNeedleInHaystack;
				}
			}
		}
	} else {
		NeedleFirst = *NeedleBuffer;

		while (TRUE) {
			NeedleBuffer = Needle->Buffer + 1;

			while (*HaystackBuffer++ != NeedleFirst) {
				if (HaystackBuffer > HaystackBufferEnd) {
					return NULL;
				}
			}

			StartOfNeedleInHaystack = HaystackBuffer - 1;

			while (*HaystackBuffer++ == *NeedleBuffer++) {
				if (HaystackBuffer > HaystackBufferRealEnd) {
					break;
				} else if (NeedleBuffer >= NeedleBufferEnd) {
					return StartOfNeedleInHaystack;
				}
			}
		}
	}
}

KEXAPI LONGLONG NTAPI RtlGetSystemTimePrecise(
	VOID)
{
	NTSTATUS Status;
	LONGLONG SystemTime;

	//
	// The real NtQuerySystemTime export from NTDLL is actually just a jump to
	// RtlQuerySystemTime, which reads from SharedUserData.
	//
	// However, if we are doing SharedUserData-based version spoofing, we will
	// overwrite that stub function with KexNtQuerySystemTime, so it is the best
	// of both worlds in terms of speed and actually working.
	//

	Status = NtQuerySystemTime(&SystemTime);
	ASSERT (NT_SUCCESS(Status));

	return SystemTime;
}

KEXAPI VOID NTAPI RtlGetDeviceFamilyInfoEnum(
	OUT	PULONGLONG	UAPInfo OPTIONAL,
	OUT	PULONG		DeviceFamily OPTIONAL,
	OUT	PULONG		DeviceForm OPTIONAL)
{
	if (UAPInfo != NULL) {
		// The 3570 is an approximate number from Win10 build 19042
		// UBR = update build revision
		*UAPInfo = (NtCurrentPeb()->OSBuildNumber << 16) + 3570;
	}

	if (DeviceFamily) {
		*DeviceFamily =	DEVICEFAMILYINFOENUM_DESKTOP;
	}

	if (DeviceForm) {
		*DeviceForm = DEVICEFAMILYDEVICEFORM_DESKTOP;
	}
}

// The real data type for the parameter is PWNF_USER_SUBSCRIPTION.
KEXAPI NTSTATUS NTAPI RtlUnsubscribeWnfNotificationWaitForCompletion(
	IN	PVOID	Subscription)
{
	return STATUS_SUCCESS;
}

// Microsoft documentation incorrectly lists TargetPath as an IN parameter, but it
// is actually an OUT parameter.
KEXAPI NTSTATUS NTAPI RtlGetPersistedStateLocation(
	IN	PCWSTR				SourceID,
	IN	PCWSTR				CustomValue OPTIONAL,
	IN	PCWSTR				DefaultPath OPTIONAL,
	IN	STATE_LOCATION_TYPE	StateLocationType,
	OUT	PWCHAR				TargetPath,
	IN	ULONG				BufferCbIn,
	OUT	PULONG				BufferCbOut OPTIONAL)
{
	if (StateLocationType >= StateLocationTypeMaximum) {
		return STATUS_INVALID_PARAMETER_3;
	}

	if (DefaultPath) {
		ULONG DefaultPathCbWithNullTerminator;

		//
		// Fun fact, the Win11 NTDLL checks for integer overflow while doing these
		// string length calculations and can return STATUS_INTEGER_OVERFLOW.
		//
		// Of course we won't be doing that because any app that passes in a >2GB
		// string which is supposed to be representing a registry path is already
		// broken beyond repair.
		//

		DefaultPathCbWithNullTerminator = ((ULONG) wcslen(DefaultPath) + 1) * sizeof(WCHAR);

		if (BufferCbOut) {
			*BufferCbOut = DefaultPathCbWithNullTerminator;
		}

		if (DefaultPathCbWithNullTerminator > BufferCbIn) {
			return STATUS_BUFFER_OVERFLOW;
		}

		RtlMoveMemory(TargetPath, DefaultPath, DefaultPathCbWithNullTerminator);
		return STATUS_SUCCESS;
	}

	KexLogDebugEvent(
		L"Failed attempt to find a persisted state location\r\n\r\n"
		L"SourceID = %s\r\n"
		L"CustomValue = %s\r\n"
		L"DefaultPath = %s\r\n"
		L"StateLocationType = %d\r\n"
		L"TargetPath = 0x%p\r\n"
		L"BufferCbIn = %d\r\n"
		L"BufferCbOut = 0x%p",
		SourceID,
		CustomValue,
		DefaultPath,
		StateLocationType,
		TargetPath,
		BufferCbIn,
		BufferCbOut);

	//
	// This is the error code that Win11 NTDLL returns when DefaultPath is not
	// supplied and it cannot find \Registry\Machine\System\CurrentControlSet\
	// Control\StateSeparation\RedirectionMap\{Keys,Files} where Keys or Files
	// is based on the StateLocationType parameter.
	//
	// If it CAN find this registry location, then it will give a target path
	// based on a subkey read out of this location. Windows 7 of course does not
	// have this redirection map, so we don't even try to find it.
	//

	return STATUS_OBJECT_NAME_NOT_FOUND;
}

//
// Full implementation based on Win10 NTDLL decompilation.
//
KEXAPI NTSTATUS NTAPI RtlCanonicalizeDomainName(
	OUT	PUNICODE_STRING		DestinationString,
	IN	PCUNICODE_STRING	SourceString,
	IN	BOOLEAN				Strict)
{
	BOOLEAN Success;
	NTSTATUS Status;
	ULONG Index;
	ULONG ScopeId;
	UNICODE_STRING RawName;
	USHORT Port;
	IN_ADDR Ipv4Address;
	IN6_ADDR Ipv6Address;
	ULONG CanonicalNameLength;
	ULONG PunycodedNameLength;
	WCHAR CanonicalNameBuffer[256];
	WCHAR PunycodedNameBuffer[256];
	WCHAR RawNameBuffer[256];

	CanonicalNameLength = ARRAYSIZE(CanonicalNameBuffer);
	PunycodedNameLength = ARRAYSIZE(PunycodedNameBuffer);

	RtlInitEmptyUnicodeString(&RawName, RawNameBuffer, sizeof(RawNameBuffer));
	RtlCopyUnicodeString(&RawName, SourceString);

	if (RawName.Length == RawName.MaximumLength) {
		return STATUS_INVALID_IDN_NORMALIZATION;
	}

	//
	// Try parse as IPv6.
	//

	Status = RtlIpv6StringToAddressExW(
		RawName.Buffer,
		&Ipv6Address,
		&ScopeId,
		&Port);

	if (NT_SUCCESS(Status) && Port == 0) {
		//
		// We could parse this address as IPv6.
		// Convert the IPv6 struct back into a string - as an IPv6 string if it
		// is a true IPv6 address, or an IPv4 string if it is a mapped IPv4 address.
		//

		if (IN6_IS_ADDR_V4MAPPED(&Ipv6Address) && ScopeId == 0) {
			// Convert the IPv6-formatted IPv4 address into a real IPv4 address
			RtlCopyMemory(
				&Ipv4Address,
				IN6_GET_ADDR_V4MAPPED(&Ipv6Address),
				sizeof(Ipv4Address));

			Status = RtlIpv4AddressToStringExW(
				&Ipv4Address,
				Port,
				CanonicalNameBuffer,
				&CanonicalNameLength);
		} else {
			Status = RtlIpv6AddressToStringExW(
				&Ipv6Address,
				ScopeId,
				Port,
				CanonicalNameBuffer,
				&CanonicalNameLength);
		}

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
		Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
		return Status;
	}

	//
	// Try parse as IPv4.
	//

	Status = RtlIpv4StringToAddressExW(
		RawName.Buffer,
		Strict,
		&Ipv4Address,
		&Port);

	if (NT_SUCCESS(Status) && Port == 0) {
		//
		// We could parse the string as IPv4. Convert it back to a string.
		//

		Status = RtlIpv4AddressToStringExW(
			&Ipv4Address,
			Port,
			CanonicalNameBuffer,
			&CanonicalNameLength);

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
		Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
		return Status;
	}

	//
	// Try parse as IDN (internationalized domain name), and convert to punycode
	//

	Status = RtlIdnToAscii(
		0,
		SourceString->Buffer,
		KexRtlUnicodeStringCch(SourceString),
		PunycodedNameBuffer,
		&PunycodedNameLength);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Lowercase the Punycode representation
	//

	for (Index = 0; Index < PunycodedNameLength; ++Index) {
		// Note: we're using towlower (ntdll CRT) instead of ToLower (vxkex macro)
		// because ToLower does not handle non-ASCII characters.
		PunycodedNameBuffer[Index] = towlower(PunycodedNameBuffer[Index]);
	}

	//
	// Convert it back to proper Unicode
	//

	Status = RtlIdnToUnicode(
		0,
		PunycodedNameBuffer,
		PunycodedNameLength,
		CanonicalNameBuffer,
		&CanonicalNameLength);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	if (CanonicalNameLength >= ARRAYSIZE(CanonicalNameBuffer)) {
		// potential buffer overflow
		return STATUS_INVALID_IDN_NORMALIZATION;
	}

	// Ensure null termination.
	// I'm not sure whether RtlIdnToUnicode guarantees a null terminated buffer,
	// but since it works with explicit length variables, it probably doesn't.
	// Win10 code does do this so it's probably required.
	CanonicalNameBuffer[CanonicalNameLength] = '\0';

	Success = RtlCreateUnicodeString(DestinationString, CanonicalNameBuffer);
	Status = Success ? STATUS_SUCCESS : STATUS_NO_MEMORY;
	return Status;
}

//
// Stubs.
//

KEXAPI NTSTATUS NTAPI RtlQueryPackageIdentity(
	IN		PVOID		TokenObject,
	OUT		PWSTR		PackageFullName,
	IN OUT	PSIZE_T		PackageSize,
	OUT		PWSTR		AppId,
	IN OUT	PSIZE_T		AppIdSize,
	OUT		PBOOLEAN	Packaged)
{
	return STATUS_NOT_FOUND;
}

KEXAPI NTSTATUS NTAPI RtlQueryPackageIdentityEx(
	IN		PVOID		TokenObject,
	OUT		PWSTR		PackageFullName,
	IN OUT	PSIZE_T		PackageSize,
	OUT		PWSTR		AppId,
	IN OUT	PSIZE_T		AppIdSize,
	OUT		LPGUID		DynamicId OPTIONAL,
	OUT		PULONG64	Flags)
{
	return STATUS_NOT_FOUND;
}

KEXAPI NTSTATUS NTAPI RtlCheckPortableOperatingSystem(
	OUT	PBOOLEAN	IsPortable)
{
	*IsPortable = FALSE;
	return STATUS_SUCCESS;
}

KEXAPI NTSTATUS NTAPI RtlQueryWnfStateData(
	PULONG		ChangeStamp,
	ULONGLONG	StateName,
	PVOID		Callback,
	PVOID		CallbackContext,
	PULONG		TypeId)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI RtlPublishWnfStateData(
	ULONGLONG	StateName,
	PVOID		TypeId,
	PVOID		StateData,
	ULONG		StateDataLength,
	PCVOID		ExplicitScope)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI RtlSubscribeWnfStateChangeNotification(
	PVOID		Subscription,
	ULONGLONG	StateName,
	ULONG		ChangeStamp,
	PVOID		Callback,
	PVOID		CallbackContext,
	PVOID		TypeId,
	ULONG		SerializationGroupIndex)
{
	return STATUS_NOT_IMPLEMENTED;
}

KEXAPI NTSTATUS NTAPI RtlUnsubscribeWnfStateChangeNotification(
	IN	PVOID	Subscription)
{
	return STATUS_NOT_IMPLEMENTED;
}

//
// Based on decompiled Win10 code.
// Does what it says on the tin: buffer must be all zeroes to return TRUE,
// otherwise return FALSE.
//
KEXAPI BOOLEAN NTAPI RtlIsZeroMemory(
	IN	PCVOID	Buffer,
	IN	SIZE_T	BufferCb)
{
	// Align the input buffer to a multiple of the pointer size.
	while (BufferCb > 0 && ((ULONG_PTR) Buffer & (sizeof(PVOID) - 1)) != 0) {
		if (*(PBYTE) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PBYTE) Buffer + 1;
		--BufferCb;
	}

	// Process the buffer in pointer-sized pieces
	while (BufferCb >= sizeof(PVOID)) {
		if (*(PULONG_PTR) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PULONG_PTR) Buffer + 1;
		BufferCb -= sizeof(PVOID);
	}

	if (BufferCb == 0) {
		return TRUE;
	}

	// Handle remaining bytes at the end.
	do {
		if (*(PBYTE) Buffer != 0) {
			return FALSE;
		}

		Buffer = (PBYTE) Buffer + 1;
		--BufferCb;
	} while (BufferCb > 0);

	return TRUE;
}

//
// This is just kernel32!IsProcessorFeaturePresent on win7, but they renamed it
// and moved it to NTDLL on Windows 10 and higher.
//
KEXAPI BOOLEAN NTAPI RtlIsProcessorFeaturePresent(
	IN	ULONG	ProcessorFeature)
{
	if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) {
		return FALSE;
	}

	return SharedUserData->ProcessorFeatures[ProcessorFeature];
}