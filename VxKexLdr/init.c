#include <NtDll.h>
#include "VxKexLdr.h"

#include <Shlwapi.h>

// Figure out the full path of the EXE based on the command line we are passed.
// Return TRUE if lpszExeFullPath contains an existing EXE file, or return
// FALSE if we couldn't determine the EXE file.
// If returned FALSE, call GetLastError() to find out what happened.
//
// This is basically a duplicate of the same functionality inside
// CreateProcessInternalW. However, it's easier to rewrite it than to try and
// mess around with hooks and trying to extract the information from inside the
// CreateProcessInternalW function.
BOOL GetExeFullPathFromCmdLine(
	IN	LPWSTR	lpszCmdLine,
	OUT	LPWSTR	lpszBuffer,
	IN	SIZE_T	cchBuffer,
	OUT	LPWSTR	*ppszBaseName OPTIONAL)
{
	BOOL bSuccess;
	LPWSTR lpTempNull = lpszCmdLine;

	// Check for a leading quote. If this happens then we are lucky because we can
	// just find the other quote, and then search for the file.
	if (*lpszCmdLine == '"') {
		lpTempNull = ++lpszCmdLine;

		while (*lpTempNull && *lpTempNull != '"') {
			lpTempNull++;
		}
		
		if (*lpTempNull == '"') {
			*lpTempNull = '\0';
		} else {
			// this means there was 1 quote at the beginning and no closing quote.
			// this is invalid, since a file name cannot contain a quote.
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		bSuccess = !!SearchPath(NULL, lpszCmdLine, L".exe", (DWORD) cchBuffer, lpszBuffer, ppszBaseName);
		*lpTempNull = '"'; // restore original character - don't forget
		return bSuccess;
	}

	// No leading quote. So we have to traverse lpszCmdLine until we find a space ' '
	// or a null '\0' and at every occurrence, try SearchPath until we find one that
	// works.

	while (TRUE) {
		while (*lpTempNull && *lpTempNull != ' ') {
			lpTempNull++;
		}

		if (*lpTempNull == ' ') {
			*lpTempNull = '\0';
		} else {
			// reached the end of lpszCmdLine
			return !!SearchPath(NULL, lpszCmdLine, L".exe", (DWORD) cchBuffer, lpszBuffer, ppszBaseName);
		}

		if (SearchPath(NULL, lpszCmdLine, L".exe", (DWORD) cchBuffer, lpszBuffer, ppszBaseName)) {
			*lpTempNull = ' ';
			return TRUE;
		} else {
			*lpTempNull++ = ' ';
		}
	}
}