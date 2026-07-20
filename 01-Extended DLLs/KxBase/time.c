#include "buildcfg.h"
#include "kxbasep.h"

//
// If strong SharedUserData spoofing is enabled, this function
// supersedes KernelBase!GetSystemTimeAsFileTime because the original
// function reads system time from SharedUserData.
//
KXBASEAPI VOID WINAPI KxBasepGetSystemTimeAsFileTimeHook(
	OUT	PFILETIME	SystemTimeAsFileTime)
{
	ASSERT (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA);
	NtQuerySystemTime((PLONGLONG) SystemTimeAsFileTime);
}

//
// Same as above but this function supersedes GetSystemTime when doing
// SharedUserData-based version spoofing.
//
KXBASEAPI VOID WINAPI KxBasepGetSystemTimeHook(
	OUT	PSYSTEMTIME	SystemTime)
{
	LONGLONG Time;
	TIME_FIELDS TimeFields;

	ASSERT (KexData->IfeoParameters.StrongVersionSpoof & KEX_STRONGSPOOF_SHAREDUSERDATA);

	NtQuerySystemTime(&Time);
	RtlTimeToTimeFields(&Time, &TimeFields);

	//
	// Annoyingly, the TIME_FIELDS structure is not directly compatible with
	// the SYSTEMTIME structure...
	//

	SystemTime->wYear			= TimeFields.Year;
	SystemTime->wMonth			= TimeFields.Month;
	SystemTime->wDay			= TimeFields.Day;
	SystemTime->wDayOfWeek		= TimeFields.Weekday;
	SystemTime->wHour			= TimeFields.Hour;
	SystemTime->wMinute			= TimeFields.Minute;
	SystemTime->wSecond			= TimeFields.Second;
	SystemTime->wMilliseconds	= TimeFields.Milliseconds;
}

KXBASEAPI VOID WINAPI GetSystemTimePreciseAsFileTime(
	OUT	PFILETIME	SystemTimeAsFileTime)
{
	*(PLONGLONG) SystemTimeAsFileTime = RtlGetSystemTimePrecise();
}

KXBASEAPI VOID WINAPI QueryUnbiasedInterruptTimePrecise(
	OUT	PULONGLONG	UnbiasedInterruptTimePrecise)
{
	QueryUnbiasedInterruptTime(UnbiasedInterruptTimePrecise);
}

// Required for some Unity games, I think the ones that use il2cpp.
// This function is a full implementation based on decompiled Win10 code.
KXBASEAPI ULONG WINAPI GetDynamicTimeZoneInformationEffectiveYears(
	IN	PCDYNAMIC_TIME_ZONE_INFORMATION			TimeZoneInformation,
	OUT	PULONG									FirstYear,
	OUT	PULONG									LastYear)
{
	ULONG Win32Error;
	HKEY TimeZonesKey;
	HKEY TimeZoneKey;
	HKEY DynamicDSTKey;
	ULONG ValueSize;

	if (TimeZoneInformation == NULL) {
		return ERROR_INVALID_PARAMETER;
	}

	Win32Error = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
		0,
		KEY_READ,
		&TimeZonesKey);

	if (Win32Error != ERROR_SUCCESS) {
		return Win32Error;
	}

	Win32Error = RegOpenKeyEx(
		TimeZonesKey,
		TimeZoneInformation->TimeZoneKeyName,
		0,
		KEY_READ,
		&TimeZoneKey);

	SafeClose(TimeZonesKey);

	if (Win32Error != ERROR_SUCCESS) {
		return Win32Error;
	}

	Win32Error = RegOpenKeyEx(
		TimeZoneKey,
		L"Dynamic DST",
		0,
		KEY_READ,
		&DynamicDSTKey);

	SafeClose(TimeZoneKey);

	if (Win32Error != ERROR_SUCCESS) {
		return Win32Error;
	}

	ValueSize = 4;

	Win32Error = RegQueryValueExW(
		DynamicDSTKey,
		L"FirstEntry",
		NULL,
		NULL,
		(PBYTE) FirstYear,
		&ValueSize);

	if (Win32Error == ERROR_SUCCESS) {
		ASSERT (ValueSize == sizeof(ULONG));

		Win32Error = RegQueryValueExW(
			DynamicDSTKey,
			L"LastEntry",
			NULL,
			NULL,
			(PBYTE) LastYear,
			&ValueSize);
	}

	SafeClose(DynamicDSTKey);
	return Win32Error;
}

