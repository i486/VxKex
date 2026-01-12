#pragma once

#include <KexComm.h>
#include <ShlDisp.h>

#ifndef KXCOMAPI
#  define KXCOMAPI
#else
#  ifdef KXCOM_WANT_INITGUID
#    include <InitGuid.h>
#  endif
#endif

#define E_STRING_NOT_NULL_TERMINATED	((HRESULT) 0x80000017L)
#define E_BOUNDS						((HRESULT) 0x8000000BL)
#define HRESULT_ARITHMETIC_OVERFLOW		((HRESULT) 0x80070216L)

#define WRHF_STRING_BUFFER_MAGIC						0xF8B1A8BE

#define WRHF_NONE										0x0
#define WRHF_STRING_REFERENCE							0x1
#define WRHF_VALID_UNICODE_FORMAT_INFO					0x2
#define WRHF_WELL_FORMED_UNICODE						0x4
#define WRHF_HAS_EMBEDDED_NULLS							0x8
#define WRHF_EMBEDDED_NULLS_COMPUTED					0x10
#define WRHF_RESERVED_FOR_PREALLOCATED_STRING_BUFFER	0x80000000

typedef struct _HSTRING_HEADER {
	ULONG	Flags;		// WRHF_*
	ULONG	Length;		// Does not include null terminator
	ULONG	Padding1;
	ULONG	Padding2;
	PCWSTR	StringRef;
} HSTRING_HEADER, *HSTRING;

typedef struct _HSTRING_ALLOCATED {
	HSTRING_HEADER	Header;
	VOLATILE LONG	RefCount;
	WCHAR			Data[1];
} HSTRING_ALLOCATED;

typedef enum _RO_INIT_TYPE {
	RO_INIT_SINGLETHREADED,
	RO_INIT_MULTITHREADED
} TYPEDEF_TYPE_NAME(RO_INIT_TYPE);

typedef HSTRING_ALLOCATED *HSTRING_BUFFER, **PHSTRING_BUFFER;

typedef HANDLE TYPEDEF_TYPE_NAME(CO_MTA_USAGE_COOKIE);

typedef enum {
	OLETLS_LOCALTID				= 0x01,		// This TID is in the current process.
	OLETLS_UUIDINITIALIZED		= 0x02,		// This Logical thread is init'd.
	OLETLS_INTHREADDETACH		= 0x04,		// This is in thread detach. Needed
											// due to NT's special thread detach
											// rules.
	OLETLS_CHANNELTHREADINITIALZED = 0x08,	// This channel has been init'd
	OLETLS_WOWTHREAD			= 0x10,		// This thread is a 16-bit WOW thread.
	OLETLS_THREADUNINITIALIZING	= 0x20,		// This thread is in CoUninitialize.
	OLETLS_DISABLE_OLE1DDE		= 0x40,		// This thread can't use a DDE window.
	OLETLS_APARTMENTTHREADED	= 0x80,		// This is an STA apartment thread
	OLETLS_MULTITHREADED		= 0x100,	// This is an MTA apartment thread
	OLETLS_IMPERSONATING		= 0x200,	// This thread is impersonating
	OLETLS_DISABLE_EVENTLOGGER	= 0x400,	// Prevent recursion in event logger
	OLETLS_INNEUTRALAPT			= 0x800,	// This thread is in the NTA
	OLETLS_DISPATCHTHREAD		= 0x1000,	// This is a dispatch thread
	OLETLS_HOSTTHREAD			= 0x2000,	// This is a host thread
	OLETLS_ALLOWCOINIT			= 0x4000,	// This thread allows inits
	OLETLS_PENDINGUNINIT		= 0x8000,	// This thread has pending uninit
	OLETLS_FIRSTMTAINIT			= 0x10000,	// First thread to attempt an MTA init
	OLETLS_FIRSTNTAINIT			= 0x20000,	// First thread to attempt an NTA init
	OLETLS_APTINITIALIZING		= 0x40000	// Apartment Object is initializing
} OLE_TLS_FLAGS;

typedef struct {
	HWND				Window;
	ULONG				FirstMessage;
	ULONG				LastMessage;
} SWindowData;

typedef struct {
	IMessageFilter		*MessageFilter;
	BOOL				InMessageFilter;
	PVOID				TopCML;			// original datatype: CCliModalLoop
	SWindowData			WindowData[2];
} CAptCallCtrl;

