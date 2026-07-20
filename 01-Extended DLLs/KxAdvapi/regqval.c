///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     regqval.c
//
// Abstract:
//
//     Contains RegQueryValue extensions.
//
// Author:
//
//     vxiiduu (23-Jun-2026)
//
// Revision History:
//
//     vxiiduu              23-Jun-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxadvapip.h"

//
// This function is called only when something calls RegQueryValueExW through
// a DLL or API-set that resolves to kxadvapi. When an application calls
// RegQueryValueExW through kxbase, this function is not called.
//
// This is fine because this extended function currently only has a single purpose,
// which is to cause SSPICLI to load KxSChanl as a SSP.
// See sspicli!SecpReadPackageList.
//
KXADVAPI LSTATUS WINAPI Ext_RegQueryValueExW(
	IN		HKEY	KeyHandle,
	IN		PCWSTR	ValueName OPTIONAL,
	IN		PULONG	Reserved OPTIONAL,
	OUT		PULONG	DataType OPTIONAL,
	OUT		PBYTE	Buffer OPTIONAL,
	IN OUT	PULONG	BufferCb OPTIONAL)
{
	LSTATUS LStatus;
	ULONG OriginalBufferCb;

	OriginalBufferCb = 0;

	if (BufferCb) {
		OriginalBufferCb = *BufferCb;
	}

	LStatus = RegQueryValueExW(
			KeyHandle,
			ValueName,
			Reserved,
			DataType,
			Buffer,
			BufferCb);

	if (LStatus != ERROR_SUCCESS) {
		return LStatus;
	}

	if (ValueName != NULL &&
		StringEqualW(ValueName, L"SecurityProviders") &&
		DataType != NULL &&
		*DataType == REG_SZ &&
		AshModuleBaseNameIs(ReturnAddress(), L"sspicli.dll")) {

		STATIC CONST WCHAR SecurityProvidersAppend[] = L",kxschanl.dll";

		ASSERT (VALID_HANDLE(KeyHandle));
		ASSERT (Reserved == NULL);
		ASSERT (BufferCb != NULL);

		*BufferCb += sizeof(SecurityProvidersAppend);

		if (Buffer != NULL) {
			HRESULT Result;

			ASSERT (OriginalBufferCb != 0);

			// SSPICLI is querying the value data.
			Result = StringCchCat(
				(PWSTR) Buffer,
				OriginalBufferCb,
				SecurityProvidersAppend);

			ASSERT (SUCCEEDED(Result));
		}
	}

	return LStatus;
}