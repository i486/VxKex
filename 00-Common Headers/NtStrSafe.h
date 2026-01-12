/******************************************************************
*                                                                 *
*  ntstrsafe.h -- This module defines safer C library string      *
*                 routine replacements for drivers. These are     *
*                 meant to make C a bit more safe in reference    *
*                 to security and robustness. A similar file,     *
*                 strsafe.h, is available for applications.       *
*                                                                 *
******************************************************************/
#ifndef _NTSTRSAFE_H_INCLUDED_
#define _NTSTRSAFE_H_INCLUDED_
#if (_MSC_VER > 1000)
#pragma once
#endif


#include <stdio.h>          // for _vsnprintf, _vsnwprintf, getc, getwc
#include <string.h>         // for memset
#include <stdarg.h>         // for va_start, etc.
#include <specstrings.h>    // for __in, etc.

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86)) && (_MSC_VER >= 1300)
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64) || defined(_M_AMD64)
#define ALIGNMENT_MACHINE
#define UNALIGNED __unaligned
#if defined(_WIN64)
#define UNALIGNED64 __unaligned
#else
#define UNALIGNED64
#endif
#else
#undef ALIGNMENT_MACHINE
#define UNALIGNED
#define UNALIGNED64
#endif

// typedefs
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif

#ifndef _NTSTATUS_DEFINED
#define _NTSTATUS_DEFINED
typedef __success(return >= 0) long NTSTATUS;
#endif

typedef unsigned long DWORD;


#ifndef SORTPP_PASS
// compiletime asserts (failure results in error C2118: negative subscript)
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else
#define C_ASSERT(e)
#endif

#ifdef __cplusplus
#define EXTERN_C    extern "C"
#else
#define EXTERN_C    extern
#endif

// use the new secure crt functions if available
#ifndef NTSTRSAFE_USE_SECURE_CRT
#if defined(__GOT_SECURE_LIB__) && (__GOT_SECURE_LIB__ >= 200402L)
#define NTSTRSAFE_USE_SECURE_CRT 0
#else
#define NTSTRSAFE_USE_SECURE_CRT 0
#endif
#endif  // !NTSTRSAFE_USE_SECURE_CRT

#ifdef _M_CEE_PURE
#define NTSTRSAFEDDI      __inline NTSTATUS __clrcall
#else
#define NTSTRSAFEDDI      __inline NTSTATUS __stdcall
#endif

#if defined(NTSTRSAFE_LIB_IMPL) || defined(NTSTRSAFE_LIB)
#define NTSTRSAFEWORKERDDI    EXTERN_C NTSTATUS __stdcall 
#else
#define NTSTRSAFEWORKERDDI    static NTSTRSAFEDDI
#endif

// The following steps are *REQUIRED* if ntstrsafe.h is used for drivers on:
//     Windows 2000
//     Windows Millennium Edition
//     Windows 98 Second Edition
//     Windows 98
//
// 1. #define NTSTRSAFE_LIB before including the ntstrsafe.h header file.
// 2. Add ntstrsafe.lib to the TARGET_LIBS line in SOURCES
//
// Drivers running on XP and later can skip these steps to create a smaller
// driver by running the functions inline.
#if defined(NTSTRSAFE_LIB)
#pragma comment(lib, "ntstrsafe.lib")
#endif

// The user can request no "Cb" or no "Cch" fuctions, but not both
#if defined(NTSTRSAFE_NO_CB_FUNCTIONS) && defined(NTSTRSAFE_NO_CCH_FUNCTIONS)
#error cannot specify both NTSTRSAFE_NO_CB_FUNCTIONS and NTSTRSAFE_NO_CCH_FUNCTIONS !!
#endif

// The user may override NTSTRSAFE_MAX_CCH, but it must always be less than INT_MAX
#ifndef NTSTRSAFE_MAX_CCH
#define NTSTRSAFE_MAX_CCH     2147483647  // max buffer size, in characters, that we support (same as INT_MAX)
#endif
C_ASSERT(NTSTRSAFE_MAX_CCH <= 2147483647);
C_ASSERT(NTSTRSAFE_MAX_CCH > 1);

#define NTSTRSAFE_MAX_LENGTH  (NTSTRSAFE_MAX_CCH - 1)   // max buffer length, in characters, that we support

// The user may override NTSTRSAFE_UNICODE_STRING_MAX_CCH, but it must always be less than (USHORT_MAX / sizeof(wchar_t))
#ifndef NTSTRSAFE_UNICODE_STRING_MAX_CCH
#define NTSTRSAFE_UNICODE_STRING_MAX_CCH    (0xffff / sizeof(wchar_t))  // max buffer size, in characters, for a UNICODE_STRING
#endif
C_ASSERT(NTSTRSAFE_UNICODE_STRING_MAX_CCH <= (0xffff / sizeof(wchar_t)));
C_ASSERT(NTSTRSAFE_UNICODE_STRING_MAX_CCH > 1);


// Flags for controling the Ex functions
//
//      STRSAFE_FILL_BYTE(0xFF)                         0x000000FF  // bottom byte specifies fill pattern
#define STRSAFE_IGNORE_NULLS                            0x00000100  // treat null string pointers as TEXT("") -- don't fault on NULL buffers
#define STRSAFE_FILL_BEHIND_NULL                        0x00000200  // on success, fill in extra space behind the null terminator with fill pattern
#define STRSAFE_FILL_ON_FAILURE                         0x00000400  // on failure, overwrite pszDest with fill pattern and null terminate it
#define STRSAFE_NULL_ON_FAILURE                         0x00000800  // on failure, set *pszDest = TEXT('\0')
#define STRSAFE_NO_TRUNCATION                           0x00001000  // instead of returning a truncated result, copy/append nothing to pszDest and null terminate it

// Flags for controling UNICODE_STRING Ex functions
//
//      STRSAFE_FILL_BYTE(0xFF)                         0x000000FF  // bottom byte specifies fill pattern
//      STRSAFE_IGNORE_NULLS                            0x00000100  // don't fault on NULL UNICODE_STRING pointers, and treat null pszSrc as L""
#define STRSAFE_FILL_BEHIND                             0x00000200  // on success, fill in extra space at the end of the UNICODE_STRING Buffer with fill pattern
//      STRSAFE_FILL_ON_FAILURE                         0x00000400  // on failure, fill the UNICODE_STRING Buffer with fill pattern and set the Length to 0
#define STRSAFE_ZERO_LENGTH_ON_FAILURE                  0x00000800  // on failure, set the UNICODE_STRING Length to 0
//      STRSAFE_NO_TRUNCATION                           0x00001000  // instead of returning a truncated result, copy/append nothing to UNICODE_STRING Buffer


#define STRSAFE_VALID_FLAGS                     (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS      (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE | STRSAFE_NO_TRUNCATION)

// helper macro to set the fill character and specify buffer filling
#define STRSAFE_FILL_BYTE(x)                    ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x)                 ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))

#define STRSAFE_GET_FILL_PATTERN(dwFlags)       ((int)(dwFlags & 0x000000FF))


//
// These typedefs are used in places where the string is guaranteed to
// be null terminated.
//
typedef __nullterminated char* NTSTRSAFE_PSTR;
typedef __nullterminated const char* NTSTRSAFE_PCSTR;
typedef __nullterminated wchar_t* NTSTRSAFE_PWSTR;
typedef __nullterminated const wchar_t* NTSTRSAFE_PCWSTR;
typedef __nullterminated const wchar_t UNALIGNED* NTSTRSAFE_PCUWSTR;

//
// These typedefs are used in places where the string is NOT guaranteed to
// be null terminated.
//
typedef __possibly_notnullterminated const char* STRSAFE_PCNZCH;
typedef __possibly_notnullterminated const wchar_t* STRSAFE_PCNZWCH;
typedef __possibly_notnullterminated const wchar_t UNALIGNED* STRSAFE_PCUNZWCH;


// prototypes for the worker functions

NTSTRSAFEWORKERDDI
RtlStringLengthWorkerA(
    __in STRSAFE_PCNZCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength);

NTSTRSAFEWORKERDDI
RtlStringLengthWorkerW(
    __in STRSAFE_PCNZWCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength);
    
#ifdef ALIGNMENT_MACHINE
NTSTRSAFEWORKERDDI
RtlUnalignedStringLengthWorkerW(
    __in STRSAFE_PCUNZWCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength);
#endif  // ALIGNMENT_MACHINE