// NtCurrentTeb()->ReservedForOle points to one of these structures.
// See tagSOleTlsData in oletls.h (nt5src) for descriptions of values.
// The full definition of this structure can be found in the public symbol
// files for ole32.dll (this one is incomplete, I've replaced most of the
// pointers with PVOIDs and there is stuff missing from the end)
typedef struct {
	PVOID pvThreadBase;
	PVOID pSmAllocator;
	ULONG dwApartmentID;
	ULONG dwFlags; // OLETLS_*
	LONG TlsMapIndex;
	PVOID *ppTlsSlot;
	ULONG cComInits;
	ULONG cOleInits;
	ULONG cCalls;
	PVOID pCallInfo;
	PVOID pFreeAsyncCall;
	PVOID pFreeClientCall;
	PVOID pObjServer;
	ULONG dwTIDCaller;
	PVOID pCurrentCtx;
	PVOID pEmptyCtx;
	PVOID pNativeCtx;
	ULONGLONG ContextId; // This is 64-bit even on the 32-bit version of ole32.dll
	PVOID pNativeApt;
	IUnknown *pCallContext;
	PVOID pCtxCall;
	PVOID pPS;
	PVOID pvPendingCallsFront;
	PVOID pvPendingCallsBack;
	CAptCallCtrl *pCallCtrl; // Initialized by CAptCallCtrl::CAptCallCtrl in ole32.dll
	PVOID pTopSCS;
	IMessageFilter *pMsgFilter;
	HWND hwndSTA;
	LONG cORPCNestingLevel;
	ULONG cDebugData;
	GUID LogicalThreadId;
	PVOID hThread;
	PVOID hRevert;
	IUnknown *pAsyncRelease;
	HWND hwndDdeServer;
	HWND hwndDdeClient;
	ULONG cServeDdeObjects;
	PVOID pSTALSvrsFront;
	HWND hwndClip;
	IDataObject *pDataObjClip;
	ULONG dwClipSeqNum;
	ULONG fIsClipWrapper;
	IUnknown *punkState;
	ULONG cCallCancellation;
	ULONG cAsyncSends;
	PVOID pAsyncCallList;
	PVOID pSurrogateList;
	PVOID pRWLockTlsEntry;

	// Too lazy to fix up all the structs for this. It's not needed anyway.
	//tagCallEntry CallEntry;
	//tagContextStackNode *pContextStack;
	//tagInitializeSpyNode *pFirstSpyReg;
	//tagInitializeSpyNode *pFirstFreeSpyReg;
	//CVerifierTlsData *pVerifierData;
	//unsigned int dwMaxSpy;
	//unsigned __int8 cCustomMarshallerRecursion;
	//void *pDragCursors;
	//IUnknown *punkError;
	//unsigned int cbErrorData;
	//int cTraceNestingLevel;
	//tagOutgoingCallData outgoingCallData;
	//tagIncomingCallData incomingCallData;
	//tagOutgoingActivationData outgoingActivationData;
	//unsigned int cReentrancyFromUserAPC;
	//_LIST_ENTRY listConditionVariableResponsibleForWaking;
} SOleTlsData;

typedef enum _TrustLevel {
	BaseTrust,
	PartialTrust,
	FullTrust
} TrustLevel;

typedef enum _DayOfWeek {
    DayOfWeek_Sunday	= 0,
    DayOfWeek_Monday	= 1,
    DayOfWeek_Tuesday	= 2,
    DayOfWeek_Wednesday	= 3,
    DayOfWeek_Thursday	= 4,
    DayOfWeek_Friday	= 5,
    DayOfWeek_Saturday	= 6,
} DayOfWeek;

// {82BA7092-4C88-427D-A7BC-16DD93FEB67E}
DEFINE_GUID(IID_IRestrictedErrorInfo, 0x82BA7092, 0x4C88, 0x427D, 0xA7, 0xBC, 0x16, 0xDD, 0x93, 0xFE, 0xB6, 0x7E);

// {66818B96-DC17-4C12-8CA1-8E1FBAA5BF80}
DEFINE_GUID(IID_IInternalErrorInfo, 0x66818B96, 0xDC17, 0x4C12, 0x8C, 0xA1, 0x8E, 0x1F, 0xBA, 0xA5, 0xBF, 0x80);

// {94EA2B94-E9CC-49E0-C0FF-EE64CA8F5B90}
DEFINE_GUID(IID_IAgileObject, 0x94EA2B94, 0xE9CC, 0x49E0, 0xC0, 0xFF, 0xEE, 0x64, 0xCA, 0x8F, 0x5B, 0x90);

