#include "buildcfg.h"
#include "kxnetp.h"

//
// libuv (which is used by some random node.js package that a lot of electron
// apps, such as VSCode, Signal, etc. use,) depends on GetHostNameW and will
// shit itself if not found.
//
KXNETAPI INT WSAAPI GetHostNameW(
	OUT	PWSTR	Name,
	IN	INT		NameCch)
{
	INT ReturnValue;
	INT ConvertedCch;
	PCHAR NameAnsi;

	if (!NameCch) {
		WSASetLastError(WSAEFAULT);
		return SOCKET_ERROR;
	}

	NameAnsi = StackAlloc(CHAR, NameCch);
	ReturnValue = gethostname(NameAnsi, NameCch);

	if (ReturnValue == SOCKET_ERROR) {
		return ReturnValue;
	}

	ConvertedCch = MultiByteToWideChar(
		CP_ACP,
		0,
		NameAnsi,
		-1,
		Name,
		NameCch);

	if (ConvertedCch == 0) {
		WSASetLastError(WSAEFAULT);
		return SOCKET_ERROR;
	}

	return ReturnValue;
}

//
// Used for app-specific hack.
//
KXNETAPI INT WSAAPI Ext_getaddrinfo(
	IN	PCSTR					NodeName,
	IN	PCSTR					ServerName,
	IN	const struct addrinfo	*Hints,
	OUT	struct addrinfo			**Results)
{
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		//
		// APPSPECIFICHACK: The game "Life is Strange: True Colors" attempts to connect
		// to a server and send some telemetry data. On Windows 7, the gathering of this
		// telemetry data fails. The error handling code in the game is faulty and will
		// cause a crash.
		//
		// As a workaround, we will simply fail "getaddrinfo" calls coming from the game, to
		// simulate the lack of an internet connection.
		//
		// For reference, the actual module that calls getaddrinfo with the game's
		// telemetry server is "osdk.dll", and the host name requested is
		// "see-siren.os.eidos.com".
		//

		if (AshExeBaseNameIs(L"Siren-Win64-Shipping.exe")) {
			return WSANO_DATA;
		}
	}

	return getaddrinfo(NodeName, ServerName, Hints, Results);
}

//
// The game osu!lazer fails to connect to the internet due to an unsupported
// socket option (SO_REUSE_UNICASTPORT) which is only available on newer
// versions of Windows. It can safely be ignored.
//
KXNETAPI INT WSAAPI Ext_setsockopt(
	IN	SOCKET	SocketHandle,
	IN	INT		Level,
	IN	INT		OptionId,
	IN	PCVOID	OptionBuffer,
	IN	INT		OptionCb)
{
	INT ReturnCode;
	INT ErrorCode;

	ReturnCode = setsockopt(
		SocketHandle,
		Level,
		OptionId,
		(PCCHAR) OptionBuffer,
		OptionCb);

	if (ReturnCode == SOCKET_ERROR) {
		ErrorCode = WSAGetLastError();
	}

	if (ReturnCode == SOCKET_ERROR &&
		ErrorCode == WSAENOPROTOOPT &&
		Level == SOL_SOCKET) {

		switch (OptionId) {
		case SO_REUSE_UNICASTPORT:
		case SO_REUSE_MULTICASTPORT:
			KexLogDebugEvent(L"Faking success (OptionId 0x%x)", OptionId);
			ReturnCode = 0;
			break;
		}
	}

	if (ReturnCode == SOCKET_ERROR) {
		KexLogWarningEvent(
			L"Windows Sockets function failed with an error code of %d\r\n\r\n"
			L"SocketHandle: 0x%p\r\n"
			L"Level:        %d\r\n"
			L"OptionId:     %d\r\n"
			L"OptionBuffer: 0x%p\r\n"
			L"OptionCb:     %d",
			ErrorCode,
			SocketHandle,
			Level,
			OptionId,
			OptionBuffer,
			OptionCb);

		KexDebugCheckpoint();
	}

	return ReturnCode;
}

KXNETAPI INT WSAAPI Ext_getsockopt(
	IN		SOCKET	SocketHandle,
	IN		INT		Level,
	IN		INT		OptionId,
	OUT		PVOID	OptionBuffer,
	IN OUT	PINT	OptionCb)
{
	INT ReturnCode;
	INT ErrorCode;

	ReturnCode = getsockopt(
		SocketHandle,
		Level,
		OptionId,
		(PCHAR) OptionBuffer,
		OptionCb);

	if (ReturnCode == SOCKET_ERROR) {
		ErrorCode = WSAGetLastError();
	}

	if (ReturnCode == SOCKET_ERROR &&
		ErrorCode == WSAENOPROTOOPT &&
		Level == SOL_SOCKET) {

		switch (OptionId) {
		case SO_REUSE_UNICASTPORT:
		case SO_REUSE_MULTICASTPORT:
			ASSERT (OptionCb != NULL);
			ASSERT (*OptionCb >= sizeof(ULONG));

			if (*OptionCb < sizeof(ULONG)) {
				return ReturnCode;
			}

			KexLogDebugEvent(L"Faking success (OptionId 0x%x)", OptionId);
			*(PULONG) OptionBuffer = 0;
			*OptionCb = sizeof(ULONG);
			ReturnCode = 0;
			break;
		}
	}

	if (ReturnCode == SOCKET_ERROR) {
		KexLogWarningEvent(
			L"Windows Sockets function failed with an error code of %d\r\n\r\n"
			L"SocketHandle: 0x%p\r\n"
			L"Level:        %d\r\n"
			L"OptionId:     %d\r\n"
			L"OptionBuffer: 0x%p\r\n"
			L"OptionCb:     %d",
			ErrorCode,
			SocketHandle,
			Level,
			OptionId,
			OptionBuffer,
			OptionCb);

		KexDebugCheckpoint();
	}

	return ReturnCode;
}