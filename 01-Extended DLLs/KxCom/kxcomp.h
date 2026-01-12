#pragma once

#include <KexComm.h>
#include <KexDll.h>
#include <KxCom.h>

#define WM_DDE_FIRST 0x3E0
#define WM_DDE_LAST 0x3E8

EXTERN PKEX_PROCESS_DATA KexData;

KXCOMAPI HRESULT STDMETHODCALLTYPE CActivationFactory_QueryInterface(
	IN	IActivationFactory	*This,
	IN	REFIID				RefIID,
	OUT	PPVOID				Object);