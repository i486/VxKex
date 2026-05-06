#include "vxlview.h"
#include "resource.h"

INT_PTR CALLBACK AboutWndProc(
	IN	HWND	AboutWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		CenterWindow(AboutWindow, MainWindow);
	} else if (Message == WM_CLOSE || Message == WM_COMMAND) {
		EndDialog(AboutWindow, 0);
	} else if (Message == WM_NOTIFY) {
		LPNMHDR Notification = (LPNMHDR) LParam;

		if (Notification->idFrom == IDC_WEBSITELINK && Notification->code == NM_CLICK || Notification->code == NM_RETURN) {
			ShellExecute(AboutWindow, L"open", L"https://github.com/vxiiduu/VxKex", NULL, NULL, SW_SHOWNORMAL);
		}
	} else {
		return FALSE;
	}

	return TRUE;
}