///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     ctltoken.c
//
// Abstract:
//
//     Contains the implementation for ApplyControlToken.
//
// Author:
//
//     vxiiduu (12-May-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               12-May-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxschanlp.h"

SECURITY_STATUS SEC_ENTRY SppApplyControlToken(
	IN	PCtxtHandle		ContextHandle,
	IN	PSecBufferDesc	Input)
{
	PKXSCHANL_CONTEXT Context;
	PSecBuffer Buffer;

	Context = SppReadContextHandle(ContextHandle);

	if (Input->cBuffers != 1) {
		return SP_LOG_RESULT(SEC_E_INVALID_TOKEN);
	}

	Buffer = &Input->pBuffers[0];

	if (Buffer->cbBuffer < sizeof(ULONG)) {
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
	}

	switch (*(PULONG) Buffer->pvBuffer) {
	case SCHANNEL_SHUTDOWN:
		if (Context->State != CONTEXTSTATE_HANDSHAKE_COMPLETE) {
			return SP_LOG_RESULT(SEC_E_INVALID_HANDLE);
		}

		Context->State = CONTEXTSTATE_SENDING_CLOSE_NOTIFY;
		return SEC_E_OK;

	default:
		return SP_LOG_RESULT(SEC_E_UNSUPPORTED_FUNCTION);
	}

	NOT_REACHED;
}