#include <Windows.h>
#define DLLAPI __declspec(dllexport)
#define WINAPI __stdcall

typedef enum _MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI,
	MDT_ANGULAR_DPI,
	MDT_RAW_DPI,
	MDT_DEFAULT
} MONITOR_DPI_TYPE;

typedef enum _SHELL_UI_COMPONENT {
	SHELL_UI_COMPONENT_TASKBARS,
	SHELL_UI_COMPONENT_NOTIFICATIONAREA,
	SHELL_UI_COMPONENT_DESKBAND
} SHELL_UI_COMPONENT;

typedef enum _PROCESS_DPI_AWARENESS {
	PROCESS_DPI_UNAWARE,
	PROCESS_SYSTEM_DPI_AWARE,
	PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef enum _DEVICE_SCALE_FACTOR {
	DEVICE_SCALE_FACTOR_INVALID,
	SCALE_100_PERCENT,
	SCALE_120_PERCENT,
	SCALE_125_PERCENT,
	SCALE_140_PERCENT,
	SCALE_150_PERCENT,
	SCALE_160_PERCENT,
	SCALE_175_PERCENT,
	SCALE_180_PERCENT,
	SCALE_200_PERCENT,
	SCALE_225_PERCENT,
	SCALE_250_PERCENT,
	SCALE_300_PERCENT,
	SCALE_350_PERCENT,
	SCALE_400_PERCENT,
	SCALE_450_PERCENT,
	SCALE_500_PERCENT
} DEVICE_SCALE_FACTOR;

static UINT GetDpiForSystem(
	VOID)
{
	HDC hdcScreen = GetDC(NULL);
	UINT uDpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
	ReleaseDC(NULL, hdcScreen);
	return uDpi;
}

DLLAPI HRESULT WINAPI GetDpiForMonitor(
	IN	HMONITOR hmonitor,
	IN	MONITOR_DPI_TYPE dpiType,
	OUT	LPUINT dpiX,
	OUT	LPUINT dpiY)
{
	HDC hdcScreen = GetDC(NULL);
	*dpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
	*dpiY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
	ReleaseDC(NULL, hdcScreen);
	return S_OK;
}

DLLAPI UINT WINAPI GetDpiForShellUIComponent(
	IN	SHELL_UI_COMPONENT	component)
{
	return GetDpiForSystem();
}

DLLAPI HRESULT WINAPI SetProcessDpiAwareness(
	IN	PROCESS_DPI_AWARENESS	value)
{
	if (value != PROCESS_DPI_UNAWARE) {
		SetProcessDPIAware();
	}

	return S_OK;
}

DLLAPI HRESULT WINAPI GetProcessDpiAwareness(
	IN	HANDLE					hprocess,
	OUT	PROCESS_DPI_AWARENESS	*value)
{
	*value = PROCESS_DPI_UNAWARE;
	return S_OK;
}

DLLAPI HRESULT WINAPI GetScaleFactorForMonitor(
	IN	HMONITOR			hMon,
	OUT	DEVICE_SCALE_FACTOR	*pScale)
{
	*pScale = SCALE_100_PERCENT;
	return S_OK;
}

#pragma comment(linker, "/export:CommandLineToArgvW=shell32.CommandLineToArgvW")
#pragma comment(linker, "/export:GetCurrentProcessExplicitAppUserModelID=shell32.GetCurrentProcessExplicitAppUserModelID")
#pragma comment(linker, "/export:IStream_Copy=shlwapi.IStream_Copy")
#pragma comment(linker, "/export:IStream_Read=shlwapi.IStream_Read")
#pragma comment(linker, "/export:IStream_ReadStr=shlwapi.IStream_ReadStr")
#pragma comment(linker, "/export:IStream_Reset=shlwapi.IStream_Reset")
#pragma comment(linker, "/export:IStream_Size=shlwapi.IStream_Size")
#pragma comment(linker, "/export:IStream_Write=shlwapi.IStream_Write")
#pragma comment(linker, "/export:IStream_WriteStr=shlwapi.IStream_WriteStr")
#pragma comment(linker, "/export:IUnknown_AtomicRelease=shlwapi.IUnknown_AtomicRelease")
#pragma comment(linker, "/export:IUnknown_GetSite=shlwapi.IUnknown_GetSite")
#pragma comment(linker, "/export:IUnknown_QueryService=shlwapi.IUnknown_QueryService")
#pragma comment(linker, "/export:IUnknown_Set=shlwapi.IUnknown_Set")
#pragma comment(linker, "/export:IUnknown_SetSite=shlwapi.IUnknown_SetSite")
#pragma comment(linker, "/export:IsOS=shlwapi.IsOS")
#pragma comment(linker, "/export:SHAnsiToAnsi=shlwapi.SHAnsiToAnsi")
#pragma comment(linker, "/export:SHAnsiToUnicode=shlwapi.SHAnsiToUnicode")
#pragma comment(linker, "/export:SHCopyKeyA=shlwapi.SHCopyKeyA")
#pragma comment(linker, "/export:SHCopyKeyW=shlwapi.SHCopyKeyW")
#pragma comment(linker, "/export:SHCreateMemStream=shlwapi.SHCreateMemStream")
#pragma comment(linker, "/export:SHCreateStreamOnFileA=shlwapi.SHCreateStreamOnFileA")
#pragma comment(linker, "/export:SHCreateStreamOnFileEx=shlwapi.SHCreateStreamOnFileEx")
#pragma comment(linker, "/export:SHCreateStreamOnFileW=shlwapi.SHCreateStreamOnFileW")
#pragma comment(linker, "/export:SHCreateThread=shlwapi.SHCreateThread")
#pragma comment(linker, "/export:SHCreateThreadRef=shlwapi.SHCreateThreadRef")
#pragma comment(linker, "/export:SHCreateThreadWithHandle=shlwapi.SHCreateThreadWithHandle")
#pragma comment(linker, "/export:SHDeleteEmptyKeyA=shlwapi.SHDeleteEmptyKeyA")
#pragma comment(linker, "/export:SHDeleteEmptyKeyW=shlwapi.SHDeleteEmptyKeyW")
#pragma comment(linker, "/export:SHDeleteKeyA=shlwapi.SHDeleteKeyA")
#pragma comment(linker, "/export:SHDeleteKeyW=shlwapi.SHDeleteKeyW")
#pragma comment(linker, "/export:SHDeleteValueA=shlwapi.SHDeleteValueA")
#pragma comment(linker, "/export:SHDeleteValueW=shlwapi.SHDeleteValueW")
#pragma comment(linker, "/export:SHEnumKeyExA=shlwapi.SHEnumKeyExA")
#pragma comment(linker, "/export:SHEnumKeyExW=shlwapi.SHEnumKeyExW")
#pragma comment(linker, "/export:SHEnumValueA=shlwapi.SHEnumValueA")
#pragma comment(linker, "/export:SHEnumValueW=shlwapi.SHEnumValueW")
#pragma comment(linker, "/export:SHGetThreadRef=shlwapi.SHGetThreadRef")
#pragma comment(linker, "/export:SHGetValueA=shlwapi.SHGetValueA")
#pragma comment(linker, "/export:SHGetValueW=shlwapi.SHGetValueW")
#pragma comment(linker, "/export:SHOpenRegStream2A=shlwapi.SHOpenRegStream2A")
#pragma comment(linker, "/export:SHOpenRegStream2W=shlwapi.SHOpenRegStream2W")
#pragma comment(linker, "/export:SHOpenRegStreamA=shlwapi.SHOpenRegStreamA")
#pragma comment(linker, "/export:SHOpenRegStreamW=shlwapi.SHOpenRegStreamW")
#pragma comment(linker, "/export:SHQueryInfoKeyA=shlwapi.SHQueryInfoKeyA")
#pragma comment(linker, "/export:SHQueryInfoKeyW=shlwapi.SHQueryInfoKeyW")
#pragma comment(linker, "/export:SHQueryValueExA=shlwapi.SHQueryValueExA")
#pragma comment(linker, "/export:SHQueryValueExW=shlwapi.SHQueryValueExW")
#pragma comment(linker, "/export:SHRegDuplicateHKey=shlwapi.SHRegDuplicateHKey")
#pragma comment(linker, "/export:SHRegGetIntW=shlwapi.SHRegGetIntW")
#pragma comment(linker, "/export:SHRegGetPathA=shlwapi.SHRegGetPathA")
#pragma comment(linker, "/export:SHRegGetPathW=shlwapi.SHRegGetPathW")
#pragma comment(linker, "/export:SHRegGetValueA=shlwapi.SHRegGetValueA")
#pragma comment(linker, "/export:SHRegGetValueFromHKCUHKLM=shlwapi.SHRegGetValueFromHKCUHKLM")
#pragma comment(linker, "/export:SHRegGetValueW=shlwapi.SHRegGetValueW")
#pragma comment(linker, "/export:SHRegSetPathA=shlwapi.SHRegSetPathA")
#pragma comment(linker, "/export:SHRegSetPathW=shlwapi.SHRegSetPathW")
#pragma comment(linker, "/export:SHReleaseThreadRef=shlwapi.SHReleaseThreadRef")
#pragma comment(linker, "/export:SHSetThreadRef=shlwapi.SHSetThreadRef")
#pragma comment(linker, "/export:SHSetValueA=shlwapi.SHSetValueA")
#pragma comment(linker, "/export:SHSetValueW=shlwapi.SHSetValueW")
#pragma comment(linker, "/export:SHStrDupA=shlwapi.SHStrDupA")
#pragma comment(linker, "/export:SHStrDupW=shlwapi.SHStrDupW")
#pragma comment(linker, "/export:SHUnicodeToAnsi=shlwapi.SHUnicodeToAnsi")
#pragma comment(linker, "/export:SHUnicodeToUnicode=shlwapi.SHUnicodeToUnicode")
#pragma comment(linker, "/export:SetCurrentProcessExplicitAppUserModelID=shlwapi.SetCurrentProcessExplicitAppUserModelID")