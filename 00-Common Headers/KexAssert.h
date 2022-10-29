///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexAssert.h
//
// Abstract:
//
//     ASSERT, ASSUME and CHECKED macros.
//
// Author:
//
//     vxiiduu (26-Sep-2022)
//
// Environment:
//
//     ASSERT displays an error box when compiling for an EXE target.
//     Otherwise, it will simply cause a breakpoint in the current process.
//
//     ASSUME and CHECKED can be used anywhere.
//
// Revision History:
//
//     vxiiduu               26-Sep-2022  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

//
// Uncomment the macro definition to keep asserts enabled in release builds.
//
//#define RELEASE_ASSERTS_ENABLED

//
// == Guidelines for ASSERT Macro Usage in the VxKex Codebase ==
//
//   1. Use a space between ASSERT and (.
//        Bad:  ASSERT(Pointer != NULL);
//        Good: ASSERT (Pointer != NULL);
//
//   2. Do not omit the conditional operator.
//        Bad:  ASSERT (Pointer);
//        Good: ASSERT (Pointer != NULL);
//
//   3. Use only one condition in an ASSERT macro.
//        Bad:  ASSERT (Pointer != NULL && Size != 0);
//
//        Good: ASSERT (Pointer != NULL);
//              ASSERT (Size != 0);
//
//   4. When asserting that a particular code-path will not be executed, use
//      the comma operator to give the user/developer an explanation for what
//      happened. Remember, no other context is given besides the condition
//      inside the ASSERT macro.
//        Bad:  ASSERT (FALSE);
//        Good: ASSERT (("The event ID is not valid", FALSE));
//
#if defined(_DEBUG) || defined(RELEASE_ASSERTS_ENABLED)
#  define ASSERTS_ENABLED
#  if defined(KEX_TARGET_TYPE_EXE)
#    define ASSERT(Condition)	if (!(Condition)) { \
									BOOLEAN ShouldRaiseException = ReportAssertionFailure( \
										__FILEW__, __LINE__, __FUNCTIONW__, L#Condition); \
									if (ShouldRaiseException) __int2c(); \
								}
#  else
#    define ASSERT(Condition)	if (!(Condition)) __int2c()
#  endif
#else
#  define ASSERT(Condition)
#endif

#ifndef _DEBUG
#  define ASSUME __assume
#else
#  define ASSUME ASSERT
#endif

#define NOT_REACHED ASSUME(FALSE)
#define NOTHING
#define CHECKED(x) if (!(x)) goto Error