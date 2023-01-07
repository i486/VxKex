///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     srvsend.c
//
// Abstract:
//
//     This program helps developers send a message to KexSrv.
//
// Author:
//
//     vxiiduu (14-Oct-2022)
//
// Environment:
//
//     win32 with kexsrv running
//
// Revision History:
//
//     vxiiduu               14-Oct-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include <KexComm.h>
#include <KexSrv.h>
#include <KexDll.h>
#include "resource.h"

HANDLE ChannelHandle = NULL;

PCWSTR MessageIdToTextLookup[] = {
	L"KexIpcKexProcessStart",
	L"KexIpcHardError",
	L"KexIpcLogEvent",
};

VOID AddMessageIdsToComboBox(
	IN	HWND	MessageIdComboBoxWindow)
{
	ULONG Index;

	ASSERT (MessageIdComboBoxWindow != NULL);
	ASSERT (ARRAYSIZE(MessageIdToTextLookup) == KexIpcMaximumMessageId);

	for (Index = 0; Index < KexIpcMaximumMessageId; ++Index) {
		ComboBox_AddString(MessageIdComboBoxWindow, MessageIdToTextLookup[Index]);
	}

	ComboBox_SetCueBannerText(MessageIdComboBoxWindow, L"Select message ID...");
}

VOID AddSeveritiesToComboBox(
	IN	HWND	SeverityComboBoxWindow)
{
	ULONG Index;

	for (Index = 0; Index < LogSeverityMaximumValue; ++Index) {
		ComboBox_AddString(SeverityComboBoxWindow, VxlSeverityToText((VXLSEVERITY) Index, FALSE));
	}

	ComboBox_SetCueBannerText(SeverityComboBoxWindow, L"Select log severity...");
}

VOID BanishAllStructureSpecificWindows(
	IN	HWND	MainWindow)
{
	ULONG Index;

	for (Index = IDC_APPNAME_DESC; Index <= IDC_LOGTEXT; ++Index) {
		BanishWindow(GetDlgItem(MainWindow, Index));
	}
}

VOID SetScene(
	IN	HWND	MainWindow,
	IN	ULONG	MessageId)
{
	USHORT VisibleStartId;
	USHORT VisibleEndId;
	ULONG Index;

	ASSERT (MessageId < KexIpcMaximumMessageId);
	ASSUME (MessageId < KexIpcMaximumMessageId);

	BanishAllStructureSpecificWindows(MainWindow);

	switch (MessageId) {
	case KexIpcKexProcessStart:
		VisibleStartId = IDC_APPNAME_DESC;
		VisibleEndId = IDC_APPNAME;
		break;
	case KexIpcHardError:
		VisibleStartId = IDC_NTSTATUS_DESC;
		VisibleEndId = IDC_STRPARAM2;
		break;
	case KexIpcLogEvent:
		VisibleStartId = IDC_SEVERITY_DESC;
		VisibleEndId = IDC_LOGTEXT;
		break;
	}

	for (Index = VisibleStartId; Index <= VisibleEndId; ++Index) {
		SummonWindow(GetDlgItem(MainWindow, Index));
	}
}

BOOLEAN ConnectToKexSrv(
	VOID)
{
	NTSTATUS Status;
	UNICODE_STRING ChannelName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;

	ASSERT (ChannelHandle == NULL);

	RtlInitConstantUnicodeString(&ChannelName, KEXSRV_IPC_CHANNEL_NAME);
	InitializeObjectAttributes(&ObjectAttributes, &ChannelName, 0, NULL, NULL);

	Status = NtOpenFile(
		&ChannelHandle,
		GENERIC_WRITE | SYNCHRONIZE,
		&ObjectAttributes,
		&IoStatusBlock,
		FILE_SHARE_WRITE,
		FILE_SYNCHRONOUS_IO_NONALERT);

	if (!NT_SUCCESS(Status)) {
		ChannelHandle = NULL;
		ErrorBoxF(L"Failed to connect to KexSrv (NTSTATUS 0x%08lx): %s", Status, NtStatusAsString(Status));
		return FALSE;
	}

	return TRUE;
}

VOID DisconnectFromKexSrv(
	VOID)
{
	ASSERT (ChannelHandle != NULL);
	ASSERT (ChannelHandle != INVALID_HANDLE_VALUE);

	NtClose(ChannelHandle);
	ChannelHandle = NULL;
}

