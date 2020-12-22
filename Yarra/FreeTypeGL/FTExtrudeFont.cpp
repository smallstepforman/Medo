/*
 * FTGL - OpenGL font library
 *
 * Copyright (c) 2001-2004 Henry Maddocks <ftgl@opengl.geek.nz>
 * Copyright (c) 2008 Sam Hocevar <sam@hocevar.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include "FTGL/ftgl.h"

#include "FTInternals.h"
#include "FTExtrudeFontImpl.h"


//
//  FTExtrudeFont
//


FTExtrudeFont::FTExtrudeFont(char const *fontFilePath) :
    FTFont(new FTExtrudeFontImpl(this, fontFilePath))
{}


FTExtrudeFont::FTExtrudeFont(const unsigned char *pBufferBytes,
                             size_t bufferSizeInBytes) :
    FTFont(new FTExtrudeFontImpl(this, pBufferBytes, bufferSizeInBytes))
{}


FTExtrudeFont::~FTExtrudeFont()
{}


FTGlyph* FTExtrudeFont::MakeGlyph(FT_GlyphSlot ftGlyph)
{
    FTExtrudeFontImpl *myimpl = (FTExtrudeFontImpl *)(impl);
    if(!myimpl)
    {
        return NULL;
    }

    return new FTExtrudeGlyph(ftGlyph, myimpl->depth, myimpl->front,
                              myimpl->back, myimpl->useDisplayLists);
}


//
//  FTExtrudeFontImpl
//


FTExtrudeFontImpl::FTExtrudeFontImpl(FTFont *ftFont, const char* fontFilePath)
: FTFontImpl(ftFont, fontFilePath),
  depth(0.0f), front(0.0f), back(0.0f)
{
    load_flags = FT_LOAD_NO_HINTING;
}


FTExtrudeFontImpl::FTExtrudeFontImpl(FTFont *ftFont,
                                     const unsigned char *pBufferBytes,
                                     size_t bufferSizeInBytes)
: FTFontImpl(ftFont, pBufferBytes, bufferSizeInBytes),
  depth(0.0f), front(0.0f), back(0.0f)
{
    load_flags = FT_LOAD_NO_HINTING;
}

/****************************
	Allow client to alter colour of 3D text
*****************************/
FTExtrudeFontCustomColours FTExtrudeFont::sCustomColours = 
{
	false,
	{1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f}
};

void FTExtrudeFont :: EnableCustomColours(bool enable)
{
	sCustomColours.enabled = enable;
}

void FTExtrudeFont :: SetFrontColour(float r, float g, float b, float a)
{
	sCustomColours.front_colour[0] = r;
	sCustomColours.front_colour[1] = g;
	sCustomColours.front_colour[2] = b;
	sCustomColours.front_colour[3] = a;
}

void FTExtrudeFont :: SetBackColour(float r, float g, float b, float a)
{
	sCustomColours.back_colour[0] = r;
	sCustomColours.back_colour[1] = g;
	sCustomColours.back_colour[2] = b;
	sCustomColours.back_colour[3] = a;
}

void FTExtrudeFont :: SetSideColour(float r, float g, float b, float a)
{
	sCustomColours.side_colour[0] = r;
	sCustomColours.side_colour[1] = g;
	sCustomColours.side_colour[2] = b;
	sCustomColours.side_colour[3] = a;
}
