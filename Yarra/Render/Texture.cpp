/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YTexture from Yarra engine
*/

#include <cstdio>
#include <interface/Bitmap.h>
#include <translation/TranslationUtils.h>

#include "Yarra/Platform.h"
#include "Texture.h"

namespace yrender
{

/*	FUNCTION:		YTexture :: YTexture
	ARGUMENTS:		width
					height
					texture_flags
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/	
YTexture :: YTexture(const unsigned int width, const unsigned int height, const unsigned int texture_flags)
{
	Init(width, height, texture_flags);	
}
	
/*	FUNCTION:		YTexture :: YTexture
	ARGUMENTS:		filename
					texture_flags
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YTexture :: YTexture(const char *filename, const unsigned int texture_flags)
{
	BBitmap *bitmap = BTranslationUtils::GetBitmap(filename);
	unsigned int width = bitmap->Bounds().IntegerWidth() + 1;
	unsigned int height = bitmap->Bounds().IntegerHeight() + 1;
	
	Init(width, height, texture_flags);
	Upload(bitmap);
	
	delete bitmap;
}

/*	FUNCTION:		YTexture :: YTexture
	ARGUMENTS:		width
					height
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
void YTexture :: Init(const unsigned int width, const unsigned int height, const unsigned int texture_flags)
{
	fWidth = width;
	fHeight = height;
	fActiveTexture = GL_TEXTURE0;
	fInternalFormat = GL_RGBA;

	glGenTextures(1, &fTextureId);
	glBindTexture(GL_TEXTURE_2D, fTextureId);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_flags & YTF_MAG_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR);		
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_flags & YTF_MIN_FILTER_NEAREST ? GL_NEAREST : GL_LINEAR);
	if (texture_flags & YTF_REPEAT)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else if (texture_flags & YTF_MIRRORED_REPEAT)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glTexImage2D(GL_TEXTURE_2D, 0, fInternalFormat, fWidth, fHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

/*	FUNCTION:		YTexture :: Upload
	ARGUMENTS:		bitmap
	RETURN:			n/a
	DESCRIPTION:	Upload bitmap to texture
*/
void YTexture :: Upload(BBitmap *bitmap)
{
	unsigned int w = bitmap->Bounds().IntegerWidth() + 1;
	unsigned int h = bitmap->Bounds().IntegerHeight() + 1;
	if (w*h > fWidth*fHeight)
	{
		w = fWidth;
		h = fHeight;	
	}
	
	//	Test color swap
#if 0
	BRect frame(0, 0, 1919, 1079);
	BBitmap *swap_bitmap = new BBitmap(frame, B_RGB32);
	unsigned char *src = (unsigned char *)bitmap->Bits();
	unsigned char *dst = (unsigned char *)swap_bitmap->Bits();
	for (int row=0; row < h; row++)
	{
		for (int column=0; column < w; column++)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = src[3];
			dst += 4;
			src += 4;
		}
	}
	glActiveTexture(fActiveTexture);
	glBindTexture(GL_TEXTURE_2D, fTextureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, fInternalFormat, GL_UNSIGNED_BYTE, swap_bitmap->Bits());
	delete swap_bitmap;
	return;
#endif
	
	glActiveTexture(fActiveTexture);
	glBindTexture(GL_TEXTURE_2D, fTextureId);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, fInternalFormat, GL_UNSIGNED_BYTE, bitmap->Bits());
}

/*	FUNCTION:		YTexture :: Upload
	ARGUMENTS:		filename
	RETURN:			n/a
	DESCRIPTION:	Upload filename to texture
*/
void YTexture :: Upload(const char *filename)
{
	BBitmap *bitmap = BTranslationUtils::GetBitmap(filename);
	Upload(bitmap);
	delete bitmap;
}

/*	FUNCTION:		YTexture :: YTexture
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YTexture :: ~YTexture()
{
	glDeleteTextures(1, &fTextureId);	
}

/*	FUNCTION:		YTexture :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Apply texture
*/
void YTexture :: Render(float delta_time)
{
	glActiveTexture(fActiveTexture);
	glBindTexture(GL_TEXTURE_2D, fTextureId);	
}

};	//	namespace yrender