// {00000035-0000-0000-C000-000000000046}
DEFINE_GUID(IID_IActivationFactory, 0x00000035, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46);

// {AF86E2E0-B12D-4C6A-9C5A-D7AA65101E90}
DEFINE_GUID(IID_IInspectable, 0xAF86E2E0, 0xB12D, 0x4C6A, 0x9C, 0x5A, 0xD7, 0xAA, 0x65, 0x10, 0x1E, 0x90);

// {BBE1FA4C-B0E3-4583-BAEF-1F1B2E483E56}
DEFINE_GUID(IID_IVectorView, 0xBBE1FA4C, 0xB0E3, 0x4583, 0xBA, 0xEF, 0x1F, 0x1B, 0x2E, 0x48, 0x3E, 0x56);

// {01BF4326-ED37-4E96-B0E9-C1340D1EA158}
DEFINE_GUID(IID_IGlobalizationPreferencesStatics, 0x01BF4326, 0xED37, 0x4E96, 0xB0, 0xE9, 0xC1, 0x34, 0x0D, 0x1E, 0xA1, 0x58);

// {3694DBF9-8F68-44BE-8FF5-195C98EDE8A6}
DEFINE_GUID(IID_IUIViewSettingsInterop, 0x3694DBF9, 0x8F68, 0x44BE, 0x8F, 0xF5, 0x19, 0x5C, 0x98, 0xED, 0xE8, 0xA6);

// {C63657F6-8850-470D-88F8-455E16EA2C26}
DEFINE_GUID(IID_IUIViewSettings, 0xC63657F6, 0x8850, 0x470D, 0x88, 0xF8, 0x45, 0x5E, 0x16, 0xEA, 0x2C, 0x26);

// {85361600-1C63-4627-BCB1-3A89E0BC9C55}
DEFINE_GUID(IID_IUISettings, 0x85361600, 0x1C63, 0x4627, 0xBC, 0xB1, 0x3A, 0x89, 0xE0, 0xBC, 0x9C, 0x55);

// {BAD82401-2721-44F9-BB91-2BB228BE442F}
DEFINE_GUID(IID_IUISettings2, 0xBAD82401, 0x2721, 0x44F9, 0xBB, 0x91, 0x2B, 0xB2, 0x28, 0xBE, 0x44, 0x2F);

// {03021BE4-5254-4781-8194-5168F7D06D7B}
DEFINE_GUID(IID_IUISettings3, 0x03021BE4, 0x5254, 0x4781, 0x81, 0x94, 0x51, 0x68, 0xF7, 0xD0, 0x6D, 0x7B);

// {52BB3002-919B-4D6B-9B78-8DD66FF4B93B}
DEFINE_GUID(IID_IUISettings4, 0x52BB3002, 0x919B, 0x4D6B, 0x9B, 0x78, 0x8D, 0xD6, 0x6F, 0xF4, 0xB9, 0x3B);

// {5349D588-0CB5-5F05-BD34-706B3231F0BD}
DEFINE_GUID(IID_IUISettings5, 0x5349D588, 0x0CB5, 0x5F05, 0xBD, 0x34, 0x70, 0x6B, 0x32, 0x31, 0xF0, 0xBD);

// {AEF19BD7-FE31-5A04-ADA4-469AAEC6DFA9}
DEFINE_GUID(IID_IUISettings6, 0xAEF19BD7, 0xFE31, 0x5A04, 0xAD, 0xA4, 0x46, 0x9A, 0xAE, 0xC6, 0xDF, 0xA9);

// {44A9796F-723E-4FDF-A218-033E75B0C084}
DEFINE_GUID(IID_IUriRuntimeClassFactory, 0x44A9796F, 0x723E, 0x4FDF, 0xA2, 0x18, 0x03, 0x3E, 0x75, 0xB0, 0xC0, 0x84);

// {9E365E57-48B2-4160-956F-C7385120BBFC}
DEFINE_GUID(IID_IUriRuntimeClass, 0x9E365E57, 0x48B2, 0x4160, 0x95, 0x6F, 0xC7, 0x38, 0x51, 0x20, 0xBB, 0xFC);

// {277151C3-9E3E-42F6-91A4-5DFDEB232451}
DEFINE_GUID(IID_ILauncherStatics, 0x277151C3, 0x9E3E, 0x42F6, 0x91, 0xA4, 0x5D, 0xFD, 0xEB, 0x23, 0x24, 0x51);

