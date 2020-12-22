/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YCamera from Yarra engine
*/

#include <cassert>

#include "Yarra/Math/Math.h"
#include "Camera.h"
#include "MatrixStack.h"

namespace yrender
{

YCamera *YCamera::sCurrentCamera = 0;
static YCamera	*sDefaultCamera = nullptr;
static int		sCameraCount = 0;
	
/*	FUNCTION:		YCamera :: YCamera
	ARGUMENTS:		projection
					view_width
					view_height
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YCamera :: YCamera(CAMERA_PROJECTION projection, float view_width, float view_height)
	: fViewPositionX(0.0f), fViewPositionY(0.0f), fViewSizeX(view_width), fViewSizeY(view_height),
	fProjectionType(projection), fUpdatePending(true)
{
	if  (++sCameraCount == 1)
	{
		sDefaultCamera = new YCamera(CAMERA_ORTHOGRAPHIC, view_width, view_height);
	}
	sCurrentCamera = this;
	
	fProjectionMatrix = (YMatrix4 *)yplatform::AlignedAlloc(16, sizeof(YMatrix4));
	fViewMatrix = (YMatrix4 *)yplatform::AlignedAlloc(16, sizeof(YMatrix4));
	fInverseViewMatrix = (YMatrix4 *)yplatform::AlignedAlloc(16, sizeof(YMatrix4));	
	
	mSpatial.SetPosition(YVector3(0, 0, 0));
	const float fov = 2.0*atanf(1.0f/(view_width/(0.5f*view_height)))*180.0f/Y_PI;
	if (fProjectionType == CAMERA_PERSPECTIVE)
	{
		ChangeFocus(1, 10000, fov);
		SetDirection(YVector3(0, 1, 0));
	}
	else
	{
		ChangeFocus(10000, -10000, fov);
		SetDirection(YVector3(0, 0, -1));
	}
}

/*	FUNCTION:		YCamera :: ~YCamera
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YCamera :: ~YCamera()
{
	if (sCurrentCamera == this)
		sCurrentCamera = sDefaultCamera;
	
	if  (--sCameraCount == 1)
	{
		delete sDefaultCamera;
		sDefaultCamera = nullptr;
	}
	
	yplatform::AlignedFree(fProjectionMatrix);
	yplatform::AlignedFree(fViewMatrix);
	yplatform::AlignedFree(fInverseViewMatrix);	
}

/*	FUNCTION:		YCamera :: ~YCamera
	ARGUMENTS:		x_pos, y_pos
					width, height
	RETURN:			n/a
	DESCRIPTION:	Update camera projection based on new view properties
*/
void YCamera :: FrameResized(float x_pos, float y_pos, float width, float height)
{
	fViewPositionX = x_pos;
	fViewPositionY = y_pos;
	fViewSizeX = width;
	fViewSizeY = height;
	fUpdatePending = true;	
}

/*	FUNCTION:		YCamera :: ChangeFocus
	ARGUMENTS:		near_plane	- see gluPerspective
					far_plane	- see gluPerspective
					fov			- field of view (gluPerspective) 
	RETURN:			n/a
	DESCRIPTION:	Configure camera parameters
*/
void YCamera :: ChangeFocus(float near_plane, float far_plane, float fov)
{
	fNearPlane = near_plane;
	fFarPlane = far_plane;
	fFOV = fov;
	fUpdatePending = true;
}

/*	FUNCTION:		YCamera :: Render
	ARGUMENTS:		delta_time
	RETURN:			n/a
	DESCRIPTION:	Focus camera
*/
void YCamera :: Render(float delta_time)
{
	sCurrentCamera = this;
	if (fUpdatePending)
		UpdateCameraMatrices();	
		
	//	Reset matrix stacks
	yrender::yMatrixStack.Reset(fProjectionMatrix, fViewMatrix);
}

/*	FUNCTION:		YCamera :: UpdateCameraMatrices
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Update projection and ModelView matrices
*/
void YCamera :: UpdateCameraMatrices()
{
	fUpdatePending = false;

	//	Calculate fProjectionMatrix
	switch (fProjectionType)
	{
		case CAMERA_PERSPECTIVE:
			fProjectionMatrix->CreateProjectionPerspective(fFOV, fViewSizeX/fViewSizeY, fNearPlane, fFarPlane);
			break;
		
		case CAMERA_ORTHOGRAPHIC:
			fProjectionMatrix->CreateProjectionOrthographic(0, fViewSizeX, 0, fViewSizeY, fNearPlane, fFarPlane);
			break;

		default:
			assert(0);
	}

	//	Calculate fViewMatrix
	ymath::YQuaternion	q;
	ymath::YQuaternion	qDir;
	ymath::YQuaternion	qAzim;
	
	// Make the Quaternions that will represent the rotations
	qAzim.SetFromAngleAxis(GetPitch() + 90.0f, YVector3(-1.0f, 0.0f, 0.0f));
	qDir.SetFromAngleAxis(GetYaw() - 90, YVector3(0.0f, 0.0f, -1.0f));
	q = qAzim * qDir;
	
	if (GetRoll() != 0.0f)
	{
		ymath::YQuaternion qTilt(GetRoll(), YVector3(0, 0, 1));
		q *= qTilt;
	}

	//	Rotation
	ymath::YMatrix4 mat_rotation;
	q.GetMatrix(&mat_rotation);

	//	Translation
	ymath::YMatrix4 mat_translation;
	mat_translation.LoadIdentity();
	mat_translation.Translate(-mSpatial.GetPosition());
	
	//	View matrix
	*fViewMatrix = mat_rotation * mat_translation;
	*fInverseViewMatrix = fViewMatrix->GetInverse();	
}

/*	FUNCTION:		YCamera :: SetDirection
	ARGUMENTS:		direction
	RETURN:			n/a
	DESCRIPTION:	Set yaw/pitch/roll based on direction vector
					NOTE - assumption - up is (0, 0, 1)
					NOTE2 - The C++ library specifies atan(0,0) as undefined.
					The caller is most probably setting dir to (0,0,1) or (0,0,-1).  We will catch this
*/
void YCamera :: SetDirection(const YVector3 &direction)
{
	YVector3 dir = direction;
	dir.Normalise();

	float pitch = Y_RAD2DEG((float)asinf(dir.z));
	
	float yaw = Y_RAD2DEG((float)atan2f(dir.y, dir.x));

	//	Trap atan(0,0)
	if ((dir.x == 0.0f) && (dir.y == 0.0f))
		yaw = (dir.z == 1.0f ? -90.0f : 90.0f);
	
	if (yaw < 0.0f)
		yaw += 360.0f;
	
	SetOrientation(yaw, pitch, 0.0f);
}

};	//	namespace yrender
