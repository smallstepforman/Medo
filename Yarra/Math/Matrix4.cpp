/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2011, ZenYes Pty Ltd
	DESCRIPTION:	Matrix operations
*/

#include <cmath>
#include <cstring>	// needed for memcpy
#include <cassert>
#include <cstdlib>

#include "Yarra/Platform.h"

#include "Math.h"
#include "Quaternion.h"
#include "Matrix4.h"

namespace ymath
{

static float kIdentityMatrix[16] alignas(16)  = {	1.0f, 0.0f, 0.0f, 0.0f,
													0.0f, 1.0f, 0.0f, 0.0f,
													0.0f, 0.0f, 1.0f, 0.0f,
													0.0f, 0.0f, 0.0f, 1.0f };

/*	FUNCTION:		YMatrix4 :: YMatrix4
	ARGUMENTS:		m0 - m15
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YMatrix4::YMatrix4(const float m0, const float m1, const float m2, const float m3,
	const float m4, const float m5, const float m6, const float m7,
	const float m8, const float m9, const float m10, const float m11,
	const float m12, const float m13, const float m14, const float m15)
{
	m[0] = m0;	m[1] = m1;	m[2] = m2;	m[3] = m3;
	m[4] = m4;	m[5] = m5;	m[6] = m6;	m[7] = m7;
	m[8] = m8;	m[9] = m9;	m[10] = m10; m[11] = m11;
	m[12] = m12;	m[13] = m13;	m[14] = m14;	m[15] = m15;
}

/*	FUNCTION:		YMatrix4 :: YMatrix4
	ARGUMENTS:		quat
	RETURN:			n/a
	DESCRIPTION:	Constructor - create rotation matrix from quaternion
*/
YMatrix4::YMatrix4(const YQuaternion &quat)
{
	quat.GetMatrix(this);
}

/*	FUNCTION:		YMatrix4 :: operator =
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator =
					Eg. a = b;
*/
const YMatrix4 &
YMatrix4 :: operator =(const YMatrix4 &aMatrix)
{
	memcpy(this->m, aMatrix.m, sizeof(YMatrix4));
	return *this;
}

/*	FUNCTION:		YMatrix4 :: operator ==
	ARGUMENTS:		aMatrix
	RETURN:			true if equal
	DESCRIPTION:	Operator ==
					Eg. if (a == b)
*/
bool
YMatrix4 :: operator ==(const YMatrix4 &aMatrix) const
{
	for (int i = 0; i < 16; i++)
	{
		if (!YIsEqual(m[i], aMatrix.m[i]))
			return false;

	}
	return true;
}

/*	FUNCTION:		YMatrix4 :: operator *
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator *
					Eg. a = b * c
*/
const YMatrix4
YMatrix4 :: operator *(const YMatrix4 &aMatrix) const
{
	YMatrix4 temp;
	YMatrixMultiply4(temp, *this, aMatrix);
	return temp;
}

/*	FUNCTION:		YMatrix4 :: operator *=
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator *=
					Eg. a *= b
*/
YMatrix4 &
YMatrix4 :: operator *=(const YMatrix4 &aMatrix)
{
	YMatrix4 temp = *this;
	YMatrixMultiply4(*this, temp, aMatrix);
	return *this;
}

/*	FUNCTION:		YMatrix4 :: LoadIdentity
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Create identity
*/
void
YMatrix4::LoadIdentity()
{
	memcpy(m, kIdentityMatrix, 16 * sizeof(float));
}

/*	FUNCTION:		YMatrix4 :: PrintToStream
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Debug support
*/
void
YMatrix4::PrintToStream() const
{
	yplatform::Debug("{\n\
   %0.2f, %0.2f, %0.2f, %0.2f,\n\
   %0.2f, %0.2f, %0.2f, %0.2f,\n\
   %0.2f, %0.2f, %0.2f, %0.2f,\n\
   %0.2f, %0.2f, %0.2f, %0.2f\n}\n",
		m[0], m[4], m[8], m[12],
		m[1], m[5], m[9], m[13],
		m[2], m[6], m[10], m[14],
		m[3], m[7], m[11], m[15]);
}

/*	FUNCTION:		YMatrix4 :: Rotate
	ARGUMENTS:		q
	RETURN:			n/a
	DESCRIPTION:	Rotate matrix
*/
void
YMatrix4::Rotate(const YQuaternion &q)
{
	YMatrix4 rot;
	q.GetMatrix(&rot);

	*this *= rot;
}

/*	FUNCTION:		YMatrix4 :: RotateEuler
	ARGUMENTS:		euler
	RETURN:			n/a
	DESCRIPTION:	Rotate matrix around angle.
					Borrowed from Irrlicht
*/
void
YMatrix4::RotateEuler(const YVector3 &euler)
{
	YVector3 angle = euler * Y_PI_DIV_180;	// convert to radian

	const float cx = cos(angle.x);
	const float sx = sin(angle.x);
	const float cy = cos(angle.y);
	const float sy = sin(angle.y);
	const float cz = cos(angle.z);
	const float sz = sin(angle.z);

	m[0] = cy*cz;
	m[1] = cy*sz;
	m[2] = -sy;

	const float sxsy = sx*sy;
	const float cxsy = cx*sy;

	m[4] = sxsy*cz - cx*sz;
	m[5] = sxsy*sz + cx*cz;
	m[6] = sx*cy;

	m[8] = cxsy*cz + sx*sz;
	m[9] = cxsy*sz - sx*cz;
	m[10] = cx*cy;
}

/*	FUNCTION:		YMatrix4 :: GetTranslation
	ARGUMENTS:		none
	RETURN:			translation
	DESCRIPTION:	Get translation from matrix
*/
const YVector3
YMatrix4::GetTranslation() const
{
	return YVector3(m[12], m[13], m[14]);
}

/*	FUNCTION:		YMatrix4 :: GetQuaternionRotation
	ARGUMENTS:		none
	RETURN:			quternion rotation
	DESCRIPTION:	Get quaternion rotation from matrix
*/
const YQuaternion
YMatrix4::GetQuaternionRotation() const
{
	YQuaternion q(*this);
	return q;
}

/*	FUNCTION:		YMatrix4 :: GetEulerRotation
	ARGUMENTS:		none
	RETURN:			Euler rotation
	DESCRIPTION:	Get euler equivelent of rotation
					Note - the euler angle might be different from that set with Rotate(),
					but the end rotation is the same.
					Borrowed from Irrlicht
*/
const YVector3
YMatrix4::GetEulerRotation() const
{
	YVector3 scale = GetScale();
	if (scale.x*scale.y*scale.z == 0.0f)
	{
		const float almost_zero = 0.0001f;
		if (YIsEqual(scale.x, 0.0f))
			scale.x = almost_zero;
		if (YIsEqual(scale.y, 0.0f))
			scale.y = almost_zero;
		if (YIsEqual(scale.z, 0.0f))
			scale.z = almost_zero;
	}
	const YVector3 invScale(1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z);

	float y = -asin(m[2] * invScale.x);
	const float c = cos(y);
	y *= Y_RADIAN;

	float rotx, roty, x, z;
	if (!YIsEqual(c, 0.0f))
	{
		const float inv_c = 1.0f / c;
		rotx = m[10] * inv_c * invScale.z;
		roty = m[6] * inv_c * invScale.y;
		x = atan2f(roty, rotx) * Y_RADIAN;
		rotx = m[0] * inv_c * invScale.x;
		roty = m[1] * inv_c * invScale.x;
		z = atan2f(roty, rotx) * Y_RADIAN;
	}
	else
	{
		x = 0.0;
		rotx = m[5] * invScale.y;
		roty = -m[4] * invScale.y;
		z = atan2f(roty, rotx) * Y_RADIAN;
	}

	//	Wrap around values below 0
	if (x < 0.0) x += 360.0f;
	if (y < 0.0) y += 360.0f;
	if (z < 0.0) z += 360.0f;
	return YVector3(x, y, z);
}

/*	FUNCTION:		YMatrix4 :: GetScale
	ARGUMENTS:		none
	RETURN:			scale
	DESCRIPTION:	Get scale from matrix
*/
const YVector3
YMatrix4::GetScale() const
{
	//	Deal with rotation(0,0,0) first
	if (YIsEqual(m[1], 0.0f) && YIsEqual(m[2], 0.0f) &&
		YIsEqual(m[4], 0.0f) && YIsEqual(m[6], 0.0f) &&
		YIsEqual(m[8], 0.0f) && YIsEqual(m[9], 0.0f))
		return YVector3(m[0], m[5], m[10]);
	else
		return YVector3(sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]),
			sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]),
			sqrtf(m[8] * m[8] + m[9] * m[9] + m[10] * m[10]));
}

