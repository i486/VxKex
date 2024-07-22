#include "buildcfg.h"
#include "kxuserp.h"

INT WINAPI GetSystemMetricsForDpi(
	IN	INT		Index,
	IN	UINT	Dpi)
{
	INT Value;

	Value = GetSystemMetrics(Index);

	switch (Index) {
	case SM_CXVSCROLL:
	case SM_CYHSCROLL:
	case SM_CYCAPTION:
	case SM_CYVTHUMB:
	case SM_CXHTHUMB:
	case SM_CXICON:
	case SM_CYICON:
	case SM_CXCURSOR:
	case SM_CYCURSOR:
	case SM_CYMENU:
	case SM_CYVSCROLL:
	case SM_CXHSCROLL:
	case SM_CXMIN:
	case SM_CXMINTRACK:
	case SM_CYMIN:
	case SM_CYMINTRACK:
	case SM_CXSIZE:
	case SM_CXFRAME:
	case SM_CYFRAME:
	case SM_CXICONSPACING:
	case SM_CYICONSPACING:
	case SM_CXSMICON:
	case SM_CYSMICON:
	case SM_CYSMCAPTION:
	case SM_CXSMSIZE:
	case SM_CYSMSIZE:
	case SM_CXMENUSIZE:
	case SM_CYMENUSIZE:
	case SM_CXMENUCHECK:
	case SM_CYMENUCHECK:
		// These are pixel values that have to be scaled according to DPI.
		Value *= Dpi;
		Value /= USER_DEFAULT_SCREEN_DPI;
		break;
	}

	return Value;
}

BOOL WINAPI SystemParametersInfoForDpi(
	IN		UINT	Action,
	IN		UINT	Parameter,
	IN OUT	PVOID	Data,
	IN		UINT	WinIni,
	IN		UINT	Dpi)
{
	switch (Action) {
	case SPI_GETICONTITLELOGFONT:
		return SystemParametersInfo(Action, Parameter, Data, 0);
	case SPI_GETICONMETRICS:
		{
			BOOL Success;
			PICONMETRICS IconMetrics;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				IconMetrics = (PICONMETRICS) Data;

				IconMetrics->iHorzSpacing *= Dpi;
				IconMetrics->iVertSpacing *= Dpi;
				IconMetrics->iHorzSpacing /= USER_DEFAULT_SCREEN_DPI;
				IconMetrics->iVertSpacing /= USER_DEFAULT_SCREEN_DPI;
			}

			return Success;
		}
	case SPI_GETNONCLIENTMETRICS:
		{
			BOOL Success;
			PNONCLIENTMETRICS NonClientMetrics;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				NonClientMetrics = (PNONCLIENTMETRICS) Data;

				NonClientMetrics->iBorderWidth			*= Dpi;
				NonClientMetrics->iScrollWidth			*= Dpi;
				NonClientMetrics->iScrollHeight			*= Dpi;
				NonClientMetrics->iCaptionWidth			*= Dpi;
				NonClientMetrics->iCaptionHeight		*= Dpi;
				NonClientMetrics->iSmCaptionWidth		*= Dpi;
				NonClientMetrics->iSmCaptionHeight		*= Dpi;
				NonClientMetrics->iMenuWidth			*= Dpi;
				NonClientMetrics->iMenuHeight			*= Dpi;
				NonClientMetrics->iPaddedBorderWidth	*= Dpi;

				NonClientMetrics->iBorderWidth			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iScrollWidth			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iScrollHeight			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iCaptionWidth			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iCaptionHeight		/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iSmCaptionWidth		/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iSmCaptionHeight		/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iMenuWidth			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iMenuHeight			/= USER_DEFAULT_SCREEN_DPI;
				NonClientMetrics->iPaddedBorderWidth	/= USER_DEFAULT_SCREEN_DPI;
			}

			return Success;
		}
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
}