///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     KexAssert.h
//
// Abstract:
//
//     ASSERT and ASSUME macros.
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
//     ASSUME can be used anywhere.
//
// Revision History:
//
//     vxiiduu               26-Sep-2022  Initial creation.
//     vxiiduu               22-Feb-2022  Remove obsolete CHECKED macro.
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
#  if defined(KEX_TARGET_TYPE_EXE) && !defined(KEX_DISABLE_GRAPHICAL_ASSERTS)
#    define ASSERT(Condition)	if (!(Condition)) { \
									BOOLEAN ShouldRaiseException = ReportAssertionFailure( \
										__FILEW__, __LINE__, __FUNCTIONW__, L#Condition); \
									if (ShouldRaiseException) __int2c(); \
								}
#    define SOFT_ASSERT ASSERT
#  else
#    define ASSERT(Condition)	do { if (!(Condition)) { DbgPrint("Assertion failure: %s (%s:%d in %s)\r\n", #Condition, __FILE__, __LINE__, __FUNCTION__);  __int2c(); } } while(0)
#    define SOFT_ASSERT (Condition) do { if (!(Condition)) { DbgPrint("Soft assertion failure: %s (%S:%d in %s)\r\n", #Condition, __FILE__, __LINE__, __FUNCTION); } while (0)
#  endif
#else
#  define ASSERT(Condition)
#endif

#ifndef _DEBUG
#  define ASSUME __assume
#  define NOT_REACHED ASSUME(FALSE)
#else
#  define ASSUME ASSERT
#  define NOT_REACHED ASSUME(("Execution should not have reached this point", FALSE))
#endif


#define NOTHING