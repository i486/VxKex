///////////////////////////////////////////////////////////////////////////////
//
// Module Name:
//
//     opengl.c
//
// Abstract:
//
//     Implements extended OpenGL functions.
//
// Author:
//
//     vxiiduu (11-Jul-2026)
//
// Environment:
//
//     Win32
//
// Revision History:
//
//     vxiiduu               11-Jul-2026  Initial creation.
//
///////////////////////////////////////////////////////////////////////////////

#include "buildcfg.h"
#include "kxdxp.h"
#include <GL/GL.h>

#pragma region Direct forwarders to system driver
STATIC VOID glBlitFramebuffer(
	IN	GLint		srcX0,
	IN	GLint		srcY0,
	IN	GLint		srcX1,
	IN	GLint		srcY1,
	IN	GLint		dstX0,
	IN	GLint		dstY0,
	IN	GLint		dstX1,
	IN	GLint		dstY1,
	IN	GLbitfield	mask,
	IN	GLenum		filter)
{
	STATIC VOID (WINAPI *pglBlitFramebuffer)(
		GLint, GLint, GLint, GLint,
		GLint, GLint, GLint, GLint,
		GLbitfield, GLenum) = NULL;

	if (pglBlitFramebuffer == NULL) {
		pglBlitFramebuffer = (PVOID) wglGetProcAddress("glBlitFramebuffer");
	}

	ASSERT (pglBlitFramebuffer != NULL);

	if (!pglBlitFramebuffer) {
		return;
	}

	pglBlitFramebuffer(
		srcX0,
		srcY0,
		srcX1,
		srcY1,
		dstX0,
		dstY0,
		dstX1,
		dstY1,
		mask,
		filter);
}

STATIC VOID glBindFramebuffer(
	IN	GLenum	Target,
	IN	GLuint	Framebuffer)
{
	STATIC VOID (WINAPI *pglBindFramebuffer)(GLenum, GLuint) = NULL;

	if (pglBindFramebuffer == NULL) {
		pglBindFramebuffer = (PVOID) wglGetProcAddress("glBindFramebuffer");
	}

	ASSERT (pglBindFramebuffer != NULL);

	if (!pglBindFramebuffer) {
		return;
	}

	pglBindFramebuffer(Target, Framebuffer);
}

STATIC CONST GLubyte *glGetStringi(
	IN	GLenum	Name,
	IN	GLuint	Index)
{
	STATIC CONST GLubyte *(WINAPI *pglGetStringi)(GLenum, GLuint) = NULL;

	if (pglGetStringi == NULL) {
		pglGetStringi = (PVOID) wglGetProcAddress("glGetStringi");
	}

	ASSERT (pglGetStringi != NULL);

	if (!pglGetStringi) {
		return NULL;
	}

	return pglGetStringi(Name, Index);
}

STATIC VOID glLinkProgram(
	IN	GLuint	Program)
{
	STATIC VOID (WINAPI *pglLinkProgram)(GLuint) = NULL;

	if (pglLinkProgram == NULL) {
		pglLinkProgram = (PVOID) wglGetProcAddress("glLinkProgram");
	}

	ASSERT (pglLinkProgram != NULL);

	if (!pglLinkProgram) {
		return;
	}

	pglLinkProgram(Program);
}
#pragma endregion

#define GL_NUM_EXTENSIONS				0x821D
#define GL_READ_FRAMEBUFFER				0x8CA8
#define GL_DRAW_FRAMEBUFFER				0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING		0x8CAA
#define GL_DRAW_FRAMEBUFFER_BINDING		0x8CA6

//
// glBlitNamedFramebuffer is required by the OpenGL renderer of the game
// "Teardown". It is an OpenGL 4.5+ function which the OpenGL 4.4 Intel
// graphics driver does not support. AMD and Nvidia graphics drivers should
// be fine without this.
//
KXDXAPI VOID WINAPI Ext_glBlitNamedFramebuffer(
	IN	GLuint		ReadFramebuffer,
	IN	GLuint		DrawFramebuffer,
	IN	GLint		srcX0,
	IN	GLint		srcY0,
	IN	GLint		srcX1,
	IN	GLint		srcY1,
	IN	GLint		dstX0,
	IN	GLint		dstY0,
	IN	GLint		dstX1,
	IN	GLint		dstY1,
	IN	GLbitfield	Mask,
	IN	GLenum		Filter)
{
	GLint PreviousReadFramebuffer;
	GLint PreviousDrawFramebuffer;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &PreviousReadFramebuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &PreviousDrawFramebuffer);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, ReadFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DrawFramebuffer);

	glBlitFramebuffer(
		srcX0,
		srcY0,
		srcX1,
		srcY1,
		dstX0,
		dstY0,
		dstX1,
		dstY1,
		Mask,
		Filter);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, PreviousReadFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, PreviousDrawFramebuffer);
}

KXDXAPI VOID WINAPI Ext_glGetIntegerv(
	IN	GLenum		Name,
	IN	GLint		*Out)
{
	glGetIntegerv(Name, Out);

	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (Name == GL_NUM_EXTENSIONS) {
			if (AshExeBaseNameIs(L"teardown.exe") ||
				AshExeBaseNameIs(L"teardown_modtest.exe")) {

				//
				// APPSPECIFICHACK: Fake the presence of GL_ARB_direct_state_access
				// for Teardown.
				//

				*Out += 1;
			}
		}
	}
}

KXDXAPI CONST GLubyte *WINAPI Ext_glGetStringi(
	IN	GLenum		Name,
	IN	GLuint		Index)
{
	unless (KexData->IfeoParameters.DisableAppSpecific) {
		if (Name == GL_EXTENSIONS) {
			GLint NumberOfDriverSupportedExtensions;

			glGetIntegerv(GL_NUM_EXTENSIONS, &NumberOfDriverSupportedExtensions);

			if (AshExeBaseNameIs(L"teardown.exe") ||
				AshExeBaseNameIs(L"teardown_modtest.exe")) {

				//
				// APPSPECIFICHACK: Fake the presence of GL_ARB_direct_state_access
				// for Teardown.
				//

				if (Index == NumberOfDriverSupportedExtensions + 0) {
					return "GL_ARB_direct_state_access";
				}
			}
		}
	}

	return glGetStringi(Name, Index);
}

KXDXAPI PROC WINAPI Ext_wglGetProcAddress(
	IN	PCSTR	ProcedureName)
{
	PROC ProcedureAddress;

	//
	// If we want to extend a function in the system OpenGL driver, we can
	// do that here.
	//

	if (StringEqualA(ProcedureName, "glGetStringi")) {
		return (PROC) Ext_glGetStringi;
	}
	
	ProcedureAddress = wglGetProcAddress(ProcedureName);

	//
	// If the system OpenGL driver doesn't have that function, we have the
	// opportunity to supply our own implementation.
	//

	if (ProcedureAddress == NULL) {
		if (StringEqualA(ProcedureName, "glBlitNamedFramebuffer")) {
			ProcedureAddress = (PROC) Ext_glBlitNamedFramebuffer;
		}
	}

	//
	// Log if neither us nor the OpenGL ICD have that function.
	//

	if (ProcedureAddress == NULL) {
		//
		// This is logged with debug severity because a very large number of
		// games/applications will just try and load everything but only call
		// a small subset. This is caused by the over-use of third party
		// opengl wrapper libraries in games.
		//

		KexLogDebugEvent(
			L"Failed to resolve \"%hs\" from your OpenGL driver",
			ProcedureName);
	}

	return ProcedureAddress;
}