typedef struct _IInspectable IInspectable;

typedef struct _IInspectableVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IInspectable *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IInspectable *);
	ULONG (STDMETHODCALLTYPE *Release)(IInspectable *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IInspectable *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IInspectable *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IInspectable *, TrustLevel *);
} IInspectableVtbl;

typedef struct _IInspectable {
	IInspectableVtbl *lpVtbl;
} IInspectable;

typedef struct _IActivationFactory IActivationFactory;

typedef struct _IActivationFactoryVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IActivationFactory *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IActivationFactory *);
	ULONG (STDMETHODCALLTYPE *Release)(IActivationFactory *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IActivationFactory *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IActivationFactory *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IActivationFactory *, TrustLevel *);

	// IActivationFactory
	HRESULT (STDMETHODCALLTYPE *ActivateInstance)(IActivationFactory *, IInspectable **);
} IActivationFactoryVtbl;

typedef struct _IActivationFactory {
	IActivationFactoryVtbl *lpVtbl;
} IActivationFactory;

extern IActivationFactory CActivationFactory;

typedef struct _IVectorView_HSTRING IVectorView_HSTRING;

typedef struct _IVectorView_HSTRINGVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IVectorView_HSTRING *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IVectorView_HSTRING *);
	ULONG (STDMETHODCALLTYPE *Release)(IVectorView_HSTRING *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IVectorView_HSTRING *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IVectorView_HSTRING *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IVectorView_HSTRING *, TrustLevel *);

	// IVectorView_HSTRING
	HRESULT (STDMETHODCALLTYPE *GetAt)(IVectorView_HSTRING *, ULONG, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Size)(IVectorView_HSTRING *, PULONG);
	HRESULT (STDMETHODCALLTYPE *IndexOf)(IVectorView_HSTRING *, HSTRING, PULONG, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *GetMany)(IVectorView_HSTRING *, ULONG, ULONG, HSTRING *, PULONG);
} IVectorView_HSTRINGVtbl;

typedef struct _IVectorView_HSTRING {
	IVectorView_HSTRINGVtbl	*lpVtbl;
	LONG					RefCount;
	ULONG					NumberOfHstrings;
	HSTRING					*HstringArray;
} IVectorView_HSTRING;

//
// WINRT: Windows.System.UserProfile.GlobalizationPreferences
//

typedef struct _IGlobalizationPreferencesStatics IGlobalizationPreferencesStatics;

typedef struct _IGlobalizationPreferencesStaticsVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IGlobalizationPreferencesStatics *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IGlobalizationPreferencesStatics *);
	ULONG (STDMETHODCALLTYPE *Release)(IGlobalizationPreferencesStatics *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IGlobalizationPreferencesStatics *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IGlobalizationPreferencesStatics *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IGlobalizationPreferencesStatics *, TrustLevel *);

	// IGlobalizationPreferencesStatics
	HRESULT (STDMETHODCALLTYPE *get_Calendars)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Clocks)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Currencies)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Languages)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_HomeGeographicRegion)(IGlobalizationPreferencesStatics *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_WeekStartsOn)(IGlobalizationPreferencesStatics *, DayOfWeek *);
} IGlobalizationPreferencesStaticsVtbl;

typedef struct _IGlobalizationPreferencesStatics {
	IGlobalizationPreferencesStaticsVtbl	*lpVtbl;
} IGlobalizationPreferencesStatics;

extern IGlobalizationPreferencesStatics CGlobalizationPreferencesStatics;

//
// WINRT: Windows.UI.ViewManagement.UIViewSettings
//

typedef struct _IUIViewSettingsInterop IUIViewSettingsInterop;

typedef struct _IUIViewSettingsInteropVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUIViewSettingsInterop *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUIViewSettingsInterop *);
	ULONG (STDMETHODCALLTYPE *Release) (IUIViewSettingsInterop *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUIViewSettingsInterop *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUIViewSettingsInterop *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUIViewSettingsInterop *, TrustLevel *);
  
	// IUIViewSettingsInterop
	HRESULT (STDMETHODCALLTYPE *GetForWindow) (IUIViewSettingsInterop *, HWND, REFIID, PPVOID);
} IUIViewSettingsInteropVtbl;

typedef struct _IUIViewSettingsInterop {
	IUIViewSettingsInteropVtbl *lpVtbl;
} IUIViewSettingsInterop;

typedef enum _UserInteractionMode {
	UserInteractionMode_Mouse,
	UserInteractionMode_Touch
} UserInteractionMode;

