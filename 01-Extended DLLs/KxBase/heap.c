#include "buildcfg.h"
#include "kxbasep.h"

KXBASEAPI HANDLE WINAPI KxBasepHeapCreateHook(
	IN	ULONG	Flags,
	IN	SIZE_T	InitialSize,
	IN	SIZE_T	MaximumSize)
{
	HANDLE HeapHandle;

	Flags &= HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE | HEAP_CREATE_ENABLE_EXECUTE;
	Flags |= HEAP_CLASS_1;

	if (MaximumSize < PAGE_SIZE) {
		if (MaximumSize == 0) {
			Flags |= HEAP_GROWABLE;
		} else {
			// Round up to the page size.
			MaximumSize = PAGE_SIZE;
		}
	}

	if (!(Flags & HEAP_GROWABLE) && InitialSize > MaximumSize) {
		MaximumSize = InitialSize;
	}

	//
	// The following line of code is the important part and is the reason why we have
	// rewritten the HeapCreate function.
	// Adding the following flag prevents RtlCreateHeap from trying to create a
	// "debug heap", which causes lots of problems with some applications (and it's
	// also slower, I'd presume).
	//

	Flags |= HEAP_SKIP_VALIDATION_CHECKS;

	HeapHandle = RtlCreateHeap(
		Flags,
		NULL,
		MaximumSize,
		InitialSize,
		NULL,
		NULL);

	if (HeapHandle == NULL) {
		RtlSetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
	}

	KexLogDebugEvent(
		L"HeapCreate called\r\n\r\n"
		L"Flags:       0x%08lx\r\n"
		L"InitialSize: %Iu\r\n"
		L"MaximumSize: %Iu\r\n"
		L"HeapHandle:  0x%p",
		Flags,
		InitialSize,
		MaximumSize,
		HeapHandle);

	return HeapHandle;
}