NTSTRSAFEWORKERDDI
RtlStringExValidateSrcA(
    __deref_in_opt_out NTSTRSAFE_PCSTR* ppszSrc,
    __inout_opt __deref_out_range(<, cchMax) size_t* pcchToRead,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateSrcW(
    __deref_in_opt_out NTSTRSAFE_PCWSTR* ppszSrc,
    __inout_opt __deref_out_range(<, cchMax) size_t* pcchToRead,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringValidateDestA(
    __in_ecount_opt(cchDest) STRSAFE_PCNZCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax);

NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthA(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax);

NTSTRSAFEWORKERDDI
RtlStringValidateDestW(
    __in_ecount_opt(cchDest) STRSAFE_PCNZWCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax);

NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthW(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCWSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestA(
    __in_ecount_opt(cchDest) STRSAFE_PCNZCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthA(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestW(
    __in_ecount_opt(cchDest) STRSAFE_PCNZWCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthW(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCWSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringCopyWorkerA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, (cchToCopy < cchDest) ? cchToCopy : cchDest - 1) size_t* pcchNewDestLength,
    __in_xcount(cchToCopy) STRSAFE_PCNZCH pszSrc,
    __in __in_range(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy);

NTSTRSAFEWORKERDDI
RtlStringCopyWorkerW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, (cchToCopy < cchDest) ? cchToCopy : cchDest - 1) size_t* pcchNewDestLength,
    __in_xcount(cchToCopy) STRSAFE_PCNZWCH pszSrc,
    __in __in_range(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy);

NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, cchDest - 1) size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList);

NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, cchDest - 1) size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList);

NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullA(
    __inout_bcount(cbRemaining) NTSTRSAFE_PSTR pszDestEnd,
    __in size_t cbRemaining,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullW(
    __inout_bcount(cbRemaining) NTSTRSAFE_PWSTR pszDestEnd,
    __in size_t cbRemaining,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExHandleOtherFlagsA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(sizeof(char), NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbDest,
    __in __in_range(<, cbDest / sizeof(char)) size_t cchOriginalDestLength,
    __deref_inout_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out __deref_out_range(<=, cbDest / sizeof(char)) size_t* pcchRemaining,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExHandleOtherFlagsW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(sizeof(wchar_t), NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbDest,
    __in __in_range(<, cbDest / sizeof(wchar_t)) size_t cchOriginalDestLength,    
    __deref_inout_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out __deref_out_range(<=, cbDest / sizeof(wchar_t)) size_t* pcchRemaining,
    __in DWORD dwFlags);

#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

NTSTRSAFEWORKERDDI
RtlUnicodeStringInitWorker(
    __out PUNICODE_STRING DestinationString,
    __in_opt NTSTRSAFE_PCWSTR pszSrc,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateWorker(
    __in_opt PCUNICODE_STRING SourceString,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateSrcWorker(
    __in PCUNICODE_STRING SourceString,
    __deref_out_ecount(*pcchSrcLength) wchar_t** ppszSrc,
    __out size_t* pcchSrcLength,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateDestWorker(
    __in PCUNICODE_STRING DestinationString,
    __deref_out_ecount(*pcchDest) wchar_t** ppszDest,
    __out size_t* pcchDest,
    __out_opt size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringCopyWideCharArrayWorker(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __out_opt size_t* pcchNewDestLength,
    __in_ecount(cchSrcLength) const wchar_t* pszSrc,
    __in size_t cchSrcLength);

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyStringWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToCopy);

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in_ecount(cchSrcLength) const wchar_t* pszSrc,
    __in size_t cchSrcLength);

NTSTRSAFEWORKERDDI
RtlWideCharArrayVPrintfWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList);

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleFill(
    __out_ecount(cchRemaining) wchar_t* pszDestEnd,
    __in size_t cchRemaining,
    __in DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleOtherFlags(
    __inout_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __in size_t cchOriginalDestLength,
    __out size_t* pcchNewDestLength,
    __deref_out_ecount(*pcchRemaining) wchar_t** ppszDestEnd,
    __out size_t* pcchRemaining,
    __in DWORD dwFlags);

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS


// To allow this to stand alone.
#define __WARNING_CYCLOMATIC_COMPLEXITY 28734
#define __WARNING_DEREF_NULL_PTR 6011
#define __WARNING_INVALID_PARAM_VALUE_1 6387
#define __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY 26015
#define __WARNING_RETURNING_BAD_RESULT 28196
#define __WARNING_BANNED_API_USAGE 28719

#pragma warning(push)
#if _MSC_VER <= 1400
#pragma warning(disable: 4616)  // turn off warning out of range so prefast pragmas won't show
                                // show up in build.wrn/build.err
#endif
#pragma warning(disable : 4996) // 'function': was declared deprecated
#pragma warning(disable : 4995) // name was marked as #pragma deprecated
#pragma warning(disable : 4793) // vararg causes native code generation
#pragma warning(disable : __WARNING_CYCLOMATIC_COMPLEXITY)


#ifndef NTSTRSAFE_LIB_IMPL

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopy(
    __out_ecount(cchDest) LPTSTR  pszDest,
    __in  size_t  cchDest,
    __in  LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is not a replacement for strncpy.  That function will pad the
    destination string with extra null termination characters if the count is
    greater than the length of the source string, and it will fail to null
    terminate the destination string if the source string length is greater
    than or equal to the count. You can not blindly use this instead of strncpy:
    it is common for code to use it to "patch" strings and you would introduce
    errors if the code started null terminating in the middle of the string.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of
    pszSrc will be copied to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCchCopyA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest,
                               cchDest,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCopyW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    
    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest,
                               cchDest,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopy(
    __out_bcount(cbDest) LPTSTR pszDest,
    __in  size_t cbDest,
    __in  LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This routine is not a replacement for strncpy.  That function will pad the
    destination string with extra null termination characters if the count is
    greater than the length of the source string, and it will fail to null
    terminate the destination string if the source string length is greater
    than or equal to the count. You can not blindly use this instead of strncpy:
    it is common for code to use it to "patch" strings and you would introduce
    errors if the code started null terminating in the middle of the string.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest        -   destination string

    cbDest         -   size of destination buffer in bytes.
                       length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
                       hold all of the source including the null terminator

    pszSrc         -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCbCopyA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest,
                               cchDest,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCopyW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest,
                               cchDest,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyEx(
    __out_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in  size_t  cchDest,
    __in  LPCTSTR pszSrc          OPTIONAL,
    __deref_opt_out_ecount(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcchRemaining   OPTIONAL,
    __in  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCopy, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCopyExA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCSTR pszSrc,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;
                
                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCopyExW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyEx(
    __out_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in  size_t  cbDest,
    __in  LPCTSTR pszSrc          OPTIONAL,
    __deref_opt_out_bcount(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcbRemaining    OPTIONAL,
    __in  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCopy, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
                        hold all of the source including the null terminator

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcbRemaining    -   pcbRemaining is non-null,the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCopyExA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCSTR pszSrc,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCopyExW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);
                
                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyN(
    __out_ecount(cchDest) LPTSTR  pszDest,
    __in  size_t  cchDest,
    __in  LPCTSTR pszSrc,
    __in  size_t  cchToCopy
    );


Routine Description:

    This routine is a safer version of the C built-in function 'strncpy'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchToCopy is greater than the length of pszSrc.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cchToCopy characters were copied
    without truncation and the resultant destination string was null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = (_tcslen(src) + 1) to hold all of the
                       source including the null terminator

    pszSrc         -   source string

    cchToCopy      -   maximum number of characters to copy from source string,
                       not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCopyNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCchCopyNA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToCopy) STRSAFE_PCNZCH pszSrc,
    __in size_t cchToCopy)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
            
            *pszDest = '\0';
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest,
                                   cchDest,
                                   NULL,
                                   pszSrc,
                                   cchToCopy);
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCopyNW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToCopy) STRSAFE_PCNZWCH pszSrc,
    __in size_t cchToCopy)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
            
            *pszDest = L'\0';
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest,
                                   cchDest,
                                   NULL,
                                   pszSrc,
                                   cchToCopy);
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyN(
    __out_bcount(cbDest) LPTSTR  pszDest,
    __in  size_t  cbDest,
    __in  LPCTSTR pszSrc,
    __in  size_t  cbToCopy
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cbToCopy is greater than the size of pszSrc.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cbToCopy characters were
    copied without truncation and the resultant destination string was null
    terminated, otherwise it will return a failure code. In failure cases as
    much of pszSrc will be copied to pszDest as possible, and pszDest will be
    null terminated.

Arguments:

    pszDest        -   destination string

    cbDest         -   size of destination buffer in bytes.
                       length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
                       hold all of the source including the null terminator

    pszSrc         -   source string

    cbToCopy       -   maximum number of bytes to copy from source string,
                       not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCbCopyNA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToCopy) STRSAFE_PCNZCH pszSrc,
    __in size_t cbToCopy)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        size_t cchToCopy = cbToCopy / sizeof(char);

        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
            
            *pszDest = '\0';
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest,
                                   cchDest,
                                   NULL,
                                   pszSrc,
                                   cchToCopy);
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCopyNW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToCopy) STRSAFE_PCNZWCH pszSrc,
    __in size_t cbToCopy)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            // Suppress espx false positive - cchDest cannot be 0 here
#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY) 
            *pszDest = L'\0';
#pragma warning(pop)
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest,
                                   cchDest,
                                   NULL,
                                   pszSrc,
                                   cchToCopy);
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCopyNEx(
    __out_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in  size_t  cchDest,
    __in  LPCTSTR pszSrc          OPTIONAL,
    __in  size_t  cchToCopy,
    __deref_opt_out_ecount(*pcchRemaining) LPTSTR* ppszDestEnd OPTIONAL,
    __out_opt size_t* pcchRemaining OPTIONAL,
    __in  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCopyN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination
    string including the null terminator. The flags parameter allows
    additional controls.

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cchToCopy is greater than the length of pszSrc.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = (_tcslen(pszSrc) + 1) to hold all of
                        the source including the null terminator

    pszSrc          -   source string

    cchToCopy       -   maximum number of characters to copy from the source
                        string

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified. If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL. An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCopyNExA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToCopy) STRSAFE_PCNZCH pszSrc,
    __in size_t cchToCopy,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCopyNExW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToCopy) STRSAFE_PCNZWCH pszSrc,
    __in size_t cchToCopy,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCopyNEx(
    __out_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in  size_t  cbDest,
    __in  LPCTSTR pszSrc          OPTIONAL,
    __in  size_t  cbToCopy,
    __deref_opt_out_bcount(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcbRemaining    OPTIONAL,
    __in  DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCopyN, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

    This routine is meant as a replacement for strncpy, but it does behave
    differently. This function will not pad the destination buffer with extra
    null termination characters if cbToCopy is greater than the size of pszSrc.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
                        hold all of the source including the null terminator

    pszSrc          -   source string

    cbToCopy        -   maximum number of bytes to copy from source string

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcbRemaining    -   pcbRemaining is non-null,the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCopyNExA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToCopy) STRSAFE_PCNZCH pszSrc,
    __in size_t cbToCopy,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchToCopy = cbToCopy / sizeof(char);

        status = RtlStringExValidateSrcA(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCopyNExW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToCopy) STRSAFE_PCNZWCH pszSrc,
    __in size_t cbToCopy,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)
        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);
