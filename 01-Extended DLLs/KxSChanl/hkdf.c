///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     hkdf.c
//
// Abstract:
//
//     Implementations of the HKDF-Extract and HKDF-Expand algorithms, which
//     are used for key derivation in TLS 1.3 only.
//     https://datatracker.ietf.org/doc/html/rfc5869#section2
//
//     Implementation of the HKDF-Expand-Label and Derive-Secret functions,
//     which are defined in the TLS 1.3 specification.
//     https://datatracker.ietf.org/doc/html/rfc8446.html#section-7.1
//
//     The HmacAlgHandle parameter to these functions must be a handle to a
//     BCrypt hash algorithm opened with BCRYPT_ALG_HANDLE_HMAC_FLAG, or a
//     BCRYPT_HMAC_*_ALG_HANDLE pre-defined handle.
//
// Author:
//
//     vxiiduu (09-Jun-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               09-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

NTSTATUS HkdfExtract(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				InputKeyingMaterial,
	IN	ULONG				InputKeyingMaterialCb,
	IN	PCUCHAR				Salt OPTIONAL,
	IN	ULONG				SaltCb,
	OUT	PUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb)
{
	NTSTATUS Status;

	ASSERT ((Salt == NULL && SaltCb == 0) ||
			(Salt != NULL && SaltCb > 0));

	//
	// If no salt is provided, a salt of all zeroes (which is the length of the
	// hash output) is used.
	//

	if (SaltCb == 0) {
		ULONG HmacOutputCb;
		ULONG Dummy;

		Status = BCryptGetProperty(
			HmacAlgHandle,
			BCRYPT_HASH_LENGTH,
			(PUCHAR) &HmacOutputCb,
			sizeof(HmacOutputCb),
			&Dummy,
			0);

		ASSERT (NT_SUCCESS(Status));
		ASSERT (Dummy == sizeof(HmacOutputCb));
		ASSERT (HmacOutputCb > 0);
		
		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		Salt = StackAlloc(BYTE, HmacOutputCb);
		SaltCb = HmacOutputCb;
		KexRtlZeroMemory(Salt, SaltCb);
	}

	Status = BCryptHash(
		HmacAlgHandle,
		Salt,
		SaltCb,
		InputKeyingMaterial,
		InputKeyingMaterialCb,
		PseudoRandomKey,
		PseudoRandomKeyCb);

	ASSERT (NT_SUCCESS(Status));
	return Status;
}

