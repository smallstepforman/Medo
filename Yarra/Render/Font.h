/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2011, ZenYes Pty Ltd
	DESCRIPTION:	Wrapper for Font engine. Current engine uses FreeType GL.
*/

#ifndef __YARRA_FONT_H__
#define __YARRA_FONT_H__

#include <string>
#include <vector>

#ifndef __YARRA_SCENE_NODE_H__
#include "Yarra/Render/SceneNode.h"
#endif

class FTTextureGlyphImpl;
class FTFont;

namespace yrender
{
	
class YGeometryNode;
class YFontObject_FreeType;

/****************************
	YFont base class.  The available fonts are:
		- YFontFreetype	(TrueType fonts)
		- YFontTextureAtlas (excluded from Medo)	
*****************************/
class YFont
{
public:
	virtual					~YFont() {}
	virtual YGeometryNode	*CreateGeometry(const char *text) = 0;
	virtual void			PreRender(const ymath::YVector4 &colour) {}
	virtual void			DrawText(const char *text) {}
	
	const ymath::YVector3	&GetGeometrySize() const	{return fSize;}
	virtual const float		GetWidth() const		{return fSize.x;}
	virtual const float		GetHeight() const		{return GetAscent() + GetDescent();}
	virtual const float		GetAscent() const		{return fSize.y;}					//	How far characters can ascend above the baseline.
	virtual const float		GetDescent() const		{return fSize.z;}					//	How far characters can descend below the baseline.

protected:
							YFont() {}
	ymath::YVector3			fSize;		// x = width, y = ascent, z = descent
};

/***************************
	YFontFreetype uses freetype and FTGL.
	Use it for loading TrueType fonts.
****************************/
class YFontFreetype : public YFont
{
public:
	explicit		YFontFreetype(const int size_pixels, const char *font_file);
	explicit		YFontFreetype(const int size_pixels, const char *font_file, const float depth);		//	3D extruded font
					~YFontFreetype();
	YGeometryNode	*CreateGeometry(const char *text);
	void			PreRender(const ymath::YVector4 &colour);
	
protected:
	explicit				YFontFreetype() {}
	void					InitFont(const int size_pixels, const char *font_file, const bool is3D = false, const float depth = 0.1f);
	FTFont					*GetFTFont();
	YFontObject_FreeType	*fCachedFont;

private:
	YGeometryNode	*CreateTextureGlyphGeometry(const char *text);
	YGeometryNode	*CreateExtrudeGlyphGeometry(const char *text);

public:
	//	These members are only accessible by class FTTextureGlyphImpl
	friend class FTTextureGlyphImpl;
	static YGeometry_P3T2	*sVertexBuffer;
	static YGeometry_P3T2	*sVertexBufferPointer;
	static GLuint			sVertexBufferCharacterCount;
	static GLuint			sFontTextureID;

	//	These members are only accessible by class FTExtrudeGlyph
	struct ExtrudeGeometry
	{
		int type;
		std::vector<YGeometry_P3N3T2>	geometry;
	};
	static std::vector<ExtrudeGeometry>		sExtrudeGeometry;
};

/***************************
	YTextScene is a scene node which can display a font
****************************/
class YTextScene : public YSpatialNode
{
public:
	enum HORIZONTAL_ALIGNMENT
	{
		ALIGN_HCENTER,
		ALIGN_LEFT,
		ALIGN_RIGHT,
	};
	enum VERTICAL_ALIGNEMENT
	{
		ALIGN_VCENTER,			//	mSpatial position is aligned to absolute center (ascent + descent).  Typically, baseline will be below this position
		ALIGN_ASCENT_CENTER,	//	mSpatial position is aligned to ascent center.  The baseline will be below this position
		ALIGN_BASELINE,			//	baseline is aligned to mSpatial position.  The majority of the scene is rendered above the baseline
	};
	
					YTextScene(YFont *font, bool acquire_font_ownership);
	virtual			~YTextScene();

	virtual void	Render(float delta_time);
	
	void			SetText(const char *text);
	void			SetColour(const ymath::YVector4 &colour);
	void			SetHorizontalAlignment(HORIZONTAL_ALIGNMENT alignment)	{fHorizontalAlignment = alignment;}
	void			SetVerticalAlignment(VERTICAL_ALIGNEMENT alignment)		{fVerticalAlignment = alignment;}
	const float		GetWidth() const	{return fGeometrySize.x;}
	const float		GetAscent() const	{return fGeometrySize.y;}	// height above baseline
	const float		GetDescent() const	{return fGeometrySize.z;}	// height below baseline	
	const float		GetHeight() const	{return fGeometrySize.y + fGeometrySize.z;}
	YFont			*GetFont() const	{return fFont;}

private:
	std::string				fText;
	YGeometryNode			*fGeometryNode;
	YFont					*fFont;
	ymath::YVector4			fColour;
	bool					fAcquireFontOwnership;
	ymath::YVector3			fGeometrySize;		// x = width, y = ascent, z = descent
	HORIZONTAL_ALIGNMENT	fHorizontalAlignment;
	VERTICAL_ALIGNEMENT		fVerticalAlignment;
};

};	//	namespace yrender

#endif	//_#ifndef __YARRA_FONT_H__

