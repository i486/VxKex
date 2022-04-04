#include <Windows.h>
#include <KexComm.h>
#include <NtDll.h>

LPWSTR GetCommandLineWithoutImageName(
	VOID)
{
	BOOL bDoubleQuoted = FALSE;
	LPWSTR lpszRaw = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;

	while (*lpszRaw > ' ' || (*lpszRaw && bDoubleQuoted)) {
		if (*lpszRaw == '"') {
			bDoubleQuoted = !bDoubleQuoted;
		}

		lpszRaw++;
	}

	while (*lpszRaw && *lpszRaw <= ' ') {
		lpszRaw++;
	}

	return lpszRaw;
}