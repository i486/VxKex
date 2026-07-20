///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     tlshello.c
//
// Abstract:
//
//     Builds the TLS ClientHello message.
//     Parses the ServerHello message.
//     Applies to both TLS 1.2 and TLS 1.3.
//
// Author:
//
//     vxiiduu (26-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               26-May-2026  Initial creation.
//     vxiiduu               28-May-2026  Add ServerHello parser
//     vxiiduu               12-Jun-2026  Move extension code to tlsext.c
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

//
// Serialize a client hello MESSAGE (not record) from the specification
// structure.
//
STATIC INLINE SECURITY_STATUS TlspSerializeClientHello(
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN		PCTLS_CLIENT_HELLO_SPEC		Spec)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION ExtensionsIoInformation;
	BYTE ExtensionsBuffer[1024];
	ULONG IoInformationExistingWrittenCb;
	ULONG MessageDataCb;
	USHORT ExtensionsCb;
	ULONG Index;

	IoInformationExistingWrittenCb = IoInformation->OutBufferWrittenCb;

	//
	// Debug: Validate the spec structure.
	//

	ASSERT (Spec->ClientVersion == TLS1_2_PROTOCOL_VERSION);
	ASSERT (!RtlIsZeroMemory(Spec->ClientRandom, sizeof(Spec->ClientRandom)));
	ASSERT (Spec->SessionIdCb >= 0 && Spec->SessionIdCb <= 32);
	ASSERT (Spec->NumberOfCipherSuites >= 1 && Spec->NumberOfCipherSuites <= 32767);
	ASSERT (Spec->NumberOfCompressionMethods >= 1 && Spec->NumberOfCompressionMethods <= 127);

	//
	// Serialize all the extensions first so we know the length of all extensions.
	//

	KexRtlZeroMemory(&ExtensionsIoInformation, sizeof(ExtensionsIoInformation));
	ExtensionsIoInformation.OutBuffer	= ExtensionsBuffer;
	ExtensionsIoInformation.OutBufferCb	= sizeof(ExtensionsBuffer);

	for (Index = 0; Index < Spec->NumberOfExtensions; ++Index) {
		SecStatus = TlspSerializeExtension(
			&ExtensionsIoInformation,
			&Spec->Extensions[Index]);

		ASSERT (SUCCEEDED(SecStatus));

		if (FAILED(SecStatus)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}
	}

	// Total length of all extensions cannot exceed 65535 bytes
	ASSERT (ExtensionsIoInformation.OutBufferWrittenCb <= 65535);

	ExtensionsCb = (USHORT) ExtensionsIoInformation.OutBufferWrittenCb;

	//
	// Figure out the length of the whole client hello message.
	//

	MessageDataCb = 0;

	MessageDataCb += sizeof(Spec->ClientVersion);
	MessageDataCb += sizeof(Spec->ClientRandom);
	MessageDataCb += sizeof(Spec->SessionIdCb);
	MessageDataCb += Spec->SessionIdCb;
	MessageDataCb += sizeof(Spec->NumberOfCipherSuites);
	MessageDataCb += Spec->NumberOfCipherSuites * sizeof(Spec->CipherSuites[0]);
	MessageDataCb += sizeof(Spec->NumberOfCompressionMethods);
	MessageDataCb += Spec->NumberOfCompressionMethods * sizeof(Spec->CompressionMethods[0]);
	MessageDataCb += sizeof(ExtensionsCb);
	MessageDataCb += ExtensionsCb;

	//
	// Write the client hello message to the IoInformation buffer
	//

	SecStatus = IoWrite8(IoInformation, TLS_HSMSG_CLIENT_HELLO);
	SecStatus = IoWriteSwap24(IoInformation, MessageDataCb);
	SecStatus = IoWriteSwap16(IoInformation, Spec->ClientVersion);
	SecStatus = IoWrite(IoInformation, Spec->ClientRandom, sizeof(Spec->ClientRandom));

	// Session ID
	SecStatus = IoWrite8(IoInformation, Spec->SessionIdCb);
	SecStatus = IoWrite(IoInformation, Spec->SessionId, Spec->SessionIdCb);

	// Cipher Suites
	SecStatus = IoWriteSwap16(
		IoInformation,
		Spec->NumberOfCipherSuites * sizeof(Spec->CipherSuites[0]));
	
	for (Index = 0; Index < Spec->NumberOfCipherSuites; ++Index) {
		SecStatus = IoWriteSwap16(IoInformation, Spec->CipherSuites[Index]);
	}

	// Compression Methods
	SecStatus = IoWrite8(
		IoInformation,
		Spec->NumberOfCompressionMethods * sizeof(Spec->CompressionMethods[0]));

	for (Index = 0; Index < Spec->NumberOfCompressionMethods; ++Index) {
		SecStatus = IoWrite8(IoInformation, Spec->CompressionMethods[Index]);
	}

	// Extensions
	SecStatus = IoWriteSwap16(IoInformation, ExtensionsCb);
	SecStatus = IoWrite(IoInformation, ExtensionsBuffer, ExtensionsCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	// Make sure we did all length calculations properly.
	ASSERT ((IoInformation->OutBufferWrittenCb - IoInformationExistingWrittenCb) -
			(1 + 3) ==		// minus message type and message length
			MessageDataCb);	// data we said we wrote

	return SEC_E_OK;
}

