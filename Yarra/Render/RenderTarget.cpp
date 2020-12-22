/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simple FrameBuffer target (from Yarra engine)
*/

#include <stack>
#include <cassert>

#include "Platform.h"
#include "Texture.h"

#include "RenderTarget.h"

namespace yrender
{
	
/*********************************************
	Render target manager
**********************************************/
static std::stack <YRenderTarget *> sRenderTargetStack;
static YRenderTarget *sRecentRenderTarget = nullptr;

/*	FUNCTION:		YRenderTarget :: PushRenderTarget
	ARGUMENTS:		target
	RETURN:			n/a
	DESCRIPTION:	Push current target to stack
*/
void YRenderTarget :: PushRenderTarget(YRenderTarget *target)
{
	sRenderTargetStack.push(target);
}

/*	FUNCTION:		YRenderTarget :: PopRenderTarget
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Pop topmost target from stack
*/
void YRenderTarget :: PopRenderTarget()
{
	assert(!sRenderTargetStack.empty());

	sRecentRenderTarget = sRenderTargetStack.top();
	sRenderTargetStack.pop();
}

/*	FUNCTION:		YRenderTarget :: PopRenderTarget
	ARGUMENTS:		none
	RETURN:			current render target
	DESCRIPTION:	Pop topmost target from stack
*/
YRenderTarget * YRenderTarget :: GetCurrentRenderTarget()
{
	if (sRenderTargetStack.empty())
		return nullptr;
	else
		return sRenderTargetStack.top();
}

/*	FUNCTION:		YRenderTarget :: GetRecentRenderTarget
	ARGUMENTS:		none
	RETURN:			last render target
	DESCRIPTION:	Pop topmost target from stack
*/
YRenderTarget * YRenderTarget :: GetRecentRenderTarget()
{
	return sRecentRenderTarget;
}

/*********************************************
	Render target instance
**********************************************/

/*	FUNCTION:		YRenderTarget :: YRenderTarget
	ARGUMENTS:		internal_format
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YRenderTarget :: YRenderTarget(const GLuint internal_format, GLuint width, GLuint height)
	: fWidth(width), fHeight(height)
{
	glGenFramebuffers(1, &fFrameBufferID);
	glGenRenderbuffers(1, &fRenderBufferID);
	glGenTextures(1, &fTexture);

	glBindFramebuffer(GL_FRAMEBUFFER, fFrameBufferID);

	glBindTexture(GL_TEXTURE_2D, fTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (GLsizei)fWidth, (GLsizei)fHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fTexture, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, fRenderBufferID);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)fWidth, (GLsizei)fHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fRenderBufferID);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		switch (fboStatus)
		{
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT error\n");			break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT error\n");	break;
#if !defined (GL_ES_VERSION_2_0)
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER error\n");			break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER error\n");			break;
			case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE error\n");			break;
			case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS error\n");		break;
#else
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:			yplatform::Debug("GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS error\n");			break;
#endif
			case GL_FRAMEBUFFER_UNSUPPORTED:					yplatform::Debug("GL_FRAMEBUFFER_UNSUPPORTED error\n");						break;
			default:											yplatform::Debug("GL_FRAMEBUFFER_COMPLETE error\n");						break;
		}
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*	FUNCTION:		YRenderTarget :: ~YRenderTarget
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YRenderTarget :: ~YRenderTarget()
{
	glDeleteFramebuffers(1, &fFrameBufferID);
	glDeleteRenderbuffers(1, &fRenderBufferID);
	glDeleteTextures(1, &fTexture);
}

/*	FUNCTION:		YRenderTarget :: Activate
	ARGUMENTS:		clear
	RETURN:			n/a
	DESCRIPTION:	Enable rendering to target
*/
void YRenderTarget :: Activate(const bool clear)
{
	PushRenderTarget(this);

	glBindFramebuffer(GL_FRAMEBUFFER, fFrameBufferID);
	if (clear)
	{
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	}
}

/*	FUNCTION:		YRenderTarget :: ActivateTransparentBuffer
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Only clear ALPHA buffer, used to blend RenderTarget to other surface
*/
void YRenderTarget :: ActivateTransparentBuffer()
{
	PushRenderTarget(this);

	glBindFramebuffer(GL_FRAMEBUFFER, fFrameBufferID);
	glClearColor(1,1,1,0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClearColor(0,0,0,0);
}

/*	FUNCTION:		YRenderTarget :: Deactivate
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Disable rendering to target
*/
void YRenderTarget :: Deactivate()
{
	PopRenderTarget();
	YRenderTarget *current = GetCurrentRenderTarget();
	if (current)
		glBindFramebuffer(GL_FRAMEBUFFER, current->fFrameBufferID);
	else
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/*	FUNCTION:		YRenderTarget :: BindTexture
	ARGUMENTS:		texture_unit
	RETURN:			n/a
	DESCRIPTION:	Bind frame buffer texture
*/
void YRenderTarget :: BindTexture(const GLuint texture_unit)
{
	glActiveTexture(texture_unit);
	glBindTexture(GL_TEXTURE_2D, fTexture);
}

};	//	namespace yrender