/*	FUNCTION:		YMatrix4 :: CreateProjectionPerspective
	ARGUMENTS:		fov, aspect
					zNear, zFar
	RETURN:			n/a
	DESCRIPTION:	Create projection matrix.  Same as YgluPerspective
*/
void
YMatrix4::CreateProjectionPerspective(float fov, float aspect, float zNear, float zFar)
{
	const float h = 1.0f / tand(0.5f*fov);
	float neg_depth = 1.0f / (zNear - zFar);
	float *p = &m[0];

	//	m[0] - m[3]
	*p++ = h / aspect;
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;

	//	m[4] - m[7]
	*p++ = 0;
	*p++ = h;
	*p++ = 0;
	*p++ = 0;

	//	m[8] - m[11]
	*p++ = 0;
	*p++ = 0;
	*p++ = (zFar + zNear) * neg_depth;
	*p++ = -1;

	//	m[12] - m[15]
	*p++ = 0;
	*p++ = 0;
	*p++ = 2.0f*(zNear*zFar) * neg_depth;
	*p++ = 0;
}

/*	FUNCTION:		YMatrix4 :: CreateProjectionOrthographic
	ARGUMENTS:		left, right
					bottom, top
					zNear, zFar
	RETURN:			n/a
	DESCRIPTION:	Create projection matrix.  Same as the following snip of code:
						glOrtho(left, right, bottom, top, zNear, zFar);
						glGetFloatv(GL_PROJECTION_MATRIX, m);
*/
void
YMatrix4::CreateProjectionOrthographic(float left, float right, float bottom, float top, float zNear, float zFar)
{
	float *p = &m[0];

	//	m[0] - m[3]
	*p++ = 2.0f / (right - left);
	*p++ = 0;
	*p++ = 0;
	*p++ = 0;

	//	m[4] - m[7]
	*p++ = 0;
	*p++ = 2.0f / (top - bottom);
	*p++ = 0;
	*p++ = 0;

	//	m[8] - m[11]
	*p++ = 0;
	*p++ = 0;
	*p++ = 2.0f / (zFar - zNear);
	*p++ = 0;

	//	m[12] - m[15]
	*p++ = -(right + left) / (right - left);
	*p++ = -(top + bottom) / (top - bottom);
	*p++ = -(zFar + zNear) / (zFar - zNear);
	*p++ = 1.0f;
}