VOID SrvSendDispatchMessage(
	IN	HWND	MainWindow)
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatusBlock;
	PKEX_IPC_MESSAGE Message;

	Message = (PKEX_IPC_MESSAGE) StackAlloc(BYTE, 0xFFFF);
	ZeroMemory(Message, sizeof(KEX_IPC_MESSAGE));

	Message->MessageId = (KEX_IPC_MESSAGE_ID) ComboBox_GetCurSel(GetDlgItem(MainWindow, IDC_MESSAGEID));

	if (Message->MessageId < 0 || Message->MessageId >= KexIpcMaximumMessageId) {
		ErrorBoxF(L"You must select a message ID.");
		return;
	}

	Message->AuxiliaryDataBlockSize = GetDlgItemInt(MainWindow, IDC_AUXDATASIZE, NULL, TRUE);

	switch (Message->MessageId) {
	case KexIpcKexProcessStart:
		{
			HWND AppNameWindow;
		
			AppNameWindow = GetDlgItem(MainWindow, IDC_APPNAME);
			Message->ProcessStartedInformation.ApplicationNameLength = GetWindowTextLength(AppNameWindow);

			ASSERT (sizeof(KEX_IPC_MESSAGE) + 
					(Message->ProcessStartedInformation.ApplicationNameLength * sizeof(WCHAR))
					<= 0xFFFF);
		
			GetWindowText(
				AppNameWindow,
				(PWSTR) Message->AuxiliaryDataBlock,
				0xFFFF - sizeof(KEX_IPC_MESSAGE));
		}

		break;
	case KexIpcHardError:
		{
			HWND StringParameter1Window;
			HWND StringParameter2Window;
			WCHAR HexStringBuffer[11];
			WCHAR HexStringBuffer2[11];
			UNICODE_STRING NtStatusHexString;
			UNICODE_STRING UlongHexString;

			StringParameter1Window = GetDlgItem(MainWindow, IDC_STRPARAM1);
			StringParameter2Window = GetDlgItem(MainWindow, IDC_STRPARAM2);

			GetDlgItemText(MainWindow, IDC_NTSTATUS, HexStringBuffer, ARRAYSIZE(HexStringBuffer));
			RtlInitUnicodeString(&NtStatusHexString, HexStringBuffer);
			RtlUnicodeStringToInteger(&NtStatusHexString, 0, &Message->HardErrorInformation.Status);

			GetDlgItemText(MainWindow, IDC_ULONGPARAM, HexStringBuffer2, ARRAYSIZE(HexStringBuffer2));
			RtlInitUnicodeString(&UlongHexString, HexStringBuffer2);
			RtlUnicodeStringToInteger(&UlongHexString, 0, &Message->HardErrorInformation.UlongParameter);

			Message->HardErrorInformation.StringParameter1Length = GetWindowTextLength(StringParameter1Window);
			Message->HardErrorInformation.StringParameter2Length = GetWindowTextLength(StringParameter2Window);

			ASSERT (sizeof(KEX_IPC_MESSAGE) +
					((Message->HardErrorInformation.StringParameter1Length +
					  Message->HardErrorInformation.StringParameter2Length) * sizeof(WCHAR))
					<= 0xFFFF);

			GetWindowText(
				StringParameter1Window,
				(PWSTR) Message->AuxiliaryDataBlock,
				0xFFFF - sizeof(KEX_IPC_MESSAGE));

			GetWindowText(
				StringParameter2Window,
				((PWSTR) Message->AuxiliaryDataBlock) + Message->HardErrorInformation.StringParameter1Length,
				0xFFFF - sizeof(KEX_IPC_MESSAGE) - Message->HardErrorInformation.StringParameter1Length);
		}

		break;
	case KexIpcLogEvent:
		{
			HWND SourceComponentWindow;
			HWND SourceFileWindow;
			HWND SourceFunctionWindow;
			HWND TextWindow;

			PNZWCH SourceComponent;
			PNZWCH SourceFile;
			PNZWCH SourceFunction;
			PNZWCH Text;

			SourceComponentWindow = GetDlgItem(MainWindow, IDC_SOURCECOMPONENT);
			SourceFileWindow = GetDlgItem(MainWindow, IDC_SOURCEFILE);
			SourceFunctionWindow = GetDlgItem(MainWindow, IDC_SOURCEFUNCTION);
			TextWindow = GetDlgItem(MainWindow, IDC_LOGTEXT);

			Message->LogEventInformation.Severity = ComboBox_GetCurSel(GetDlgItem(MainWindow, IDC_SEVERITY));
			Message->LogEventInformation.SourceLine = GetDlgItemInt(MainWindow, IDC_SOURCELINE, NULL, TRUE);
			Message->LogEventInformation.SourceComponentLength = GetWindowTextLength(SourceComponentWindow);
			Message->LogEventInformation.SourceFileLength = GetWindowTextLength(SourceFileWindow);
			Message->LogEventInformation.SourceFunctionLength = GetWindowTextLength(SourceFunctionWindow);
			Message->LogEventInformation.TextLength = GetWindowTextLength(TextWindow);

			ASSERT (sizeof(KEX_IPC_MESSAGE) + 
				    ((Message->LogEventInformation.SourceComponentLength +
					  Message->LogEventInformation.SourceFileLength +
					  Message->LogEventInformation.SourceFunctionLength +
					  Message->LogEventInformation.TextLength) * sizeof(WCHAR))
					<= 0xFFFF);

			SourceComponent = (PNZWCH) Message->AuxiliaryDataBlock;
			SourceFile = SourceComponent + Message->LogEventInformation.SourceComponentLength;
			SourceFunction = SourceFile + Message->LogEventInformation.SourceFileLength;
			Text = SourceFunction + Message->LogEventInformation.SourceFunctionLength;

			GetWindowText(
				SourceComponentWindow,
				SourceComponent,
				Message->LogEventInformation.SourceComponentLength + 1);

			GetWindowText(
				SourceFileWindow,
				SourceFile,
				Message->LogEventInformation.SourceFileLength + 1);

			GetWindowText(
				SourceFunctionWindow,
				SourceFunction,
				Message->LogEventInformation.SourceFunctionLength + 1);

			GetWindowText(
				TextWindow,
				Text,
				Message->LogEventInformation.TextLength + 1);
		}

		break;
	default:
		NOT_REACHED;
	}

	Status = NtWriteFile(
		ChannelHandle,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		Message,
		sizeof(KEX_IPC_MESSAGE) + Message->AuxiliaryDataBlockSize,
		NULL,
		NULL);
	ASSERT (NT_SUCCESS(Status));
}

