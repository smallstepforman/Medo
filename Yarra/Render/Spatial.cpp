/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Simplified YSpatial node from Yarra engine
*/

#include "Yarra/Platform.h"

#include "SceneNode.h"
#include "MatrixStack.h"

namespace yrender
{
		
/*	FUNCTION:		YSpatial :: YSpatial
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YSpatial :: YSpatial()
{
	fTransform = (YMatrix4 *) yplatform::AlignedAlloc(16, sizeof(YMatrix4));
	Reset();
}

/*	FUNCTION:		YSpatial :: ~YSpatial
	ARGUMENTS:		n/a
	RETURN:			n/a
	DESCRIPTION:	Destructor
*/
YSpatial :: ~YSpatial()
{
	yplatform::AlignedFree(fTransform);
}

/*	FUNCTION:		YSpatial :: Reset
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Reset transform
*/
void YSpatial :: Reset()
{
	fTransform->LoadIdentity();
	fPosition.Set(0.0f, 0.0f, 0.0f);
	fRotation.SetZero();
	fRotationEuler.Set(0.0f, 0.0f, 0.0f);
	fScale.Set(1.0f, 1.0f, 1.0f);
}

/*	FUNCTION:		YSpatial :: operator =
	ARGUMENTS:		aSpatial
	RETURN:			n/a
	DESCRIPTION:	Assignment operator
*/
void YSpatial :: operator = (const YSpatial& aSpatial)
{
	fPosition = aSpatial.GetPosition();
	fRotation = aSpatial.GetRotation();
	fRotationEuler = aSpatial.GetEulerRotation();
	fScale = aSpatial.GetScale();
	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: SetPosition
	ARGUMENTS:		aVector
	RETURN:			n/a
	DESCRIPTION:	Specify current translation (position)
*/
void YSpatial :: SetPosition(const YVector3 &aVector)
{
	fTransform->m[12] = aVector.x;
	fTransform->m[13] = aVector.y;
	fTransform->m[14] = aVector.z;
	fPosition = aVector;
}

/*	FUNCTION:		YSpatial :: AddTranslation
	ARGUMENTS:		tran
	RETURN:			n/a
	DESCRIPTION:	Add translation to current matrix
*/
void YSpatial :: AddTranslation(const YVector3 &tran)
{
	fTransform->m[12] += tran.x;
	fTransform->m[13] += tran.y;
	fTransform->m[14] += tran.z;
	fPosition += tran;
}

/*	FUNCTION:		YSpatial :: SetScale
	ARGUMENTS:		scale
	RETURN:			n/a
	DESCRIPTION:	Specify current scale
*/
void YSpatial :: SetScale(const YVector3 &scale)
{
	fScale = scale;
	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: SetRotation
	ARGUMENTS:		q
	RETURN:			n/a
	DESCRIPTION:	Set rotation from quaternion
*/
void YSpatial :: SetRotation(const YQuaternion &q)
{
	fRotation = q;
	
	//	Get Euler equivalent of rotation
	YMatrix4 temp;
	fRotation.GetMatrix(&temp);
	fRotationEuler = temp.GetEulerRotation();
	
	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: SetRotation
	ARGUMENTS:		angle
					axis
	RETURN:			n/a
	DESCRIPTION:	Set rotation from angle/axis pair
*/
void YSpatial :: SetRotation(const float angle, const YVector3 &axis)
{
	fRotation.SetFromAngleAxis(angle, axis);
	fRotationEuler = axis*angle;
	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: SetRotation
	ARGUMENTS:		euler
	RETURN:			n/a
	DESCRIPTION:	Set rotation from euler
*/
void YSpatial :: SetRotation(const YVector3 &euler)
{
	fRotation.SetFromEuler(euler);
	fRotationEuler = euler;
	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: AddRotation
	ARGUMENTS:		q
	RETURN:			n/a
	DESCRIPTION:	Add rotation from quaternion
*/
void YSpatial :: AddRotation(const YQuaternion &q)
{
	fRotation *= q;

	//	Get Euler equivalent of rotation
	YMatrix4 temp;
	fRotation.GetMatrix(&temp);
	fRotationEuler = temp.GetEulerRotation();

	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: AddRotation
	ARGUMENTS:		angle
					axis
	RETURN:			n/a
	DESCRIPTION:	Add rotation from angle/axis pair
*/
void YSpatial :: AddRotation(const float angle, const YVector3 &axis)
{
	YQuaternion q(angle, axis);
	fRotation *= q;
	fRotationEuler += axis*angle;

	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: AddEulerRotation
	ARGUMENTS:		euler
	RETURN:			n/a
	DESCRIPTION:	Rotate around a euler 
*/
void YSpatial :: AddRotation(const YVector3 &euler)
{
	YQuaternion q;
	q.SetFromEuler(euler);
	fRotation *= q;
	fRotationEuler += euler;

	UpdateTransform();
}

/*	FUNCTION:		YSpatial :: UpdateTransform
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Update fTransform based on fPosition/fRotation/fScale
*/
void YSpatial :: UpdateTransform()
{
	fTransform->LoadIdentity();

	//	Translate
	fTransform->m[12] = fPosition.x;
	fTransform->m[13] = fPosition.y;
	fTransform->m[14] = fPosition.z;

	//	Rotation
	fTransform->Rotate(fRotation);
	
	//	Scale
	*fTransform *= YMatrix4(fScale.x,0,0,0,
							0,fScale.y,0,0,
							0,0,fScale.z,0,
							0,0,0,1);

	//	fTransform = translation * rotation * scale
}

/*	FUNCTION:		YSpatial :: Transform
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Apply spatial transformation 
*/
void YSpatial :: Transform()
{
	yrender::yMatrixStack.MultiplyMatrix(fTransform);
}

};	//	namespace yrender