#pragma warning(pop)

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                                       cchDest,
                                       &cchCopied,
                                       pszSrc,
                                       cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCat(
    __inout_ecount(cchDest) LPTSTR  pszDest,
    __in     size_t  cchDest,
    __in     LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat'.
    The size of the destination buffer (in characters) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be appended to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest     -  destination string which must be null terminated

    cchDest     -  size of destination buffer in characters.
                   length must be = (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                   to hold all of the combine string plus the null
                   terminator

    pszSrc      -  source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCchCatEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error occurs,
                       the destination buffer is modified to contain a truncated
                       version of the ideal result and is null terminated. This
                       is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCatA(
    __inout_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_CCH);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCatW(
    __inout_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_CCH);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCat(
    __inout_bcount(cbDest) LPTSTR  pszDest,
    __in     size_t  cbDest,
    __in     LPCTSTR pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat'.
    The size of the destination buffer (in bytes) is a parameter and this
    function will not write past the end of this buffer and it will ALWAYS
    null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
    otherwise it will return a failure code. In failure cases as much of pszSrc
    will be appended to pszDest as possible, and pszDest will be null
    terminated.

Arguments:

    pszDest     -  destination string which must be null terminated

    cbDest      -  size of destination buffer in bytes.
                   length must be = ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
                   to hold all of the combine string plus the null
                   terminator

    pszSrc      -  source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL.  See RtlStringCbCatEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error occurs,
                       the destination buffer is modified to contain a truncated
                       version of the ideal result and is null terminated. This
                       is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCatA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestAndLengthA(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_CCH);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCatW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestAndLengthW(pszDest,
                                      cchDest,
                                      &cchDestLength, 
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                               cchDest - cchDestLength,
                               NULL,
                               pszSrc,
                               NTSTRSAFE_MAX_CCH);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatEx(
    __inout_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in     size_t  cchDest,
    __in     LPCTSTR pszSrc          OPTIONAL,
    __deref_opt_out_ecount(*pcchRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt    size_t* pcchRemaining   OPTIONAL,
    __in     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCat, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters
                        length must be (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcat

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCatExA(
    __inout_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCSTR pszSrc,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;
            
                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);
            
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCatExW(
    __inout_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(wchar_t);
            
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatEx(
    __inout_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in     size_t  cbDest,
    __in     LPCTSTR pszSrc          OPTIONAL,
    __deref_opt_out_bcount(*pcbRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt    size_t* pcbRemaining    OPTIONAL,
    __in     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCat, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string which must be null terminated

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcat

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated
                       and the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCatExA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCSTR pszSrc,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCatExW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatN(
    __inout_ecount(cchDest) LPTSTR  pszDest,
    __in     size_t  cchDest,
    __in     LPCTSTR pszSrc,
    __in     size_t  cchToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat'.
    The size of the destination buffer (in characters) is a parameter as well as
    the maximum number of characters to append, excluding the null terminator.
    This function will not write past the end of the destination buffer and it will
    ALWAYS null terminate pszDest (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if all of pszSrc or the first cchToAppend characters were appended
    to the destination string and it was null terminated, otherwise it will
    return a failure code. In failure cases as much of pszSrc will be appended
    to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchToAppend     -   maximum number of characters to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCchCatNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters
                       were concatenated to pszDest and the resultant dest
                       string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCatNA(
    __inout_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToAppend) STRSAFE_PCNZCH pszSrc,
    __in size_t cchToAppend)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                                   cchDest - cchDestLength,
                                   NULL,
                                   pszSrc,
                                   cchToAppend);
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCatNW(
    __inout_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToAppend) STRSAFE_PCNZWCH pszSrc,
    __in size_t cchToAppend)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                                   cchDest - cchDestLength,
                                   NULL,
                                   pszSrc,
                                   cchToAppend);
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatN(
    __inout_bcount(cbDest) LPTSTR  pszDest,
    __in     size_t  cbDest,
    __in     LPCTSTR pszSrc,
    __in     size_t  cbToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat'.
    The size of the destination buffer (in bytes) is a parameter as well as
    the maximum number of bytes to append, excluding the null terminator.
    This function will not write past the end of the destination buffer and it will
    ALWAYS null terminate pszDest (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if all of pszSrc or the first cbToAppend bytes were appended
    to the destination string and it was null terminated, otherwise it will
    return a failure code. In failure cases as much of pszSrc will be appended
    to pszDest as possible, and pszDest will be null terminated.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + min(cbToAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cbToAppend      -   maximum number of bytes to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL. See RtlStringCbCatNEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
                       concatenated to pszDest and the resultant dest string
                       was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCatNA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToAppend) STRSAFE_PCNZCH pszSrc,
    __in size_t cbToAppend)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(char);
        
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                                   cchDest - cchDestLength,
                                   NULL,
                                   pszSrc,
                                   cchToAppend);
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCatNW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToAppend) STRSAFE_PCNZWCH pszSrc,
    __in size_t cbToAppend)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
                                      cchDest,
                                      &cchDestLength,
                                      NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);
        
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                                   cchDest - cchDestLength,
                                   NULL,
                                   pszSrc,
                                   cchToAppend);
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchCatNEx(
    __inout_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in     size_t  cchDest,
    __in     LPCTSTR pszSrc          OPTIONAL,
    __in     size_t  cchToAppend,
    __deref_opt_out_ecount(*pcchRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt    size_t* pcchRemaining   OPTIONAL,
    __in     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchCatN, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cchDest         -   size of destination buffer in characters.
                        length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cchToAppend     -   maximum number of characters to append

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters
                       were concatenated to pszDest and the resultant dest
                       string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchCatNExA(
    __inout_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToAppend) STRSAFE_PCNZCH pszSrc,
    __in size_t cchToAppend,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchCatNExW(
    __inout_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in_ecount(cchToAppend) STRSAFE_PCNZWCH pszSrc,
    __in size_t cchToAppend,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       cchToAppend);
                
                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbCatNEx(
    __inout_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in     size_t  cbDest,
    __in     LPCTSTR pszSrc          OPTIONAL,
    __in     size_t  cbToAppend,
    __deref_opt_out_bcount(*pcbRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt    size_t* pcchRemaining   OPTIONAL,
    __in     DWORD   dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbCatN, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string which must be null terminated

    cbDest          -   size of destination buffer in bytes.
                        length must be ((_tcslen(pszDest) + min(cbToAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
                        to hold all of the combine string plus the null
                        terminator.

    pszSrc          -   source string

    cbToAppend      -   maximum number of bytes to append

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function appended any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any pre-existing
                    or truncated string

        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any pre-existing or
                    truncated string

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
                       concatenated to pszDest and the resultant dest string
                       was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the operation
                       failed due to insufficient space. When this error
                       occurs, the destination buffer is modified to contain
                       a truncated version of the ideal result and is null
                       terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbCatNExA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToAppend) STRSAFE_PCNZCH pszSrc,
    __in size_t cbToAppend,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(char);

        status = RtlStringExValidateSrcA(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbCatNExW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in_bcount(cbToAppend) STRSAFE_PCNZWCH pszSrc,
    __in size_t cbToAppend,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
                                        cchDest,
                                        &cchDestLength,
                                        NTSTRSAFE_MAX_CCH,
                                        dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)
        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);
#pragma warning(pop)

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                                       cchRemaining,
                                       &cchCopied,
                                       pszSrc,
                                       cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      cchDestLength,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchVPrintf(
    __out_ecount(cchDest) LPTSTR  pszDest,
    __in  size_t  cchDest,
    __in __format_string  LPCTSTR pszFormat,
    __in  va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    argList     -  va_list from the variable arguments according to the
                   stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCchVPrintfEx if you
    require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchVPrintfA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerA(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchVPrintfW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerW(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbVPrintf(
    __out_bcount(cbDest) LPTSTR  pszDest,
    __in size_t  cbDest,
    __in __format_string LPCTSTR pszFormat,
    __in va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf'.
    The size of the destination buffer (in bytes) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cbDest      -  size of destination buffer in bytes
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    argList     -  va_list from the variable arguments according to the
                   stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCbVPrintfEx if you
    require the handling of NULL values.


Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbVPrintfA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerA(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbVPrintfW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
 
    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerW(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef _M_CEE_PURE

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchPrintf(
    __out_ecount(cchDest) LPTSTR  pszDest,
    __in size_t  cchDest,
    __in __format_string  LPCTSTR pszFormat,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf'.
    The size of the destination buffer (in characters) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cchDest     -  size of destination buffer in characters
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    ...         -  additional parameters to be formatted according to
                   the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCchPrintfEx if you
    require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchPrintfA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    ...)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);

        va_end(argList);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchPrintfW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);

        va_end(argList);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbPrintf(
    __out_bcount(cbDest) LPTSTR  pszDest,
    __in size_t  cbDest,
    __in __format_string LPCTSTR pszFormat,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf'.
    The size of the destination buffer (in bytes) is a parameter and
    this function will not write past the end of this buffer and it will
    ALWAYS null terminate the destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was printed without truncation and null terminated,
    otherwise it will return a failure code. In failure cases it will return
    a truncated version of the ideal result.

Arguments:

    pszDest     -  destination string

    cbDest      -  size of destination buffer in bytes
                   length must be sufficient to hold the resulting formatted
                   string, including the null terminator.

    pszFormat   -  format string which must be null terminated

    ...         -  additional parameters to be formatted according to
                   the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL.  See RtlStringCbPrintfEx if you
    require the handling of NULL values.


Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbPrintfA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);

        va_end(argList);
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbPrintfW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);
    
    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest,
                                  cchDest,
                                  NULL,
                                  pszFormat,
                                  argList);

        va_end(argList);
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchPrintfEx(
    __out_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in  size_t  cchDest,
    __deref_opt_out_ecount(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcchRemaining   OPTIONAL,
    __in DWORD   dwFlags,
    __in __format_string LPCTSTR pszFormat       OPTIONAL,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    ...             -   additional parameters to be formatted according to
                        the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string and it was null terminated.

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchPrintfExA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    ...)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerA(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

             // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchPrintfExW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerW(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbPrintfEx(
    __out_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in size_t  cbDest,
    __deref_opt_out_bcount(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcbRemaining    OPTIONAL,
    __in DWORD   dwFlags,
    __in __format_string LPCTSTR pszFormat       OPTIONAL,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbPrintf, this routine also returns a pointer to the end of the
    destination string and the number of bytes left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    ...             -   additional parameters to be formatted according to
                        the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbPrintfExA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerA(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbPrintfExW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerW(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS

#endif  // !_M_CEE_PURE


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchVPrintfEx(
    __out_ecount(cchDest) LPTSTR  pszDest         OPTIONAL,
    __in size_t  cchDest,
    __deref_opt_out_ecount(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcchRemaining   OPTIONAL,
    __in DWORD   dwFlags,
    __in __format_string LPCTSTR pszFormat       OPTIONAL,
    __in va_list argList
    );


Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCchVPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return
                        the number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    argList         -   va_list from the variable arguments according to the
                        stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCchVPrintfExA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cchDest,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerA(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCchVPrintfExW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerW(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbVPrintfEx(
    __out_bcount(cbDest) LPTSTR  pszDest         OPTIONAL,
    __in size_t  cbDest,
    __deref_opt_out_bcount(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
    __out_opt size_t* pcbRemaining    OPTIONAL,
    __in DWORD   dwFlags,
    __in __format_string LPCTSTR pszFormat       OPTIONAL,
    __in va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters.  In addition to functionality provided by
    RtlStringCbVPrintf, this routine also returns a pointer to the end of the
    destination string and the number of characters left in the destination string
    including the null terminator. The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cbDest          -   size of destination buffer in bytes.
                        length must be sufficient to contain the resulting
                        formatted string plus the null terminator.

    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return
                        a pointer to the end of the destination string.  If the
                        function printed any data, the result will point to the
                        null termination character

    pcbRemaining    -   if pcbRemaining is non-null, the function will return
                        the number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT(""))

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat       -   format string which must be null terminated

    argList         -   va_list from the variable arguments according to the
                        stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated and
                       the resultant dest string was null terminated

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlStringCbVPrintfExA(
    __out_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in size_t cbDest,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerA(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

NTSTRSAFEDDI
RtlStringCbVPrintfExW(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);
    
    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerW(pszDest,
                                          cchDest,
                                          &cchNewDestLength,
                                          pszFormat,
                                          argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlStringCchLength(
    __in    LPCTSTR psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t  cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength  OPTIONAL
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strlen'.
    It is used to make sure a string is not larger than a given length, and
    it optionally returns the current length in characters not including
    the null terminator.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cchMax characters.

Arguments:

    psz         -   string to check the length of

    cchMax      -   maximum number of characters including the null terminator
                    that psz is allowed to contain

    pcch        -   if the function succeeds and pcch is non-null, the current length
                    in characters of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz)

Notes:
    psz can be null but the function will fail

    cchMax should be greater than zero or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cchMax characters

    failure        -   the operation did not succeed


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

__checkReturn
NTSTRSAFEDDI
RtlStringCchLengthA(
    __in STRSAFE_PCNZCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, pcchLength);
    }
    
    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}

__checkReturn
NTSTRSAFEDDI
RtlStringCchLengthW(
    __in STRSAFE_PCNZWCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, pcchLength);
    }
    
    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlStringCbLength(
    __in    LPCTSTR psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH * sizeof(TCHAR)) size_t  cbMax,
    __out_opt __deref_out_range(<, cbMax) size_t* pcbLength   OPTIONAL
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strlen'.
    It is used to make sure a string is not larger than a given length, and
    it optionally returns the current length in bytes not including
    the null terminator.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cbMax bytes.

Arguments:

    psz         -   string to check the length of

    cbMax       -   maximum number of bytes including the null terminator
                    that psz is allowed to contain

    pcb         -   if the function succeeds and pcb is non-null, the current length
                    in bytes of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz) * sizeof(TCHAR)

Notes:
    psz can be null but the function will fail

    cbMax should be greater than or equal to sizeof(TCHAR) or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cbMax bytes

    failure        -   the operation did not succeed


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

__checkReturn
NTSTRSAFEDDI
RtlStringCbLengthA(
    __in STRSAFE_PCNZCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbMax,
    __out_opt __deref_out_range(<, cbMax) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(char);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
             // safe to multiply cchLength * sizeof(char) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbLength = cchLength * sizeof(char);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}

__checkReturn
NTSTRSAFEDDI
RtlStringCbLengthW(
    __in STRSAFE_PCNZWCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
    __out_opt __deref_out_range(<, cbMax - 1) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(wchar_t);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
            // safe to multiply cchLength * sizeof(wchar_t) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbLength = cchLength * sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnalignedStringCchLength(
    __in LPCUTSTR    psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t  cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t*     pcchLength  OPTIONAL
    );

Routine Description:

    This routine is a version of RtlStringCchLength that accepts an unaligned string pointer.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cchMax characters.

Arguments:

    psz         -   string to check the length of

    cchMax      -   maximum number of characters including the null terminator
                    that psz is allowed to contain

    pcch        -   if the function succeeds and pcch is non-null, the current length
                    in characters of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz)

Notes:
    psz can be null but the function will fail

    cchMax should be greater than zero or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cchMax characters

    failure        -   the operation did not succeed


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

#ifdef ALIGNMENT_MACHINE
__checkReturn
NTSTRSAFEDDI
RtlUnalignedStringCchLengthW(
    __in STRSAFE_PCUNZWCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnalignedStringLengthWorkerW(psz, cchMax, pcchLength);
    }
    
    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}
#else
#define RtlUnalignedStringCchLengthW   RtlStringCchLengthW
#endif  // !ALIGNMENT_MACHINE
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS

#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnalignedStringCbLength(
    __in LPCUTSTR    psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH * sizeof(TCHAR)) size_t  cbMax,
    __out_opt __deref_out_range(<, cbMax) size_t*   pcbLength   OPTIONAL
    );

Routine Description:

    This routine is a version of RtlStringCbLength that accepts an unaligned string pointer.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string is non-null and the length including the null
    terminator is less than or equal to cbMax bytes.

Arguments:

    psz         -   string to check the length of

    cbMax       -   maximum number of bytes including the null terminator
                    that psz is allowed to contain

    pcb         -   if the function succeeds and pcb is non-null, the current length
                    in bytes of psz excluding the null terminator will be returned.
                    This out parameter is equivalent to the return value of strlen(psz) * sizeof(TCHAR)

Notes:
    psz can be null but the function will fail

    cbMax should be greater than or equal to sizeof(TCHAR) or the function will fail

Return Value:

    STATUS_SUCCESS -   psz is non-null and the length including the null
                       terminator is less than or equal to cbMax bytes

    failure        -   the operation did not succeed


    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

#ifdef ALIGNMENT_MACHINE
__checkReturn
NTSTRSAFEDDI
RtlUnalignedStringCbLengthW(
    __in STRSAFE_PCUNZWCH psz,
    __in __in_range(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
    __out_opt __deref_out_range(<, cbMax - 1) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(wchar_t);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnalignedStringLengthWorkerW(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
            // safe to multiply cchLength * sizeof(wchar_t) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbLength = cchLength * sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}
#else
#define RtlUnalignedStringCbLengthW    RtlStringCbLengthW
#endif  // !ALIGNMENT_MACHINE
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS

#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

/*++

NTSTATUS
RtlUnicodeStringInit(
    __out PUNICODE_STRING DestinationString,
    __in_opt NTSTRSAFE_PCWSTR pszSrc              OPTIONAL
    );

Routine Description:

    The RtlUnicodeStringInit function initializes a counted unicode string from
    pszSrc.

    This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the 
    counted unicode string was sucessfully initialized from pszSrc. In failure
    cases the unicode string buffer will be set to NULL, and the Length and 
    MaximumLength members will be set to zero.

Arguments:

    DestinationString - pointer to the counted unicode string to be initialized

    pszSrc            - source string which must be null or null terminated
    
Notes:
    DestinationString should not be NULL. See RtlUnicodeStringInitEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   

    failure        -   the operation did not succeed

      STATUS_INVALID_PARAMETER
                   -   this return value is an indication that the source string
                       was too large and DestinationString could not be initialized

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringInit(
    __out PUNICODE_STRING DestinationString,
    __in_opt NTSTRSAFE_PCWSTR pszSrc)
{
    return RtlUnicodeStringInitWorker(DestinationString,
                                      pszSrc,
                                      NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                      0);
}


/*++

NTSTATUS
RtlUnicodeStringInitEx(
    __out PUNICODE_STRING DestinationString,
    __in_opt  NTSTRSAFE_PCWSTR pszSrc              OPTIONAL,
    __in DWORD           dwFlags
    );

Routine Description:

    In addition to functionality provided by RtlUnicodeStringInit, this routine
    includes the flags parameter allows additional controls.

    This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the 
    counted unicode string was sucessfully initialized from pszSrc. In failure
    cases the unicode string buffer will be set to NULL, and the Length and 
    MaximumLength members will be set to zero.

Arguments:

    DestinationString - pointer to the counted unicode string to be initialized

    pszSrc            - source string which must be null terminated

    dwFlags           - controls some details of the initialization:

        STRSAFE_IGNORE_NULLS
                    do not fault on a NULL DestinationString pointer

Return Value:

    STATUS_SUCCESS -   

    failure        -   the operation did not succeed

      STATUS_INVALID_PARAMETER

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringInitEx(
    __out PUNICODE_STRING DestinationString,
    __in_opt NTSTRSAFE_PCWSTR pszSrc,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnicodeStringInitWorker(DestinationString,
                                            pszSrc,
                                            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                            dwFlags);
    }
    
    if (!NT_SUCCESS(status) && DestinationString)
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringValidate(
    __in PCUNICODE_STRING    SourceString
    );

Routine Description:

    The RtlUnicodeStringValidate function checks the counted unicode string to make
    sure that is is valid.

    This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the 
    counted unicode string is valid.

Arguments:

    SourceString   - pointer to the counted unicode string to be checked
    
Notes:
    SourceString should not be NULL. See RtlUnicodeStringValidateEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   SourceString is a valid counted unicode string

    failure        -   the operation did not succeed

      STATUS_INVALID_PARAMETER
                   -   this return value is an indication that SourceString is not a valid
                       counted unicode string

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringValidate(
    __in PCUNICODE_STRING SourceString)
{
    return RtlUnicodeStringValidateWorker(SourceString, NTSTRSAFE_UNICODE_STRING_MAX_CCH, 0);
}


/*++

NTSTATUS
RtlUnicodeStringValidateEx(
    __in PCUNICODE_STRING    SourceString     OPTIONAL,
    __in DWORD               dwFlags
    );

Routine Description:

    In addition to functionality provided by RtlUnicodeStringValidate, this routine
    includes the flags parameter allows additional controls.

    This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the 
    counted unicode string is valid.

Arguments:

    SourceString   - pointer to the counted unicode string to be checked

    dwFlags        - controls some details of the validation:

        STRSAFE_IGNORE_NULLS
                    allows SourceString to be NULL (will return STATUS_SUCCESS for this case).

Return Value:

    STATUS_SUCCESS -   SourceString is a valid counted unicode string

    failure        -   the operation did not succeed

      STATUS_INVALID_PARAMETER
                   -   this return value is an indication that the source string
                       is not a valide counted unicode string given the flags passed.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringValidateEx(
    __in PCUNICODE_STRING SourceString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    
    if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnicodeStringValidateWorker(SourceString, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);
    }

    return status;
}


/*++

NTSTATUS
RtlStringCchCopyUnicodeString(
    __out_ecount(cchDest) PWSTR               pszDest,
    __in size_t              cchDest,
    __in PCUNICODE_STRING    SourceString,
    );

Routine Description:

    This routine copies a PUNICODE_STRING to a PWSTR. This function will not
    write past the end of this buffer and it will ALWAYS null terminate the
    destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if the string was copied without truncation, otherwise it
    will return a failure code. In failure cases as much of SourceString will be
    copied to pszDest as possible.

Arguments:

    pszDest        -   destination string

    cchDest        -   size of destination buffer in characters.
                       length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
                       to hold all of the source and null terminate the string.
                       
    SourceString   -   pointer to the counted unicode source string
                       
Notes:
    Behavior is undefined if source and destination strings overlap.

    SourceString and pszDest should not be NULL.  See RtlStringCchCopyUnicodeStringEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCchCopyUnicodeString(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    
    status = RtlStringValidateDestW(pszDest,
                                    cchDest,
                                    NTSTRSAFE_UNICODE_STRING_MAX_CCH);
    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            status = RtlStringCopyWideCharArrayWorker(pszDest,
                                                      cchDest,
                                                      NULL,
                                                      pszSrc,
                                                      cchSrcLength);
        }
        else
        {
            *pszDest = L'\0';
        }
    }
    
    return status;
}


/*++

NTSTATUS
RtlStringCbCopyUnicodeString(
    __out_bcount(cbDest) PWSTR               pszDest,
    __in size_t              cbDest,
    __in PCUNICODE_STRING    SourceString,
    );

Routine Description:

    This routine copies a PUNICODE_STRING to a PWSTR. This function will not
    write past the end of this buffer and it will ALWAYS null terminate the
    destination buffer (unless it is zero length).

    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if the string was copied without truncation, otherwise it
    will return a failure code. In failure cases as much of SourceString will be
    copied to pszDest as possible.

Arguments:

    pszDest        -   destination string

    cbDest         -   size of destination buffer in bytes.
                       length must be = (DestinationString->Length + sizeof(wchar_t))
                       to hold all of the source and null terminate the string.
                       
    SourceString   -   pointer to the counted unicode source string
                       
Notes:
    Behavior is undefined if source and destination strings overlap.

    SourceString and pszDest should not be NULL.  See RtlStringCbCopyUnicodeStringEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result and is
                       null terminated. This is useful for situations where
                       truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCbCopyUnicodeString(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    
    status = RtlStringValidateDestW(pszDest,
                                    cchDest,
                                    NTSTRSAFE_UNICODE_STRING_MAX_CCH);
    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            status = RtlStringCopyWideCharArrayWorker(pszDest,
                                                      cchDest,
                                                      NULL,
                                                      pszSrc,
                                                      cchSrcLength);
        }
        else
        {
            // Suppress espx false positive - cchDest cannot be 0 here
#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)
            *pszDest = L'\0';
#pragma warning(pop)
        }
    }
    
    return status;
}


/*++

NTSTATUS
RtlStringCchCopyUnicodeStringEx(
    __out_ecount(cchDest) PWSTR               pszDest         OPTIONAL,
    __in size_t              cchDest,
    __in PCUNICODE_STRING    SourceString    OPTIONAL,
    __deref_opt_out_ecount(*pcchRemaining) PWSTR*              ppszDestEnd     OPTIONAL,
    __out_opt size_t*             pcchRemaining   OPTIONAL,
    __in DWORD               dwFlags
    );

Routine Description:

    This routine copies a PUNICODE_STRING to a PWSTR. In addition to
    functionality provided by RtlStringCchCopyUnicodeString, this routine also
    returns a pointer to the end of the destination string and the number of
    characters left in the destination string including the null terminator.
    The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
                        to hold all of the source and null terminate the string.
                       
    SourceString    -   pointer to the counted unicode source string
    
    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcchRemaining   -   if pcchRemaining is non-null, the function will return the
                        number of characters left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.
                       
Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCchCopyUnicodeStringEx(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __in PCUNICODE_STRING SourceString,
    __deref_opt_out_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    
    status = RtlStringExValidateDestW(pszDest,
                                      cchDest,
                                      NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                      dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;
                
                status = RtlStringCopyWideCharArrayWorker(pszDest,
                                                          cchDest,
                                                          &cchCopied,
                                                          pszSrc,
                                                          cchSrcLength);
                                                            
                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;
                
                if (NT_SUCCESS(status)                           &&
                    (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                    (cchRemaining > 1))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }
        
        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);
            
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }   
    }
    
    return status;
}


/*++

NTSTATUS
RtlStringCbCopyUnicodeStringEx(
    __out_bcount(cbDest) PWSTR               pszDest         OPTIONAL,
    __in size_t              cbDest,
    __in PCUNICODE_STRING    SourceString    OPTIONAL,
    __deref_opt_out_bcount(*pcbRemaining) PWSTR*              ppszDestEnd     OPTIONAL,
    __out_opt size_t*             pcbRemaining    OPTIONAL,
    __in DWORD               dwFlags
    );

Routine Description:

    This routine copies a PUNICODE_STRING to a PWSTR. In addition to
    functionality provided by RtlStringCbCopyUnicodeString, this routine also
    returns a pointer to the end of the destination string and the number of
    characters left in the destination string including the null terminator.
    The flags parameter allows additional controls.

Arguments:

    pszDest         -   destination string

    cchDest         -   size of destination buffer in characters.
                        length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
                        to hold all of the source and null terminate the string.
                       
    SourceString    -   pointer to the counted unicode source string
    
    ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
                        pointer to the end of the destination string.  If the
                        function copied any data, the result will point to the
                        null termination character

    pcbRemaining    -   pcbRemaining is non-null,the function will return the
                        number of bytes left in the destination string,
                        including the null terminator

    dwFlags         -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND_NULL
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer
                    behind the null terminator

        STRSAFE_IGNORE_NULLS
                    treat NULL string pointers like empty strings (TEXT("")).
                    this flag is useful for emulating functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer, and it will
                    be null terminated. This will overwrite any truncated
                    string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_NULL_ON_FAILURE
                    if the function fails, the destination buffer will be set
                    to the empty string. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.
                       
Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied and the
                       resultant dest string was null terminated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlStringCbCopyUnicodeStringEx(
    __out_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cbDest,
    __in PCUNICODE_STRING SourceString,
    __deref_opt_out_bcount(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out_opt size_t* pcbRemaining,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    
    status = RtlStringExValidateDestW(pszDest,
                                      cchDest,
                                      NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                      dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
                
                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;
                
                status = RtlStringCopyWideCharArrayWorker(pszDest,
                                                          cchDest,
                                                          &cchCopied,
                                                          pszSrc,
                                                          cchSrcLength);
                                                            
                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;
                
                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;
                    
                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }
        
        if (!NT_SUCCESS(status)                                                                              &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
            (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                                      cbDest,
                                      0,
                                      &pszDestEnd,
                                      &cchRemaining,
                                      dwFlags);
        }
                                     
        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }
            
            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }
    
    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCopyString(
    __out PUNICODE_STRING DestinationString,
    __in  LPCTSTR         pszSrc
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' for
    UNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation, otherwise it
    will return a failure code. In failure cases as much of pszSrc will be
    copied to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCopyStringEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyString(
    __out PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;
        
        status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                  cchDest,
                                                  &cchNewDestLength,
                                                  pszSrc,
                                                  NTSTRSAFE_UNICODE_STRING_MAX_CCH);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCopy(
    __out PUNICODE_STRING    DestinationString,
    __in PCUNICODE_STRING    SourceString
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' for
    UNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was copied without truncation, otherwise it
    will return a failure code. In failure cases as much of SourceString
    will be copied to Dest as possible.

Arguments:

    DestinationString   - pointer to the counted unicode destination string

    SourceString        -  pointer to the counted unicode source string

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL.  See RtlUnicodeStringCopyEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopy(
    __out PUNICODE_STRING DestinationString, 
    __in PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            status = RtlWideCharArrayCopyWorker(pszDest,
                                                cchDest,
                                                &cchNewDestLength,
                                                pszSrc,
                                                cchSrcLength);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCopyStringEx(
    __out PUNICODE_STRING DestinationString   OPTIONAL,
    __in  LPCTSTR         pszSrc              OPTIONAL,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in  DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' for
    UNICODE_STRINGs with some additional parameters. In addition to the
    functionality provided by RtlUnicodeStringCopyString, this routine also
    returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string which must be null terminated

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored.

    Behavior is undefined if DestinationString and RemainingString are the same pointer.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyStringEx(
    __out PUNICODE_STRING DestinationString, 
    __in NTSTRSAFE_PCWSTR pszSrc,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {        
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                          cchDest,
                                                          &cchNewDestLength,
                                                          pszSrc,
                                                          NTSTRSAFE_UNICODE_STRING_MAX_CCH);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        
        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCopyEx(
    __out PUNICODE_STRING     DestinationString   OPTIONAL,
    __in  PCUNICODE_STRING    SourceString        OPTIONAL,
    __out_opt PUNICODE_STRING     RemainingString     OPTIONAL,
    __in  DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcpy' for
    UNICODE_STRINGs with some additional parameters. In addition to the
    functionality provided by RtlUnicodeStringCopy, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string
    
    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored.

    Behavior is undefined if DestinationString and RemainingString are the same pointer.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyEx(
    __out PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest; 
        size_t cchNewDestLength = 0;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyWorker(pszDest,
                                                    cchDest,
                                                    &cchNewDestLength,
                                                    pszSrc,
                                                    cchSrcLength);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCopyStringN(
    __out PUNICODE_STRING DestinationString,
    __in LPCTSTR         pszSrc,
    __in size_t          cchToCopy
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cchToCopy characters were
    copied without truncation, otherwise it will return a failure code. In
    failure cases as much of pszSrc will be copied to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cchToCopy           -   maximum number of characters to copy from source string,
                            not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCchCopyStringNEx if
    you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyStringN(
    __out PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;

        if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                      cchDest,
                                                      &cchNewDestLength,
                                                      pszSrc,
                                                      cchToCopy);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCopyStringN(
    __out PUNICODE_STRING DestinationString,
    __in LPCTSTR         pszSrc,
    __in size_t          cbToCopy
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cbToCopy bytes were
    copied without truncation, otherwise it will return a failure code. In
    failure cases as much of pszSrc will be copied to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cbToCopy            -   maximum number of bytes to copy from source string,
                            not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCopyCbStringEx if you require
    the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok
                       
    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyStringN(
    __out PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cbToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                      cchDest,
                                                      &cchNewDestLength,
                                                      pszSrc,
                                                      cchToCopy);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCopyN(
    __out PUNICODE_STRING     DestinationString,
    __in  PCUNICODE_STRING    SourceString,
    __in  size_t              cchToCopy
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cchToCopy characters were
    copied without truncation, otherwise it will return a failure code. In
    failure cases as much of SourceString will be copied to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string
    
    cchToCopy           -   maximum number of characters to copy from source string,
                            not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL. See RtlUnicodeStringCchCopyNEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyN(
    __out PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cchToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest,
                                                    cchDest,
                                                    &cchNewDestLength,
                                                    pszSrc,
                                                    cchToCopy);
            }
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCopyN(
    __out PUNICODE_STRING     DestinationString,
    __in  PCUNICODE_STRING    SourceString,
    __in  size_t              cbToCopy
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the entire string or the first cbToCopy bytes were
    copied without truncation, otherwise it will return a failure code. In
    failure cases as much of SourceString will be copied to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cbToCopy            -   maximum number of bytes to copy from source string,
                            not including the null terminator.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL.  See RtlUnicodeStringCbCopyNEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok
                       
    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/


NTSTRSAFEDDI
RtlUnicodeStringCbCopyN(
    __out PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cbToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            size_t cchToCopy = cbToCopy / sizeof(wchar_t);

            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest,
                                                    cchDest,
                                                    &cchNewDestLength,
                                                    pszSrc,
                                                    cchToCopy);
            }
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCopyStringNEx(
    __out PUNICODE_STRING DestinationString   OPTIONAL,
    __in LPCTSTR         pszSrc              OPTIONAL,
    __in size_t          cchToCopy,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCchCopyStringN, this routine also
    returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cchToCopy           -   maximum number of characters to copy from source string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyStringNEx(
    __out PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToCopy,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                          cchDest,
                                                          &cchNewDestLength,
                                                          pszSrc,
                                                          cchToCopy);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCopyStringNEx(
    __out PUNICODE_STRING DestinationString   OPTIONAL,
    __in LPCTSTR         pszSrc              OPTIONAL,
    __in size_t          cbToCopy,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCbCopyStringN, this routine also
    returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cbToCopy            -   maximum number of bytes to copy from source string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.


Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyStringNEx(
    __out PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cbToCopy,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                                                          cchDest,
                                                          &cchNewDestLength,
                                                          pszSrc,
                                                          cchToCopy);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCopyNEx(
    __out PUNICODE_STRING     DestinationString   OPTIONAL,
    __in  PCUNICODE_STRING    SourceString        OPTIONAL,
    __in  size_t              cchToCopy,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in  DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCchCopyN, this
    routine also returns a PUNICODE_STRING which points to the end of the
    destination string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string
    
    SourceString        -   pointer to the counted unicode source string

    cchToCopy           -   maximum number of characters to copy from source string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL SourceString like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.
Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied
    
    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyNEx(
    __out PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cchToCopy,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchDest == 0)
                {
                    // only fail if there was actually src data to copy
                    if (cchToCopy != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    status = RtlWideCharArrayCopyWorker(pszDest,
                                                        cchDest,
                                                        &cchNewDestLength,
                                                        pszSrc,
                                                        cchToCopy);

                    pszDestEnd = pszDest + cchNewDestLength;
                    cchRemaining = cchDest - cchNewDestLength;
                    
                    if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCopyNEx(
    __out PUNICODE_STRING     DestinationString   OPTIONAL,
    __in  PCUNICODE_STRING    SourceString        OPTIONAL,
    __in  size_t              cbToCopy,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in  DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncpy' with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCbCopyN, this
    routine also returns a PUNICODE_STRING which points to the end of the
    destination string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cbToCopy            -   maximum number of bytes to copy from source string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL SourceString like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all copied

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the copy
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyNEx(
    __out PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cbToCopy,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            size_t cchToCopy = cbToCopy / sizeof(wchar_t);

            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchDest == 0)
                {
                    // only fail if there was actually src data to copy
                    if (cchToCopy != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    status = RtlWideCharArrayCopyWorker(pszDest,
                                                        cchDest,
                                                        &cchNewDestLength,
                                                        pszSrc,
                                                        cchToCopy);

                    pszDestEnd = pszDest + cchNewDestLength;
                    cchRemaining = cchDest - cchNewDestLength;
                    
                    if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


/*++

NTSTATUS
RtlUnicodeStringCatString(
    __inout  PUNICODE_STRING DestinationString,
    __in     LPCTSTR         pszSrc
    );

 Routine Description:

    This routine is a safer version of the C built-in function 'strcat' for
    UNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation, otherwise
    it will return a failure code. In failure cases as much of pszSrc will be
    appended to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string which must be null terminated

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCatStringEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this 
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatString(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchCopied = 0;

        status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                                                  cchDest - cchDestLength,
                                                  &cchCopied,
                                                  pszSrc,
                                                  NTSTRSAFE_UNICODE_STRING_MAX_CCH);

        // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCat(
    __inout  PUNICODE_STRING     DestinationString,
    __in     PCUNICODE_STRING    SourceString
    );
    
 Routine Description:

    This routine is a safer version of the C built-in function 'strcat' for
    UNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer.  It returns
    STATUS_SUCCESS if the string was concatenated without truncation, otherwise
    it will return a failure code. In failure cases as much of SourceString will be
    appended to DestinationString as possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCatEx
    if you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this 
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCat(
    __inout PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        
        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                                                cchDest - cchDestLength,
                                                &cchCopied,
                                                pszSrc,
                                                cchSrcLength);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCatStringEx(
    __inout PUNICODE_STRING DestinationString   OPTTONAL,
    __in      LPCTSTR         pszSrc              OPTIONAL,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in      DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' for
    PUNICODE_STRINGs with some additional parameters.  In addition to the
    functionality provided by RtlUnicodeStringCatString, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string which must be null terminated

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:
    
        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap or if
    DestinationString and RemainingString are the same pointer.

    DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatStringEx(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;
                
                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                                                          cchRemaining,
                                                          &cchCopied,
                                                          pszSrc,
                                                          NTSTRSAFE_UNICODE_STRING_MAX_CCH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;
                
                cchNewDestLength = cchDestLength + cchCopied;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringCatEx(
    __inout   PUNICODE_STRING     DestinationString   OPTIONAL,
    __in      PCUNICODE_STRING    SourceString        OPTIONAL,
    __out_opt PUNICODE_STRING     RemainingString     OPTIONAL,
    __in      DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strcat' for
    PUNICODE_STRINGs with some additional parameters. In addition to the
    functionality provided by RtlUnicodeStringCat, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap or if
    DestinationString and RemainingString are the same pointer.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was source data and it was all concatenated

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.
                       
    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatEx(
    __inout PUNICODE_STRING DestinationString, 
    __in PCUNICODE_STRING SourceString,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;
                
                status = RtlWideCharArrayCopyWorker(pszDestEnd,
                                                    cchRemaining,
                                                    &cchCopied,
                                                    pszSrc,
                                                    cchSrcLength);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;
                
                cchNewDestLength = cchDestLength + cchCopied;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCatStringN(
    __inout  PUNICODE_STRING DestinationString,
    __in     LPCTSTR         pszSrc,
    __in     size_t          cchToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat' for
    PUNICODE_STRINGs.
    
    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if all of pszSrc or the first cchToAppend characters were
    appended to the destination string, otherwise it will return a failure
    code. In failure cases as much of pszSrc will be appended to DestinationString as
    possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cchToAppend         -   maximum number of characters to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCchCatStringNEx if
    you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters were
                       concatenated to DestinationString

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok
                       
    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatStringN(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                                                      cchDest - cchDestLength,
                                                      &cchCopied,
                                                      pszSrc,
                                                      cchToAppend);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCatStringN(
    __inout   PUNICODE_STRING DestinationString,
    __in      LPCTSTR         pszSrc,
    __in      size_t          cbToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat' for
    PUNICODE_STRINGs.
    
    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if all of pszSrc or the first cbToAppend bytes were
    appended to the destination string, otherwise it will return a failure
    code. In failure cases as much of pszSrc will be appended to DestinationString as
    possible.
    
Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cbToAppend          -   maximum number of bytes to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCbCatStringNEx if
    you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
                       concatenated to pszDest
                       
    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatStringN(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cbToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                                                      cchDest - cchDestLength,
                                                      &cchCopied,
                                                      pszSrc,
                                                      cchToAppend);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCatN(
    __inout  PUNICODE_STRING     DestinationString,
    __in     PCUNICODE_STRING    SourceString,
    __in     size_t              cchToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat' for
    PUNICODE_STRINGs.
    
    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if all of SourceString or the first cchToAppend characters were
    appended to the destination string, otherwise it will return a failure
    code. In failure cases as much of SourceString will be appended to DestinationString as
    possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cchToAppend         -   maximum number of characters to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL. See RtlUnicodeStringCchCatNEx if
    you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of SourceString or the first cchToAppend characters were
                       concatenated to DestinationString

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatN(
    __inout PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cchToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                size_t cchCopied = 0;

                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                                                    cchDest - cchDestLength,
                                                    &cchCopied,
                                                    pszSrc,
                                                    cchToAppend);

                // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCatN(
    __inout PUNICODE_STRING     DestinationString,
    __in    PCUNICODE_STRING    SourceString,
    __in    size_t              cbToAppend
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat' for
    PUNICODE_STRINGs.
    
    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if all of SourceString or the first cbToAppend bytes were
    appended to the destination string, otherwise it will return a failure
    code. In failure cases as much of SourceString will be appended to DestinationString as
    possible.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cbToAppend          -   maximum number of bytes to append

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL. See RtlUnicodeStringCbCatNEx if
    you require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if all of SourceString or the first cbToAppend bytes were
                       concatenated to pszDest

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok
                       
    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatN(
    __inout PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cbToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   0);

        if (NT_SUCCESS(status))
        {
            size_t cchToAppend = cbToAppend / sizeof(wchar_t);

            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                size_t cchCopied = 0;

                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                                                    cchDest - cchDestLength,
                                                    &cchCopied,
                                                    pszSrc,
                                                    cchToAppend);

                // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCatStringNEx(
    __inout PUNICODE_STRING DestinationString   OPTIONAL,
    __in    LPCTSTR         pszSrc              OPTIONAL,
    __in    size_t          cchToAppend,
    __out_opt PUNICODE_STRING RemainingString     OPTIONAL,
    __in    DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCchCatStringN, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cchToAppend         -   maximum number of characters to append

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters were
                       concatenated to DestinationString

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatStringNEx(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToAppend,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                                                          cchRemaining,
                                                          &cchCopied,
                                                          pszSrc,
                                                          cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCatStringNEx(
    __inout                PUNICODE_STRING DestinationString   OPTIONAL,
    __in   LPCTSTR         pszSrc              OPTIONAL,
    __in                   size_t          cbToAppend,
    __out_opt              PUNICODE_STRING RemainingString     OPTIONAL,
    __in                   DWORD           dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCbCatStringN, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.
    
Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    pszSrc              -   source string

    cbToAppend          -   maximum number of bytes to append

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszSrc like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
                       concatenated to pszDest

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatStringNEx(
    __inout PUNICODE_STRING DestinationString,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cbToAppend,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);
        
        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;
                
                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                                                          cchRemaining,
                                                          &cchCopied,
                                                          pszSrc,
                                                          cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCchCatNEx(
    __inout   PUNICODE_STRING     DestinationString   OPTIONAL,
    __in      PCUNICODE_STRING    SourceString        OPTIONAL,
    __in      size_t              cchToAppend,
    __out_opt PUNICODE_STRING     RemainingString     OPTIONAL,
    __in      DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCchCatN, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cchToAppend         -   maximum number of characters to append

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL SourceString like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of SourceString or the first cchToAppend characters were
                       concatenated to DestinationString

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatNEx(
    __inout   PUNICODE_STRING  DestinationString,
    __in      PCUNICODE_STRING SourceString,
    __in      size_t           cchToAppend,
    __out_opt PUNICODE_STRING  RemainingString,
    __in      DWORD            dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchRemaining == 0)
                {
                    // only fail if there was actually src data to append
                    if (cchToAppend != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    size_t cchCopied = 0;
                    
                    status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                                                              cchRemaining,
                                                              &cchCopied,
                                                              pszSrc,
                                                              cchToAppend);

                    pszDestEnd = pszDestEnd + cchCopied;
                    cchRemaining = cchRemaining - cchCopied;

                    cchNewDestLength = cchDestLength + cchCopied;
                    
                    if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

NTSTATUS
RtlUnicodeStringCbCatNEx(
    __inout   PUNICODE_STRING     DestinationString   OPTIONAL,
    __in      PCUNICODE_STRING    SourceString        OPTIONAL,
    __in      size_t              cbToAppend,
    __out_opt PUNICODE_STRING     RemainingString     OPTIONAL,
    __in      DWORD               dwFlags
    );

Routine Description:

    This routine is a safer version of the C built-in function 'strncat', with
    some additional parameters and for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringCbCatN, this routine
    also returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    SourceString        -   pointer to the counted unicode source string

    cbToAppend          -   maximum number of bytes to append

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL SourceString like
                    empty strings (L""). This flag is useful for emulating
                    functions like lstrcpy
                    
        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

        STRSAFE_NO_TRUNCATION
                    if the function returns STATUS_BUFFER_OVERFLOW, pszDest
                    will not contain a truncated string, it will remain unchanged.

Notes:
    Behavior is undefined if source and destination strings overlap.

    DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
    is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
    may be NULL.  An error may still be returned even though NULLS are ignored
    due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if all of SourceString or the first cbToAppend bytes were
                       concatenated to DestinationString

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the 
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result.
                       This is useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatNEx(
    __inout PUNICODE_STRING DestinationString,
    __in PCUNICODE_STRING SourceString,
    __in size_t cbToAppend,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                &cchDestLength,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                                                   &pszSrc,
                                                   &cchSrcLength,
                                                   NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                   dwFlags);

        if (NT_SUCCESS(status))
        {
            size_t cchToAppend = cbToAppend / sizeof(wchar_t);

            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchRemaining == 0)
                {
                    // only fail if there was actually src data to append
                    if (cchToAppend != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    size_t cchCopied = 0;
                    
                    status = RtlWideCharArrayCopyWorker(pszDestEnd,
                                                        cchRemaining,
                                                        &cchCopied,
                                                        pszSrc,
                                                        cchToAppend);

                    pszDestEnd = pszDestEnd + cchCopied;
                    cchRemaining = cchRemaining - cchCopied;

                    cchNewDestLength = cchDestLength + cchCopied;
                    
                    if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               cchDestLength,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


/*++

NTSTATUS
RtlUnicodeStringVPrintf(
    __out                 PUNICODE_STRING DestinationString,
    __in __format_string  PCWSTR          pszFormat,
    __in                  va_list         argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if the string was printed without truncation, otherwise it
    will return a failure code. In failure cases it will return a truncated
    version of the ideal result.

Arguments:

    DestinationString   -  pointer to the counted unicode destination string

    pszFormat           -  format string which must be null terminated

    argList             -  va_list from the variable arguments according to the
                           stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    DestinationString and pszFormat should not be NULL. See RtlUnicodeStringVPrintfEx if you
    require the handling of NULL values.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringVPrintf(
    __out PUNICODE_STRING DestinationString, 
    __in __format_string NTSTRSAFE_PCWSTR pszFormat, 
    __in va_list argList)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;

        status = RtlWideCharArrayVPrintfWorker(pszDest,
                                               cchDest,
                                               &cchNewDestLength,
                                               pszFormat,
                                               argList);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringVPrintfEx(
    __out                PUNICODE_STRING DestinationString   OPTIONAL,
    __out_opt            PUNICODE_STRING RemainingString     OPTIONAL,
    __in                 DWORD   dwFlags,
    __in __format_string PCWSTR  pszFormat                   OPTIONAL,
    __in                 va_list argList
    );

Routine Description:

    This routine is a safer version of the C built-in function 'vsprintf' with
    some additional parameters for PUNICODE_STRING. In addition to the
    functionality provided by RtlUnicodeStringVPrintf, this routine also
    returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.
    
Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszFormat like
                    empty strings (L"").

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat           -   format string which must be null terminated

    argList             -   va_list from the variable arguments according to the
                            stdarg.h convention

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    DestinationString and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringVPrintfEx(
    __out PUNICODE_STRING DestinationString, 
    __out_opt PUNICODE_STRING RemainingString, 
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat, 
    __in va_list argList)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayVPrintfWorker(pszDest,
                                                       cchDest,
                                                       &cchNewDestLength,
                                                       pszFormat,
                                                       argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef _M_CEE_PURE

/*++

NTSTATUS
RtlUnicodeStringPrintf(
    __out                PUNICODE_STRING DestinationString,
    __in __format_string PCWSTR          pszFormat,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' for
    PUNICODE_STRINGs.

    This function returns an NTSTATUS value, and not a pointer. It returns
    STATUS_SUCCESS if the string was printed without truncation, otherwise it
    will return a failure code. In failure cases it will return a truncated
    version of the ideal result.

Arguments:

    DestinationString   -  pointer to the counted unicode destination string

    pszFormat           -  format string which must be null terminated

    ...                 -  additional parameters to be formatted according to
                           the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    DestinationString and pszFormat should not be NULL.  See RtlUnicodeStringPrintfEx if you
    require the handling of NULL values.
    
Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string

    failure        -   the operation did not succeed

      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringPrintf(
    __out PUNICODE_STRING DestinationString,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    
    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                0);

    if (NT_SUCCESS(status))
    {
        va_list argList;
        size_t cchNewDestLength = 0;

        va_start(argList, pszFormat);

        status = RtlWideCharArrayVPrintfWorker(pszDest,
                                               cchDest,
                                               &cchNewDestLength,
                                               pszFormat,
                                               argList);

        va_end(argList);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

NTSTATUS
RtlUnicodeStringPrintfEx(
    __out                PUNICODE_STRING DestinationString   OPTIONAL,
    __out_opt            PUNICODE_STRING RemainingString     OPTIONAL,
    __in                 DWORD           dwFlags,
    __in __format_string PCWSTR          pszFormat           OPTIONAL,
    ...
    );

Routine Description:

    This routine is a safer version of the C built-in function 'sprintf' with
    some additional parameters for PUNICODE_STRINGs. In addition to the
    functionality provided by RtlUnicodeStringPrintf, this routine also
    returns a PUNICODE_STRING which points to the end of the destination
    string. The flags parameter allows additional controls.

Arguments:

    DestinationString   -   pointer to the counted unicode destination string

    RemainingString     -   if RemainingString is non-null, the function will format
                            the pointer with the remaining buffer and number of
                            bytes left in the destination string

    dwFlags             -   controls some details of the string copy:

        STRSAFE_FILL_BEHIND
                    if the function succeeds, the low byte of dwFlags will be
                    used to fill the uninitialize part of destination buffer

        STRSAFE_IGNORE_NULLS
                    do not fault if DestinationString is null and treat NULL pszFormat like
                    empty strings (L"").

        STRSAFE_FILL_ON_FAILURE
                    if the function fails, the low byte of dwFlags will be
                    used to fill all of the destination buffer. This will
                    overwrite any truncated string returned when the failure is
                    STATUS_BUFFER_OVERFLOW

        STRSAFE_NO_TRUNCATION /
        STRSAFE_ZERO_LENGTH_ON_FAILURE
                    if the function fails, the destination Length will be set
                    to zero. This will overwrite any truncated string
                    returned when the failure is STATUS_BUFFER_OVERFLOW.

    pszFormat           -   format string which must be null terminated

    ...                 -   additional parameters to be formatted according to
                            the format string

Notes:
    Behavior is undefined if destination, format strings or any arguments
    strings overlap.

    DestinationString and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
    flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and
    pszFormat may be NULL.  An error may still be returned even though NULLS
    are ignored due to insufficient space.

Return Value:

    STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
                       the resultant string

    failure        -   the operation did not succeed


      STATUS_BUFFER_OVERFLOW
      Note: This status has the severity class Warning - IRPs completed with this
            status do have their data copied back to user mode
                   -   this return value is an indication that the print
                       operation failed due to insufficient space. When this
                       error occurs, the destination buffer is modified to
                       contain a truncated version of the ideal result. This is
                       useful for situations where truncation is ok.

    It is strongly recommended to use the NT_SUCCESS() macro to test the
    return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringPrintfEx(
    __out PUNICODE_STRING DestinationString,
    __out_opt PUNICODE_STRING RemainingString,
    __in DWORD dwFlags,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    ...)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
                                                &pszDest,
                                                &cchDest,
                                                NULL,
                                                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                                                dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                va_list argList;

                va_start(argList, pszFormat);
                
                status = RtlWideCharArrayVPrintfWorker(pszDest,
                                                       cchDest,
                                                       &cchNewDestLength,
                                                       pszFormat,
                                                       argList);

                va_end(argList);
                
                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;
                
                if (NT_SUCCESS(status)              &&
                    (dwFlags & STRSAFE_FILL_BEHIND) &&
                    (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
            (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
            (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                                               cchDest,
                                               0,
                                               &cchNewDestLength,
                                               &pszDestEnd,
                                               &cchRemaining,
                                               dwFlags);
        }
        
        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {   
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}

#endif  // !_M_CEE_PURE

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

#endif  // !NTSTRSAFE_LIB_IMPL


// Below here are the worker functions that actually do the work

#if defined(NTSTRSAFE_LIB_IMPL) || !defined(NTSTRSAFE_LIB)

NTSTRSAFEWORKERDDI
RtlStringLengthWorkerA(
    __in STRSAFE_PCNZCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlStringLengthWorkerW(
    __in STRSAFE_PCNZWCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}

#ifdef ALIGNMENT_MACHINE
NTSTRSAFEWORKERDDI
RtlUnalignedStringLengthWorkerW(
    __in STRSAFE_PCUNZWCH psz,
    __in __in_range(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    __out_opt __deref_out_range(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}
#endif  // ALIGNMENT_MACHINE

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_RETURNING_BAD_RESULT)

NTSTRSAFEWORKERDDI
RtlStringExValidateSrcA(
    __deref_in_opt_out NTSTRSAFE_PCSTR* ppszSrc,
    __inout_opt __deref_out_range(<, cchMax) size_t* pcchToRead,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pcchToRead && (*pcchToRead >= cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if ((dwFlags & STRSAFE_IGNORE_NULLS) && (*ppszSrc == NULL))
    {
        *ppszSrc = "";

        if (pcchToRead)
        {
            *pcchToRead = 0;
        }
    }
    
    return status;
}

NTSTRSAFEWORKERDDI
RtlStringExValidateSrcW(
    __deref_in_opt_out NTSTRSAFE_PCWSTR* ppszSrc,
    __inout_opt __deref_out_range(<, cchMax) size_t* pcchToRead,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pcchToRead && (*pcchToRead >= cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if ((dwFlags & STRSAFE_IGNORE_NULLS) && (*ppszSrc == NULL))
    {
        *ppszSrc = L"";
        
        if (pcchToRead)
        {
            *pcchToRead = 0;
        }
    }

    return status;
}

#pragma warning(pop)    // allow null deref

#pragma warning(push)
#pragma warning(disable : 4100) // Unused parameter (pszDest)
NTSTRSAFEWORKERDDI
RtlStringValidateDestA(
    __in_ecount_opt(cchDest) STRSAFE_PCNZCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax)
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((cchDest == 0) || (cchDest > cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}
#pragma warning(pop)

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthA(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, cchMax);

    if (NT_SUCCESS(status))
    {
        status = RtlStringLengthWorkerA(pszDest, cchDest, pcchDestLength);
    }
    else
    {
        *pcchDestLength = 0;
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4100) // Unused parameter (pszDest)
NTSTRSAFEWORKERDDI
RtlStringValidateDestW(
    __in_ecount_opt(cchDest) STRSAFE_PCNZWCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax)
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((cchDest == 0) || (cchDest > cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}
#pragma warning(pop)

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthW(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCWSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, cchMax);

    if (NT_SUCCESS(status))
    {
        status = RtlStringLengthWorkerW(pszDest, cchDest, pcchDestLength);
    }
    else
    {
        *pcchDestLength = 0;
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)

NTSTRSAFEWORKERDDI
RtlStringExValidateDestA(
    __in_ecount_opt(cchDest) STRSAFE_PCNZCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        if (((pszDest == NULL) && (cchDest != 0))   ||
            (cchDest > cchMax))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = RtlStringValidateDestA(pszDest, cchDest, cchMax);
    }

    return status;
}

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthA(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        status = RtlStringExValidateDestA(pszDest, cchDest, cchMax, dwFlags);

        if (!NT_SUCCESS(status) || (cchDest == 0))
        {
            *pcchDestLength = 0;
        }
        else
        {
            status = RtlStringLengthWorkerA(pszDest, cchDest, pcchDestLength);
        }
    }
    else
    {
        status = RtlStringValidateDestAndLengthA(pszDest,
                                          cchDest,
                                          pcchDestLength,
                                          cchMax);
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)

NTSTRSAFEWORKERDDI
RtlStringExValidateDestW(
    __in_ecount_opt(cchDest) STRSAFE_PCNZWCH pszDest,
    __in size_t cchDest,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        if (((pszDest == NULL) && (cchDest != 0))   ||
            (cchDest > cchMax))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = RtlStringValidateDestW(pszDest, cchDest, cchMax);
    }

    return status;
}

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthW(
    __in_ecount_opt(cchDest) NTSTRSAFE_PCWSTR pszDest,
    __in size_t cchDest,
    __out __deref_out_range(<, cchDest) size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status;
    
    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        status = RtlStringExValidateDestW(pszDest, cchDest, cchMax, dwFlags);

        if (!NT_SUCCESS(status) || (cchDest == 0))
        {
            *pcchDestLength = 0;
        }
        else
        {
            status = RtlStringLengthWorkerW(pszDest, cchDest, pcchDestLength);
        }
    }
    else
    {
        status = RtlStringValidateDestAndLengthW(pszDest,
                                          cchDest,
                                          pcchDestLength,
                                          cchMax);
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)

NTSTRSAFEWORKERDDI
RtlStringCopyWorkerA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, (cchToCopy < cchDest) ? cchToCopy : cchDest - 1) size_t* pcchNewDestLength,
    __in_xcount(cchToCopy) STRSAFE_PCNZCH pszSrc,
    __in __in_range(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;
    
    // ASSERT(cchDest != 0);

    while (cchDest && cchToCopy && (*pszSrc != '\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = '\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlStringCopyWorkerW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, (cchToCopy < cchDest) ? cchToCopy : cchDest - 1) size_t* pcchNewDestLength,
    __in_xcount(cchToCopy) STRSAFE_PCNZWCH pszSrc,
    __in __in_range(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;
    
    // ASSERT(cchDest != 0);

    while (cchDest && cchToCopy && (*pszSrc != L'\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = L'\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerA(
    __out_ecount(cchDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, cchDest - 1) size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    size_t cchMax;
    size_t cchNewDestLength = 0;

    // leave the last space for the null terminator
    cchMax = cchDest - 1;

#if (NTSTRSAFE_USE_SECURE_CRT == 1) && !defined(NTSTRSAFE_LIB_IMPL)
    iRet = _vsnprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
    #pragma warning(push)
    #pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
    #pragma warning(pop)
#endif
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchMax))
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (((size_t)iRet) == cchMax)
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;
    }
    else
    {
        cchNewDestLength = (size_t)iRet;
    }

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerW(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    __out_opt __deref_out_range(<=, cchDest - 1) size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    size_t cchMax;
    size_t cchNewDestLength = 0;

    // leave the last space for the null terminator
    cchMax = cchDest - 1;

#if (NTSTRSAFE_USE_SECURE_CRT == 1) && !defined(NTSTRSAFE_LIB_IMPL)
    iRet = _vsnwprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
    #pragma warning(push)
    #pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
    #pragma warning(pop)
#endif
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchMax))
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = L'\0';

        cchNewDestLength = cchMax;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (((size_t)iRet) == cchMax)
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = L'\0';

        cchNewDestLength = cchMax;
    }
    else
    {
        cchNewDestLength = (size_t)iRet;
    }

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}


NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullA(
    __inout_bcount(cbRemaining) NTSTRSAFE_PSTR pszDestEnd,
    __in size_t cbRemaining,
    __in DWORD dwFlags)
{
    if (cbRemaining > sizeof(char))
    {
        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining - sizeof(char));
    }
    
    return STATUS_SUCCESS;
}

NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullW(
    __inout_bcount(cbRemaining) NTSTRSAFE_PWSTR pszDestEnd,
    __in size_t cbRemaining,
    __in DWORD dwFlags)
{
    if (cbRemaining > sizeof(wchar_t))
    {
        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining - sizeof(wchar_t));
    }

    return STATUS_SUCCESS;
}

NTSTRSAFEWORKERDDI
RtlStringExHandleOtherFlagsA(
    __inout_bcount(cbDest) NTSTRSAFE_PSTR pszDest,
    __in __in_range(sizeof(char), NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbDest,
    __in __in_range(<, cbDest / sizeof(char)) size_t cchOriginalDestLength,
    __deref_inout_ecount(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
    __out __deref_out_range(<=, cbDest / sizeof(char)) size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    size_t cchDest = cbDest / sizeof(char);
    
    if ((cchDest > 0) && (dwFlags & STRSAFE_NO_TRUNCATION))
    {
        char* pszOriginalDestEnd;

        pszOriginalDestEnd = pszDest + cchOriginalDestLength;

        *ppszDestEnd = pszOriginalDestEnd;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        // null terminate the end of the original string
        *pszOriginalDestEnd = '\0';
    }

    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
        {
            *ppszDestEnd = pszDest;
            *pcchRemaining = cchDest;
        }
        else if (cchDest > 0)
        {
            char* pszDestEnd;
            
            pszDestEnd = pszDest + cchDest - 1;

            *ppszDestEnd = pszDestEnd;
            *pcchRemaining = 1;

            // null terminate the end of the string
            *pszDestEnd = L'\0';
        }
    }

    if ((cchDest > 0) && (dwFlags & STRSAFE_NULL_ON_FAILURE))
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        // null terminate the beginning of the string
        *pszDest = '\0';
    }

    return STATUS_SUCCESS;
}

NTSTRSAFEWORKERDDI
RtlStringExHandleOtherFlagsW(
    __inout_bcount(cbDest) NTSTRSAFE_PWSTR pszDest,
    __in __in_range(sizeof(wchar_t), NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbDest,
    __in __in_range(<, cbDest / sizeof(wchar_t)) size_t cchOriginalDestLength,    
    __deref_inout_ecount(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
    __out __deref_out_range(<=, cbDest / sizeof(wchar_t)) size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    size_t cchDest = cbDest / sizeof(wchar_t);
    
    if ((cchDest > 0) && (dwFlags & STRSAFE_NO_TRUNCATION))
    {
        wchar_t* pszOriginalDestEnd;

        pszOriginalDestEnd = pszDest + cchOriginalDestLength;

        *ppszDestEnd = pszOriginalDestEnd;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        // null terminate the end of the original string
        *pszOriginalDestEnd = L'\0';
    }

    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
        {
            *ppszDestEnd = pszDest;
            *pcchRemaining = cchDest;
        }
        else if (cchDest > 0)
        {
            wchar_t* pszDestEnd;
            
            pszDestEnd = pszDest + cchDest - 1;

            *ppszDestEnd = pszDestEnd;
            *pcchRemaining = 1;

            // null terminate the end of the string
            *pszDestEnd = L'\0';
        }
    }

    if ((cchDest > 0) && (dwFlags & STRSAFE_NULL_ON_FAILURE))
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        // null terminate the beginning of the string
        *pszDest = L'\0';
    }

    return STATUS_SUCCESS;
}

#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

NTSTRSAFEWORKERDDI
RtlUnicodeStringInitWorker(
    __out PUNICODE_STRING DestinationString,
    __in_opt NTSTRSAFE_PCWSTR pszSrc,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (DestinationString || !(dwFlags & STRSAFE_IGNORE_NULLS))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    if (pszSrc)
    {
        size_t cchSrcLength;

        status = RtlStringLengthWorkerW(pszSrc, cchMax, &cchSrcLength);

        if (NT_SUCCESS(status))
        {
            if (DestinationString)
            {
                size_t cbLength;
                
                // safe to multiply cchSrcLength * sizeof(wchar_t) since cchSrcLength < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                cbLength = cchSrcLength * sizeof(wchar_t);

                DestinationString->Length = (USHORT)cbLength;
                // safe to add cbLength + sizeof(wchar_t) since cchSrcLength < NTSTRSAFE_UNICODE_STRING_MAX_CCH
                DestinationString->MaximumLength = (USHORT)(cbLength + sizeof(wchar_t));
                DestinationString->Buffer = (PWSTR)pszSrc;
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return status;
}

// Intentionally allow null deref in error cases.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_RETURNING_BAD_RESULT)

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateWorker(
    __in_opt PCUNICODE_STRING SourceString,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SourceString || !(dwFlags & STRSAFE_IGNORE_NULLS))
    {
        if (((SourceString->Length % sizeof(wchar_t)) != 0)         ||
            ((SourceString->MaximumLength % sizeof(wchar_t)) != 0)  ||
            (SourceString->Length > SourceString->MaximumLength)    ||
            (SourceString->MaximumLength > (cchMax * sizeof(wchar_t))))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if ((SourceString->Buffer == NULL) &&
                 ((SourceString->Length != 0) || (SourceString->MaximumLength != 0)))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateSrcWorker(
    __in PCUNICODE_STRING SourceString,
    __deref_out_ecount(*pcchSrcLength) wchar_t** ppszSrc,
    __out size_t* pcchSrcLength,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    *ppszSrc = NULL;
    *pcchSrcLength = 0;

    status = RtlUnicodeStringValidateWorker(SourceString, cchMax, dwFlags);

    if (NT_SUCCESS(status))
    {
        if (SourceString)
        {
            *ppszSrc = SourceString->Buffer;
            *pcchSrcLength = SourceString->Length / sizeof(wchar_t);
        }

        if ((*ppszSrc == NULL) && (dwFlags & STRSAFE_IGNORE_NULLS))
        {
            *ppszSrc = L"";
        }
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateDestWorker(
    __in PCUNICODE_STRING DestinationString,
    __deref_out_ecount(*pcchDest) wchar_t** ppszDest,
    __out size_t* pcchDest,
    __out_opt size_t* pcchDestLength,
    __in const size_t cchMax,
    __in DWORD dwFlags)
{
    NTSTATUS status;

    *ppszDest = NULL;
    *pcchDest = 0;

    if (pcchDestLength)
    {
        *pcchDestLength = 0;
    }

    status = RtlUnicodeStringValidateWorker(DestinationString, cchMax, dwFlags);

    if (NT_SUCCESS(status) && DestinationString)
    {
        *ppszDest = DestinationString->Buffer;
        *pcchDest = DestinationString->MaximumLength / sizeof(wchar_t);

        if (pcchDestLength)
        {
            *pcchDestLength = DestinationString->Length / sizeof(wchar_t);
        }
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlStringCopyWideCharArrayWorker(
    __out_ecount(cchDest) NTSTRSAFE_PWSTR pszDest,
    __in size_t cchDest,
    __out_opt size_t* pcchNewDestLength,
    __in_ecount(cchSrcLength) const wchar_t* pszSrc,
    __in size_t cchSrcLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    // ASSERT(cchDest != 0);
    
    while (cchDest && cchSrcLength)
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchSrcLength--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = L'\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyStringWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in NTSTRSAFE_PCWSTR pszSrc,
    __in size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    while (cchDest && cchToCopy && (*pszSrc != L'\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if ((cchDest == 0) && (cchToCopy != 0) && (*pszSrc != L'\0'))
    {
        // we are going to truncate pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }

    *pcchNewDestLength = cchNewDestLength;

    return status;
}

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in_ecount(cchSrcLength) const wchar_t* pszSrc,
    __in size_t cchSrcLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    while (cchDest && cchSrcLength)
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchSrcLength--;

        cchNewDestLength++;
    }
    
    if ((cchDest == 0) && (cchSrcLength != 0))
    {
        // we are going to truncate pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }

    *pcchNewDestLength = cchNewDestLength;

    return status;
}

NTSTRSAFEWORKERDDI
RtlWideCharArrayVPrintfWorker(
    __out_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __out size_t* pcchNewDestLength,
    __in __format_string NTSTRSAFE_PCWSTR pszFormat,
    __in va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    
    #pragma warning(push)
    #pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnwprintf(pszDest, cchDest, pszFormat, argList);
    #pragma warning(pop)
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchDest))
    {
        *pcchNewDestLength = cchDest;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        *pcchNewDestLength = (size_t)iRet;
    }

    return status;
}

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleFill(
    __out_ecount(cchRemaining) wchar_t* pszDestEnd,
    __in size_t cchRemaining,
    __in DWORD dwFlags)
{
    size_t cbRemaining;

    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
    cbRemaining = cchRemaining * sizeof(wchar_t);

    memset(pszDestEnd, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining);
    
    return STATUS_SUCCESS;
}

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleOtherFlags(
    __inout_ecount(cchDest) wchar_t* pszDest,
    __in size_t cchDest,
    __in size_t cchOriginalDestLength,
    __out size_t* pcchNewDestLength,
    __deref_out_ecount(*pcchRemaining) wchar_t** ppszDestEnd,
    __out size_t* pcchRemaining,
    __in DWORD dwFlags)
{
    if (dwFlags & STRSAFE_NO_TRUNCATION)
    {
        *ppszDestEnd = pszDest + cchOriginalDestLength;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        *pcchNewDestLength = cchOriginalDestLength;
    }
    
    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        size_t cbDest;
        
        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        *pcchNewDestLength = 0;
    }
    
    if (dwFlags & STRSAFE_ZERO_LENGTH_ON_FAILURE)
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        *pcchNewDestLength = 0;
    }

    return STATUS_SUCCESS;
}

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

#endif  // defined(NTSTRSAFE_LIB_IMPL) || !defined(NTSTRSAFE_LIB)


// Do not call these functions, they are worker functions for internal use within this file
#ifdef DEPRECATE_SUPPORTED
#pragma deprecated(RtlStringLengthWorkerA)
#pragma deprecated(RtlStringLengthWorkerW)
#pragma deprecated(RtlUnalignedStringLengthWorkerW)
#pragma deprecated(RtlStringExValidateSrcA)
#pragma deprecated(RtlStringExValidateSrcW)
#pragma deprecated(RtlStringValidateDestA)
#pragma deprecated(RtlStringValidateDestAndLengthA)
#pragma deprecated(RtlStringValidateDestW)
#pragma deprecated(RtlStringValidateDestAndLengthW)
#pragma deprecated(RtlStringExValidateDestA)
#pragma deprecated(RtlStringExValidateDestAndLengthA)
#pragma deprecated(RtlStringExValidateDestW)
#pragma deprecated(RtlStringExValidateDestAndLengthW)
#pragma deprecated(RtlStringCopyWorkerA)
#pragma deprecated(RtlStringCopyWorkerW)
#pragma deprecated(RtlStringVPrintfWorkerA)
#pragma deprecated(RtlStringVPrintfWorkerW)
#pragma deprecated(RtlStringExHandleFillBehindNullA)
#pragma deprecated(RtlStringExHandleFillBehindNullW)
#pragma deprecated(RtlStringExHandleOtherFlagsA)
#pragma deprecated(RtlStringExHandleOtherFlagsW)
#pragma deprecated(RtlUnicodeStringInitWorker)
#pragma deprecated(RtlUnicodeStringValidateWorker)
#pragma deprecated(RtlUnicodeStringValidateSrcWorker)
#pragma deprecated(RtlUnicodeStringValidateDestWorker)
#pragma deprecated(RtlStringCopyWideCharArrayWorker)
#pragma deprecated(RtlWideCharArrayCopyStringWorker)
#pragma deprecated(RtlWideCharArrayCopyWorker)
#pragma deprecated(RtlWideCharArrayVPrintfWorker)
#pragma deprecated(RtlUnicodeStringExHandleFill)
#pragma deprecated(RtlUnicodeStringExHandleOtherFlags)
#else
#define RtlStringLengthWorkerA             RtlStringLengthWorkerA_instead_use_StringCchLengthA_or_StringCbLengthA
#define RtlStringLengthWorkerW             RtlStringLengthWorkerW_instead_use_StringCchLengthW_or_StringCbLengthW
#define RtlUnalignedStringLengthWorkerW    RtlUnalignedStringLengthWorkerW_instead_use_UnalignedStringCchLengthW
#define RtlStringExValidateSrcA            RtlStringExValidateSrcA_do_not_call_this_function
#define RtlStringExValidateSrcW            RtlStringExValidateSrcW_do_not_call_this_function
#define RtlStringValidateDestA             RtlStringValidateDestA_do_not_call_this_function
#define RtlStringValidateDestAndLengthA    RtlStringValidateDestAndLengthA_do_not_call_this_function
#define RtlStringValidateDestW             RtlStringValidateDestW_do_not_call_this_function
#define RtlStringValidateDestAndLengthW    RtlStringValidateDestAndLengthW_do_not_call_this_function
#define RtlStringExValidateDestA           RtlStringExValidateDestA_do_not_call_this_function
#define RtlStringExValidateDestAndLengthA  RtlStringExValidateDestAndLengthA_do_not_call_this_function
#define RtlStringExValidateDestW           RtlStringExValidateDestW_do_not_call_this_function
#define RtlStringExValidateDestAndLengthW  RtlStringExValidateDestAndLengthW_do_not_call_this_function
#define RtlStringCopyWorkerA               RtlStringCopyWorkerA_instead_use_StringCchCopyA_or_StringCbCopyA
#define RtlStringCopyWorkerW               RtlStringCopyWorkerW_instead_use_StringCchCopyW_or_StringCbCopyW
#define RtlStringVPrintfWorkerA            RtlStringVPrintfWorkerA_instead_use_StringCchVPrintfA_or_StringCbVPrintfA
#define RtlStringVPrintfWorkerW            RtlStringVPrintfWorkerW_instead_use_StringCchVPrintfW_or_StringCbVPrintfW
#define RtlStringExHandleFillBehindNullA   RtlStringExHandleFillBehindNullA_do_not_call_this_function
#define RtlStringExHandleFillBehindNullW   RtlStringExHandleFillBehindNullW_do_not_call_this_function
#define RtlStringExHandleOtherFlagsA       RtlStringExHandleOtherFlagsA_do_not_call_this_function
#define RtlStringExHandleOtherFlagsW       RtlStringExHandleOtherFlagsW_do_not_call_this_function
#define RtlUnicodeStringInitWorker          RtlUnicodeStringInitWorker_instead_use_RtlUnicodeStringInit_or_RtlUnicodeStringInitEx
#define RtlUnicodeStringValidateWorker      RtlUnicodeStringValidateWorker_instead_use_RtlUnicodeStringValidate_or_RtlUnicodeStringValidateEx
#define RtlUnicodeStringValidateSrcWorker   RtlUnicodeStringValidateSrcWorker_do_not_call_this_function
#define RtlUnicodeStringValidateDestWorker  RtlUnicodeStringValidateDestWorker_do_not_call_this_function
#define RtlStringCopyWideCharArrayWorker    RtlStringCopyWideCharArrayWorker_instead_use_RtlStringCchCopyUnicodeString_or_RtlStringCbCopyUnicodeString
#define RtlWideCharArrayCopyStringWorker    RtlWideCharArrayCopyStringWorker_instead_use_RtlUnicodeStringCopyString_or_RtlUnicodeStringCopyStringEx
#define RtlWideCharArrayCopyWorker          RtlWideCharArrayCopyWorker_instead_use_RtlUnicodeStringCopy_or_RtlUnicodeStringCopyEx
#define RtlWideCharArrayVPrintfWorker       RtlWideCharArrayVPrintfWorker_instead_use_RtlUnicodeStringVPrintf_or_RtlUnicodeStringPrintf
#define RtlUnicodeStringExHandleFill        RtlUnicodeStringExHandleFill_do_not_call_this_function
#define RtlUnicodeStringExHandleOtherFlags  RtlUnicodeStringExHandleOtherFlags_do_not_call_this_function
#endif // !DEPRECATE_SUPPORTED


#ifndef NTSTRSAFE_NO_DEPRECATE
// Deprecate all of the unsafe functions to generate compiletime errors. If you do not want
// this then you can #define NTSTRSAFE_NO_DEPRECATE before including this file
#ifdef DEPRECATE_SUPPORTED

#pragma deprecated(strcpy)
#pragma deprecated(wcscpy)
#pragma deprecated(strcat)
#pragma deprecated(wcscat)
#pragma deprecated(sprintf)
#pragma deprecated(swprintf)
#pragma deprecated(vsprintf)
#pragma deprecated(vswprintf)
#pragma deprecated(_snprintf)
#pragma deprecated(_snwprintf)
#pragma deprecated(_vsnprintf)
#pragma deprecated(_vsnwprintf)

#else // DEPRECATE_SUPPORTED

#undef strcpy
#define strcpy      strcpy_instead_use_StringCchCopyA_or_StringCbCopyA;

#undef wcscpy
#define wcscpy      wcscpy_instead_use_StringCchCopyW_or_StringCbCopyW;

#undef strcat
#define strcat      strcat_instead_use_StringCchCatA_or_StringCbCatA;

#undef wcscat
#define wcscat      wcscat_instead_use_StringCchCatW_or_StringCbCatW;

#undef sprintf
#define sprintf     sprintf_instead_use_StringCchPrintfA_or_StringCbPrintfA;

#undef swprintf
#define swprintf    swprintf_instead_use_StringCchPrintfW_or_StringCbPrintfW;

#undef vsprintf
#define vsprintf    vsprintf_instead_use_StringCchVPrintfA_or_StringCbVPrintfA;

#undef vswprintf
#define vswprintf   vswprintf_instead_use_StringCchVPrintfW_or_StringCbVPrintfW;

#undef _snprintf
#define _snprintf   _snprintf_instead_use_StringCchPrintfA_or_StringCbPrintfA;

#undef _snwprintf
#define _snwprintf  _snwprintf_instead_use_StringCchPrintfW_or_StringCbPrintfW;

#undef _vsnprintf
#define _vsnprintf  _vsnprintf_instead_use_StringCchVPrintfA_or_StringCbVPrintfA;

#undef _vsnwprintf
#define _vsnwprintf _vsnwprintf_instead_use_StringCchVPrintfW_or_StringCbVPrintfW;

#endif  // DEPRECATE_SUPPORTED
#endif  // !NTSTRSAFE_NO_DEPRECATE

#pragma warning(pop)

#endif  // _NTSTRSAFE_H_INCLUDED_

