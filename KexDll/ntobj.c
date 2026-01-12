#include "buildcfg.h"
#include "kexdllp.h"

//
// Note: This function is not perfect and does not give the right answer in
// all cases.
//
// If we had a kernel mode support driver, this would be trivial to implement.
//
// Returns STATUS_SUCCESS if the two handles refer to the same kernel object.
// Returns STATUS_NOT_SAME_OBJECT if the handles refer to different objects.
// Returns another NTSTATUS value on error.
//
NTSTATUS NTAPI NtCompareObjects(
	IN	HANDLE	FirstObjectHandle,
	IN	HANDLE	SecondObjectHandle)
{
	NTSTATUS Status;
	OBJECT_BASIC_INFORMATION BasicInformation1;
	OBJECT_BASIC_INFORMATION BasicInformation2;
	POBJECT_TYPE_INFORMATION TypeInformation1;
	POBJECT_TYPE_INFORMATION TypeInformation2;
	PUNICODE_STRING NameInformation1;
	PUNICODE_STRING NameInformation2;
	BOOLEAN ObjectTypesAreEqual;
	BOOLEAN ObjectNamesAreEqual;
	ULONG RequiredSize;

	UNICODE_STRING ProcessTypeString;
	UNICODE_STRING ThreadTypeString;
	UNICODE_STRING EventTypeString;

	//
	// Fetch and compare type information for both objects.
	// This shouldn't fail.
	//

	Status = NtQueryObject(
		FirstObjectHandle,
		ObjectTypeInformation,
		NULL,
		0,
		&RequiredSize);

	if (Status != STATUS_INFO_LENGTH_MISMATCH) {
		// e.g. invalid object handle
		ASSERT (Status == STATUS_INFO_LENGTH_MISMATCH);
		return Status;
	}

	if (FirstObjectHandle == SecondObjectHandle) {
		//
		// If the two handles are numerically the same, then of course they refer to the
		// same object.
		//
		// Note that we do this simple check after calling NtQueryObject on one handle.
		// The reason for this is that if the caller passes two garbage values that aren't
		// actual handles, then NtQueryObject will catch that.
		//

		return STATUS_SUCCESS;
	}

	TypeInformation1 = (POBJECT_TYPE_INFORMATION) StackAlloc(BYTE, RequiredSize);

	Status = NtQueryObject(
		FirstObjectHandle,
		ObjectTypeInformation,
		TypeInformation1,
		RequiredSize,
		NULL);

	if (!NT_SUCCESS(Status)) {
		ASSERT (NT_SUCCESS(Status));
		return Status;
	}

	// Get object type of 2nd handle.
	// Re-use the same length value as for the 1st handle.
	TypeInformation2 = (POBJECT_TYPE_INFORMATION) StackAlloc(BYTE, RequiredSize);

	Status = NtQueryObject(
		SecondObjectHandle,
		ObjectTypeInformation,
		TypeInformation2,
		RequiredSize,
		NULL);

	if (Status == STATUS_INFO_LENGTH_MISMATCH) {
		// If NtQueryObject returns STATUS_INFO_LENGTH_MISMATCH, then it means the
		// object types are different, because the lengths are different.
		return STATUS_NOT_SAME_OBJECT;
	}

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	ObjectTypesAreEqual = RtlEqualUnicodeString(
		&TypeInformation1->TypeName,
		&TypeInformation2->TypeName,
		FALSE);

	if (!ObjectTypesAreEqual) {
		// The two objects are of different type.
		// We know for certain that they can't be the same.
		return STATUS_NOT_SAME_OBJECT;
	}

	//
	// Now, we've confirmed that the type of both objects are the same. We could
	// check the name of the objects to find out whether they're the same object.
	// However, this isn't enough. There are many objects which have no name.
	//
	// Our ability to detect differences in these cases is rather limited. For
	// processes and threads, we can check the process ID and thread ID to obtain
	// the answer with certainty.
	//
	// For events, we can check the EVENT_TYPE.
	//
	// For all other object types, we will use a heuristic approach which works in
	// some cases. Unfortunately, in the cases this doesn't work, we simply have to
	// give an answer which may be incorrect.
	//

	RtlInitConstantUnicodeString(&ProcessTypeString, L"Process");
	RtlInitConstantUnicodeString(&ThreadTypeString, L"Thread");
	RtlInitConstantUnicodeString(&EventTypeString, L"Event");

	if (RtlEqualUnicodeString(&TypeInformation1->TypeName, &ProcessTypeString, FALSE)) {
		PROCESS_BASIC_INFORMATION ProcessBasicInformation1;
		PROCESS_BASIC_INFORMATION ProcessBasicInformation2;

		//
		// Both handles refer to a process.
		//

		Status = NtQueryInformationProcess(
			FirstObjectHandle,
			ProcessBasicInformation,
			&ProcessBasicInformation1,
			sizeof(ProcessBasicInformation1),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		Status = NtQueryInformationProcess(
			SecondObjectHandle,
			ProcessBasicInformation,
			&ProcessBasicInformation2,
			sizeof(ProcessBasicInformation2),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		//
		// Ok, now check whether the process IDs are equal.
		//

		if (ProcessBasicInformation1.UniqueProcessId == ProcessBasicInformation2.UniqueProcessId) {
			return STATUS_SUCCESS;
		} else {
			return STATUS_NOT_SAME_OBJECT;
		}
	} else if (RtlEqualUnicodeString(&TypeInformation1->TypeName, &ThreadTypeString, FALSE)) {
		THREAD_BASIC_INFORMATION ThreadBasicInformation1;
		THREAD_BASIC_INFORMATION ThreadBasicInformation2;

		//
		// Both handles refer to a thread.
		//

		Status = NtQueryInformationThread(
			FirstObjectHandle,
			ThreadBasicInformation,
			&ThreadBasicInformation1,
			sizeof(ThreadBasicInformation1),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		Status = NtQueryInformationThread(
			SecondObjectHandle,
			ThreadBasicInformation,
			&ThreadBasicInformation2,
			sizeof(ThreadBasicInformation2),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		//
		// Check whether thread IDs are equal.
		//

		if (ThreadBasicInformation1.ClientId.UniqueThread == ThreadBasicInformation2.ClientId.UniqueThread) {
			return STATUS_SUCCESS;
		} else {
			return STATUS_NOT_SAME_OBJECT;
		}
	} else if (RtlEqualUnicodeString(&TypeInformation1->TypeName, &EventTypeString, FALSE)) {
		EVENT_BASIC_INFORMATION EventBasicInformation1;
		EVENT_BASIC_INFORMATION EventBasicInformation2;

		//
		// Both handles refer to an event.
		//

		Status = NtQueryEvent(
			FirstObjectHandle,
			EventBasicInformation,
			&EventBasicInformation1,
			sizeof(EventBasicInformation1),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		Status = NtQueryEvent(
			SecondObjectHandle,
			EventBasicInformation,
			&EventBasicInformation2,
			sizeof(EventBasicInformation2),
			NULL);

		if (Status == STATUS_ACCESS_DENIED) {
			goto SkipTypeSpecificDetection;
		} else if (!NT_SUCCESS(Status)) {
			ASSERT (NT_SUCCESS(Status));
			return Status;
		}

		if (EventBasicInformation1.EventType != EventBasicInformation2.EventType) {
			return STATUS_NOT_SAME_OBJECT;
		}
	}

SkipTypeSpecificDetection:

	//
	// Heuristic approach.
	//
	// 1. Fetch and compare name information for both objects. If names are
	//    different, then the objects are different. This is more reliable since
	//    object names don't change that often.
	//
	// 2. Fetch and compare basic information for both objects. In the
	//    OBJECT_BASIC_INFORMATION struct, there are a few things we can
	//    check to immediately determine that the two objects are different.
	//    However, this is less reliable, because the things we're checking can
	//    be influenced by the actions of other threads.
	//
	// 3. In all other cases, assume the objects are the same.
	//

	//
	// Compare names.
	//

	Status = NtQueryObject(
		FirstObjectHandle,
		ObjectNameInformation,
		NULL,
		0,
		&RequiredSize);

	if (Status != STATUS_INFO_LENGTH_MISMATCH) {
		ASSERT (Status == STATUS_INFO_LENGTH_MISMATCH);
		return Status;
	}

	NameInformation1 = (PUNICODE_STRING) StackAlloc(BYTE, RequiredSize);

	Status = NtQueryObject(
		FirstObjectHandle,
		ObjectNameInformation,
		NameInformation1,
		RequiredSize,
		NULL);

	if (!NT_SUCCESS(Status)) {
		ASSERT (NT_SUCCESS(Status));
		return Status;
	}

	NameInformation2 = (PUNICODE_STRING) StackAlloc(BYTE, RequiredSize);

	Status = NtQueryObject(
		SecondObjectHandle,
		ObjectNameInformation,
		NameInformation2,
		RequiredSize,
		NULL);

	if (!NT_SUCCESS(Status)) {
		ASSERT (NT_SUCCESS(Status));
		return Status;
	}

	ObjectNamesAreEqual = RtlEqualUnicodeString(
		NameInformation1,
		NameInformation2,
		FALSE);

	if (!ObjectNamesAreEqual) {
		return STATUS_NOT_SAME_OBJECT;
	}

	//
	// Get OBJECT_BASIC_INFORMATION structs.
	//

	Status = NtQueryObject(
		FirstObjectHandle,
		ObjectBasicInformation,
		&BasicInformation1,
		sizeof(BasicInformation1),
		NULL);

	if (!NT_SUCCESS(Status)) {
		ASSERT (NT_SUCCESS(Status));
		return Status;
	}

	Status = NtQueryObject(
		SecondObjectHandle,
		ObjectBasicInformation,
		&BasicInformation2,
		sizeof(BasicInformation2),
		NULL);

	if (!NT_SUCCESS(Status)) {
		ASSERT (NT_SUCCESS(Status));
		return Status;
	}

	//
	// If either of the two objects has OBJ_EXCLUSIVE set in the attributes, then we
	// know for certain that they are different objects, because only one handle with
	// OBJ_EXCLUSIVE can be open to an object at a time.
	//
	// I think OBJ_EXCLUSIVE is a rare attribute to find, though. This code probably
	// doesn't really do anything.
	//

	if (BasicInformation1.Attributes & OBJ_EXCLUSIVE) {
		return STATUS_NOT_SAME_OBJECT;
	}

	if (BasicInformation2.Attributes & OBJ_EXCLUSIVE) {
		return STATUS_NOT_SAME_OBJECT;
	}

	//
	// If HandleCount or PointerCount are different between the two objects, then we
	// can guess that they're different objects. Not 100% certain, due to race conditions.
	//

	if (BasicInformation1.HandleCount != BasicInformation2.HandleCount) {
		return STATUS_NOT_SAME_OBJECT;
	}

	if (BasicInformation1.PointerCount != BasicInformation2.PointerCount) {
		return STATUS_NOT_SAME_OBJECT;
	}

	//
	// Check if the objects have differently sized security descriptors. Since security
	// descriptors are associated with the object, rather than the handle, this can tell
	// us if the objects are different.
	//

	if (BasicInformation1.SecurityDescriptorSize != BasicInformation2.SecurityDescriptorSize) {
		return STATUS_NOT_SAME_OBJECT;
	}

	//
	// Otherwise, we'll just say that the objects are the same.
	//

	return STATUS_SUCCESS;
}