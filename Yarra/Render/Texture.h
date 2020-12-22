/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YTexture from Yarra engine
*/

#ifndef __YARRA_TEXTURE_H__
#define __YARRA_TEXTURE_H__

#ifndef _YARRA_PLATFORM_H_
#include "Yarra/Platform.h"
#endif

class BBitmap;

namespace yrender
{

/***********************
	YTexture
************************/	
class YTexture
{
public:
	enum TEXTURE_FLAGS
	{
		YTF_REPEAT				= 1 << 0,
		YTF_MIRRORED_REPEAT		= 1 << 1,
		YTF_MAG_FILTER_NEAREST	= 1 << 2,		// default is linear
		YTF_MIN_FILTER_NEAREST	= 1 << 3,		// default is linear
	};
			YTexture(const unsigned int width, const unsigned int height, const unsigned int texture_flags = 0);
			YTexture(const char *filename, const unsigned int texture_flags = 0);
			~YTexture();
	void	SetTextureUnitIndex(const unsigned int index) {fActiveTexture = (GLenum) (GL_TEXTURE0 + index);}
	void	Upload(BBitmap *bitmap);
	void	Upload(const char *filename);
	void	Render(float delta_time);
	
	const unsigned int	GetWidth() const	{return fWidth;}
	const unsigned int	GetHeight() const	{return fHeight;}
	
private:
	GLuint			fTextureId;
	GLenum			fInternalFormat;
	GLenum			fActiveTexture;
	unsigned int	fWidth;
	unsigned int	fHeight;
	
	void	Init(const unsigned int width, const unsigned int height, const unsigned int texture_flags);
};

};	//	namespace yrender

#endif	//#ifndef __YARRA_TEXTURE_H__