// Required for some Unity games
// Full implementation based on decompiled Win10 code
KXBASEAPI ULONG WINAPI EnumDynamicTimeZoneInformation(
	IN	ULONG							Index,
	OUT	PDYNAMIC_TIME_ZONE_INFORMATION	TimeZoneInformation)
{
	ULONG Win32Error;
	HKEY TimeZonesKey;
	HKEY TimeZoneKey;
	WCHAR KeyName[ARRAYSIZE(TimeZoneInformation->TimeZoneKeyName)];
	ULONG KeyNameCch;
	REG_TZI_FORMAT Tzi;
	ULONG TziCb;
	DYNAMIC_TIME_ZONE_INFORMATION TemporaryTimeZoneInformation;

	TimeZonesKey = NULL;
	TimeZoneKey = NULL;

	if (TimeZoneInformation == NULL) {
		return ERROR_INVALID_PARAMETER;
	}

	//
	// open time zones key
	//

	Win32Error = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
		0,
		KEY_READ,
		&TimeZonesKey);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}

	//
	// get the time zone with the index which is specified by the caller
	//

	KeyNameCch = ARRAYSIZE(KeyName);

	Win32Error = RegEnumKeyEx(
		TimeZonesKey,
		Index,
		KeyName,
		&KeyNameCch,
		NULL,
		NULL,
		NULL,
		NULL);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}

	Win32Error = RegOpenKeyEx(
		TimeZonesKey,
		KeyName,
		0,
		KEY_READ,
		&TimeZoneKey);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}

	//
	// query values TZI (REG_BINARY, REG_TZI_FORMAT), MUI_Dlt, and MUI_Std
	//

	TziCb = sizeof(Tzi);

	Win32Error = RegQueryValueExW(
		TimeZoneKey,
		L"TZI",
		NULL,
		NULL,
		(PBYTE) &Tzi,
		&TziCb);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}
	
	RtlZeroMemory(&TemporaryTimeZoneInformation, sizeof(TemporaryTimeZoneInformation));
	KexRtlCopyMemory(&TemporaryTimeZoneInformation.TimeZoneKeyName, KeyName, sizeof(KeyName));
	
	TemporaryTimeZoneInformation.Bias			= Tzi.Bias;
	TemporaryTimeZoneInformation.StandardBias	= Tzi.StandardBias;
	TemporaryTimeZoneInformation.DaylightBias	= Tzi.DaylightBias;
	TemporaryTimeZoneInformation.StandardDate	= Tzi.StandardDate;
	TemporaryTimeZoneInformation.DaylightDate	= Tzi.DaylightDate;

	Win32Error = RegLoadMUIString(
		TimeZoneKey,
		L"MUI_Dlt",
		TemporaryTimeZoneInformation.DaylightName,
		sizeof(TemporaryTimeZoneInformation.DaylightName),
		NULL,
		0,
		NULL);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}

	Win32Error = RegLoadMUIString(
		TimeZoneKey,
		L"MUI_Std",
		TemporaryTimeZoneInformation.StandardName,
		sizeof(TemporaryTimeZoneInformation.StandardName),
		NULL,
		0,
		NULL);

	if (Win32Error != ERROR_SUCCESS) {
		goto Cleanup;
	}

	//
	// Once all registry calls have succeeded, we will copy
	// TemporaryTimeZoneInformation into the caller's structure.
	//

	*TimeZoneInformation = TemporaryTimeZoneInformation;

Cleanup:
	SafeClose(TimeZonesKey);
	SafeClose(TimeZoneKey);
	return Win32Error;
}