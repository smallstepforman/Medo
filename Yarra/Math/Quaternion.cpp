/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Quaternion math support.
					Quaternion rotation is described in http://www.cprogramming.com/tutorial/3d/quaternions.html
					and in Mathematics for 3D Game Programming and Computer Graphics (Eric Lengyel)
*/

#include <cassert>

#include "Yarra/Platform.h"
#include "Math.h"
#include "Matrix4.h"

#include "Quaternion.h"

namespace ymath
{

/*	FUNCTION:		YQuaternion :: YQuaternion
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Constructor.  No rotation.
*/
YQuaternion::YQuaternion()
{
	SetZero();
}

/*	FUNCTION:		YQuaternion :: YQuaternion
	ARGUMENTS:		angle
					axis		- must be unit length
	RETURN:			n/a
	DESCRIPTION:	Constructor with angle / axis parameters
*/
YQuaternion::YQuaternion(const float angle, const YVector3 &axis)
{
	SetFromAngleAxis(angle, axis);
}

/*	FUNCTION:		YQuaternion :: YQuaternion
	ARGUMENTS:		matrix		rotation matrix
	RETURN:			n/a
	DESCRIPTION:	Constructor from rotation matrix.
					Algorithm in Ken Shoemake's article in 1987 SIGGRAPH course notes
					article "Quaternion Calculus and Fast Animation".
					Borrowed from WildMagic (http://www.geometrictools.com)
					Note - indexing is row major in WildMagic
*/
YQuaternion::YQuaternion(const YMatrix4 &matrix)
{
#define ROT(row, column)	(matrix.m[(row) + (column)*4])

	float trace = ROT(0, 0) + ROT(1, 1) + ROT(2, 2);
	float root;

	if (trace > 0.0f)
	{
		// |w| > 1/2, may as well choose w > 1/2
		root = (float)sqrt(trace + 1.0f);	// 2w
		w = 0.5f * root;
		root = 0.5f / root;					// 1/(4w)
		x = (ROT(2, 1) - ROT(1, 2)) * root;
		y = (ROT(0, 2) - ROT(2, 0)) * root;
		z = (ROT(1, 0) - ROT(0, 1)) * root;
	}
	else
	{
		const int next[3] = { 1, 2, 0 };

		// |w| <= 1/2
		int i = 0;
		if (ROT(1, 1) > ROT(0, 0))
			i = 1;
		if (ROT(2, 2) > ROT(i, i))
			i = 2;
		int j = next[i];
		int k = next[j];

		root = (float)sqrt(ROT(i, i) - ROT(j, j) - ROT(k, k) + 1.0f);
		float *quat[3] = { &x, &y, &z };
		*quat[i] = 0.5f * root;
		root = 0.5f / root;
		w = (ROT(k, j) - ROT(j, k)) * root;
		*quat[j] = (ROT(j, i) + ROT(i, j)) * root;
		*quat[k] = (ROT(k, i) + ROT(i, k)) * root;
	}
#undef ROT
}

/*	FUNCTION:		YQuaternion :: SetZero
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Reset quaternion to zero
*/
void YQuaternion::SetZero()
{
	x = y = z = 0.0f;
	w = 1.0f;
}

/*	FUNCTION:		YQuaternion :: SetFromAngleAxis
	ARGUMENTS:		angle
					axis		- must be unit length
	RETURN:			n/a
	DESCRIPTION:	Create unit quaterion corresponding to a rotation through angle <angle>
					about axis <x,y,z>.
						q = cos (angle/2) + A(x,y,z)*sin(angle/2)

*/
void YQuaternion::SetFromAngleAxis(float angle, const YVector3 &axis)
{
	assert(YIsEqual(1.0f, axis.LengthSquared()));

	float temp = sind(0.5f*angle);

	w = cosd(0.5f*angle);
	x = axis.x * temp;
	y = axis.y * temp;
	z = axis.z * temp;
}

/*	FUNCTION:		YQuaternion :: SetFromEuler
	ARGUMENTS:		euler.x		roll (degrees)
					euler.y		pitch (degrees)
					euler.z		yaw (degrees)
	RETURN:			n/a
	DESCRIPTION:	Creates quaternion from euler angles.
*/
void YQuaternion::SetFromEuler(const YVector3 &euler)
{
	float halfRoll = 0.5f * euler.x;
	float halfPitch = 0.5f * euler.y;
	float halfYaw = 0.5f * euler.z;

	float cosRoll = cosd(halfRoll);
	float sinRoll = sind(halfRoll);
	float cosPitch = cosd(halfPitch);
	float sinPitch = sind(halfPitch);
	float cosYaw = cosd(halfYaw);
	float sinYaw = sind(halfYaw);

	x = sinRoll*cosPitch*cosYaw - cosRoll*sinPitch*sinYaw;
	y = cosRoll*sinPitch*cosYaw + sinRoll*cosPitch*sinYaw;
	z = cosRoll*cosPitch*sinYaw - sinRoll*sinPitch*cosYaw;
	w = cosRoll*cosPitch*cosYaw + sinRoll*sinPitch*sinYaw;
}

/*	FUNCTION:		YQuaternion :: SetFromDirectionVector
	ARGUMENTS:		direction
	RETURN:			n/a
	DESCRIPTION:	Create a quaternion from a unit direction vector
*/
void YQuaternion::SetFromDirectionVector(const YVector3 &direction)
{
	YMatrix4 mat;
	mat.LookAt(direction, YVector3(0, 0, 0));
	*this = mat.GetQuaternionRotation();
}

/*	FUNCTION:		YQuaternion :: GetAngleAxis
	ARGUMENTS:		angle
					axis
	RETURN:			via supplied arguments
	DESCRIPTION:	The quaternion representing the rotation is:
						q = cos(A/2)+sin(A/2)*(x*i+y*j+z*k)
*/
void YQuaternion::GetAngleAxis(float *angle, YVector3 *axis) const
{
	float len_squared = x*x + y*y + z*z;

	if (len_squared > 0.0f)
	{
		*angle = 2.0f*acosf(w);
		float inv_len = YInverseSquareRoot(len_squared);
		axis->x = x * inv_len;
		axis->y = y * inv_len;
		axis->z = z * inv_len;
	}
	else
	{
		// Angle is 0 (mod 2*pi), so any axis will do.
		*angle = 0.0f;
		axis->x = 0.0f;
		axis->y = 0.0f;
		axis->z = 1.0f;
	}
	*angle *= Y_RADIAN;
}

/*	FUNCTION:		YQuaternion :: CreateMatrix
	ARGUMENTS:		matrix
	RETURN:			n/a
	DESCRIPTION:	Converts quaternion to matrix format
*/
void YQuaternion::GetMatrix(YMatrix4 *matrix) const
{
	assert(matrix != 0);

	// first column	
	matrix->m[0] = 1.0f - 2.0f*(y*y + z*z);
	matrix->m[1] = 2.0f*(x*y + w*z);
	matrix->m[2] = 2.0f*(x*z - w*y);
	matrix->m[3] = 0.0f;

	// second column	
	matrix->m[4] = 2.0f*(x*y - w*z);
	matrix->m[5] = 1.0f - 2.0f*(x*x + z*z);
	matrix->m[6] = 2.0f*(y*z + w*x);
	matrix->m[7] = 0.0f;

	// third column
	matrix->m[8] = 2.0f*(x*z + w*y);
	matrix->m[9] = 2.0f*(y*z - w*x);
	matrix->m[10] = 1.0f - 2.0f*(x*x + y*y);
	matrix->m[11] = 0.0f;

	// fourth column
	matrix->m[12] = 0;
	matrix->m[13] = 0;
	matrix->m[14] = 0;
	matrix->m[15] = 1.0f;
}

/*	FUNCTION:		YQuaternion :: GetEulerRotation
	ARGUMENTS:		none
	RETURN:			Rotation expressed as euler degrees
	DESCRIPTION:	Get euler equivelent of rotation
					Note - the euler angle might be different from that set with Rotate(),
					but the end rotation is the same.
					Borrowed from Irrlicht
*/
const YVector3
YQuaternion::GetEulerRotation() const
{
	YVector3 euler;

	const float sqw = w*w;
	const float sqx = x*x;
	const float sqy = y*y;
	const float sqz = z*z;

	// heading = rotation about z-axis
	euler.z = (float)atan2f(2.0f*(x*y + z*w), (sqx - sqy - sqz + sqw));

	// bank = rotation about x-axis
	euler.x = (float)atan2f(2.0f*(y*z + x*w), (-sqx - sqy + sqz + sqw));

	// attitude = rotation about y-axis
	euler.y = (float)asinf(YClamp(-2.0f*(x*z - y*w), -1.0f, 1.0f));

	euler *= Y_RADIAN;
	return euler;
}

/*	FUNCTION:		YQuaternion :: operator *
	ARGUMENTS:		q
	RETURN:			multiplied quaternion
	DESCRIPTION:	Creates rotated quaternion
*/
const YQuaternion
YQuaternion :: operator *(const YQuaternion &q) const
{
	YQuaternion p;

	p.x = w*q.x + x*q.w + y*q.z - z*q.y;	// (w1x2 + x1w2 + y1z2 - z1y2)i
	p.y = w*q.y - x*q.z + y*q.w + z*q.x;	// (w1y2 - x1z2 + y1w2 + z1x2)j
	p.z = w*q.z + x*q.y - y*q.x + z*q.w;	// (w1z2 + x1y2 - y1x2 + z1w2)k
	p.w = w*q.w - x*q.x - y*q.y - z*q.z;	// (w1w2 - x1x2 - y1y2 - z1z2)

	return(p);
}

/*	FUNCTION:		YQuaternion :: DotProduct
	ARGUMENTS:		q
	RETURN:			dot product
	DESCRIPTION:	dot product
*/
const float
YQuaternion::DotProduct(const YQuaternion &q) const
{
	return (x*q.x + y*q.y + z*q.z + w*q.w);
}

/*	FUNCTION:		YQuaternion :: Slerp
	ARGUMENTS:		time
					p, q
	RETURN:			this
	DESCRIPTION:	Spherical linear interpolation
*/
const YQuaternion &
YQuaternion::Slerp(float time, const YQuaternion &p, const YQuaternion &q)
{
	float cs = p.DotProduct(q);
	float angle = acosf(cs);

	if (fabs(angle) > 0.0f)
	{
		float sn = sinf(angle);
		float invSn = 1.0f / sn;
		float tAngle = time*angle;
		float a = sinf(angle - tAngle) * invSn;
		float b = sinf(tAngle) * invSn;

		x = a*p.x + b*q.x;
		y = a*p.y + b*q.y;
		z = a*p.z + b*q.z;
		w = a*p.w + b*q.w;
	}
	else
	{
		x = p.x;
		y = p.y;
		z = p.z;
		w = p.w;
	}

	return *this;
}

/*	FUNCTION:		YQuaternion :: RotateVector
	ARGUMENTS:		vec
	RETURN:			rotate vector
	DESCRIPTION:	Rotate vector based on quaternion rotation
*/
const YVector3
YQuaternion::RotateVector(const YVector3 &vec) const
{
	YMatrix4 temp;
	this->GetMatrix(&temp);
	return temp.Transform(vec);
}

/*	FUNCTION:		YQuaternion :: PrintToStream
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Debug support
*/
void YQuaternion::PrintToStream() const
{
	yplatform::Debug("x=%f, y=%f, z=%f, w=%f\n", x, y, z, w);
}

};