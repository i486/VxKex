///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlsext.c
//
// Abstract:
//
//     Handles TLS Extensions.
//     This code is separate from tlshello.c because in TLS 1.3 we need to be
//     able to handle extensions outside of the server hello (in the encrypted
//     extensions message).
//
// Author:
//
//     vxiiduu (12-Jun-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               12-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

// The format of a TLS Extension is defined:
// https://datatracker.ietf.org/doc/html/rfc5246.html#section-7.4.1.4
// https://datatracker.ietf.org/doc/html/rfc6066.html#section-3
// https://datatracker.ietf.org/doc/html/rfc7301.html#section-3.1
// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.2
SECURITY_STATUS TlspSerializeExtension(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCTLS_EXTENSION_SPEC		Spec)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	USHORT ExtensionDataCb;
	ULONG IoInformationExistingWrittenCb;
	ULONG Index;

	ExtensionDataCb = 0;
	IoInformationExistingWrittenCb = IoInformation->OutBufferWrittenCb;

	//
	// All extensions begin with:
	//   1. uint16be extension type (TLS_EXT_*)
	//   2. uint16be extension data length (0..65535)
	//
	// The extension data length does not include the length of those two
	// uint16 values.
	//

	SecStatus = IoWriteSwap16(IoInformation, Spec->ExtensionType);

	switch (Spec->ExtensionType) {
	case TLS_EXT_SERVER_NAME:
		ASSERT (Spec->ServerName.ServerNameCb >= 1);
		ASSERT (Spec->ServerName.ServerNameCb <= 65532);

		ExtensionDataCb = Spec->ServerName.ServerNameCb + 5;
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);

		SecStatus = IoWriteSwap16(IoInformation, Spec->ServerName.ServerNameCb + 3);
		SecStatus = IoWrite8(IoInformation, 0);
		SecStatus = IoWriteSwap16(IoInformation, Spec->ServerName.ServerNameCb);

		SecStatus = IoWrite(
			IoInformation,
			Spec->ServerName.ServerName,
			Spec->ServerName.ServerNameCb);

		break;
	case TLS_EXT_SUPPORTED_GROUPS:
		ASSERT (Spec->SupportedGroups.NumberOfSupportedGroups >= 1);
		ASSERT (Spec->SupportedGroups.NumberOfSupportedGroups <= 32766);

		ExtensionDataCb = Spec->SupportedGroups.NumberOfSupportedGroups * sizeof(USHORT) + 2;
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);

		SecStatus = IoWriteSwap16(
			IoInformation,
			Spec->SupportedGroups.NumberOfSupportedGroups * sizeof(USHORT));

		for (Index = 0; Index < Spec->SupportedGroups.NumberOfSupportedGroups; ++Index) {
			SecStatus = IoWriteSwap16(
				IoInformation,
				Spec->SupportedGroups.SupportedGroups[Index]);
		}

		break;
	case TLS_EXT_SIGNATURE_ALGORITHMS:
		ASSERT (Spec->SignatureAlgorithms.NumberOfSignatureSchemes >= 1);
		ASSERT (Spec->SignatureAlgorithms.NumberOfSignatureSchemes <= 32766);

		ExtensionDataCb = Spec->SignatureAlgorithms.NumberOfSignatureSchemes * sizeof(USHORT) + 2;
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);

		SecStatus = IoWriteSwap16(
			IoInformation,
			Spec->SignatureAlgorithms.NumberOfSignatureSchemes * sizeof(USHORT));

		for (Index = 0; Index < Spec->SignatureAlgorithms.NumberOfSignatureSchemes; ++Index) {
			SecStatus = IoWriteSwap16(
				IoInformation,
				Spec->SignatureAlgorithms.SignatureSchemes[Index]);
		}

		break;
	case TLS_EXT_ALPN:
		ASSERT (Spec->ApplicationProtocols.ProtocolListCb >= 2);
		ASSERT (Spec->ApplicationProtocols.ProtocolListCb <= 65533);

		ExtensionDataCb = Spec->ApplicationProtocols.ProtocolListCb + 2;
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);

		SecStatus = IoWriteSwap16(
			IoInformation,
			Spec->ApplicationProtocols.ProtocolListCb);

		SecStatus = IoWrite(
			IoInformation,
			Spec->ApplicationProtocols.ProtocolList,
			Spec->ApplicationProtocols.ProtocolListCb);

		break;
	case TLS_EXT_SUPPORTED_VERSIONS:
		ASSERT (Spec->SupportedVersions.NumberOfProtocolVersions >= 1);
		ASSERT (Spec->SupportedVersions.NumberOfProtocolVersions <= 127);

		ExtensionDataCb = (Spec->SupportedVersions.NumberOfProtocolVersions * sizeof(USHORT)) + 1;
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);

		SecStatus = IoWrite8(
			IoInformation,
			Spec->SupportedVersions.NumberOfProtocolVersions * sizeof(USHORT));

		for (Index = 0; Index < Spec->SupportedVersions.NumberOfProtocolVersions; ++Index) {
			SecStatus = IoWriteSwap16(
				IoInformation,
				Spec->SupportedVersions.ProtocolVersions[Index]);
		}

		break;
	case TLS_EXT_KEY_SHARE:
		ASSERT (Spec->KeyShare.NumberOfPublicKeys >= 1);

		ExtensionDataCb = 2;

		// Add up all the ecc key lengths together.
		for (Index = 0; Index < Spec->KeyShare.NumberOfPublicKeys; ++Index) {
			ULONG KeyBits;
			ULONG WrittenCb;
			ULONG NewExtensionDataCb;

			Status = BCryptGetProperty(
				Spec->KeyShare.PublicKeys[Index],
				BCRYPT_KEY_LENGTH,
				(PUCHAR) &KeyBits,
				sizeof(KeyBits),
				&WrittenCb,
				0);

			ASSERT (NT_SUCCESS(Status));
			ASSERT (WrittenCb == sizeof(KeyBits));

			if (!NT_SUCCESS(Status)) {
				return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
			}

			// EccKeyType (TLS_GROUP_*) + KeyCb + 0x04 + X + Y
			NewExtensionDataCb = ExtensionDataCb + 2 + 2 + 1 + (2 * BITS_TO_BYTES(KeyBits));
			ASSERT (NewExtensionDataCb < USHRT_MAX);
			ExtensionDataCb = (USHORT) NewExtensionDataCb;
		}

		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb);
		SecStatus = IoWriteSwap16(IoInformation, ExtensionDataCb - 2);

		if (FAILED(SecStatus)) {
			break;
		}

		for (Index = 0; Index < Spec->KeyShare.NumberOfPublicKeys; ++Index) {
			WCHAR AlgorithmName[64];
			ULONG AlgorithmNameCb;
			BYTE KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB) + (BITS_TO_BYTES(521) * 2)];
			ULONG KeyBlobCb;
			USHORT KeyBits;
			USHORT Group;

			Status = BCryptGetProperty(
				Spec->KeyShare.PublicKeys[Index],
				BCRYPT_ALGORITHM_NAME,
				(PUCHAR) AlgorithmName,
				sizeof(AlgorithmName),
				&AlgorithmNameCb,
				0);

			ASSERT (NT_SUCCESS(Status));
			ASSERT (AlgorithmNameCb > 0);

			if (!NT_SUCCESS(Status)) {
				return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
			}

			if (StringEqual(AlgorithmName, BCRYPT_ECDH_P256_ALGORITHM)) {
				Group = TLS_GROUP_SECP256R1;
				KeyBits = 256;
			} else if (StringEqual(AlgorithmName, BCRYPT_ECDH_P384_ALGORITHM)) {
				Group = TLS_GROUP_SECP384R1;
				KeyBits = 384;
			} else if (StringEqual(AlgorithmName, BCRYPT_ECDH_P521_ALGORITHM)) {
				Group = TLS_GROUP_SECP521R1;
				KeyBits = 521;
			} else {
				NOT_REACHED;
			}

			Status = BCryptExportKey(
				Spec->KeyShare.PublicKeys[Index],
				NULL,
				BCRYPT_ECCPUBLIC_BLOB,
				KeyBlob,
				sizeof(KeyBlob),
				&KeyBlobCb,
				0);

			ASSERT (NT_SUCCESS(Status));
			ASSERT (KeyBlobCb == sizeof(BCRYPT_ECCKEY_BLOB) + (2 * BITS_TO_BYTES(KeyBits)));

			if (!NT_SUCCESS(Status)) {
				return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
			}

			SecStatus = IoWriteSwap16(IoInformation, Group);
			SecStatus = IoWriteSwap16(IoInformation, 1 + (2 * BITS_TO_BYTES(KeyBits)));
			SecStatus = IoWrite8(IoInformation, 0x04);

			SecStatus = IoWrite(
				IoInformation,
				&KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB)],
				BITS_TO_BYTES(KeyBits));

			SecStatus = IoWrite(
				IoInformation,
				&KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB) + BITS_TO_BYTES(KeyBits)],
				BITS_TO_BYTES(KeyBits));

			if (FAILED(SecStatus)) {
				break;
			}
		}

		break;
	default:
		NOT_REACHED;
	}

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	// Make sure we did all length calculations properly.
	ASSERT ((IoInformation->OutBufferWrittenCb - IoInformationExistingWrittenCb) -	// data we actually wrote
			(2 * sizeof(USHORT)) ==													// minus type and length
			ExtensionDataCb);														// data we said we wrote

	return SEC_E_OK;
}

