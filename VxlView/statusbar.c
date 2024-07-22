#include "vxlview.h"
#include "resource.h"

VOID ResizeStatusBar(
	IN	ULONG	MainWindowNewWidth)
{
	INT StatusBarPartWidths[2];

	// tell the status bar window to correctly size itself
	SendMessage(StatusBarWindow, WM_SIZE, 0, 0);

	// resize statusbar parts
	StatusBarPartWidths[0] = (MainWindowNewWidth * 70) / 100;	// 70%
	StatusBarPartWidths[1] = -1;
	SendMessage(StatusBarWindow, SB_SETPARTS, ARRAYSIZE(StatusBarPartWidths), (LPARAM) StatusBarPartWidths);
}