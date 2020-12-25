/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YPicture node (from Yarra engine)
*/

#include <cassert>

#include "Picture.h"
#include "Texture.h"
#include "Shader.h"

namespace yrender
{
	
static const YGeometry_P3T2 kGeometryP3T2[] = 
{
	{-1, -1, 0,		0, 1},
	{1, -1, 0,		1, 1},
	{-1, 1, 0,		0, 0},
	{1, 1, 0,		1, 0},
};
static const YGeometry_P3T2 kGeometryP3T2_InverseTexY[] = 
{
	{-1, -1, 0,		0, 0},
	{1, -1, 0,		1, 0},
	{-1, 1, 0,		0, 1},
	{1, 1, 0,		1, 1},
};

static const YGeometry_P3T2 kGeometryP3T2TwoSided[] = 
{
	//	Vertices
	{-1.0f, 1.0f, 0.0f,		0, 0},
	{-1.0f, -1.0f, 0.0f,	0, 1},
	{1.0f, 1.0f, 0.0f,		1, 0},
	{1.0f, -1.0f, 0.0f,		1, 1},
	{-1.0f, 1.0f, 0.0f,		0, 0},
	{-1.0f, -1.0f, 0.0f,	0, 1},
};
static const YGeometry_P3T2 kGeometryP3T2TwoSided_InverseTexY[] = 
{
	//	Vertices
	{-1.0f, 1.0f, 0.0f,		0, 1},
	{-1.0f, -1.0f, 0.0f,	0, 0},
	{1.0f, 1.0f, 0.0f,		1, 1},
	{1.0f, -1.0f, 0.0f,		1, 0},
	{-1.0f, 1.0f, 0.0f,		0, 1},
	{-1.0f, -1.0f, 0.0f,	0, 0},
};
	
/*	FUNCTION:		YPicture :: YPicture
	ARGUMENTS:		filename
					double_sided
					inverse_texture_y
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YPicture :: YPicture(const char *filename, const bool double_sided, const bool inverse_texture_y)
{
	assert(filename);	
	
	mTexture = new YTexture(filename, YTexture::YTF_REPEAT);
	mShaderNode = new YMinimalShader;
	CreateGeometry(double_sided, inverse_texture_y);	
	mSpatial.SetScale(ymath::YVector3(0.5f*mTexture->GetWidth(), 0.5f*mTexture->GetHeight(), 0));
}

/*	FUNCTION:		YPicture :: YPicture
	ARGUMENTS:		width, height
					double_sided
					inverse_texture_y
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YPicture :: YPicture(unsigned int width, unsigned int height, const bool double_sided, const bool inverse_texture_y)
{
	mTexture = new YTexture(width, height, YTexture::YTF_REPEAT);
	mShaderNode = new YMinimalShader;
	CreateGeometry(double_sided, inverse_texture_y);
	mSpatial.SetScale(ymath::YVector3(0.5f*width, 0.5f*height, 0));
}

/*	FUNCTION:		YPicture :: CreateGeometry
	ARGUMENTS:		double_sided
					inverse_texture
	RETURN:			n/a
	DESCRIPTION:	Init geometry node
*/
void YPicture :: CreateGeometry(const bool double_sided, const bool inverse_texture_y)
{
	if (double_sided)
	{
		mGeometryNode = new YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, 
							inverse_texture_y ? (float *)kGeometryP3T2TwoSided_InverseTexY : (float *)kGeometryP3T2TwoSided, 6);
	}
	else
	{
		mGeometryNode = new YGeometryNode(GL_TRIANGLE_STRIP, Y_GEOMETRY_P3T2, 
							inverse_texture_y ? (float *)kGeometryP3T2_InverseTexY : (float *)kGeometryP3T2, 4);
	}
}

};	//	namespace yrender
