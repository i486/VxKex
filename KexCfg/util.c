///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     util.c
//
// Abstract:
//
//     Miscellaneous utility functions for KexCfg
//
// Author:
//
//     vxiiduu (09-Feb-2024)
//
// Environment:
//
//     Win32 mode. Sometimes this program is run as Administrator, sometimes
//     as a standard account, sometimes as the local SYSTEM account.
//
// Revision History:
//
//     vxiiduu              09-Feb-2024  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <WtsApi32.h>

BOOLEAN Interactive;
ULONG SessionId;

BOOLEAN RunningInInteractiveWindowStation(
	VOID)
{
	BOOLEAN Success;
	WCHAR WindowStationName[16];
	HWINSTA WindowStation;

	WindowStation = GetProcessWindowStation();
	if (WindowStation == NULL) {
		return FALSE;
	}

	Success = GetUserObjectInformation(
		WindowStation,
		UOI_NAME,
		WindowStationName,
		sizeof(WindowStationName),
		NULL);

	if (!Success) {
		return FALSE;
	}

	return StringEqualI(WindowStationName, L"WinSta0");
}

INT KexCfgMessageBox(
	IN	HWND	ParentWindow OPTIONAL,
	IN	PWSTR	Message,
	IN	PWSTR	Title,
	IN	ULONG	Flags)
{
	BOOLEAN Success;
	ULONG Response;

	if (Interactive) {
		//
		// Use the normal MessageBox function when running interactively.
		// This allows the message box to have comctl v6 themes.
		//

		return MessageBox(
			ParentWindow,
			Message,
			Title,
			Flags);
	} else {
		if (Title == NULL) {
			// WTSSendMessage will error out upon receiving a NULL pointer
			// for Title.
			Title = L"Error";
		}

		Success = WTSSendMessage(
			WTS_CURRENT_SERVER_HANDLE,
			SessionId,
			Title,
			(ULONG) wcslen(Title) * sizeof(WCHAR),
			Message,
			(ULONG) wcslen(Message) * sizeof(WCHAR),
			Flags,
			0,
			&Response,
			TRUE);
	}

	if (!Success) {
		return 0;
	}

	return Response;
}

BOOLEAN KexCfgParseBooleanParameter(
	IN	PCWSTR	Parameter)
{
	WCHAR ParameterValue[32];
	ULONG Index;

	Index = 0;

	until (Parameter[Index] == ' ' || Parameter[Index] == '\0' || Index >= ARRAYSIZE(ParameterValue - 2)) {
		ParameterValue[Index] = Parameter[Index];
		++Index;
	}

	ParameterValue[Index] = '\0';

	if (StringEqualI(ParameterValue, L"TRUE") ||
		StringEqualI(ParameterValue, L"YES") ||
		StringEqualI(ParameterValue, L"1")) {

		return TRUE;
	} else if (StringEqualI(ParameterValue, L"FALSE") ||
			   StringEqualI(ParameterValue, L"NO") ||
			   StringEqualI(ParameterValue, L"0")) {

		return FALSE;
	} else {
		KexCfgMessageBox(
			NULL,
			L"A boolean argument was invalid. Pass the /? argument for more information.",
			FRIENDLYAPPNAME,
			MB_ICONERROR | MB_OK);

		ExitProcess(STATUS_INVALID_PARAMETER);
	}
}