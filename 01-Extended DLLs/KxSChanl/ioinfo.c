///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ioinfo.c
//
// Abstract:
//
//     Functions to read/write to/from buffers described by a
//     KXSCHANL_IO_INFORMATION structure.
//
//     These functions always succeed or fail, never write incomplete data
//     to the buffer, and never do a partial read.
//
// Author:
//
//     vxiiduu (16-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               16-May-2026  Initial creation.
//     vxiiduu               02-Jun-2026  Move non-TLS related functions from
//                                        tlsrdwr.c to this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Peeks at a certain number of bytes from the input buffer but does not consume
// any data. *DataPointerOut will point to the start of the requested data.
//
SECURITY_STATUS IoPeek(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PPCVOID						DataPointerOut,
	IN		ULONG						RequiredCb)
{
	PCVOID InBufferUnreadData;
	ULONG InBufferUnreadCb;

	ASSERT (IoInformation != NULL);
	ASSERT (DataPointerOut != NULL);

	*DataPointerOut = NULL;

	if (RequiredCb == 0) {
		// Requesting zero bytes is valid (e.g. ServerHelloDone
		// has a 0-byte message body, or an empty SNI extension).
		// Return NULL. Dereferencing it is a caller bug.
		return SEC_E_OK;
	}

	if (IoInformation->InBuffer == NULL) {
		// No buffer
		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	ASSERT (IoInformation->InBufferReadCb <= IoInformation->InBufferCb);

	if (IoInformation->InBufferReadCb >= IoInformation->InBufferCb) {
		// No more unread data in buffer
		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	InBufferUnreadCb = IoInformation->InBufferCb - IoInformation->InBufferReadCb;

	if (RequiredCb > InBufferUnreadCb) {
		// Not enough unread data in buffer
		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	InBufferUnreadData = RVA_TO_VA(
		IoInformation->InBuffer,
		IoInformation->InBufferReadCb);

	*DataPointerOut = InBufferUnreadData;
	return SEC_E_OK;
}

//
// Consumes a specified amount of data from the buffer and returns
// a pointer to that data.
//
// If less than BufferCb bytes of data are in the buffer, returns
// SEC_E_INCOMPLETE_MESSAGE.
//
// Returns SEC_E_OK on success.
//
SECURITY_STATUS IoRead(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PPCVOID						DataPointerOut,
	IN		ULONG						RequiredCb)
{
	SECURITY_STATUS SecStatus;

	SecStatus = IoPeek(IoInformation, DataPointerOut, RequiredCb);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	IoInformation->InBufferReadCb += RequiredCb;
	return SEC_E_OK;
}

//
// Similar to IoRead but copies the data into the supplied buffer rather than
// just returning a pointer.
//
SECURITY_STATUS IoReadCopy(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PVOID						Buffer,
	IN		ULONG						BufferCb)
{
	SECURITY_STATUS SecStatus;
	PCVOID DataPointer;

	SecStatus = IoRead(IoInformation, &DataPointer, BufferCb);
	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	KexRtlCopyMemory(Buffer, DataPointer, BufferCb);
	return SEC_E_OK;
}

//
// Writes a specified amount of data to the buffer.
//
// If the buffer is too small and IoInformation->OutBufferMustAllocate is TRUE,
// then the buffer will be resized to accomodate the new data. If the buffer
// cannot be resized, returns SEC_E_INSUFFICIENT_MEMORY.
//
// If the buffer is too small and IoInformation->OutBufferMustAllocate is FALSE,
// returns SEC_E_BUFFER_TOO_SMALL.
//
// Returns SEC_E_OK on success.
//
SECURITY_STATUS IoWrite(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCVOID						Buffer,
	IN		ULONG						BufferCb)
{
	PVOID OutBufferEndOfData;

	ASSERT (IoInformation != NULL);

	if (BufferCb == 0) {
		return SEC_E_OK;
	}

	ASSERT (Buffer != NULL);
	ASSERT (BufferCb != 0);

	//
	// If the IoInformation->OutputBuffer is NULL it means that either we have
	// to allocate the buffer (if the caller has specified OutBufferMustAllocate,
	// which happens when InitializeSecurityContext gets ISC_REQ_ALLOCATE_MEMORY),
	// or we don't have an output buffer.
	//

	if (IoInformation->OutBuffer == NULL) {
		if (!IoInformation->OutBufferMustAllocate) {
			// No buffer and we're forbidden from allocating one.
			return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
		}

		IoInformation->OutBuffer = SafeAlloc(BYTE, BufferCb);
		ASSERT (IoInformation->OutBuffer != NULL);

		if (IoInformation->OutBuffer == NULL) {
			return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
		}

		IoInformation->OutBufferCb = BufferCb;
	}

	ASSERT (IoInformation->OutBufferWrittenCb <= IoInformation->OutBufferCb);

	//
	// If there's no space in the buffer, we can either reallocate the buffer (if
	// the caller has specified OutBufferMustAllocate), or we have no more space
	// to write and therefore must stop here.
	//

	if (IoInformation->OutBufferWrittenCb + BufferCb > IoInformation->OutBufferCb) {
		PVOID NewBuffer;
		ULONG NewBufferCb;

		if (!IoInformation->OutBufferMustAllocate) {
			// Not enough space and we can't reallocate.
			return SP_LOG_RESULT(SEC_E_BUFFER_TOO_SMALL);
		}

		NewBufferCb = IoInformation->OutBufferWrittenCb + BufferCb;

		if ((NewBufferCb - IoInformation->OutBufferCb) < 32) {
			// If we're enlarging the buffer by a small amount, increase the amount
			// of allocation in order to avoid large numbers of unnecessary heap
			// allocations.
			NewBufferCb = IoInformation->OutBufferCb + 32;
		}

		// Round up to multiples of 16 bytes (which is the heap granularity on x64)
		NewBufferCb = (NewBufferCb + 15) & ~15;
		NewBuffer = SafeReAlloc(IoInformation->OutBuffer, BYTE, NewBufferCb);
		ASSERT (NewBuffer != NULL);

		if (NewBuffer == NULL) {
			return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
		}

		IoInformation->OutBuffer = NewBuffer;
		IoInformation->OutBufferCb = NewBufferCb;
	}

	//
	// At this point we have a valid buffer and there is enough space.
	//

	OutBufferEndOfData = RVA_TO_VA(
		IoInformation->OutBuffer,
		IoInformation->OutBufferWrittenCb);

	KexRtlCopyMemory(OutBufferEndOfData, Buffer, BufferCb);
	IoInformation->OutBufferWrittenCb += BufferCb;

	return SEC_E_OK;
}

//
// Helper functions for reading/writing big endian integers.
//

INLINE SECURITY_STATUS IoRead8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						Value)
{
	return IoReadCopy(IoInformation, Value, sizeof(*Value));
}

INLINE SECURITY_STATUS IoReadSwap16(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUSHORT						Value)
{
	SECURITY_STATUS SecStatus;

	SecStatus = IoReadCopy(IoInformation, Value, sizeof(*Value));
	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	*Value = ByteSwap16(*Value);
	return SEC_E_OK;
}

INLINE SECURITY_STATUS IoReadSwap24(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PULONG						Value)
{
	SECURITY_STATUS SecStatus;

	SecStatus = IoReadCopy(IoInformation, Value, 3);
	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	*Value = ByteSwap24(*Value);
	return SEC_E_OK;
}

INLINE SECURITY_STATUS IoReadSwap32(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PULONG						Value)
{
	SECURITY_STATUS SecStatus;

	SecStatus = IoReadCopy(IoInformation, Value, sizeof(*Value));
	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	*Value = ByteSwap32(*Value);
	return SEC_E_OK;
}

//
// Peeks at the next byte in the input buffer without consuming it.
// Returns SEC_E_INCOMPLETE_MESSAGE if no data is available.
//
SECURITY_STATUS IoPeek8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PUCHAR						Value)
{
	PCBYTE Data;

	ASSERT (IoInformation != NULL);
	ASSERT (Value != NULL);

	if (IoInformation->InBuffer == NULL) {
		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	if (IoInformation->InBufferReadCb >= IoInformation->InBufferCb) {
		return SP_LOG_RESULT(SEC_E_INCOMPLETE_MESSAGE);
	}

	Data = (PCBYTE) RVA_TO_VA(
		IoInformation->InBuffer,
		IoInformation->InBufferReadCb);

	*Value = *Data;
	return SEC_E_OK;
}

INLINE SECURITY_STATUS IoWrite8(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		UCHAR						Value)
{
	return IoWrite(IoInformation, &Value, sizeof(Value));
}

INLINE SECURITY_STATUS IoWriteSwap16(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		USHORT						Value)
{
	Value = ByteSwap16(Value);
	return IoWrite(IoInformation, &Value, sizeof(Value));
}

INLINE SECURITY_STATUS IoWriteSwap24(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		ULONG						Value)
{
	ASSERT ((Value & 0xFF000000) == 0);

	if (Value & 0xFF000000) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	Value = ByteSwap24(Value);
	return IoWrite(IoInformation, &Value, 3);
}

INLINE SECURITY_STATUS IoWriteSwap32(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		ULONG						Value)
{
	Value = ByteSwap32(Value);
	return IoWrite(IoInformation, &Value, sizeof(Value));
}