typedef struct _IUIViewSettings IUIViewSettings;

typedef struct _IUIViewSettingsVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUIViewSettings *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUIViewSettings *);
	ULONG (STDMETHODCALLTYPE *Release) (IUIViewSettings *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUIViewSettings *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUIViewSettings *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUIViewSettings *, TrustLevel *);

	// IUIViewSettings
	HRESULT (STDMETHODCALLTYPE *get_UserInteractionMode) (IUIViewSettings *, UserInteractionMode *);
} IUIViewSettingsVtbl;

typedef struct _IUIViewSettings {
	IUIViewSettingsVtbl *lpVtbl;
} IUIViewSettings;

extern IUIViewSettingsInterop CUIViewSettingsInterop;
extern IUIViewSettings CUIViewSettings;

//
// WINRT: Windows.UI.ViewManagement.UISettings
//

typedef enum _UIElementType {
	UIElementType_ActiveCaption			= 0,
	UIElementType_Background			= 1,
	UIElementType_ButtonFace			= 2,
	UIElementType_ButtonText			= 3,
	UIElementType_CaptionText			= 4,
	UIElementType_GrayText				= 5,
	UIElementType_Highlight				= 6,
	UIElementType_HighlightText			= 7,
	UIElementType_Hotlight				= 8,
	UIElementType_InactiveCaption		= 9,
	UIElementType_InactiveCaptionText	= 10,
	UIElementType_Window				= 11,
	UIElementType_WindowText			= 12,
	UIElementType_AccentColor			= 1000,
	UIElementType_TextHigh				= 1001,
	UIElementType_TextMedium			= 1002,
	UIElementType_TextLow				= 1003,
	UIElementType_TextContrastWithHigh	= 1004,
	UIElementType_NonTextHigh			= 1005,
	UIElementType_NonTextMediumHigh		= 1006,
	UIElementType_NonTextMedium			= 1007,
	UIElementType_NonTextMediumLow		= 1008,
	UIElementType_NonTextLow			= 1009,
	UIElementType_PageBackground		= 1010,
	UIElementType_PopupBackground		= 1011,
	UIElementType_OverlayOutsidePopup	= 1012,
} UIElementType;

typedef enum _UIHandPreference {
	HandPreference_LeftHanded,
	HandPreference_RightHanded
} UIHandPreference;

typedef struct _UISettingsSize {
	FLOAT	Width;
	FLOAT	Height;
} UISettingsSize;

typedef struct _UIColor {
	BYTE A;
	BYTE R;
	BYTE G;
	BYTE B;
} UIColor;

typedef struct _IUISettings IUISettings;

typedef struct _IUISettingsVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings *, TrustLevel *);

	// IUISettings
	HRESULT (STDMETHODCALLTYPE *get_HandPreference) (IUISettings *, UIHandPreference *);
	HRESULT (STDMETHODCALLTYPE *get_CursorSize) (IUISettings *, UISettingsSize *);
	HRESULT (STDMETHODCALLTYPE *get_ScrollBarSize) (IUISettings *, UISettingsSize *);
	HRESULT (STDMETHODCALLTYPE *get_ScrollBarArrowSize) (IUISettings *, UISettingsSize *);
	HRESULT (STDMETHODCALLTYPE *get_ScrollBarThumbBoxSize) (IUISettings *, UISettingsSize *);
	HRESULT (STDMETHODCALLTYPE *get_MessageDuration) (IUISettings *, PULONG);
	HRESULT (STDMETHODCALLTYPE *get_AnimationsEnabled) (IUISettings *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *get_CaretBrowsingEnabled) (IUISettings *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *get_CaretBlinkRate) (IUISettings *, PULONG);
	HRESULT (STDMETHODCALLTYPE *get_CaretWidth) (IUISettings *, PULONG);
	HRESULT (STDMETHODCALLTYPE *get_DoubleClickTime) (IUISettings *, PULONG);
	HRESULT (STDMETHODCALLTYPE *get_MouseHoverTime) (IUISettings *, PULONG);
	HRESULT (STDMETHODCALLTYPE *UIElementColor) (IUISettings *, UIElementType, UIColor *);
} IUISettingsVtbl;

typedef struct _IUISettings {
	IUISettingsVtbl *lpVtbl;
} IUISettings;

extern IUISettings CUISettings;

typedef struct _IUISettings2 IUISettings2;

