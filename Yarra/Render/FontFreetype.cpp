/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2011, ZenYes Pty Ltd
	DESCRIPTION:	FreeType GL fonts
*/

#include <string>
#include <cassert>

#include "Yarra/Render/Camera.h"
#include "Yarra/Render/MatrixStack.h"
#include "Yarra/Render/SceneNode.h"
#include "Yarra/Render/Shader.h"
#include "Yarra/Render/Texture.h"

#include "FTGL/ftgl.h"
#include "Yarra/Render/Font.h"

namespace yrender
{
	
/*************************************************************************
	Font creation is expensive.  We can maintain every created font 
	in a cache to accelerate font creation time for the scenario where
	a destroyed font is later requested again.
	This behaviour can be disabled if requested.
**************************************************************************/
#define KEEP_FONT_CACHE			

static const int kVertexBufferSize = 0x200;

//======================
static std::vector <YFontObject_FreeType *> sFontCache;
static int sTotalFontCount = 0;

YGeometry_P3T2 * YFontFreetype :: sVertexBuffer = NULL;
YGeometry_P3T2 * YFontFreetype :: sVertexBufferPointer = NULL;
GLuint YFontFreetype :: sVertexBufferCharacterCount = 0;
GLuint YFontFreetype :: sFontTextureID = 0;

std::vector<YFontFreetype::ExtrudeGeometry> YFontFreetype::sExtrudeGeometry;

/************************************
	TextureGlyph Shader
*************************************/
static const char *kVertexShader_P3T2 = "\
	uniform mat4	uTransform;\
	in vec3			aPosition;\
	in vec2			aTexture0;\
	out vec2		vTexCoord0;\
	void main(void) {\
		gl_Position = uTransform * vec4(aPosition, 1.0);\
		vTexCoord0 = aTexture0;\
	}";

static const char *kFragmentShader_P3T2 = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uColour;\
	in vec2				vTexCoord0;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = uColour * vec4(vec3(1.0), texture(uTextureUnit0, vTexCoord0).r);\
	}";
class Shader2D
{
	YShader		*fShader;
	GLint		fLocation_uTransform;
	GLint		fLocation_uColour;
	ymath::YVector4		fColour;
public:
	Shader2D()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader_P3T2, kFragmentShader_P3T2);
		fLocation_uTransform = fShader->GetUniformLocation("uTransform");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
	}
	~Shader2D()
	{
		delete fShader;
	}
	void	SetColour(const ymath::YVector4	&colour)	{fColour = colour;}
	void	Activate()
	{
		fShader->EnableProgram();
		glUniformMatrix4fv(fLocation_uTransform, 1, GL_FALSE, yrender::yMatrixStack.GetMVPMatrix().m);
		glUniform4fv(fLocation_uColour, 1, fColour.v);
	}
};
static Shader2D *sShader2D = nullptr;

/************************************
	ExtrudeGlyph Shader
*************************************/
static const char *kVertexShader_P3N3T2 = "\
	uniform mat4	uMvp;\
	uniform mat4	uNormal;\
	in vec3			aPosition;\
	in vec3			aNormal;\
	in vec2			aTexCoord;\
	out vec2		vTexCoord;\
	out vec4		vColour;\
	\
	void main(void) {\
		gl_Position = uMvp * vec4(aPosition, 1.0);\
		vTexCoord = aTexCoord;\
		vec3 normal = normalize(mat3(uNormal) * aNormal);\
		//	Ambient factor\n\
		vColour = vec4(0.16, 0.32, 0.32, 1.0);\
		//	Lambert model (diffuse)\n\
		float ndotl = max(0.0, dot(normal, vec3(-0.276, 0.276, 0.921)));\
		if (ndotl > 0.0)\
		{\
			vColour += ndotl * vec4(0.8, 0.8, 0.8, 1.0);\
			//	Blinn-Phong model (specular)\n\
			float ndoth = max(0.0, dot(normal, vec3(-0.141, 0.141, 0.980)));\
			if (ndoth > 0.0)\
				vColour += pow(ndoth, 8.0);\
		}\
	}";

