/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd 
	DESCRIPTION:	Simple FrameBuffer target (from Yarra engine)
*/

#ifndef __YARRA_RENDER_TARGET_H__
#define __YARRA_RENDER_TARGET_H__

namespace yrender
{
	
/*********************************
	Render target
**********************************/
class YRenderTarget
{
	GLuint		fFrameBufferID;
	GLuint		fRenderBufferID;
	GLuint		fTexture;
	
	GLuint		fWidth; 
	GLuint		fHeight;

public:
				YRenderTarget(const GLuint internal_format = GL_RGBA, GLuint width = 1920, GLuint height = 1080);
				~YRenderTarget();

	void		Activate(const bool clear);
	void		ActivateTransparentBuffer();
	void		Deactivate();
	void		BindTexture(const GLuint texture_unit = 0);

/**********************************
	Render target manager
***********************************/
	static void				PushRenderTarget(YRenderTarget *target);
	static void				PopRenderTarget();
	static YRenderTarget	*GetCurrentRenderTarget();
	static YRenderTarget	*GetRecentRenderTarget();

};

};	//	namespace yrender

#endif	//#ifndef __YARRA_RENDER_TARGET_H__	
