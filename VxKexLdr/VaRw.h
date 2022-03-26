#pragma once
#include <KexComm.h>

VOID VaRead(
	IN	ULONG_PTR	ulSrc,
	OUT	LPVOID		lpDst,
	IN	SIZE_T		nSize);

VOID VaWrite(
	IN	ULONG_PTR	ulDst,
	IN	LPCVOID		lpSrc,
	IN	SIZE_T		nSize);

VOID VaWriteByte(
	IN	ULONG_PTR	ulDst,
	IN	BYTE		btData);

VOID VaWriteWord(
	IN	ULONG_PTR	ulDst,
	IN	WORD		wData);

VOID VaWriteDword(
	IN	ULONG_PTR	ulDst,
	IN	DWORD		dwData);

VOID VaWriteQword(
	IN	ULONG_PTR	ulDst,
	IN	QWORD		qwData);

VOID VaWritePtr(
	IN	ULONG_PTR	ulDst,
	IN	ULONG_PTR	ulData);

VOID VaWriteSzA(
	IN	ULONG_PTR	ulDst,
	IN	LPCTSTR		lpszSrc);

BYTE VaReadByte(
	IN	ULONG_PTR	ulSrc);

WORD VaReadWord(
	IN	ULONG_PTR	ulSrc);

DWORD VaReadDword(
	IN	ULONG_PTR	ulSrc);

QWORD VaReadQword(
	IN	ULONG_PTR	ulSrc);

ULONG_PTR VaReadPtr(
	IN	ULONG_PTR	ulSrc);

VOID VaReadSzA(
	IN	ULONG_PTR	ulSrc,
	IN	LPSTR		lpszDest,
	IN	DWORD		dwcchDest);

VOID VaReadSzW(
	IN	ULONG_PTR	ulSrc,
	IN	LPWSTR		lpszDest,
	IN	DWORD		dwcchDest);