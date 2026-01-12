#pragma once

#include <KexComm.h>
#include <KexGui.h>
#include <KxCfgHlp.h>

#include <ShObjIdl.h>

typedef struct {
	IShellExtInitVtbl		*ShellExtInitVtbl;
	IShellPropSheetExtVtbl	*ShellPropSheetExtVtbl;

	LONG					RefCount;
	WCHAR					ExeFullPath[MAX_PATH];
} IKexShlEx;

typedef struct {
	PCWSTR						ExeFullPath;
	BOOLEAN						SettingsChanged;
} TYPEDEF_TYPE_NAME(KEXSHLEX_PROPSHEET_DATA);

//
// lnkfile.c
//

HRESULT GetTargetFromLnkfile(
	IN OUT	PWSTR	FileName);

//
// ckxshlex.c
//

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface(
	IN	IKexShlEx		*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut);

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface_thunk0(
	IN	IShellExtInit	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut);

HRESULT STDMETHODCALLTYPE CKexShlEx_QueryInterface_thunk1(
	IN	IShellPropSheetExt	*This,
	IN	REFIID				RefIID,
	OUT	PPVOID				ObjectOut);

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef(
	IN	IKexShlEx	*This);

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef_thunk0(
	IN	IShellExtInit	*This);

ULONG STDMETHODCALLTYPE CKexShlEx_AddRef_thunk1(
	IN	IShellPropSheetExt	*This);

ULONG STDMETHODCALLTYPE CKexShlEx_Release(
	IN	IKexShlEx		*This);

ULONG STDMETHODCALLTYPE CKexShlEx_Release_thunk0(
	IN	IShellExtInit	*This);

ULONG STDMETHODCALLTYPE CKexShlEx_Release_thunk1(
	IN	IShellPropSheetExt	*This);

HRESULT STDMETHODCALLTYPE CKexShlEx_Initialize(
	IN	IKexShlEx		*This,
	IN	LPCITEMIDLIST	FolderIdList OPTIONAL,
	IN	LPDATAOBJECT	DataObject,
	IN	HKEY			ProgIdKey);

HRESULT STDMETHODCALLTYPE CKexShlEx_AddPages(
	IN	IKexShlEx				*This,
	IN	LPFNADDPROPSHEETPAGE	AddPage,
	IN	LPARAM					LParam);

HRESULT STDMETHODCALLTYPE CKexShlEx_ReplacePage(
	IN	IKexShlEx				*This,
	IN	UINT					PageId,
	IN	LPFNADDPROPSHEETPAGE	ReplacePage,
	IN	LPARAM					LParam);

//
// clsfctry.c
//

HRESULT STDMETHODCALLTYPE CClassFactory_QueryInterface(
	IN	IClassFactory	*This,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut);

ULONG STDMETHODCALLTYPE CClassFactory_AddRef(
	IN	IClassFactory	*This);

ULONG STDMETHODCALLTYPE CClassFactory_Release(
	IN	IClassFactory	*This);

HRESULT STDMETHODCALLTYPE CClassFactory_CreateInstance(
	IN	IClassFactory	*This,
	IN	IUnknown		*OuterIUnknown OPTIONAL,
	IN	REFIID			RefIID,
	OUT	PPVOID			ObjectOut);

HRESULT STDMETHODCALLTYPE CClassFactory_LockServer(
	IN	IClassFactory	*This,
	IN	BOOL			Lock);

//
// dllmain.c
//

EXTERN HMODULE DllHandle;
EXTERN LONG DllReferenceCount;
EXTERN CONST IKexShlEx IKexShlExTemplate;

//
// gui.c
//

INT_PTR CALLBACK DialogProc(
	IN	HWND	Window,
	IN	UINT	Message,
	IN	WPARAM	WParam,
	IN	LPARAM	LParam);

UINT WINAPI PropSheetCallbackProc(
	IN		HWND				Window,
	IN		UINT				Message,
	IN OUT	LPPROPSHEETPAGE		PropSheetPage);