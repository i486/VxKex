#include "buildcfg.h"
#include "kxnetp.h"
#include <WinSock.h>

//
// libuv (which is used by some random node.js package that a lot of electron
// apps, such as VSCode, Signal, etc.) depends on GetHostNameW and will shit
// itself if not found.
//
KXNETAPI INT WINAPI GetHostNameW(
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