typedef struct _IUISettings2Vtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings2 *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings2 *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings2 *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings2 *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings2 *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings2 *, TrustLevel *);

	// IUISettings2
	HRESULT (STDMETHODCALLTYPE *get_TextScaleFactor) (IUISettings2 *, DOUBLE *);
	HRESULT (STDMETHODCALLTYPE *add_TextScaleFactorChanged) (IUISettings2 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_TextScaleFactorChanged) (IUISettings2 *, PVOID);
} IUISettings2Vtbl;

typedef struct _IUISettings2 {
	IUISettings2Vtbl *lpVtbl;
} IUISettings2;

extern IUISettings2 CUISettings2;

typedef enum _UIColorType {
	UIColorType_Background		= 0,
	UIColorType_Foreground		= 1,
	UIColorType_AccentDark3		= 2,
	UIColorType_AccentDark2		= 3,
	UIColorType_AccentDark1		= 4,
	UIColorType_Accent			= 5,
	UIColorType_AccentLight1	= 6,
	UIColorType_AccentLight2	= 7,
	UIColorType_AccentLight3	= 8,
	UIColorType_Complement		= 9,
} UIColorType;

typedef struct _IUISettings3 IUISettings3;

typedef struct _IUISettings3Vtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings3 *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings3 *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings3 *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings3 *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings3 *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings3 *, TrustLevel *);

	// IUISettings3
	HRESULT (STDMETHODCALLTYPE *GetColorValue) (IUISettings3 *, UIColorType, UIColor *);
	HRESULT (STDMETHODCALLTYPE *add_ColorValuesChanged) (IUISettings3 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_ColorValuesChanged) (IUISettings3 *, PVOID);
} IUISettings3Vtbl;

typedef struct _IUISettings3 {
	IUISettings3Vtbl *lpVtbl;
} IUISettings3;

extern IUISettings3 CUISettings3;

typedef struct _IUISettings4 IUISettings4;

typedef struct _IUISettings4Vtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings4 *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings4 *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings4 *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings4 *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings4 *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings4 *, TrustLevel *);

	// IUISettings4
	HRESULT (STDMETHODCALLTYPE *get_AdvancedEffectsEnabled) (IUISettings4 *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *add_AdvancedEffectsEnabledChanged) (IUISettings4 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_AdvancedEffectsEnabledChanged) (IUISettings4 *, PVOID);
} IUISettings4Vtbl;

typedef struct _IUISettings4 {
	IUISettings4Vtbl *lpVtbl;
} IUISettings4;

extern IUISettings4 CUISettings4;

typedef struct _IUISettings5 IUISettings5;

typedef struct _IUISettings5Vtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings5 *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings5 *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings5 *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings5 *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings5 *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings5 *, TrustLevel *);

	// IUISettings5
	HRESULT (STDMETHODCALLTYPE *get_AutoHideScrollBars) (IUISettings5 *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *add_AutoHideScrollBarsChanged) (IUISettings5 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_AutoHideScrollBarsChanged) (IUISettings5 *, PVOID);
} IUISettings5Vtbl;

typedef struct _IUISettings5 {
	IUISettings5Vtbl *lpVtbl;
} IUISettings5;

extern IUISettings5 CUISettings5;

typedef struct _IUISettings6 IUISettings6;

typedef struct _IUISettings6Vtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUISettings6 *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUISettings6 *);
	ULONG (STDMETHODCALLTYPE *Release) (IUISettings6 *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUISettings6 *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUISettings6 *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUISettings6 *, TrustLevel *);

	// IUISettings6
	HRESULT (STDMETHODCALLTYPE *add_AnimationsEnabledChanged) (IUISettings6 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_AnimationsEnabledChanged) (IUISettings6 *, PVOID);
	HRESULT (STDMETHODCALLTYPE *add_MessageDurationChanged) (IUISettings6 *, PVOID, PPVOID);
	HRESULT (STDMETHODCALLTYPE *remove_MessageDurationChanged) (IUISettings6 *, PVOID);
} IUISettings6Vtbl;

typedef struct _IUISettings6 {
	IUISettings6Vtbl *lpVtbl;
} IUISettings6;

extern IUISettings6 CUISettings6;

typedef struct _IRestrictedErrorInfo IRestrictedErrorInfo;

typedef struct _IRestrictedErrorInfoVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IRestrictedErrorInfo *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IRestrictedErrorInfo *);
	ULONG (STDMETHODCALLTYPE *Release)(IRestrictedErrorInfo *);

	// IRestrictedErrorInfo
	HRESULT (STDMETHODCALLTYPE *GetErrorDetails)(IRestrictedErrorInfo *, BSTR *, HRESULT *, BSTR *, BSTR *);
	HRESULT (STDMETHODCALLTYPE *GetReference)(IRestrictedErrorInfo *, BSTR *);
} IRestrictedErrorInfoVtbl;

typedef struct _IRestrictedErrorInfo {
	IRestrictedErrorInfoVtbl *lpVtbl;
} IRestrictedErrorInfo;

typedef struct _IUriRuntimeClass IUriRuntimeClass;

typedef struct _IUriRuntimeClassVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUriRuntimeClass *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUriRuntimeClass *);
	ULONG (STDMETHODCALLTYPE *Release) (IUriRuntimeClass *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUriRuntimeClass *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUriRuntimeClass *, TrustLevel *);

	// IUriRuntimeClass
	HRESULT (STDMETHODCALLTYPE *get_AbsoluteUri) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_DisplayUri) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Domain) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Extension) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Fragment) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Host) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Password) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Path) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Query) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_QueryParsed) (IUriRuntimeClass *, IUnknown **); // actually IWwwFormUrlDecoderRuntimeClass **
	HRESULT (STDMETHODCALLTYPE *get_RawUri) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_SchemeName) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_UserName) (IUriRuntimeClass *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *get_Port) (IUriRuntimeClass *, PULONG);
	HRESULT (STDMETHODCALLTYPE *get_Suspicious) (IUriRuntimeClass *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *Equals) (IUriRuntimeClass *, IUriRuntimeClass *, PBOOLEAN);
	HRESULT (STDMETHODCALLTYPE *CombineUri) (IUriRuntimeClass *, HSTRING, IUriRuntimeClass **);
} IUriRuntimeClassVtbl;

typedef struct _IUriRuntimeClass {
	IUriRuntimeClassVtbl	*lpVtbl;
	LONG					RefCount;
	IUri					*Uri;
} IUriRuntimeClass;

typedef struct _IUriRuntimeClassFactory IUriRuntimeClassFactory;

typedef struct _IUriRuntimeClassFactoryVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (IUriRuntimeClassFactory *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (IUriRuntimeClassFactory *);
	ULONG (STDMETHODCALLTYPE *Release) (IUriRuntimeClassFactory *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (IUriRuntimeClassFactory *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (IUriRuntimeClassFactory *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (IUriRuntimeClassFactory *, TrustLevel *);

	// IUriRuntimeClassFactory
	HRESULT (STDMETHODCALLTYPE *CreateUri) (IUriRuntimeClassFactory *, HSTRING, IUriRuntimeClass **);
	HRESULT (STDMETHODCALLTYPE *CreateWithRelativeUri) (IUriRuntimeClassFactory *, HSTRING, HSTRING, IUriRuntimeClass **);
} IUriRuntimeClassFactoryVtbl;

typedef struct _IUriRuntimeClassFactory {
	IUriRuntimeClassFactoryVtbl *lpVtbl;
} IUriRuntimeClassFactory;

extern IUriRuntimeClassFactory CUriRuntimeClassFactory;

typedef struct _ILauncherStatics ILauncherStatics;

typedef struct _ILauncherStaticsVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface) (ILauncherStatics *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef) (ILauncherStatics *);
	ULONG (STDMETHODCALLTYPE *Release) (ILauncherStatics *);
	
	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids) (ILauncherStatics *, PULONG, IID **);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName) (ILauncherStatics *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel) (ILauncherStatics *, TrustLevel *);

	// ILauncherStatics
	HRESULT (STDMETHODCALLTYPE *LaunchFileAsync) (ILauncherStatics *, IUnknown *, IAsyncOperation **);
	HRESULT (STDMETHODCALLTYPE *LaunchFileWithOptionsAsync) (ILauncherStatics *, IUnknown *, IUnknown *, IAsyncOperation **);
	HRESULT (STDMETHODCALLTYPE *LaunchUriAsync) (ILauncherStatics *, IUriRuntimeClass *, IAsyncOperation **);
	HRESULT (STDMETHODCALLTYPE *LaunchUriWithOptionsAsync) (ILauncherStatics *, IUriRuntimeClass *, IUnknown *, IAsyncOperation **);
} ILauncherStaticsVtbl;

typedef struct _ILauncherStatics {
	ILauncherStaticsVtbl *lpVtbl;
} ILauncherStatics;

