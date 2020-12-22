/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YCamera from Yarra engine
*/

#ifndef __YARRA_CAMERA_H__
#define __YARRA_CAMERA_H__

#ifndef __YARRA_SCENE_NODE_H__
#include "Yarra/Render/SceneNode.h"
#endif

namespace yrender
{
	
class YCamera : public YSpatialNode
{
public:
	enum CAMERA_PROJECTION
	{
		CAMERA_PERSPECTIVE,				//	3D perspective
		CAMERA_ORTHOGRAPHIC,			//	2D orthographic (all objects scale to fit viewport)
	};
			YCamera(CAMERA_PROJECTION projection, float view_width, float view_height);
			~YCamera();
		
	void					Render(float delta_time);
	void					Invalidate() {fUpdatePending = true;}
	void					FrameResized(float x_pos, float y_pos, float width, float height);
	
	/**********************************
		Convinience functions to rotate and position camera
			X axis (Pitch, Azimuth)	+90 look at sky, 0 straight, -90 look at ground
			Y axis (Roll, Tilt)		+90 tilt head right, 0 straight, -90 tilt head left
			Z axis (Yaw, Direction)	0 (1,0 = east), 90 (0,1 = north), 180 (-1,0 = west), 270 (-1,-1 = south)
		
		Prefer these functions instead of directly modifying mSpatial.
		If you chose to update mSpatial, ALWAYS call Invalidate() to recalculate view frustum
	***********************************/
	inline void				SetPosition(const YVector3 &pos)
							{
								mSpatial.SetPosition(pos);
								fUpdatePending = true;
							}
	inline void				SetOrientation(float yaw/*direction*/, float pitch/*azimuth*/, float roll/*tilt*/)
							{
								mSpatial.SetRotation(YVector3(pitch, roll, yaw));
								fUpdatePending = true;
							}
	void					SetDirection(const YVector3 &direction);
	void					ChangeFocus(float near_plane, float far_plane, float FOV = 30);

	inline const float		GetYaw() const			{return mSpatial.GetRotationZ();}
	inline const float		GetPitch() const		{return mSpatial.GetRotationX();}
	inline const float		GetRoll() const			{return mSpatial.GetRotationY();}
	inline const float		GetDirection() const	{return mSpatial.GetRotationZ();}
	inline const float		GetAzimuth() const		{return mSpatial.GetRotationX();}
	inline const float		GetTilt() const			{return mSpatial.GetRotationY();}
	
	inline const YMatrix4	*GetViewMatrix() const			{return fViewMatrix;}
	inline const YMatrix4	*GetInverseViewMatrix() const	{return fInverseViewMatrix;}
	inline const YMatrix4	*GetProjectionMatrix() const	{return fProjectionMatrix;}
	
	
private:
	bool					fUpdatePending;
	CAMERA_PROJECTION		fProjectionType;
	float					fNearPlane, fFarPlane, fFOV;
	float					fViewSizeX, fViewSizeY;
	float					fViewPositionX, fViewPositionY;
	ymath::YMatrix4			*fProjectionMatrix;
	ymath::YMatrix4			*fViewMatrix;
	ymath::YMatrix4			*fInverseViewMatrix;
		
	void					UpdateCameraMatrices();
	
private:
	static YCamera			*sCurrentCamera;
public:
	static inline YCamera	*GetCurrent()	{return sCurrentCamera;}
};

};	//	namespace yrender

#endif	//#ifndef __YARRA_CAMERA_H__
