/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2011, ZenYes Pty Ltd
	DESCRIPTION:	Render definitions and data structures.  
*/

#ifndef __YARRA_RENDER_DEFINITIONS_H__
#define __YARRA_RENDER_DEFINITIONS_H__

/****************************************************************
	Y_GEOMETRY_FORMAT explains how data is stored in memory.
	An interleaved data format is preferred due to limitied hardware caches.
	Also known as "Arrays of Structure" (AoS).
	Note - OpenGL ES 2.0 only supports float constant vertex attributes,
	while vertex arrays can have other data types (byte/short/float/fixed etc)
*****************************************************************/
enum Y_GEOMETRY_FORMAT
{
	Y_GEOMETRY_P3,
	Y_GEOMETRY_P3C4U,	// colour passed as 4 unsigned bytes
	Y_GEOMETRY_P3C4,	
	Y_GEOMETRY_P3T2,
	Y_GEOMETRY_P3T2C4,
	Y_GEOMETRY_P3T2C4U,
	Y_GEOMETRY_P3N3,
	Y_GEOMETRY_P3N3T2,
	Y_GEOMETRY_P3N3T4,
	Y_GEOMETRY_P3T4C4U,		// colour passed as 4 unsigned bytes
	Y_GEOMETRY_P3N3T2W2B2,
	Y_GEOMETRY_P3T2W2B2,
	Y_GEOMETRY_P3N3T2TG4,
	Y_GEOMETRY_FORMAT_NUMBER_DEFINITIONS,
};
/********	Y_GEOMETRY_P3			********/
struct YGeometry_P3
{
	float		mPosition[3];
};
/********	Y_GEOMETRY_P3C4U			********/
struct YGeometry_P3C4U
{
	float			mPosition[3];
	unsigned char	mColour[4];
};
/********	Y_GEOMETRY_P3C4			********/
struct YGeometry_P3C4
{
	float		mPosition[3];
	float		mColour[4];
};
/********	Y_GEOMETRY_P3T2			********/
struct YGeometry_P3T2
{
	float		mPosition[3];
	float		mTexture[2];
};
/********	Y_GEOMETRY_P3T2C4		********/
struct YGeometry_P3T2C4
{
	float		mPosition[3];
	float		mTexture[2];
	float		mColour[4];
};
/********	Y_GEOMETRY_P3T4C4U		********/
struct YGeometry_P3T2C4U
{
	float			mPosition[3];
	float			mTexture[2];
	unsigned char	mColour[4];
};
/********	Y_GEOMETRY_P3N3		********/
struct YGeometry_P3N3
{
	float		mPosition[3];
	float		mNormal[3];
};
/********	Y_GEOMETRY_P3N3T2		********/
struct YGeometry_P3N3T2
{
	float		mPosition[3];
	float		mNormal[3];
	float		mTexture[2];
};
/********	Y_GEOMETRY_P3N3T4		********/
struct YGeometry_P3N3T4
{
	float		mPosition[3];
	float		mNormal[3];
	float		mTexture0[2];
	float		mTexture1[2];
};
/********	Y_GEOMETRY_P3T4C4U		********/
struct YGeometry_P3T4C4U
{
	float			mPosition[3];
	float			mTexture0[2];
	float			mTexture1[2];
	unsigned char	mColour[4];
};
/********	Y_GEOMETRY_P3N3T2W2B2	********/
struct YGeometry_P3N3T2W2B2
{
	float		mPosition[3];
	float		mNormal[3];
	float		mTexture[2];
	float		mWeights[2];
	float		mBoneIndices[2];
};
/********	Y_GEOMETRY_P3T2W2B2		********/
struct YGeometry_P3T2W2B2
{
	float		mPosition[3];
	float		mTexture[2];
	float		mWeights[2];
	float		mBoneIndices[2];
};
/********	Y_GEOMETRY_P3N3T2TG4	********/
struct YGeometry_P3N3T2TG4
{
	float		mPosition[3];
	float		mNormal[3];
	float		mTexture[2];
	float		mTangent[4];
};

/******************************
	Y_GEOMETRY_FORMAT buffer sizes
*******************************/
namespace yarra
{
	const GLsizei yGeometryBufferSize[] = 
	{
		sizeof(YGeometry_P3),			//	Y_GEOMETRY_P3
		sizeof(YGeometry_P3C4U),		//	Y_GEOMETRY_P3C4U
		sizeof(YGeometry_P3C4),			//	Y_GEOMETRY_P3C4
		sizeof(YGeometry_P3T2),			//	Y_GEOMETRY_P3T2
		sizeof(YGeometry_P3T2C4),		//	Y_GEOMETRY_P3T2C4
		sizeof(YGeometry_P3T2C4U),		//	Y_GEOMETRY_P3T2C4U
		sizeof(YGeometry_P3N3),			//	Y_GEOMETRY_P3N3
		sizeof(YGeometry_P3N3T2),		//	Y_GEOMETRY_P3N3T2
		sizeof(YGeometry_P3N3T4),		//	Y_GEOMETRY_P3N3T4
		sizeof(YGeometry_P3T4C4U),		//	Y_GEOMETRY_P3T4C4U
		sizeof(YGeometry_P3N3T2W2B2),	//	Y_GEOMETRY_P3N3T2W2B2
		sizeof(YGeometry_P3T2W2B2),		//	Y_GEOMETRY_P3T2W2B2
		sizeof(YGeometry_P3N3T2TG4),	//	Y_GEOMETRY_P3N3T2TG4
	};
};
static_assert(sizeof(yarra::yGeometryBufferSize)/sizeof(GLsizei) == Y_GEOMETRY_FORMAT_NUMBER_DEFINITIONS, "yGeometryBufferSize[] size mismatch");

/******************************
	Yarra Engine has several built-in shaders which clients can utilise.
	These default shaders are run-time customisable,
	and can handle features like skinning and normal maps.
*******************************/
enum Y_SHADER
{
	Y_SHADER_DEFAULT,					//	Same as fixed function pipeline
	Y_SHADER_PER_PIXEL_LIGHTING,		//	Per pixel lighting
	Y_SHADER_SHADOW,					//	Render projection shadows or generate shadow map
	Y_SHADER_CUSTOM,					//	Client responsible for shader management	 
};

#endif	//#ifndef __YARRA_RENDER_DEFINITIONS_H__
