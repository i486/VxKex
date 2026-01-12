///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     SafeAlloc.h
//
// Abstract:
//
//     Safe memory allocation macros.
//
//     SafeAlloc and SafeFree are the safe memory management macros that should
//     be strongly preferred for code in VxKex. Advantages include:
//
//       1. No type-casting required in most cases.
//       2. Reduced chance of stupid typo bugs with sizeof().
//       3. Use-after-free will always result in a crash instead of UB.
//
// Author:
//
//     vxiiduu (26-Sep-2022)
//
// Environment:
//
//     Anywhere where you can call the NTDLL heap functions.
//
// Revision History:
//
//     vxiiduu               26-Sep-2022  Initial creation.
//     vxiiduu               30-Sep-2022  Removed *Early and *Nullable variants
//                                        because they are useless.
//     vxiiduu               01-Oct-2022  Add *Ex variants
//     vxiiduu               22-Oct-2022  Fix SafeReAlloc typos
//     vxiiduu               12-Nov-2022  Add *Seh variants
//     vxiiduu               20-Nov-2022  Add SafeClose
//     vxiiduu               19-Feb-2024  Add SafeRelease
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include <NtDll.h>
#include <KexTypes.h>

#define SafeAllocEx(Heap, Flags, Type, NumberOfElements) \
	((Type *) RtlAllocateHeap((Heap), (Flags), sizeof(Type) * (NumberOfElements)))

#define SafeReAllocEx(Heap, Flags, OriginalPointer, Type, NumberOfElements) \
	((Type *) RtlReAllocateHeap((Heap), (Flags), (OriginalPointer), sizeof(Type) * (NumberOfElements)))

#define SafeFreeEx(Heap, Flags, Pointer) \
	do { RtlFreeHeap(Heap, Flags, (Pointer)); (Pointer) = NULL; } while (0)

#define SafeAlloc(Type, NumberOfElements) \
	SafeAllocEx(RtlProcessHeap(), 0, Type, (NumberOfElements))

#define SafeReAlloc(OriginalPointer, Type, NumberOfElements) \
	SafeReAllocEx(RtlProcessHeap(), 0, (OriginalPointer), Type, (NumberOfElements))

#define SafeFree(Pointer) \
	SafeFreeEx(RtlProcessHeap(), 0, (Pointer))

#define SafeAllocSeh(Type, NumberOfElements) \
	SafeAllocEx(RtlProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, Type, (NumberOfElements))

#define SafeReAllocSeh(OriginalPointer, Type, NumberOfElements) \
	SafeReAllocEx(RtlProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (OriginalPointer), Type, (NumberOfElements))

#define SafeFreeSeh(Pointer) \
	SafeFreeEx(RtlProcessHeap(), HEAP_GENERATE_EXCEPTIONS, (Pointer))

//
// _alloca() is a compiler intrinsic.
//

#ifndef __cplusplus
PVOID CDECL _alloca(
	IN	SIZE_T NumberOfBytes);
#endif

#define StackAlloc(Type, NumberOfElements) ((Type *) _alloca(sizeof(Type) * (NumberOfElements)))

//
// SafeClose is for handles.
//

#define SafeClose(Handle) do { if (Handle) { NTSTATUS SafeCloseStatus = NtClose(Handle); ASSERT (NT_SUCCESS(SafeCloseStatus)); (Handle) = NULL; } } while(0)

//
// SafeRelease is for COM interfaces.
//

#define SafeRelease(Interface) do { if (Interface) { (Interface)->lpVtbl->Release(Interface); (Interface) = NULL; } } while (0)