static const char *kFragmentShader_P3N3T2 = "\
	uniform sampler2D	uTextureUnit0;\
	uniform vec4		uColour;\
	in vec2				vTexCoord;\
	in vec4				vColour;\
	out vec4			fFragColour;\
	void main(void) {\
		fFragColour = uColour * vColour;\
	}";

	//fFragColour = uColour * texture(uTextureUnit0, vTexCoord).rgba * vColour;
class Shader3D
{
	YShader		*fShader;
	GLint		fLocation_uMvpMatrix;
	GLint		fLocation_uNormalMatrix;
	GLint		fLocation_uColour;
	ymath::YVector4		fColour;
public:
	Shader3D()
	{
		std::vector <std::string> attributes;
		attributes.push_back("aPosition");
		attributes.push_back("aNormal");
		attributes.push_back("aTexture0");
		fShader = new YShader(&attributes, kVertexShader_P3N3T2, kFragmentShader_P3N3T2);
		fLocation_uMvpMatrix = fShader->GetUniformLocation("uMvp");
		fLocation_uNormalMatrix = fShader->GetUniformLocation("uNormal");
		fLocation_uColour = fShader->GetUniformLocation("uColour");
	}
	~Shader3D()
	{
		delete fShader;
	}
	void	SetColour(const ymath::YVector4	&colour)	{fColour = colour;}
	void	Activate()
	{
		fShader->EnableProgram();
		ymath::YMatrix4 mvp = *yrender::YCamera::GetCurrent()->GetProjectionMatrix() * *yrender::yMatrixStack.GetTopMatrix();
		ymath::YMatrix4 normal_matrix = yrender::yMatrixStack.GetTopMatrix()->GetInverse().GetTranspose();

		glUniformMatrix4fv(fLocation_uMvpMatrix, 1, GL_FALSE, mvp.m);
		glUniformMatrix4fv(fLocation_uNormalMatrix, 1, GL_FALSE,normal_matrix.m);
		glUniform4fv(fLocation_uColour, 1, fColour.v);
	}
};
static Shader3D *sShader3D = nullptr;

/**********************************
	YFontObject_FreeType shared cache
***********************************/
class YFontObject_FreeType
{
public:
	std::string			mFontFile;
	int					mFontSize;
	bool				mIs3D;
	int					mCount;

public:
	FTFont				*mFont;
	GLuint				mTextureID;

	/*	FUNCTION:		YFontObject_FreeType :: YFontObject_FreeType
		ARGUMENTS:		font_size		- font size
						font_file		- path to font file
						is3D
						depth (only used for 3D fonts)
		RETURN:			n/a
		DESCRIPTION:	Constructor.
	*/
	YFontObject_FreeType(const int font_size, const char *font_file, bool is3D, float depth)
	{
		mFontFile.assign(font_file);
		mFontSize = font_size;
		mIs3D = is3D;
		mCount = 1;
		mTextureID = 0;

		//	create font
		if (is3D)
		{
			mFont = new FTExtrudeFont(font_file);
			mFont->Depth(depth);
		}
		else
			mFont = new FTGLTextureFont(font_file);

		if (mFont->Error())
			yplatform::Exit("[YFontObject_FreeType] Cannot load font (%s)", font_file);
		mFont->FaceSize(font_size);
		if (mFont->Error())
			yplatform::Debug("[YFontObject_FreeType] Font error\n");
	}

	~YFontObject_FreeType()
	{
		delete mFont;
	}
};

/**********************************************
	A YFontFreetype is a wrapper for FreeType GL Fonts.
	YFontFreetype objects share font objects of same size/type.
***********************************************/