extern ILauncherStatics CLauncherStatics;

//
// roinit.c
//

KXCOMAPI HRESULT WINAPI RoInitialize(
	IN	RO_INIT_TYPE	InitType);

KXCOMAPI VOID WINAPI RoUninitialize(
	VOID);

//
// rofactry.c
//

HRESULT WINAPI RoGetActivationFactory(
	IN	HSTRING	ActivatableClassId,
	IN	REFIID	RefIID,
	OUT	PPVOID	Factory);

HRESULT WINAPI RoActivateInstance(
	IN	HSTRING			ActivatableClassId,
	OUT	IInspectable	**Instance);

//
// roerror.c
//

BOOL WINAPI RoOriginateError(
	IN	HRESULT	Result,
	IN	HSTRING	Message);

BOOL WINAPI RoOriginateErrorW(
	IN	HRESULT	Result,
	IN	ULONG	Length,
	IN	PCWSTR	Message);

KXCOMAPI BOOL WINAPI RoOriginateLanguageException(
	IN	HRESULT		Result,
	IN	HSTRING		Message OPTIONAL,
	IN	IUnknown	*LanguageException);

HRESULT WINAPI GetRestrictedErrorInfo(
	OUT	IUnknown	**RestrictedErrorInfo);

//
// roapi.c
//

KXCOMAPI HRESULT WINAPI RoGetAgileReference(
	IN	ULONG		Options,
	IN	REFIID		RefIID,
	IN	IUnknown	*pUnknown,
	OUT	IUnknown	**AgileReference);

//
// mta.c
//

KXCOMAPI HRESULT WINAPI CoIncrementMTAUsage(
	OUT	PCO_MTA_USAGE_COOKIE	Cookie);

KXCOMAPI HRESULT WINAPI CoDecrementMTAUsage(
	IN	CO_MTA_USAGE_COOKIE		Cookie);

//
// winrt.c
//

KXCOMAPI ULONG WINAPI WindowsGetStringLen(
	IN	HSTRING	String);

KXCOMAPI PCWSTR WINAPI WindowsGetStringRawBuffer(
	IN	HSTRING	String,
	OUT	PULONG	Length OPTIONAL);

KXCOMAPI HRESULT WINAPI WindowsCreateString(
	IN	PCNZWCH			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING			*String);

KXCOMAPI HRESULT WINAPI WindowsCreateStringReference(
	IN	PCWSTR			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING_HEADER	*StringHeader,
	OUT	HSTRING			*String);

KXCOMAPI HRESULT WINAPI WindowsDuplicateString(
	IN	HSTRING	OriginalString,
	OUT	HSTRING	*DuplicatedString);

KXCOMAPI HRESULT WINAPI WindowsDeleteString(
	IN	HSTRING	String);

KXCOMAPI BOOL WINAPI WindowsIsStringEmpty(
	IN	HSTRING	String);

KXCOMAPI HRESULT WINAPI WindowsStringHasEmbeddedNull(
	IN	HSTRING	String,
	OUT	PBOOL	HasEmbeddedNull);

KXCOMAPI HRESULT WINAPI WindowsCompareStringOrdinal(
	IN	HSTRING	String1,
	IN	HSTRING	String2,
	OUT	PINT	ComparisonResult);

KXCOMAPI HRESULT WINAPI WindowsSubstring(
	IN	HSTRING	String,
	IN	ULONG	StartIndex,
	OUT	HSTRING	*NewString);

KXCOMAPI HRESULT WINAPI WindowsSubstringWithSpecifiedLength(
	IN	HSTRING	OriginalString,
	IN	ULONG	StartIndex,
	IN	ULONG	SubstringLength,
	OUT	HSTRING	*NewString);

KXCOMAPI HRESULT WINAPI WindowsConcatString(
	IN	HSTRING	String1,
	IN	HSTRING	String2,
	OUT	HSTRING	*NewString);

KXCOMAPI HRESULT WINAPI WindowsPreallocateStringBuffer(
	IN	ULONG			Length,
	OUT	PPWSTR			CharacterBuffer,
	OUT	PHSTRING_BUFFER	BufferHandle);

KXCOMAPI HRESULT WINAPI WindowsDeleteStringBuffer(
	IN	HSTRING_BUFFER	BufferHandle);

KXCOMAPI HRESULT WINAPI WindowsPromoteStringBuffer(
	IN	HSTRING_BUFFER	BufferHandle,
	OUT	HSTRING			*NewString);