/*	FUNCTION:		YMatrix4 :: Transform
	ARGUMENTS:		v3
	RETURN:			out
	DESCRIPTION:	Transform a point, eg. out = M4x4 * v3
*/
const YVector3
YMatrix4::Transform(const YVector3 &v3) const
{
	YVector3 out;
	out.x = m[0] * v3.x + m[4] * v3.y + m[8] * v3.z + m[12];
	out.y = m[1] * v3.x + m[5] * v3.y + m[9] * v3.z + m[13];
	out.z = m[2] * v3.x + m[6] * v3.y + m[10] * v3.z + m[14];

	return out;
}

/*	FUNCTION:		YMatrix4 :: Transform
	ARGUMENTS:		v4
	RETURN:			out
	DESCRIPTION:	Transform a point, eg. out = M4x4 * v4
*/
const YVector4
YMatrix4::Transform(const YVector4 &v4) const
{
	YVector4 out;
	out.x = m[0] * v4.x + m[4] * v4.y + m[8] * v4.z + m[12] * v4.w;
	out.y = m[1] * v4.x + m[5] * v4.y + m[9] * v4.z + m[13] * v4.w;
	out.z = m[2] * v4.x + m[6] * v4.y + m[10] * v4.z + m[14] * v4.w;
	out.w = m[3] * v4.x + m[7] * v4.y + m[11] * v4.z + m[15] * v4.w;

	return out;
}

/*	FUNCTION:		YMatrix4 :: TransformDirection
	ARGUMENTS:		dir
	RETURN:			out
	DESCRIPTION:	Transform a direction by a matrix.
					Apply rotational transform from upper 3x3 matrix.
*/
const YVector3
YMatrix4::TransformDirection(const YVector3 &dir) const
{
	YVector3 out;
	out.x = m[0] * dir.x + m[4] * dir.y + m[8] * dir.z;
	out.y = m[1] * dir.x + m[5] * dir.y + m[9] * dir.z;
	out.z = m[2] * dir.x + m[6] * dir.y + m[10] * dir.z;

	return out;
}

/*	FUNCTION:		YMatrix4 :: GetTranspose
	ARGUMENTS:		direction
					position
					up
	RETURN:			n/a
	DESCRIPTION:	Create camera matrix which looks at direction
*/
void YMatrix4::LookAt(const YVector3 &direction, const YVector3 &position, const YVector3 &up)
{
	YVector3 zaxis = -direction;
	zaxis.Normalise();

	YVector3 xaxis = up.CrossProduct(zaxis);
	xaxis.Normalise();

	YVector3 yaxis = zaxis.CrossProduct(xaxis);

	m[0] = xaxis.x;
	m[1] = yaxis.x;
	m[2] = zaxis.x;
	m[3] = 0;

	m[4] = xaxis.y;
	m[5] = yaxis.y;
	m[6] = zaxis.y;
	m[7] = 0;

	m[8] = xaxis.z;
	m[9] = yaxis.z;
	m[10] = zaxis.z;
	m[11] = 0;

	m[12] = -xaxis.DotProduct(position);
	m[13] = -yaxis.DotProduct(position);
	m[14] = -zaxis.DotProduct(position);
	m[15] = 1;
}

/*	FUNCTION:		YMatrix4 :: GetTranspose
	ARGUMENTS:		none
	RETURN:			Transponse matrix
	DESCRIPTION:	Get transpose matrix
*/
const YMatrix4
YMatrix4::GetTranspose() const
{
	YMatrix4 transpose;
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
			transpose.m[(j * 4) + i] = m[(i * 4) + j];
	}
	return transpose;
}

/*	FUNCTION:		YMatrix4 :: GetInverse
	ARGUMENTS:		none
	RETURN:			Inverse matrix
	DESCRIPTION:	Get inverse matrix
*/
const YMatrix4
YMatrix4::GetInverse() const
{
	YMatrix4 inverse;
	if (!YInvertMatrix4(m, inverse.m))
		inverse.LoadIdentity();
	return inverse;
}

};	//	namespace ymath