/*	FUNCTION:		YFontFreetype :: YFontFreetype
	ARGUMENTS:		size_pixels 
					font_file
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YFontFreetype :: YFontFreetype(const int size_pixels, const char *font_file)
{
	InitFont(size_pixels, font_file);
}

YFontFreetype :: YFontFreetype(const int size_pixels, const char *font_file, const float depth)
{
	InitFont(size_pixels, font_file, true, depth);
}

/*	FUNCTION:		YFontFreetype :: InitFont
	ARGUMENTS:		font_size 
					font_file
					is3D
					depth
	RETURN:			n/a
	DESCRIPTION:	Private constructor
*/
void YFontFreetype :: InitFont(const int font_size, const char *font_file, bool is3D, float depth)
{
	if (++sTotalFontCount == 1)
	{
		sVertexBuffer = new YGeometry_P3T2 [kVertexBufferSize * 6];
		memset(sVertexBuffer, 0, sizeof(YGeometry_P3T2) * kVertexBufferSize * 6);

		sShader2D = new Shader2D;
		sShader3D = new Shader3D;
	}

	assert((font_file != NULL) && (*font_file != 0));
	assert(font_size > 0);
		
	//	Check if font with size already created
	for (std::vector <YFontObject_FreeType *>::iterator i = sFontCache.begin(); i != sFontCache.end(); ++i)
	{
		if (((*i)->mFontSize == font_size) && ((*i)->mFontFile.compare(font_file) == 0) && ((*i)->mIs3D == is3D))
		{
			fCachedFont = (*i);
			(*i)->mCount++;
			return;
		}
	}
	//	Otherwise, create the font object
	fCachedFont = new YFontObject_FreeType(font_size, font_file, is3D, depth);
	sFontCache.push_back(fCachedFont);
}

/*	FUNCTION:		YFontFreetype :: ~YFontFreetype
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YFontFreetype :: ~YFontFreetype()
{
#if defined (KEEP_FONT_CACHE)
	/*	We will NOT destroy YFontObject_FreeType until the total font count is zero.
		This way we avoid reinitialising the FreeType font if the font is recreated moments later.
	*/
	if (--sTotalFontCount == 0)
	{
		for (std::vector <YFontObject_FreeType *>::iterator i = sFontCache.begin(); i != sFontCache.end(); ++i)
			delete *i;
		sFontCache.clear();
		delete [] sVertexBuffer;
		delete sShader2D;		sShader2D = nullptr;
		delete sShader3D;		sShader3D = nullptr;
	}
#else
	/*	Release font object from cache last instance used. Beware - subsequent creation will be expensive. */
	for (std::vector <YFontObject_FreeType *>::iterator i = sFontCache.begin(); i != sFontCache.end(); ++i)
	{
		if (fCachedFont == *i)
		{
			if (--(*i)->mCount == 0)
			{
				delete *i;
				sFontCache.erase(i);
				break;
			}
		}
	}
	if (--sTotalFontCount == 0)
	{
		YAssert(sFontCache.empty());
		delete [] sVertexBuffer;
		sVertexBuffer = NULL;
		delete sShader;
		sShader = 0;
	}
#endif
}

/*	FUNCTION:		YFontFreetype :: CreateGeometry
	ARGUMENTS:		text
	RETURN:			geometry node - client acquires ownership
	DESCRIPTION:	Create text geometry node
*/
YGeometryNode * YFontFreetype :: CreateGeometry(const char *text)
{
	if (fCachedFont->mIs3D)
		return CreateExtrudeGlyphGeometry(text);
	else
		return CreateTextureGlyphGeometry(text);
}

