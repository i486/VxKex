///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     moz2sst.c
//
// Abstract:
//
//     Downloads Mozilla root certificates (PEM format) from curl.se and
//     converts to SST format.
//
// Author:
//
//     vxiiduu (30-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               30-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexGui.h>
#include <KexW32ML.h>
#include <WinInet.h>

#define CERT_START_MARKER "-----BEGIN CERTIFICATE-----"
#define CERT_END_MARKER "-----END CERTIFICATE-----"

//
// Wrapper function for WinInet that downloads the specified page and
// prints it to the standard output. Returns the number of bytes
// downloaded, or ((ULONG)-1) on error.
//

NTSTATUS EntryPoint(
	VOID)
{
	// the .pem file is currently 186kb, we'll have a buffer of 256kb
	// (256kb should be enough for everybody)
	BYTE Buffer[262144];
	ULONG BufferWrittenCb;
	PCSTR Cursor;
	HCERTSTORE CertStore;
	HINTERNET InternetHandle;
	HINTERNET UrlHandle;
	BOOL Success;

	KexgApplicationFriendlyName = L"VxKex ROOT.SST Downloader";

	{
		INT UserResponse;

		UserResponse = MessageBoxF(
			TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
			NULL,
			KexgApplicationFriendlyName,
			NULL,
			L"The downloader will connect to the internet to fetch "
			L"a new cacert.pem file. Is this acceptable?");

		if (UserResponse != IDYES) {
			ExitProcess(STATUS_USER_DISABLED);
		}
	}

	//
	// Download the PEM file into a buffer
	//

	InternetHandle = InternetOpen(
		L"moz2sst",
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);

	if (InternetHandle == NULL) {
		// internet error codes don't work with GetLastErrorAsString apparently
		CriticalErrorBoxF(L"InternetOpen failed: %lu", GetLastError());
		NOT_REACHED;
	}

	UrlHandle = InternetOpenUrl(
		InternetHandle,
		L"https://curl.se/ca/cacert.pem",
		NULL,
		0,
		INTERNET_FLAG_RELOAD,
		0);

	if (UrlHandle == NULL) {
		CriticalErrorBoxF(L"InternetOpenUrl failed: %lu", GetLastError());
		NOT_REACHED;
	}

	{
		ULONG HttpStatusCode;
		ULONG HttpStatusCodeCb;

		HttpStatusCodeCb = sizeof(HttpStatusCode);

		HttpQueryInfo(
			UrlHandle,
			HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
			&HttpStatusCode,
			&HttpStatusCodeCb,
			NULL);

		if (HttpStatusCode != HTTP_STATUS_OK) {
			CriticalErrorBoxF(L"HTTP status code %lu", HttpStatusCode);
			NOT_REACHED;
		}
	}

	BufferWrittenCb = 0;

	while (TRUE) {
		ULONG BytesRead;

		Success = InternetReadFile(
			UrlHandle,
			&Buffer[BufferWrittenCb],
			sizeof(Buffer) - BufferWrittenCb - 1, // -1 for null terminator
			&BytesRead);

		if (!Success) {
			CriticalErrorBoxF(L"InternetReadFile failed: %lu", GetLastError());
			NOT_REACHED;
		}

		if (BytesRead == 0) {
			break;
		}

		BufferWrittenCb += BytesRead;
	}

	InternetCloseHandle(UrlHandle);
	InternetCloseHandle(InternetHandle);

	// null terminate it
	Buffer[BufferWrittenCb] = '\0';
	
	//
	// make a cert store to store all the certificates
	//

	{
		WCHAR SstPath[MAX_PATH];

		GetModuleFileName(NULL, SstPath, ARRAYSIZE(SstPath));
		PathCchRemoveFileSpec(SstPath, ARRAYSIZE(SstPath));
		PathCchAppend(SstPath, ARRAYSIZE(SstPath), L"ROOT.sst");

		// CertOpenStore never overwrites files so we have to delete if it exists
		DeleteFile(SstPath);

		CertStore = CertOpenStore(
			CERT_STORE_PROV_FILENAME,
			0,
			0,
			CERT_STORE_CREATE_NEW_FLAG | CERT_FILE_STORE_COMMIT_ENABLE_FLAG,
			SstPath);

		if (!CertStore) {
			CriticalErrorBoxF(
				L"Failed to create certificate store file: %s",
				GetLastErrorAsString());

			NOT_REACHED;
		}
	}

	//
	// Parse the buffer
	//

	Cursor = (PCSTR) Buffer;

	while (TRUE) {
		PCSTR Base64Data;
		ULONG Base64DataCb;
		BYTE CertificateData[8192];
		ULONG CertificateDataCb;

		Cursor = StringFindA(Cursor, CERT_START_MARKER);

		if (!Cursor) {
			break;
		}

		Cursor += StringLiteralLength(CERT_START_MARKER);
		Base64Data = Cursor;

		Cursor = StringFindA(Cursor, CERT_END_MARKER);

		if (!Cursor) {
			CriticalErrorBoxF(L"Malformed input data: unterminated certificate");
			NOT_REACHED;
		}

		Base64DataCb = Cursor - Base64Data;
		Cursor += StringLiteralLength(CERT_END_MARKER);

		CertificateDataCb = sizeof(CertificateData);

		Success = CryptStringToBinaryA(
			Base64Data,
			Base64DataCb,
			CRYPT_STRING_BASE64,
			CertificateData,
			&CertificateDataCb,
			NULL,
			NULL);

		if (!Success) {
			CriticalErrorBoxF(L"Malformed input data: unable to parse base64");
			NOT_REACHED;
		}

		Success = CertAddEncodedCertificateToStore(
			CertStore,
			X509_ASN_ENCODING,
			CertificateData,
			CertificateDataCb,
			CERT_STORE_ADD_NEW,
			NULL);

		if (!Success) {
			CriticalErrorBoxF(
				L"Failed to add certificate data to store: %s",
				GetLastErrorAsString());

			NOT_REACHED;
		}
	}

	CertCloseStore(CertStore, CERT_CLOSE_STORE_FORCE_FLAG);
	InfoBoxF(L"Success");
	ExitProcess(0);
}