//
// Deserialize a ServerHello MESSAGE (not record) into the given spec structure.
// Upon reading malformed data, this function will return an error code.
//
// The input data to this function should not include the 4-byte message header.
//
// The Spec structure should have NumberOfExtensions and Extensions pre-filled
// to an appropriate pointer and count.
// The other members of the structure are not read.
//
STATIC INLINE SECURITY_STATUS TlspDeserializeServerHello(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation,
	IN OUT	PTLS_SERVER_HELLO_SPEC		Spec)
{
	SECURITY_STATUS SecStatus;
	KXSCHANL_IO_INFORMATION ExtensionsIoInformation;
	USHORT ExtensionsCb;
	USHORT MaximumNumberOfExtensions;
	ULONG OriginalInBufferReadCb;

	//
	// Read what we want out of the spec structure and zero the rest.
	//

	MaximumNumberOfExtensions = Spec->NumberOfExtensions;

	{
		PTLS_EXTENSION_SPEC ExtensionSpec;

		// Zero everything except for Spec->Extensions.
		ExtensionSpec = Spec->Extensions;
		KexRtlZeroMemory(Spec, sizeof(*Spec));
		Spec->Extensions = ExtensionSpec;
	}

	// Save this so we can later check that we've read all the data.
	OriginalInBufferReadCb = IoInformation->InBufferReadCb;

	//
	// Parse all the fields of the server hello.
	//

	SecStatus = IoReadSwap16(IoInformation, &Spec->ServerVersion);
	SecStatus = IoReadCopy(IoInformation, &Spec->ServerRandom, sizeof(Spec->ServerRandom));
	SecStatus = IoRead8(IoInformation, &Spec->SessionIdCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	SecStatus = IoRead(IoInformation, &Spec->SessionId, Spec->SessionIdCb);
	SecStatus = IoReadSwap16(IoInformation, &Spec->CipherSuite);
	SecStatus = IoRead8(IoInformation, &Spec->CompressionMethod);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Extensions. It is legal for there to be no extensions and no extension
	// length at all.
	//

	SecStatus = IoReadSwap16(IoInformation, &ExtensionsCb);

	if (FAILED(SecStatus)) {
		// No problem, it just means there are no extensions.
		return SEC_E_OK;
	}

	// Set up IoInformation for just the extensions
	KexRtlZeroMemory(&ExtensionsIoInformation, sizeof(ExtensionsIoInformation));
	
	SecStatus = IoRead(
		IoInformation,
		&ExtensionsIoInformation.InBuffer,
		ExtensionsCb);

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	ExtensionsIoInformation.InBufferCb = ExtensionsCb;

	//
	// Deserialize all extensions.
	//

	do {
		if (Spec->NumberOfExtensions >= MaximumNumberOfExtensions) {
			// Too many extensions.
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		SecStatus = TlspDeserializeExtension(
			Context,
			&ExtensionsIoInformation,
			&Spec->Extensions[Spec->NumberOfExtensions]);

		if (FAILED(SecStatus)) {
			return SecStatus;
		}

		++Spec->NumberOfExtensions;
	} until (ExtensionsIoInformation.InBufferReadCb == ExtensionsIoInformation.InBufferCb);

	//
	// Make sure there is no junk data left over.
	//

	if (IoInformation->InBufferReadCb != IoInformation->InBufferCb) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	return SEC_E_OK;
}

//
// Writes a ClientHello handshake record into the given IoInformation output
// buffer, using the parameters in the given Context.
// Does not make any use of the input buffer.
// The client hello message (not including record header) is also saved into a
// buffer in the context so that it can be hashed later.
//
SECURITY_STATUS TlspWriteClientHello(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	SECURITY_STATUS SecStatus;
	USHORT SupportedGroups[3];
	UCHAR NumberOfSupportedGroups;
	USHORT SignatureSchemes[9];
	UCHAR NumberOfSignatureSchemes;
	USHORT ProtocolVersions[2];
	UCHAR NumberOfProtocolVersions;
	USHORT CipherSuites[ARRAYSIZE(Context->EnabledCipherSuites)];
	UCHAR NumberOfCipherSuites;
	UCHAR CompressionMethods[1];
	UCHAR NumberOfCompressionMethods;
	UCHAR NumberOfExtensions;
	TLS_EXTENSION_SPEC ExtensionSpecs[6];
	TLS_CLIENT_HELLO_SPEC ClientHelloSpec;
	TLS_RECORD_HEADER RecordHeader;
	BYTE ClientHelloBuffer[1024];
	KXSCHANL_IO_INFORMATION ClientHelloIoInformation;

	//
	// Add compression method(s)
	//

	NumberOfCompressionMethods = 0;
	CompressionMethods[NumberOfCompressionMethods++] = 0;

	ASSERT (NumberOfCompressionMethods <= ARRAYSIZE(CompressionMethods));

	//
	// Add supported key-exchange groups
	//

	NumberOfSupportedGroups = 0;
	SupportedGroups[NumberOfSupportedGroups++] = TLS_GROUP_SECP256R1;
	SupportedGroups[NumberOfSupportedGroups++] = TLS_GROUP_SECP384R1;
	SupportedGroups[NumberOfSupportedGroups++] = TLS_GROUP_SECP521R1;

	ASSERT (NumberOfSupportedGroups <= ARRAYSIZE(SupportedGroups));

	//
	// Add signature schemes
	//

	NumberOfSignatureSchemes = 0;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_ECDSA_SECP256R1_SHA256;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_ECDSA_SECP384R1_SHA384;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_ECDSA_SECP521R1_SHA512;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PSS_RSAE_SHA256;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PSS_RSAE_SHA384;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PSS_RSAE_SHA512;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PKCS1_SHA256;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PKCS1_SHA384;
	SignatureSchemes[NumberOfSignatureSchemes++] = TLS_SIGSCHEME_RSA_PKCS1_SHA512;

	ASSERT (NumberOfSignatureSchemes <= ARRAYSIZE(SignatureSchemes));

	//
	// Add protocol versions and cipher suites depending on what's enabled
	// in the credential. Most preferred first.
	//

	NumberOfProtocolVersions = 0;
	NumberOfCipherSuites = 0;

	if (Context->Credential->EnabledProtocols & SP_PROT_TLS1_3) {
		ProtocolVersions[NumberOfProtocolVersions++] = TLS1_3_PROTOCOL_VERSION;

		CipherSuites[NumberOfCipherSuites++] = TLS_AES_128_GCM_SHA256;
		CipherSuites[NumberOfCipherSuites++] = TLS_AES_256_GCM_SHA384;
	}

	if (Context->Credential->EnabledProtocols & SP_PROT_TLS1_2) {
		ProtocolVersions[NumberOfProtocolVersions++] = TLS1_2_PROTOCOL_VERSION;

		//
		// The list of TLS 1.2 cipher suites is loosely modeled after the list that
		// Chromium supports.
		//

		// Good
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384;
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256;
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384;

		// Meh
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA;
		CipherSuites[NumberOfCipherSuites++] = TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA;

		// Non-Perfect Forward Secrecy suites only for backwards compatibility.
		CipherSuites[NumberOfCipherSuites++] = TLS_RSA_WITH_AES_128_CBC_SHA;
		CipherSuites[NumberOfCipherSuites++] = TLS_RSA_WITH_AES_256_CBC_SHA;
	}

	ASSERT (NumberOfProtocolVersions <= ARRAYSIZE(ProtocolVersions));
	ASSERT (NumberOfCipherSuites <= ARRAYSIZE(CipherSuites));

	if (NumberOfProtocolVersions == 0 || NumberOfCipherSuites == 0) {
		// Caller did not enable any of the protocols we support
		return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
	}

	// Record the cipher suites in the context.
	// We'll check upon receiving server hello whether the server chose one of the
	// enabled suites.
	Context->NumberOfEnabledCipherSuites = NumberOfCipherSuites;
	KexRtlCopyMemory(
		Context->EnabledCipherSuites,
		CipherSuites,
		NumberOfCipherSuites * sizeof(CipherSuites[0]));

	//
	// Add extensions.
	// SNI and ALPN are optional.
	// The Key Share extension is only used for TLS 1.3.
	//

	KexRtlZeroMemory(ExtensionSpecs, sizeof(ExtensionSpecs));

	NumberOfExtensions = 0;

	ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_SUPPORTED_VERSIONS;
	ExtensionSpecs[NumberOfExtensions].SupportedVersions.NumberOfProtocolVersions = NumberOfProtocolVersions;
	ExtensionSpecs[NumberOfExtensions].SupportedVersions.ProtocolVersions = ProtocolVersions;
	++NumberOfExtensions;

	ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_SUPPORTED_GROUPS;
	ExtensionSpecs[NumberOfExtensions].SupportedGroups.NumberOfSupportedGroups = NumberOfSupportedGroups;
	ExtensionSpecs[NumberOfExtensions].SupportedGroups.SupportedGroups = SupportedGroups;
	++NumberOfExtensions;

	ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_SIGNATURE_ALGORITHMS;
	ExtensionSpecs[NumberOfExtensions].SignatureAlgorithms.NumberOfSignatureSchemes = NumberOfSignatureSchemes;
	ExtensionSpecs[NumberOfExtensions].SignatureAlgorithms.SignatureSchemes = SignatureSchemes;
	++NumberOfExtensions;

	if (Context->TargetNameCch > 0) {
		ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_SERVER_NAME;

		ExtensionSpecs[NumberOfExtensions].ServerName.ServerNameCb =
			Context->TargetNameCch * sizeof(Context->TargetName[0]);

		ExtensionSpecs[NumberOfExtensions].ServerName.ServerName = Context->TargetName;

		++NumberOfExtensions;
	}

	if (Context->ApplicationProtocolsCb > 0) {
		ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_ALPN;

		ExtensionSpecs[NumberOfExtensions].ApplicationProtocols.ProtocolList =
			Context->ApplicationProtocols;

		ExtensionSpecs[NumberOfExtensions].ApplicationProtocols.ProtocolListCb =
			Context->ApplicationProtocolsCb;

		++NumberOfExtensions;
	}

	if (Context->Credential->EnabledProtocols & SP_PROT_TLS1_3) {
		NTSTATUS Status;

		Status = BCryptGenerateKeyPair(
			BCRYPT_ECDH_P256_ALG_HANDLE,
			&Context->EphemeralKey13,
			256,
			0);

		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		Status = BCryptFinalizeKeyPair(Context->EphemeralKey13, 0);
		ASSERT (NT_SUCCESS(Status));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		ExtensionSpecs[NumberOfExtensions].ExtensionType = TLS_EXT_KEY_SHARE;
		ExtensionSpecs[NumberOfExtensions].KeyShare.NumberOfPublicKeys = 1;
		ExtensionSpecs[NumberOfExtensions].KeyShare.PublicKeys = &Context->EphemeralKey13;
		++NumberOfExtensions;

		Context->EccKeyType = TLS_GROUP_SECP256R1;
	}

	ASSERT (NumberOfExtensions <= ARRAYSIZE(ExtensionSpecs));

	//
	// Fill in the ClientHelloSpec structure.
	//

	KexRtlZeroMemory(&ClientHelloSpec, sizeof(ClientHelloSpec));
	ClientHelloSpec.ClientVersion = TLS1_2_PROTOCOL_VERSION;
	
	KexRtlCopyMemory(
		ClientHelloSpec.ClientRandom,
		Context->ClientRandom,
		sizeof(Context->ClientRandom));

	ClientHelloSpec.SessionIdCb = sizeof(ClientHelloSpec.ClientRandom);
	ClientHelloSpec.SessionId = Context->ClientRandom;

	ClientHelloSpec.NumberOfCipherSuites = NumberOfCipherSuites;
	ClientHelloSpec.CipherSuites = CipherSuites;

	ClientHelloSpec.NumberOfCompressionMethods = NumberOfCompressionMethods;
	ClientHelloSpec.CompressionMethods = CompressionMethods;

	ClientHelloSpec.NumberOfExtensions = NumberOfExtensions;
	ClientHelloSpec.Extensions = ExtensionSpecs;

	//
	// Serialize the token into a temporary buffer.
	//

	KexRtlZeroMemory(&ClientHelloIoInformation, sizeof(ClientHelloIoInformation));
	ClientHelloIoInformation.OutBuffer = ClientHelloBuffer;
	ClientHelloIoInformation.OutBufferCb = sizeof(ClientHelloBuffer);

	SecStatus = TlspSerializeClientHello(
		&ClientHelloIoInformation,
		&ClientHelloSpec);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	//
	// Write the TLS record header + client hello into the caller's IoInformation
	//

	ASSERT (ClientHelloIoInformation.OutBufferWrittenCb <= USHRT_MAX);

	TlspSerializeRecordHeader(
		&RecordHeader,
		CT_HANDSHAKE,
		TLS1_2_PROTOCOL_VERSION,
		(USHORT) ClientHelloIoInformation.OutBufferWrittenCb);

	SecStatus = IoWrite(
		IoInformation,
		&RecordHeader,
		sizeof(RecordHeader));

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	SecStatus = IoWrite(
		IoInformation,
		ClientHelloIoInformation.OutBuffer,
		ClientHelloIoInformation.OutBufferWrittenCb);

	ASSERT (SUCCEEDED(SecStatus));

	if (FAILED(SecStatus)) {
		return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
	}

	//
	// Save ClientHello into the context.
	//

	ASSERT (Context->SavedClientHello == NULL);

	Context->SavedClientHelloCb = ClientHelloIoInformation.OutBufferWrittenCb;
	Context->SavedClientHello = SafeAlloc(BYTE, Context->SavedClientHelloCb);

	if (Context->SavedClientHello == NULL) {
		return SP_LOG_RESULT(SEC_E_INSUFFICIENT_MEMORY);
	}

	KexRtlCopyMemory(
		Context->SavedClientHello,
		ClientHelloIoInformation.OutBuffer,
		ClientHelloIoInformation.OutBufferWrittenCb);

	// Indicate that the output token needs to be sent to the server
	Context->State = CONTEXTSTATE_EXPECTING_SERVER_HELLO;
	return SEC_I_CONTINUE_NEEDED;
}

//
// Reads a ServerHello handshake MESSAGE (not record) and places the information
// contained within it into the given Context.
// Does not make any use of the output buffer.
//
// The input data to this function should not include the 4-byte message header.
//
// Upon parsing invalid data, an error code will be returned.
//
// This function will set the protocol version, server random, and cipher suite
// in the context structure, which will remain as the final values for the rest
// of this TLS session.
//
SECURITY_STATUS TlspHandleServerHello(
	IN OUT	PKXSCHANL_CONTEXT			Context,
	IN OUT	PKXSCHANL_IO_INFORMATION	IoInformation)
{
	NTSTATUS Status;
	SECURITY_STATUS SecStatus;
	TLS_EXTENSION_SPEC Extensions[3];
	TLS_SERVER_HELLO_SPEC ServerHello;
	ULONG Index;

	ASSERT (Context->ProtocolVersion == 0);
	ASSERT (Context->CipherSuite == 0);
	ASSERT (Context->SelectedApplicationProtocolCb == 0);
	ASSERT (Context->State == CONTEXTSTATE_EXPECTING_SERVER_HELLO);

	// Give TlspDeserializeServerHello a place to put extension information.
	ServerHello.NumberOfExtensions = ARRAYSIZE(Extensions);
	ServerHello.Extensions = Extensions;

	SecStatus = TlspDeserializeServerHello(
		Context,
		IoInformation,
		&ServerHello);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	// Handle extensions inside the Server Hello.
	SecStatus = TlspHandleExtensions(
		Context,
		ServerHello.Extensions,
		ServerHello.NumberOfExtensions);

	if (FAILED(SecStatus)) {
		return SecStatus;
	}

	// If Context->ProtocolVersion has not been set, it means the server did not
	// respond with a Supported Versions extension, which means we use the "legacy"
	// version field in the server hello.
	if (Context->ProtocolVersion == 0) {
		Context->ProtocolVersion = ServerHello.ServerVersion;
	}

	//
	// Check that the final protocol version value is supported.
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		if (!(Context->Credential->EnabledProtocols & SP_PROT_TLS1_2)) {
			goto BadProtocolVersion;
		}

		break;
	case TLS1_3_PROTOCOL_VERSION:
		if (!(Context->Credential->EnabledProtocols & SP_PROT_TLS1_3)) {
			goto BadProtocolVersion;
		}

		break;
	default:
	BadProtocolVersion:
		return SP_LOG_RESULT(SEC_E_ALGORITHM_MISMATCH);
	}

	//
	// We may have created an ephemeral key to send a TLS 1.3-compatible
	// client hello. If we haven't selected TLS 1.3, we can get rid of it now.
	//

	if (Context->ProtocolVersion != TLS1_3_PROTOCOL_VERSION) {
		SafeBCryptDestroyKey(Context->EphemeralKey13);
	}

	//
	// If the current protocol version is TLS 1.2 but we offered TLS 1.3,
	// then we need to check for the downgrade sentinel in the server random.
	// If the downgrade sentinel is present under these conditions, we will
	// abort the connection.
	//

	if (Context->ProtocolVersion <= TLS1_2_PROTOCOL_VERSION &&
		(Context->Credential->EnabledProtocols & SP_PROT_TLS1_3)) {

		STATIC CONST BYTE Downgrade12[] = {
			0x44, 0x4F, 0x57, 0x4E, 0x47, 0x52, 0x44, 0x01
		};

		STATIC CONST BYTE Downgrade11orBelow[] = {
			0x44, 0x4F, 0x57, 0x4E, 0x47, 0x52, 0x44, 0x00
		};
		
		PCBYTE DowngradeSentinel;

		// Check for downgrade.
		// https://datatracker.ietf.org/doc/html/rfc8446.html#section-4.1.3
		//
		// If negotiating TLS 1.2, TLS 1.3 servers MUST set the last 8 bytes of
		// their Random value to the bytes:
		//
		//   44 4F 57 4E 47 52 44 01
		//
		// If negotiating TLS 1.1 or below, TLS 1.3 servers MUST, and TLS 1.2
		// servers SHOULD, set the last 8 bytes of their ServerHello.Random
		// value to the bytes:
		//
		//   44 4F 57 4E 47 52 44 00

		DowngradeSentinel = &ServerHello.ServerRandom[ARRAYSIZE(ServerHello.ServerRandom) - 8];

		if (RtlEqualMemory(DowngradeSentinel, Downgrade12, 8) ||
			RtlEqualMemory(DowngradeSentinel, Downgrade11orBelow, 8)) {

			return SP_LOG_RESULT(SEC_E_DOWNGRADE_DETECTED);
		}
	}

	//
	// Check that the key_share extension was present if TLS 1.3 is selected,
	// and make sure that it is NOT present if TLS 1.2 was selected.
	//

	if (Context->ProtocolVersion == TLS1_2_PROTOCOL_VERSION) {
		if (Context->ServerEphemeralKey13 != NULL) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	} else if (Context->ProtocolVersion == TLS1_3_PROTOCOL_VERSION) {
		ULONG ServerKeyBits;
		ULONG ClientKeyBits;
		ULONG WrittenCb;

		if (Context->ServerEphemeralKey13 == NULL) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}

		//
		// Check that the server's key length matches ours.
		//

		Status = BCryptGetProperty(
			Context->ServerEphemeralKey13,
			BCRYPT_KEY_LENGTH,
			(PUCHAR) &ServerKeyBits,
			sizeof(ServerKeyBits),
			&WrittenCb,
			0);

		ASSERT (NT_SUCCESS(Status));
		ASSERT (WrittenCb == sizeof(ServerKeyBits));

		if (!NT_SUCCESS(Status)) {
			return SP_LOG_RESULT(SEC_E_INTERNAL_ERROR);
		}

		SecStatus = TlspGetEccKeyBits(Context->EccKeyType, &ClientKeyBits);
		ASSERT (SUCCEEDED(SecStatus));

		if (ServerKeyBits != ClientKeyBits) {
			return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
		}
	} else {
		NOT_REACHED;
	}

	//
	// Copy server random into the context structure.
	//

	STATIC_ASSERT (sizeof(ServerHello.ServerRandom) == sizeof(Context->ServerRandom));

	KexRtlCopyMemory(
		Context->ServerRandom,
		ServerHello.ServerRandom,
		sizeof(Context->ServerRandom));

	//
	// Check that the cipher suite is one that we support.
	// If so, put it in the context.
	//

	for (Index = 0; Index < Context->NumberOfEnabledCipherSuites; ++Index) {
		if (ServerHello.CipherSuite == Context->EnabledCipherSuites[Index]) {
			Context->CipherSuite = ServerHello.CipherSuite;
			break;
		}
	}

	if (Context->CipherSuite == 0) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Check that the compression method is 0.
	//

	if (ServerHello.CompressionMethod != 0) {
		return SP_LOG_RESULT(SEC_E_ILLEGAL_MESSAGE);
	}

	//
	// Set the next expected message based on the now-known protocol version.
	//

	switch (Context->ProtocolVersion) {
	case TLS1_2_PROTOCOL_VERSION:
		KexLogDebugEvent(L"Negotiated TLS 1.2");
		Context->State = CONTEXTSTATE_TLS1_2_EXPECTING_CERTIFICATE;
		break;
	case TLS1_3_PROTOCOL_VERSION:
		KexLogDebugEvent(L"Negotiated TLS 1.3");
		Context->State = CONTEXTSTATE_TLS1_3_EXPECTING_ENCRYPTED_EXTENSIONS;
		break;
	default:
		NOT_REACHED;
	}

	return SEC_E_OK;
}