NTSTATUS HkdfExpand(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				ApplicationData OPTIONAL,
	IN	ULONG				ApplicationDataCb,
	OUT	PUCHAR				OutputKeyingMaterial,
	IN	ULONG				OutputKeyingMaterialCb)
{
	NTSTATUS Status;
	ULONG HmacOutputCb;
	ULONG Dummy;
	ULONG HmacBufferCb;
	PBYTE HmacBuffer;
	PBYTE Counter;
	PBYTE OkmPointer;
	ULONG OkmRemainingCb;

	ASSERT (PseudoRandomKey != NULL);
	ASSERT (PseudoRandomKeyCb > 0);

	ASSERT ((ApplicationData == NULL && ApplicationDataCb == 0) ||
			(ApplicationData != NULL && ApplicationDataCb > 0));

	ASSERT (ApplicationDataCb < 4096);

	ASSERT (OutputKeyingMaterial != NULL);
	ASSERT (OutputKeyingMaterialCb > 0);

	//
	// Figure out the number of bytes the HMAC function produces.
	//

	Status = BCryptGetProperty(
		HmacAlgHandle,
		BCRYPT_HASH_LENGTH,
		(PUCHAR) &HmacOutputCb,
		sizeof(HmacOutputCb),
		&Dummy,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (Dummy == sizeof(HmacOutputCb));
	ASSERT (HmacOutputCb > 0);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	ASSERT (PseudoRandomKeyCb >= HmacOutputCb);
	ASSERT (OutputKeyingMaterialCb <= HmacOutputCb * UCHAR_MAX);

	if (OutputKeyingMaterialCb > HmacOutputCb * UCHAR_MAX) {
		return STATUS_INVALID_BUFFER_SIZE;
	}

	//
	// Allocate the HMAC buffer. This stores both the HMAC input, output, the
	// application-specific data (if any), and the counter.
	//
	// The first HmacOutputCb bytes are used to store the HMAC output, which changes
	// on each iteration.
	// The next ApplicationDataCb bytes are used to store the application data.
	// The next 1 byte is used to store the 8-bit counter.
	//

	HmacBufferCb = HmacOutputCb + ApplicationDataCb + 1;
	HmacBuffer = StackAlloc(BYTE, HmacBufferCb);

	// Safe to call this even when ApplicationData is NULL (as long as Cb == 0)
	KexRtlCopyMemory(&HmacBuffer[HmacOutputCb], ApplicationData, ApplicationDataCb);
	
	Counter = &HmacBuffer[HmacBufferCb - 1];
	*Counter = 1;

	//
	// Perform the first HMAC iteration, which is special: we hash only the
	// app-specific data + counter, since there is no previous HMAC output.
	//

	Status = BCryptHash(
		HmacAlgHandle,
		PseudoRandomKey,
		PseudoRandomKeyCb,
		&HmacBuffer[HmacOutputCb],
		HmacBufferCb - HmacOutputCb,
		HmacBuffer,
		HmacOutputCb);

	ASSERT (NT_SUCCESS(Status));

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	OkmPointer = OutputKeyingMaterial;
	OkmRemainingCb = OutputKeyingMaterialCb;

	KexRtlCopyMemory(OkmPointer, HmacBuffer, min(HmacOutputCb, OkmRemainingCb));
	OkmPointer += min(HmacOutputCb, OkmRemainingCb);
	OkmRemainingCb -= min(HmacOutputCb, OkmRemainingCb);

	//
	// Perform the remaining HMAC iterations.
	// For each of these iterations the previous HMAC output is also included in the
	// hash.
	//

	while (OkmRemainingCb > 0) {
		++*Counter;

		Status = BCryptHash(
			HmacAlgHandle,
			PseudoRandomKey,
			PseudoRandomKeyCb,
			HmacBuffer,
			HmacBufferCb,
			HmacBuffer,
			HmacOutputCb);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return Status;
		}

		KexRtlCopyMemory(OkmPointer, HmacBuffer, min(HmacOutputCb, OkmRemainingCb));
		OkmPointer += min(HmacOutputCb, OkmRemainingCb);
		OkmRemainingCb -= min(HmacOutputCb, OkmRemainingCb);
	};

	return STATUS_SUCCESS;
}

