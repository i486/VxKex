#pragma once

#include <KexComm.h>

#define E_STRING_NOT_NULL_TERMINATED	((HRESULT) 0x80000017L)
#define E_BOUNDS						((HRESULT) 0x8000000BL)
#define HRESULT_ARITHMETIC_OVERFLOW		((HRESULT) 0x80070216L)

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

typedef HANDLE TYPEDEF_TYPE_NAME(CO_MTA_USAGE_COOKIE);

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

typedef struct _IVectorView_HSTRING IVectorView_HSTRING;

typedef struct _IVectorView_HSTRINGVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IVectorView_HSTRING *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IVectorView_HSTRING *);
	ULONG (STDMETHODCALLTYPE *Release)(IVectorView_HSTRING *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IVectorView_HSTRING *, PULONG, REFIID *);
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
} IVectorView_HSTRING;

typedef struct _IGlobalizationPreferencesStatics IGlobalizationPreferencesStatics;

typedef struct _IGlobalizationPreferencesStaticsVtbl {
	// IUnknown
	HRESULT (STDMETHODCALLTYPE *QueryInterface)(IGlobalizationPreferencesStatics *, REFIID, PPVOID);
	ULONG (STDMETHODCALLTYPE *AddRef)(IGlobalizationPreferencesStatics *);
	ULONG (STDMETHODCALLTYPE *Release)(IGlobalizationPreferencesStatics *);

	// IInspectable
	HRESULT (STDMETHODCALLTYPE *GetIids)(IGlobalizationPreferencesStatics *, PULONG, REFIID *);
	HRESULT (STDMETHODCALLTYPE *GetRuntimeClassName)(IGlobalizationPreferencesStatics *, HSTRING *);
	HRESULT (STDMETHODCALLTYPE *GetTrustLevel)(IGlobalizationPreferencesStatics *, TrustLevel *);

	// IGlobalizationPreferencesStatics
	HRESULT (STDMETHODCALLTYPE *get_Calendars)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Clocks)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Currencies)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_Languages)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_HomeGeographicRegion)(IGlobalizationPreferencesStatics *, IVectorView_HSTRING **);
	HRESULT (STDMETHODCALLTYPE *get_WeekStartsOn)(IGlobalizationPreferencesStatics *, DayOfWeek *);
} IGlobalizationPreferencesStaticsVtbl;

typedef struct _IGlobalizationPreferencesStatics {
	IGlobalizationPreferencesStaticsVtbl	*lpVtbl;
} IGlobalizationPreferencesStatics;

extern IGlobalizationPreferencesStatics CGlobalizationPreferencesStatics;

//
// roinit.c
//

KXCOMAPI HRESULT WINAPI RoInitialize(
	IN	RO_INIT_TYPE	InitType);

KXCOMAPI VOID WINAPI RoUninitialize(
	VOID);

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

ULONG WINAPI WindowsGetStringLen(
	IN	HSTRING	String);

PCWSTR WINAPI WindowsGetStringRawBuffer(
	IN	HSTRING	String,
	OUT	PULONG	Length OPTIONAL);

HRESULT WINAPI WindowsCreateString(
	IN	PCNZWCH			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING			*String);

HRESULT WINAPI WindowsCreateStringReference(
	IN	PCWSTR			SourceString,
	IN	ULONG			SourceStringCch,
	OUT	HSTRING_HEADER	*StringHeader,
	OUT	HSTRING			*String);

HRESULT WINAPI WindowsDuplicateString(
	IN	HSTRING	OriginalString,
	OUT	HSTRING	*DuplicatedString);

HRESULT WINAPI WindowsDeleteString(
	IN	HSTRING	String);

BOOL WINAPI WindowsIsStringEmpty(
	IN	HSTRING	String);

HRESULT WINAPI WindowsStringHasEmbeddedNull(
	IN	HSTRING	String,
	OUT	PBOOL	HasEmbeddedNull);

HRESULT WINAPI WindowsCompareStringOrdinal(
	IN	HSTRING	String1,
	IN	HSTRING	String2,
	OUT	PINT	ComparisonResult);

HRESULT WINAPI WindowsSubstring(
	IN	HSTRING	String,
	IN	ULONG	StartIndex,
	OUT	HSTRING	*NewString);

HRESULT WINAPI WindowsSubstringWithSpecifiedLength(
	IN	HSTRING	OriginalString,
	IN	ULONG	StartIndex,
	IN	ULONG	SubstringLength,
	OUT	HSTRING	*NewString);