/*	FUNCTION:		YFontFreetype :: CreateTextureGlyphGeometry
	ARGUMENTS:		text
	RETURN:			geometry node - client acquires ownership
	DESCRIPTION:	Create text geometry node
*/
YGeometryNode * YFontFreetype :: CreateTextureGlyphGeometry(const char *text)
{
	fSize.Set(0.0f, 0.0f, 0.0f);
	if ((text == nullptr) || (*text == 0))
		return nullptr;
		
	assert(strlen(text) < kVertexBufferSize);
#if 0
	sVertexBufferPointer = sVertexBuffer;
	sVertexBufferCharacterCount = 0;
	fCachedFont->mFont->Render(text);
	fCachedFont->mTextureID = sFontTextureID;
	YGeometryNode *node = new YGeometryNode(GL_TRIANGLES, Y_GEOMETRY_P3T2, (float *)sVertexBuffer, sVertexBufferCharacterCount * 6, Y_SHADER_CUSTOM);
#else
	//	Haiku Mesa for unknown reasons does not draw the very first triangle - Work around the issue by adding a zero triangle
	sVertexBufferPointer = sVertexBuffer + 3;
	sVertexBufferCharacterCount = 0;
	fCachedFont->mFont->Render(text);
	fCachedFont->mTextureID = sFontTextureID;
	YGeometryNode *node = new YGeometryNode(GL_TRIANGLES, Y_GEOMETRY_P3T2, (float *)sVertexBuffer, sVertexBufferCharacterCount * 6 + 3, Y_SHADER_CUSTOM);
#endif

#if 0
	YGeometry_P3T2 *p = sVertexBuffer;
	int line_break = 0;
	for (int i=0; i < sVertexBufferCharacterCount*6; i++)
	{
		printf("(%f, %f, %f) (%f, %f)\n", p->mPosition[0], p->mPosition[1], p->mPosition[2], p->mTexture[0], p->mTexture[1]);
		p++;
		if (++line_break % 6 == 0)
			printf("\n");
	}
#endif
	fSize.x = fCachedFont->mFont->Advance(text);
	fSize.y = fCachedFont->mFont->Ascender();
	fSize.z = fCachedFont->mFont->Descender();

	return node;
}

/*	FUNCTION:		YFontFreetype :: CreateExtrudeGlyphGeometry
	ARGUMENTS:		text
	RETURN:			geometry node - client acquires ownership
	DESCRIPTION:	Create text geometry node
*/
YGeometryNode * YFontFreetype :: CreateExtrudeGlyphGeometry(const char *text)
{
	fSize.Set(0.0f, 0.0f, 0.0f);
	if ((text == nullptr) || (*text == 0))
		return nullptr;

	sExtrudeGeometry.clear();
	fCachedFont->mFont->Render(text);
	fCachedFont->mTextureID = sFontTextureID;

	std::vector<YGeometry_P3N3T2> vertices;
	vertices.reserve(3*1024);
	for (auto &i : sExtrudeGeometry)
	{
		switch (i.type)
		{
			case GL_TRIANGLES:
			{
				for (auto &j : i.geometry)
					vertices.push_back(j);
				break;
			}
			case GL_TRIANGLE_FAN:
			{
				int idx=2;
				while (idx < i.geometry.size())
				{
					vertices.push_back(i.geometry[0]);
					vertices.push_back(i.geometry[idx-1]);
					vertices.push_back(i.geometry[idx]);
					idx++;
				}
				break;
			}
			case GL_TRIANGLE_STRIP:
			{
				int idx=0;
				while (idx + 2 < i.geometry.size())
				{
					if (idx%2==0)
					{
						vertices.push_back(i.geometry[idx]);
						vertices.push_back(i.geometry[idx+1]);
						vertices.push_back(i.geometry[idx+2]);
					}
					else
					{
						vertices.push_back(i.geometry[idx]);
						vertices.push_back(i.geometry[idx+2]);
						vertices.push_back(i.geometry[idx+1]);
					}
					idx++;
				}
				break;
			}
			default:
				printf("YFontFreetype::CreateExtrudeGlyphGeometry() - unhandled type (%d)\n", i.type);
		}
	}
	YGeometryNode *node = new YGeometryNode(GL_TRIANGLES, Y_GEOMETRY_P3N3T2, (float *)vertices.data(), vertices.size());
	fSize.x = fCachedFont->mFont->Advance(text);
	fSize.y = fCachedFont->mFont->Ascender();
	fSize.z = fCachedFont->mFont->Descender();
	return node;
}

/*	FUNCTION:		YFontFreetype :: PreRender
	ARGUMENTS:		colour
	RETURN:			n/a
	DESCRIPTION:	Initialise shader before rendering
*/
void YFontFreetype :: PreRender(const YVector4 &colour)
{
	if (fCachedFont->mIs3D)
	{
		sShader3D->SetColour(colour);
		sShader3D->Activate();
	}
	else
	{
		sShader2D->SetColour(colour);
		sShader2D->Activate();
	}
	glBindTexture(GL_TEXTURE_2D, fCachedFont->mTextureID);
}

};	// namespace yrender

