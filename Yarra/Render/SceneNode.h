/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified scene nodes (from Yarra engine)
*/

#ifndef __YARRA_SCENE_NODE_H__
#define __YARRA_SCENE_NODE_H__

#ifndef __YARRA_PLATFORM_H__
#include "Yarra/Platform.h"
#endif

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

#ifndef __YARRA_QUATERNION_H__
#include "Yarra/Math/Quaternion.h"
#endif

#ifndef __YARRA_MATRIX4_H__
#include "Yarra/Math/Matrix4.h"
#endif

#include "RenderDefinitions.h"

namespace yrender
{
using namespace ymath;

class YShaderNode;
class YTexture;

/*********************************
	Spatial data.
	Rotations are stored as YQuaternions,
	Euler equivalents are also available (in degrees)
**********************************/
class YSpatial
{
private:
	YMatrix4			*fTransform;		//	local transform => final = camera (* parent) * transform)
	YVector3			fPosition;				
	YQuaternion			fRotation;
	YVector3			fRotationEuler;		//	human readable format, in degrees		
	YVector3			fScale;
	void				UpdateTransform();
	
public:
						YSpatial();
						~YSpatial();
	void				Transform();
	void				Reset();

	void				operator = (const YSpatial& aSpatial);

	//	Set spatial parameters
	void				SetPosition(const YVector3 &position);
	void				AddTranslation(const YVector3 &translation);
		
	void				SetRotation(const YQuaternion &q);
	void				SetRotation(const float angle, const YVector3 &axis);
	void				SetRotation(const YVector3 &euler);
	void				AddRotation(const YQuaternion &q);
	void				AddRotation(const float angle, const YVector3 &axis);
	void				AddRotation(const YVector3 &euler);
	
	void				SetScale(const YVector3 &scale);
	
	//	Get spatial parameters
	inline const YVector3 &		GetPosition() const		{return fPosition;}
	inline const float			GetPositionX() const	{return fPosition.x;}
	inline const float			GetPositionY() const	{return fPosition.y;}
	inline const float			GetPositionZ() const	{return fPosition.z;}

	inline const YQuaternion &	GetRotation() const		{return fRotation;}
	inline const YVector3 &		GetEulerRotation() const {return fRotationEuler;}
	inline const float			GetRotationX() const	{return fRotationEuler.x;}
	inline const float			GetRotationY() const	{return fRotationEuler.y;}
	inline const float			GetRotationZ() const	{return fRotationEuler.z;}

	inline const YVector3 &		GetScale() const		{return fScale;}
	inline const float			GetScaleX() const		{return fScale.x;}
	inline const float			GetScaleY() const		{return fScale.y;}
	inline const float			GetScaleZ() const		{return fScale.z;}

	inline const YMatrix4		*GetTransform() const	{return fTransform;}
};
	
/****************************************************************
	A YSceneNode is an abstract base class which defines the
	interface for all objects transversable by the scene graph.
	All derived classes must implement the Render(time) method.
	This base class is inherited by geometry nodes, render nodes, 
	animator nodes, material nodes, etc.
*****************************************************************/
class YSceneNode
{
public:
	virtual						~YSceneNode() {}
	virtual void				Render(float delta_time) = 0;
	inline virtual YSpatial		*GetSpatial()		{return nullptr;}		// only overriddable by YSpatialSceneNode
};

/****************************************************************
	A YSpatialNode has embedded spatial data. 
*****************************************************************/
class YSpatialNode : public YSceneNode
{
public:
	virtual					~YSpatialNode() {}

	YSpatial				mSpatial;
	inline YSpatial			*GetSpatial() {return &mSpatial;}
	virtual void			UpdateSpatial() {}
	virtual void			Render(float delta_time) {mSpatial.Transform();}
};

/****************************************************************
	YGeometryNode uses Vertex Buffer Objects (VBO) for rendering.
	If an indices buffer is supplied, glDrawElements is used,
	otherwise glDrawArrays.
	Data is interleaved in a Y_GEOMETRY_FORMAT format.
*****************************************************************/
class YGeometryNode : public YSceneNode
{
	Y_GEOMETRY_FORMAT		fVerticesBufferFormat;
	GLuint					fVerticesBuffer;
	GLenum					fIndicesBufferFormat;
	GLuint					fIndicesBuffer;

	GLenum					fDrawingMode;
	GLint					fFirst;
	GLint					fCount;
	GLuint					fVertexArrayObject;
	
	GLint					fMaxNumberVertexAttribs;
	void					PrepareRender();

public:
					YGeometryNode(const GLenum mode, const Y_GEOMETRY_FORMAT buffer_format,
							const float *vertices, const GLsizei vertex_count,
							const GLint first = 0, const GLenum usage = GL_STATIC_DRAW);
					YGeometryNode(const GLenum mode, const Y_GEOMETRY_FORMAT buffer_format,
							const float *vertices, const GLsizei vertex_count,
							const void *indices, const GLenum indices_format, const GLsizei indices_count,
							const GLenum usage = GL_STATIC_DRAW);
	virtual			~YGeometryNode();
	virtual void	Render(float delta_time);
	void			UpdateVertices(const float *vertices);

	void			SetVertexCount(GLsizei vertex_count) {fCount = vertex_count;}		//	Caution - Used to draw LESS vertices without recreating buffers.
};

/****************************************************************
	YRenderNode is a typical spatial scene with YGeometryNode and YShader
*****************************************************************/
class YRenderNode : public YSpatialNode
{
	bool		fAutoDestruct;
	
public:
					YRenderNode(const bool autodestruct = true);
	virtual			~YRenderNode();
	virtual void	Render(float delta_time);

	YGeometryNode	*mGeometryNode;
	YShaderNode		*mShaderNode;
	YTexture		*mTexture;
};
	
};	//	namespace yrender

#endif	//#ifndef __YARRA_SCENE_NODE_H__

