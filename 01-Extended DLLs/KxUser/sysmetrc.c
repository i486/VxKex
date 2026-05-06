#include "buildcfg.h"
#include "kxuserp.h"

STATIC INT ScaleForDpi(
	IN	INT		Value,
	IN	UINT	DesiredDpi)
{
	return MulDiv(
		Value,
		DesiredDpi,
		GetDpiForSystem());
}

STATIC VOID ScaleLogFontForDpi(
	IN OUT	PLOGFONT	LogFont,
	IN		UINT		DesiredDpi)
{
	LogFont->lfWidth = ScaleForDpi(LogFont->lfWidth, DesiredDpi);
	LogFont->lfHeight = ScaleForDpi(LogFont->lfHeight, DesiredDpi);
}

INT WINAPI GetSystemMetricsForDpi(
	IN	INT		Index,
	IN	UINT	Dpi)
{
	INT Value;

	if (Dpi > INT_MAX || Index > 0x60) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

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
	case SM_CXSIZE:
	case SM_CYSIZE:
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
	case SM_CXPADDEDBORDER:
		// These are pixel values that have to be scaled according to DPI.
		// The exact SM_* values that are scaled by this function are obtained
		// by decompiling Win10 user32.dll.
		Value = ScaleForDpi(Value, Dpi);
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
		{
			BOOL Success;
			PLOGFONT LogFont;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				LogFont = (PLOGFONT) Data;
				ScaleLogFontForDpi(LogFont, Dpi);
			}

			return Success;
		}
	case SPI_GETICONMETRICS:
		{
			BOOL Success;
			PICONMETRICS IconMetrics;

			Success = SystemParametersInfo(Action, Parameter, Data, 0);

			if (Success) {
				IconMetrics = (PICONMETRICS) Data;

				IconMetrics->iHorzSpacing = ScaleForDpi(IconMetrics->iHorzSpacing, Dpi);
				IconMetrics->iVertSpacing = ScaleForDpi(IconMetrics->iVertSpacing, Dpi);
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

				NonClientMetrics->iBorderWidth			= ScaleForDpi(NonClientMetrics->iBorderWidth, Dpi);
				NonClientMetrics->iScrollWidth			= ScaleForDpi(NonClientMetrics->iScrollWidth, Dpi);
				NonClientMetrics->iScrollHeight			= ScaleForDpi(NonClientMetrics->iScrollHeight, Dpi);
				NonClientMetrics->iCaptionWidth			= ScaleForDpi(NonClientMetrics->iCaptionWidth, Dpi);
				NonClientMetrics->iCaptionHeight		= ScaleForDpi(NonClientMetrics->iCaptionHeight, Dpi);
				NonClientMetrics->iSmCaptionWidth		= ScaleForDpi(NonClientMetrics->iSmCaptionWidth, Dpi);
				NonClientMetrics->iSmCaptionHeight		= ScaleForDpi(NonClientMetrics->iSmCaptionHeight, Dpi);
				NonClientMetrics->iMenuWidth			= ScaleForDpi(NonClientMetrics->iMenuWidth, Dpi);
				NonClientMetrics->iMenuHeight			= ScaleForDpi(NonClientMetrics->iMenuHeight, Dpi);
				NonClientMetrics->iPaddedBorderWidth	= ScaleForDpi(NonClientMetrics->iPaddedBorderWidth, Dpi);

				ScaleLogFontForDpi(&NonClientMetrics->lfCaptionFont, Dpi);
				ScaleLogFontForDpi(&NonClientMetrics->lfSmCaptionFont, Dpi);
				ScaleLogFontForDpi(&NonClientMetrics->lfMenuFont, Dpi);
				ScaleLogFontForDpi(&NonClientMetrics->lfStatusFont, Dpi);
				ScaleLogFontForDpi(&NonClientMetrics->lfMessageFont, Dpi);
			}

			return Success;
		}
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}
}