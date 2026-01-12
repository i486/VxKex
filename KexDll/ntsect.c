#include "buildcfg.h"
#include "kexdllp.h"

// Windows 8+ flag which is invalid on Win7.
#define SEC_IMAGE_NO_EXECUTE (SEC_IMAGE | SEC_NOCACHE)

NTSTATUS NTAPI Ext_NtCreateSection(
	OUT		PHANDLE						SectionHandle,
	IN		ULONG						DesiredAccess,
	IN		POBJECT_ATTRIBUTES			ObjectAttributes OPTIONAL,
	IN		PLONGLONG					MaximumSize OPTIONAL,
	IN		ULONG						PageAttributes,
	IN		ULONG						SectionAttributes,
	IN		HANDLE						FileHandle OPTIONAL) PROTECTED_FUNCTION
{
	NTSTATUS Status;
	BOOLEAN HaveRenamedObject;
	BOOLEAN NonDuplicableHandle;
	POBJECT_ATTRIBUTES OriginalObjectAttributes;
	OBJECT_ATTRIBUTES NewObjectAttributes;
	UNICODE_STRING NewObjectName;

	NonDuplicableHandle = FALSE;
	HaveRenamedObject = FALSE;

	if ((SectionAttributes & SEC_IMAGE_NO_EXECUTE) == SEC_IMAGE_NO_EXECUTE) {
		//
		// SEC_IMAGE_NO_EXECUTE is a combination of SEC_IMAGE and SEC_NOCACHE.
		// This combination is invalid on Win7, so we will remove SEC_NOCACHE.
		// SEC_IMAGE is still valid and desired in this case.
		//

		SectionAttributes &= ~SEC_NOCACHE;
	}

	//
	// Windows 7 ignores security attributes on unnamed sections. Chromium checks
	// this and crashes. In order to solve this problem we will give a random name
	// to any unnamed section that has a DACL.
	//

	if (ObjectAttributes &&
		ObjectAttributes->RootDirectory == NULL &&
		ObjectAttributes->ObjectName == NULL &&
		ObjectAttributes->SecurityDescriptor != NULL &&
		KexData->BaseNamedObjects != NULL) {
		
		WCHAR ObjectName[64];
		ULONGLONG RandomIdentifier;
		ULONG AdvancedLength;
		PTEB Teb;

		Teb = NtCurrentTeb();

		//
		// Create a random identifier.
		//

		NtQuerySystemTime((PLONGLONG) &RandomIdentifier);
		RandomIdentifier *= (ULONG_PTR) Teb->ClientId.UniqueProcess;
		RandomIdentifier *= (ULONG_PTR) Teb->ClientId.UniqueThread;
		RandomIdentifier += RtlRandomEx((PULONG) &RandomIdentifier);

		RtlInitEmptyUnicodeString(&NewObjectName, ObjectName, sizeof(ObjectName));
		Status = RtlAppendUnicodeToString(&NewObjectName, L"VxKexRandomSectionName_");
		ASSERT (NT_SUCCESS(Status));
		AdvancedLength = NewObjectName.Length;
		KexRtlAdvanceUnicodeString(&NewObjectName, NewObjectName.Length);
		Status = RtlInt64ToUnicodeString(RandomIdentifier, 16, &NewObjectName);
		ASSERT (NT_SUCCESS(Status));
		KexRtlRetreatUnicodeString(&NewObjectName, (USHORT) AdvancedLength);

		//
		// Fill out the new OBJECT_ATTRIBUTES structure which we will use instead
		// of the caller-supplied one.
		//

		ASSERT (VALID_HANDLE(KexData->BaseNamedObjects));
		NewObjectAttributes = *ObjectAttributes;
		NewObjectAttributes.RootDirectory = KexData->BaseNamedObjects;
		NewObjectAttributes.ObjectName = &NewObjectName;

		//
		// TODO: set some kind of security which stops other people opening this
		// named object.
		//

		OriginalObjectAttributes = ObjectAttributes;
		ObjectAttributes = &NewObjectAttributes;

		HaveRenamedObject = TRUE;
	}

RetryAfterError:
	Status = KexNtCreateSection(
		SectionHandle,
		DesiredAccess,
		ObjectAttributes,
		MaximumSize,
		PageAttributes,
		SectionAttributes,
		FileHandle);

	if (HaveRenamedObject && !NT_SUCCESS(Status)) {
		if (ObjectAttributes->RootDirectory == KexData->UntrustedNamedObjects) {
			KexDebugCheckpoint();

			// fall back to original ObjectAttributes structure
			ObjectAttributes = OriginalObjectAttributes;
			HaveRenamedObject = FALSE;
		} else {
			ASSERT (VALID_HANDLE(KexData->UntrustedNamedObjects));
			ObjectAttributes->RootDirectory = KexData->UntrustedNamedObjects;
		}
		
		goto RetryAfterError;
	}

	return Status;
} PROTECTED_FUNCTION_END