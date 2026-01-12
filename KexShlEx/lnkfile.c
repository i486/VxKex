#include "buildcfg.h"
#include "KexShlEx.h"
#include <shlobj.h>

//
// FileName must be MAX_PATH characters in size.
//
HRESULT GetTargetFromLnkfile(
	IN OUT	PWSTR	FileName)
{
	HRESULT Result;
	WCHAR LnkTarget[MAX_PATH];
	IShellLink *ShellLink;
	IPersistFile *PersistFile;

	ShellLink = NULL;
	PersistFile = NULL;

	ASSERT (FileName != NULL);

	Result = CoCreateInstance(
		&CLSID_ShellLink,
		NULL,
		CLSCTX_INPROC_SERVER,
		&IID_IShellLink,
		(PPVOID) &ShellLink);

	ASSERT (SUCCEEDED(Result));
	ASSERT (ShellLink != NULL);

	if (FAILED(Result)) {
		return Result;
	}

	try {
		Result = IShellLinkW_QueryInterface(
			ShellLink,
			&IID_IPersistFile,
			(PPVOID) &PersistFile);

		ASSERT (SUCCEEDED(Result));
		ASSERT (PersistFile != NULL);

		if (FAILED(Result)) {
			leave;
		}

		Result = IPersistFile_Load(PersistFile, FileName, STGM_READ);
		ASSERT (SUCCEEDED(Result));

		if (FAILED(Result)) {
			leave;
		}

		Result = IShellLinkW_Resolve(
			ShellLink,
			NULL,
			SLR_NOTRACK | SLR_NOSEARCH | SLR_NO_UI | SLR_NOUPDATE);

		if (FAILED(Result)) {
			leave;
		}

		Result = IShellLinkW_GetPath(
			ShellLink,
			LnkTarget,
			ARRAYSIZE(LnkTarget),
			NULL,
			SLGP_UNCPRIORITY);

		if (FAILED(Result)) {
			leave;
		}
	} finally {
		SafeRelease(ShellLink);
		SafeRelease(PersistFile);
	}

	if (SUCCEEDED(Result)) {
		Result = StringCchCopy(FileName, MAX_PATH, LnkTarget);
		ASSERT (SUCCEEDED(Result));
	} else {
		FileName[0] = '\0';
	}

	return Result;
}