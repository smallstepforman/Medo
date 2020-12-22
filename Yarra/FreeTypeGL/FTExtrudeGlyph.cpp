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

#include <iostream>

#include "FTGL/ftgl.h"

#include "FTInternals.h"
#include "FTExtrudeGlyphImpl.h"
#include "FTVectoriser.h"

#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Font.h"

//
//  FTGLExtrudeGlyph
//


FTExtrudeGlyph::FTExtrudeGlyph(FT_GlyphSlot glyph, float depth,
                               float frontOutset, float backOutset,
                               bool useDisplayList) :
    FTGlyph(new FTExtrudeGlyphImpl(glyph, depth, frontOutset, backOutset,
                                   useDisplayList))
{}


FTExtrudeGlyph::~FTExtrudeGlyph()
{}


const FTPoint& FTExtrudeGlyph::Render(const FTPoint& pen, int renderMode)
{
    FTExtrudeGlyphImpl *myimpl = (FTExtrudeGlyphImpl *)(impl);
    return myimpl->RenderImpl(pen, renderMode);
}


//
//  FTGLExtrudeGlyphImpl
//


FTExtrudeGlyphImpl::FTExtrudeGlyphImpl(FT_GlyphSlot glyph, float _depth,
                                       float _frontOutset, float _backOutset,
                                       bool useDisplayList)
:   FTGlyphImpl(glyph),
    vectoriser(0)
{
    bBox.SetDepth(-_depth);

    if(ft_glyph_format_outline != glyph->format)
    {
        err = 0x14; // Invalid_Outline
        return;
    }

    vectoriser = new FTVectoriser(glyph);

    if((vectoriser->ContourCount() < 1) || (vectoriser->PointCount() < 3))
    {
        delete vectoriser;
        vectoriser = NULL;
        return;
    }

    hscale = glyph->face->size->metrics.x_ppem * 64;
    vscale = glyph->face->size->metrics.y_ppem * 64;
    depth = _depth;
    frontOutset = _frontOutset;
    backOutset = _backOutset;

    GenerateFront();
    GenerateBack();
    GenerateSide();
    
	delete vectoriser;
    vectoriser = NULL;
}


FTExtrudeGlyphImpl::~FTExtrudeGlyphImpl()
{
    if(vectoriser)
    {
        delete vectoriser;
    }
	//	Delete front buffers
	for (int i = 0; i < (int)fRenderBuffers_Front.size(); i++)
		delete [] fRenderBuffers_Front[i].vertices;
	fRenderBuffers_Front.clear();
	//	Delete back buffers
	for (int i = 0; i < (int)fRenderBuffers_Back.size(); i++)
		delete [] fRenderBuffers_Back[i].vertices;
	fRenderBuffers_Back.clear();
	//	Delete side buffers
	for (int i = 0; i < (int)fRenderBuffers_Side.size(); i++)
		delete [] fRenderBuffers_Side[i].vertices;
	fRenderBuffers_Side.clear();
}

//	Medo: Buffer geometry for faster rendering
const FTPoint& FTExtrudeGlyphImpl::RenderImpl(const FTPoint& pen, int renderMode)
{
	ymath::YVector3 pos(pen.Xf(), pen.Yf(), pen.Zf());

	if(renderMode & FTGL::RENDER_FRONT)
	{
		for (std::size_t i = 0; i < fRenderBuffers_Front.size(); i++)
		{
			yrender::YFontFreetype::ExtrudeGeometry geom;
			geom.type = fRenderBuffers_Front[i].type;
			for (int v=0; v < fRenderBuffers_Front[i].count; v++)
			{
				YGeometry_P3N3T2 g = fRenderBuffers_Front[i].vertices[v];
				g.mPosition[0] += pos.x;
				g.mPosition[1] += pos.y;
				g.mPosition[2] += pos.z;
				geom.geometry.push_back(g);
			}
			yrender::YFontFreetype::sExtrudeGeometry.push_back(geom);
		}
	}

	if(renderMode & FTGL::RENDER_BACK)
	{
		for (std::size_t i = 0; i < fRenderBuffers_Back.size(); i++)
		{
			yrender::YFontFreetype::ExtrudeGeometry geom;
			geom.type = fRenderBuffers_Back[i].type;
			for (int v=0; v < fRenderBuffers_Back[i].count; v++)
			{
				YGeometry_P3N3T2 g = fRenderBuffers_Back[i].vertices[v];
				g.mPosition[0] += pos.x;
				g.mPosition[1] += pos.y;
				g.mPosition[2] += pos.z;
				geom.geometry.push_back(g);
			}
			yrender::YFontFreetype::sExtrudeGeometry.push_back(geom);
		}
	}

	if(renderMode & FTGL::RENDER_SIDE)
	{
		for (std::size_t i = 0; i < fRenderBuffers_Side.size(); i++)
		{
			yrender::YFontFreetype::ExtrudeGeometry geom;
			geom.type = fRenderBuffers_Side[i].type;
			for (int v=0; v < fRenderBuffers_Side[i].count; v++)
			{
				YGeometry_P3N3T2 g = fRenderBuffers_Side[i].vertices[v];
				g.mPosition[0] += pos.x;
				g.mPosition[1] += pos.y;
				g.mPosition[2] += pos.z;
				geom.geometry.push_back(g);
			}
			yrender::YFontFreetype::sExtrudeGeometry.push_back(geom);
		}
	}

	return advance;
}