//
// All pointers within the populated Spec structure point to inside the
// IoInformation buffer, so make sure you copy them if you need them to
// be valid beyond the lifetime of the IoInformation buffer.
//
// Upon reading malformed data, this function will return an error code.
//
SECURITY_STATUS TlspDeserializeExtension(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	OUT		PTLS_EXTENSION_SPEC			Spec)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION ExtensionIoInformation;
	USHORT ExtensionDataCb;

	ASSERT (IoInformation != NULL);
	ASSERT (IoInformation->InBufferReadCb < IoInformation->InBufferCb);

	KexRtlZeroMemory(Spec, sizeof(*Spec));

	//
	// Read the extension header.
	//

	SecStatus = IoReadSwap16(IoInformation, &Spec->ExtensionType);
	SecStatus = IoReadSwap16(IoInformation, &ExtensionDataCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Set up an IO information for just the extension data.
	//

	KexRtlZeroMemory(&ExtensionIoInformation, sizeof(ExtensionIoInformation));

	SecStatus = IoRead(
		IoInformation,
		&ExtensionIoInformation.InBuffer,
		ExtensionDataCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	ExtensionIoInformation.InBufferCb = ExtensionDataCb;

	//
	// Parse each type of extension depending on what the type code is.
	//

	switch (Spec->ExtensionType) {
	case TLS_EXT_SERVER_NAME:
		//
		// An empty Server Name Indication extension is permitted in a server
		// hello. It is meaningless to us. But it is not permitted to have any
		// data in it.
		//

		if (ExtensionIoInformation.InBufferCb != ExtensionIoInformation.InBufferReadCb) {
			SecStatus = SEC_E_ILLEGAL_MESSAGE;
		}

		break;

	case TLS_EXT_ALPN:
		SecStatus = IoReadSwap16(
			&ExtensionIoInformation,
			&Spec->ApplicationProtocols.ProtocolListCb);

		if (FAILED(SecStatus)) {
			break;
		}

		// Check that the protocol list length is >=2.
		if (Spec->ApplicationProtocols.ProtocolListCb < 2) {
			Spec->ApplicationProtocols.ProtocolListCb = 0;
			SecStatus = SEC_E_ILLEGAL_MESSAGE;
			break;
		}

		SecStatus = IoRead(
			&ExtensionIoInformation,
			(PPCVOID) &Spec->ApplicationProtocols.ProtocolList,
			Spec->ApplicationProtocols.ProtocolListCb);

		if (FAILED(SecStatus)) {
			break;
		}

		break;

	case TLS_EXT_SUPPORTED_VERSIONS:
		// Supported versions in a server hello message is not a list/vector, but just
		// a single value containing the selected protocol version.
		SecStatus = IoReadSwap16(
			&ExtensionIoInformation,
			&Spec->SupportedVersions.SelectedProtocolVersion);

		break;

	case TLS_EXT_SUPPORTED_GROUPS: {
		USHORT SupportedGroupsCb;

		//
		// In TLS 1.3 the server is allowed to send a supported groups extension.
		//

		SecStatus = IoReadSwap16(
			&ExtensionIoInformation,
			&SupportedGroupsCb);

		if (FAILED(SecStatus)) {
			break;
		}

		if (SupportedGroupsCb & 1) {
			// must be a multiple of 2
			SecStatus = SEC_E_ILLEGAL_MESSAGE;
			break;
		}

		Spec->SupportedGroups.NumberOfSupportedGroups = SupportedGroupsCb / sizeof(USHORT);

		// Note that we don't reverse the endianness as we kind of should. This is to
		// avoid a heap allocation. Currently, KxSChanl does not make use of the server
		// supported groups extension.
		SecStatus = IoRead(
			&ExtensionIoInformation,
			(PPCVOID) &Spec->SupportedGroups.SupportedGroups,
			SupportedGroupsCb);

		break;
								   }
	case TLS_EXT_KEY_SHARE: {
		BCRYPT_KEY_HANDLE ServerPublicKey;
		BCRYPT_ALG_HANDLE AlgHandle;
		USHORT Group;
		ULONG KeyBits;
		ULONG ExpectedKeyExchangeCb;
		USHORT KeyExchangeCb;
		PCBYTE KeyExchange;
		PBYTE KeyBlob;
		ULONG KeyBlobCb;
		ULONG KeyBlobMagic;
		PBCRYPT_ECCKEY_BLOB KeyBlobStruct;

		SecStatus = IoReadSwap16(&ExtensionIoInformation, &Group);
		SecStatus = IoReadSwap16(&ExtensionIoInformation, &KeyExchangeCb);
		SecStatus = IoRead(&ExtensionIoInformation, (PPCVOID) &KeyExchange, KeyExchangeCb);

		if (FAILED(SecStatus)) {
			break;
		}

		switch (Group) {
		case TLS_GROUP_SECP256R1:
			KeyBits = 256;
			AlgHandle = BCRYPT_ECDH_P256_ALG_HANDLE;
			KeyBlobMagic = BCRYPT_ECDH_PUBLIC_P256_MAGIC;
			break;
		case TLS_GROUP_SECP384R1:
			KeyBits = 384;
			AlgHandle = BCRYPT_ECDH_P384_ALG_HANDLE;
			KeyBlobMagic = BCRYPT_ECDH_PUBLIC_P384_MAGIC;
			break;
		case TLS_GROUP_SECP521R1:
			KeyBits = 521;
			AlgHandle = BCRYPT_ECDH_P521_ALG_HANDLE;
			KeyBlobMagic = BCRYPT_ECDH_PUBLIC_P521_MAGIC;
			break;
		default:
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		ExpectedKeyExchangeCb = 1 + (2 * BITS_TO_BYTES(KeyBits));

		if (KeyExchangeCb != ExpectedKeyExchangeCb) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (KeyExchange[0] != 0x04) {
			ASSERT (FALSE);
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		KeyBlobCb = sizeof(BCRYPT_ECCKEY_BLOB) + (2 * BITS_TO_BYTES(KeyBits));
		KeyBlob = StackAlloc(BYTE, KeyBlobCb);

		KeyBlobStruct = (PBCRYPT_ECCKEY_BLOB) KeyBlob;
		KeyBlobStruct->dwMagic = KeyBlobMagic;
		KeyBlobStruct->cbKey = BITS_TO_BYTES(KeyBits);

		KexRtlCopyMemory(
			&KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB)],
			&KeyExchange[1],
			BITS_TO_BYTES(KeyBits));

		KexRtlCopyMemory(
			&KeyBlob[sizeof(BCRYPT_ECCKEY_BLOB) + BITS_TO_BYTES(KeyBits)],
			&KeyExchange[1 + BITS_TO_BYTES(KeyBits)],
			BITS_TO_BYTES(KeyBits));

		Status = BCryptImportKeyPair(
			AlgHandle,
			NULL,
			BCRYPT_ECCPUBLIC_BLOB,
			&ServerPublicKey,
			KeyBlob,
			KeyBlobCb,
			0);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		Spec->KeyShare.PublicKey = ServerPublicKey;
		break;
							}
	default:
		KexLogErrorEvent(
			L"Server sent unsupported extension %hu",
			Spec->ExtensionType);

		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Check that all data in this extension was consumed and we didn't have any
	// junk data left over at the end.
	//

	if (ExtensionIoInformation.InBufferReadCb != ExtensionIoInformation.InBufferCb) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	return SEC_E_OK;
}

//
// Handle extensions which were parsed by TlspDeserializeExtension.
// e.g. from Server Hello or (TLS 1.3) Encrypted Extensions.
//
SECURITY_STATUS TlspHandleExtensions(
	IN OUT	PKXSCHANL_CONTEXT		Context,
	IN		PCTLS_EXTENSION_SPEC	Extensions,
	IN		ULONG					NumberOfExtensions)
{
	SECURITY_STATUS SecStatus;
	ULONG Index;

	ASSERT (Context != NULL);
	ASSERT (Extensions != NULL);

	//
	// Go look through all the extensions and see whether everything is reasonable.
	// For instance, if there is an ALPN extension, then not only must the application
	// have specified ALPN protocols, but the returned protocol must be one of the
	// ones in the application-supplied list.
	//

	for (Index = 0; Index < NumberOfExtensions; ++Index) {
		PCTLS_EXTENSION_SPEC Extension;
		BOOLEAN IsExtensionOk;

		Extension = &Extensions[Index];
		IsExtensionOk = FALSE;

		switch (Extension->ExtensionType) {
		case TLS_EXT_SERVER_NAME:
			if (Context->TargetNameCch != 0) {
				IsExtensionOk = TRUE;
			}

			break;
		case TLS_EXT_SUPPORTED_VERSIONS:
			if (Context->ProtocolVersion != 0) {
				// Server sent multiple supported versions extensions
				break;
			}

			if (Extension->SupportedVersions.SelectedProtocolVersion == 0) {
				// a value of 0 is always invalid
				break;
			}

			if (Context->State > CONTEXTSTATE_EXPECTING_SERVER_HELLO) {
				// supported versions in TLS 1.3 encrypted extensions is not allowed
				break;
			}

			Context->ProtocolVersion = Extension->SupportedVersions.SelectedProtocolVersion;
			IsExtensionOk = TRUE;
			break;
		case TLS_EXT_ALPN:
			if (Context->SelectedApplicationProtocolCb > 0) {
				// server sent 2 ALPN extensions
				break;
			}

			SecStatus = TlsValidateApplicationProtocolList(
				Context,
				Extension->ApplicationProtocols.ProtocolList,
				Extension->ApplicationProtocols.ProtocolListCb,
				FALSE);

			if (FAILED(SecStatus)) {
				break;
			}

			//
			// Check if the server supplied protocol is in the protocol list.
			//

			IsExtensionOk = TlsIsProtocolInProtocolList(
				&Extension->ApplicationProtocols.ProtocolList[1],
				Extension->ApplicationProtocols.ProtocolList[0],
				Context->ApplicationProtocols,
				Context->ApplicationProtocolsCb);

			if (!IsExtensionOk) {
				// Not in the list
				break;
			}

			//
			// Record the selected protocol in the context.
			//

			Context->SelectedApplicationProtocolCb = Extension->ApplicationProtocols.ProtocolList[0];

			KexRtlCopyMemory(
				Context->SelectedApplicationProtocol,
				&Extension->ApplicationProtocols.ProtocolList[1],
				Context->SelectedApplicationProtocolCb);

			break;
		case TLS_EXT_KEY_SHARE:
			if (Context->ServerEphemeralKey13 != NULL) {
				// server sent multiple key share extensions
				BCryptDestroyKey(Extension->KeyShare.PublicKey);
				break;
			}

			if (Context->State > CONTEXTSTATE_EXPECTING_SERVER_HELLO) {
				// key share extension not permitted in encrypted extensions for
				// TLS 1.3
				BCryptDestroyKey(Extension->KeyShare.PublicKey);
				break;
			}

			Context->ServerEphemeralKey13 = Extension->KeyShare.PublicKey;
			IsExtensionOk = TRUE;
			break;

		case TLS_EXT_SUPPORTED_GROUPS:
			// Ignore it, not useful.
			IsExtensionOk = TRUE;
			break;

		default:
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		if (!IsExtensionOk) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	}

	return SEC_E_OK;
}