INT_PTR CALLBACK DlgProc(
	IN	HWND	MainWindow,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam)
{
	if (Message == WM_INITDIALOG) {
		KexgApplicationMainWindow = MainWindow;
		AddMessageIdsToComboBox(GetDlgItem(MainWindow, IDC_MESSAGEID));
		AddSeveritiesToComboBox(GetDlgItem(MainWindow, IDC_SEVERITY));
		BanishAllStructureSpecificWindows(MainWindow);
		SetWindowTextF(MainWindow, L"Send Message to KexSrv (PID: %Iu)", NtCurrentTeb()->ClientId.UniqueProcess);
	} else if (Message == WM_CLOSE) {
		EndDialog(MainWindow, 0);
	} else if (Message == WM_COMMAND) {
		HWND Window;
		USHORT ControlId;
		USHORT NotificationCode;

		Window = (HWND) LParam;
		ControlId = LOWORD(WParam);
		NotificationCode = HIWORD(WParam);

		if (ControlId == IDC_MESSAGEID && NotificationCode == CBN_SELCHANGE) {
			STATIC INT LastMessageId = KexIpcMaximumMessageId; // this mechanism reduces flickering
			INT MessageId;

			MessageId = ComboBox_GetCurSel(Window);

			if (MessageId == CB_ERR) {
				return FALSE;
			}

			if (MessageId != LastMessageId) {
				SetScene(MainWindow, MessageId);
				LastMessageId = MessageId;
				return TRUE;
			}
		} else if (ControlId == IDC_BTNCONNECT) {
			if (ChannelHandle == NULL) {
				if (ConnectToKexSrv() == TRUE) {
					SetDlgItemText(MainWindow, IDC_BTNCONNECT, L"Dis&connect");
					SetDlgItemText(MainWindow, IDC_STATUSBAR, L"Successfully connected to KexSrv.");
				}
			} else {
				DisconnectFromKexSrv();
				SetDlgItemText(MainWindow, IDC_BTNCONNECT, L"&Connect");
				SetDlgItemText(MainWindow, IDC_STATUSBAR, L"Not connected to KexSrv.");
			}
		} else if (ControlId == IDC_BTNSEND) {
			if (ChannelHandle == NULL) {
				InfoBoxF(L"You need to be connected to the server to send a message.");
				return TRUE;
			}

			SrvSendDispatchMessage(MainWindow);
		} else if (ControlId == IDCANCEL) {
			PostMessage(MainWindow, WM_CLOSE, 0, 0);
		} else {
			return FALSE;
		}
	} else {
		return FALSE;
	}
	
	return TRUE;
}

VOID EntryPoint(
	VOID)
{
	InitCommonControls();
	KexgApplicationFriendlyName = L"KexSrv Test Utility";
	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	ExitProcess(0);
}