NTSTATUS HkdfExpandLabel(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				Label,
	IN	ULONG				LabelCb,
	IN	PCUCHAR				Context OPTIONAL,
	IN	ULONG				ContextCb,
	OUT	PUCHAR				OutputKeyingMaterial,
	IN	ULONG				OutputKeyingMaterialCb)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION HkdfLabelIoInformation;

	ASSERT ((Context != NULL && ContextCb > 0) ||
			(Context == NULL && ContextCb == 0));

	//
	// Create an IO information structure to hold the temporary "HkdfLabel"
	// structure specified in RFC8446 ss. 7.1
	//

	KexRtlZeroMemory(&HkdfLabelIoInformation, sizeof(HkdfLabelIoInformation));
	HkdfLabelIoInformation.OutBufferCb =
		sizeof(USHORT) +
		sizeof(UCHAR) +
		StringLiteralLength("tls13 ") +
		LabelCb +
		sizeof(UCHAR) +
		ContextCb;

	HkdfLabelIoInformation.OutBuffer = StackAlloc(BYTE, HkdfLabelIoInformation.OutBufferCb);

	ASSERT (OutputKeyingMaterialCb <= USHRT_MAX);
	ASSERT (StringLiteralLength("tls13 ") + LabelCb <= UCHAR_MAX);
	ASSERT (ContextCb <= UCHAR_MAX);

	SecStatus = IoWriteSwap16(&HkdfLabelIoInformation, (USHORT) OutputKeyingMaterialCb);
	SecStatus = IoWrite8(&HkdfLabelIoInformation, (UCHAR) (StringLiteralLength("tls13 ") + LabelCb));
	SecStatus = IoWrite(&HkdfLabelIoInformation, "tls13 ", StringLiteralLength("tls13 "));
	SecStatus = IoWrite(&HkdfLabelIoInformation, Label, LabelCb);
	SecStatus = IoWrite8(&HkdfLabelIoInformation, (UCHAR) ContextCb);
	SecStatus = IoWrite(&HkdfLabelIoInformation, Context, ContextCb);
	
	ASSERT (SUCCEEDED(SecStatus));
	ASSERT (HkdfLabelIoInformation.OutBufferWrittenCb == HkdfLabelIoInformation.OutBufferCb);

	return HkdfExpand(
		HmacAlgHandle,
		PseudoRandomKey,
		PseudoRandomKeyCb,
		HkdfLabelIoInformation.OutBuffer,
		HkdfLabelIoInformation.OutBufferWrittenCb,
		OutputKeyingMaterial,
		OutputKeyingMaterialCb);
}

NTSTATUS HkdfDeriveSecret(
	IN	BCRYPT_ALG_HANDLE	HmacAlgHandle,
	IN	PCUCHAR				PseudoRandomKey,
	IN	ULONG				PseudoRandomKeyCb,
	IN	PCUCHAR				Label,
	IN	ULONG				LabelCb,
	IN	BCRYPT_HASH_HANDLE	TranscriptHash,
	OUT	PUCHAR				DerivedSecret,
	IN	ULONG				DerivedSecretCb)
{
	NTSTATUS Status;
	BCRYPT_HASH_HANDLE DuplicatedHash;
	PBYTE TranscriptHashOutput;
	ULONG TranscriptHashOutputCb;
	ULONG Dummy;

	//
	// Get the output length of the transcript hash (e.g. 32 for SHA256)
	// and the object length for the upcoming hash duplication.
	//

	Status = BCryptGetProperty(
		TranscriptHash,
		BCRYPT_HASH_LENGTH,
		(PUCHAR) &TranscriptHashOutputCb,
		sizeof(TranscriptHashOutputCb),
		&Dummy,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (Dummy == sizeof(TranscriptHashOutputCb));
	ASSERT (TranscriptHashOutputCb > 0);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	ASSERT (TranscriptHashOutputCb == DerivedSecretCb);

	//
	// Duplicate the transcript hash and get the hash result up to this point
	// into the output buffer.
	//

	Status = BCryptDuplicateHash(
		TranscriptHash,
		&DuplicatedHash,
		NULL,
		0,
		0);

	ASSERT (NT_SUCCESS(Status));
	ASSERT (DuplicatedHash != NULL);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	TranscriptHashOutput = StackAlloc(BYTE, TranscriptHashOutputCb);

	Status = BCryptFinishHash(
		DuplicatedHash,
		TranscriptHashOutput,
		TranscriptHashOutputCb,
		0);

	ASSERT (NT_SUCCESS(Status));
	SafeBCryptDestroyHash(DuplicatedHash);

	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	//
	// Use HkdfExpandLabel to perform the operation described in RFC8446 ss. 7.1.
	//

	return HkdfExpandLabel(
		HmacAlgHandle,
		PseudoRandomKey,
		PseudoRandomKeyCb,
		Label,
		LabelCb,
		TranscriptHashOutput,
		TranscriptHashOutputCb,
		DerivedSecret,
		DerivedSecretCb);
}