void FTExtrudeGlyphImpl :: GenerateFront()
{
	vectoriser->MakeMesh(static_cast<FTGL_DOUBLE>(1.0), 1, frontOutset);
    
	const FTMesh *mesh = vectoriser->GetMesh();
    for(unsigned int j = 0; j < mesh->TesselationCount(); ++j)
    {
        const FTTesselation* subMesh = mesh->Tesselation(j);
        
		FTExtrudeGlyphRenderBuffer aBuffer;
		aBuffer.type = subMesh->PolygonType();
		aBuffer.count = subMesh->PointCount();
		aBuffer.vertices = new YGeometry_P3N3T2 [aBuffer.count];
		float *v = (float *)aBuffer.vertices;
		
		for(unsigned int i = 0; i < subMesh->PointCount(); ++i)
		{
			FTPoint pt = subMesh->Point(i);
			//	position
			*v++ = pt.Xf() / 64.0f;		
			*v++ = pt.Yf() / 64.0f;
			*v++ = 0.0f;
			//	normal
			*v++ = 0.0f;
			*v++ = 0.0f;
			*v++ = 1.0f;
			//	texture
			*v++ = pt.Xf() / hscale;
			*v++ = pt.Yf() / vscale;
		}
		fRenderBuffers_Front.push_back(aBuffer);
    }
}


void FTExtrudeGlyphImpl :: GenerateBack()
{
	vectoriser->MakeMesh(-1.0, 2, backOutset);
	const FTMesh *mesh = vectoriser->GetMesh();
    for(unsigned int j = 0; j < mesh->TesselationCount(); ++j)
    {

		const FTTesselation* subMesh = mesh->Tesselation(j);

		FTExtrudeGlyphRenderBuffer aBuffer;
		aBuffer.type = subMesh->PolygonType();
		aBuffer.count = subMesh->PointCount();
		aBuffer.vertices = new YGeometry_P3N3T2 [aBuffer.count];
		float *v = (float *)aBuffer.vertices;
		
		for(unsigned int i = 0; i < subMesh->PointCount(); ++i)
		{
			//	position
			*v++ = subMesh->Point(i).Xf() / 64.0f;
			*v++ = subMesh->Point(i).Yf() / 64.0f;
			*v++ = -depth;
			//	normal
			*v++ = 0.0f;
			*v++ = 0.0f;
			*v++ = -1.0f;
			//	texture
			*v++ = subMesh->Point(i).Xf() / hscale;
			*v++ = subMesh->Point(i).Yf() / vscale;
		}
		fRenderBuffers_Back.push_back(aBuffer);
	}
}


void FTExtrudeGlyphImpl :: GenerateSide()
{
	int contourFlag = vectoriser->ContourFlag();
	
	for(size_t c = 0; c < vectoriser->ContourCount(); ++c)
	{
		const FTContour* contour = vectoriser->Contour(c);
		size_t nc = contour->PointCount();

		if (nc < 2)
			continue;
		
		FTExtrudeGlyphRenderBuffer aBuffer;
		aBuffer.type = GL_TRIANGLE_STRIP;
		aBuffer.count = nc*2+2;
		aBuffer.vertices = new YGeometry_P3N3T2 [aBuffer.count];
		float *v = (float *)aBuffer.vertices;
		
		for(size_t j = 0; j <= nc; ++j)
		{
			size_t cur = (j == nc) ? 0 : j;
			size_t next = (cur == nc - 1) ? 0 : cur + 1;

			FTPoint frontPt = contour->FrontPoint(cur);
			FTPoint nextPt = contour->FrontPoint(next);
			FTPoint backPt = contour->BackPoint(cur);

			FTPoint normal = FTPoint(0.f, 0.f, 1.f) ^ (frontPt - nextPt);

			//	position
			if(contourFlag & ft_outline_reverse_fill)
			{
				*v++ = backPt.Xf() / 64.0f;
				*v++ = backPt.Yf() / 64.0f;
				*v++ = 0.0f;
			}
			else
			{
				*v++ = backPt.Xf() / 64.0f;
				*v++ = backPt.Yf() / 64.0f;
				*v++ = -depth;
			}

			//	normal
			if (normal != FTPoint(0.0f, 0.0f, 0.0f))
				normal.Normalise();
			*v++ = normal.Xf();
			*v++ = normal.Yf();
			*v++ = normal.Zf();
			//	texture
			*v++ = frontPt.Xf() / hscale;
			*v++ = frontPt.Yf() / vscale;
			
			//	position
			if(contourFlag & ft_outline_reverse_fill)
			{
				*v++ = frontPt.Xf() / 64.0f;
				*v++ = frontPt.Yf() / 64.0f;
				*v++ = -depth;
			}
			else
			{
				*v++ = frontPt.Xf() / 64.0f;
				*v++ = frontPt.Yf() / 64.0f;
				*v++ = 0.0f;
			}
			//	normal
			*v++ = normal.Xf();
			*v++ = normal.Yf();
			*v++ = normal.Zf();
			//	texture
			*v++ = frontPt.Xf() / hscale;
			*v++ = frontPt.Yf() / vscale;
		}
		fRenderBuffers_Side.push_back(aBuffer);
	}
}

