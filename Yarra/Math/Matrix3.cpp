/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Matrix 3x3
*/

#include <cstring>	// needed for memcpy
#include <cassert>

#include "Yarra/Platform.h"

#include "Math.h"
#include "Matrix3.h"

namespace ymath
{

/****************************************************
	YMatrix3 is primarily used for lighting equations
	when dealing with fixed function pipeline
*****************************************************/

static float kIdentityMatrix3x3[9] = { 1.0f, 0.0f, 0.0f,
										0.0f, 1.0f, 0.0f,
										0.0f, 0.0f, 1.0f };

/*	FUNCTION:		YMatrixMultiply3
	ARGUMENTS:		p
					m1, m2
	RETURN:			n/a
	DESCRIPTION:	Perform a 3x3 matrix multiplication  (p = m1 x m2)
*/
void YMatrixMultiply3(float * p, const float * a, const float * b)
{
	assert((p != a) && (p != b));

	//	p[0] - p[2]
	*p++ = a[0] * b[0] + a[3] * b[1] + a[6] * b[2];
	*p++ = a[1] * b[0] + a[4] * b[1] + a[7] * b[2];
	*p++ = a[2] * b[0] + a[5] * b[1] + a[8] * b[2];
	//	p[3] - p[5]
	*p++ = a[0] * b[3] + a[3] * b[4] + a[6] * b[5];
	*p++ = a[1] * b[3] + a[4] * b[4] + a[7] * b[5];
	*p++ = a[2] * b[3] + a[5] * b[4] + a[8] * b[5];
	//	p[6] - p[8]
	*p++ = a[0] * b[6] + a[3] * b[7] + a[6] * b[8];
	*p++ = a[1] * b[6] + a[4] * b[7] + a[7] * b[8];
	*p++ = a[2] * b[6] + a[5] * b[7] + a[8] * b[8];
}

/*	FUNCTION:		YMatrix3 :: YMatrix3
	ARGUMENTS:		m4
	RETURN:			n/a
	DESCRIPTION:	Constructor
*/
YMatrix3::YMatrix3(const YMatrix4 &m4)
{
	m[0] = m4.m[0];		m[1] = m4.m[1];		m[2] = m4.m[2];
	m[3] = m4.m[4];		m[4] = m4.m[5];		m[5] = m4.m[6];
	m[6] = m4.m[8];		m[7] = m4.m[9];		m[8] = m4.m[10];
}

/*	FUNCTION:		YMatrix3 :: operator3 =
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator =
					Eg. a = b;
*/
const YMatrix3 &
YMatrix3 :: operator =(const YMatrix3 &aMatrix)
{
	memcpy(this->m, aMatrix.m, sizeof(YMatrix3));
	return *this;
}

/*	FUNCTION:		YMatrix3 :: operator *
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator *
					Eg. a = b * c
*/
const YMatrix3
YMatrix3 :: operator *(const YMatrix3 &aMatrix) const
{
	YMatrix3 temp;
	YMatrixMultiply3(temp.m, m, aMatrix.m);
	return temp;
}

/*	FUNCTION:		YMatrix3 :: operator *=
	ARGUMENTS:		aMatrix
	RETURN:			rvalue
	DESCRIPTION:	Operator *=
					Eg. a *= b
*/
YMatrix3 &
YMatrix3 :: operator *=(const YMatrix3 &aMatrix)
{
	YMatrix3 temp = *this;
	YMatrixMultiply3(m, temp.m, aMatrix.m);
	return *this;
}

/*	FUNCTION:		YMatrix3 :: PrintToStream
	ARGUMENTS:		none
	RETURN:			n/a
	DESCRIPTION:	Debug support
*/
void
YMatrix3::PrintToStream() const
{
	yplatform::Debug("{\n   %0.2f, %0.2f, %0.2f,\n   %0.2f, %0.2f, %0.2f,\n   %0.2f, %0.2f, %0.2f\n}\n",
		m[0], m[3], m[6],
		m[1], m[4], m[7],
		m[2], m[5], m[8]);
}

/*	FUNCTION:		YMatrix3 :: Transform
	ARGUMENTS:		vec
	RETURN:			out
	DESCRIPTION:	Apply rotational transform
*/
const YVector3
YMatrix3::Transform(const YVector3 &vec) const
{
	YVector3 out;
	out.x = m[0] * vec.x + m[3] * vec.y + m[6] * vec.z;
	out.y = m[1] * vec.x + m[4] * vec.y + m[7] * vec.z;
	out.z = m[2] * vec.x + m[5] * vec.y + m[8] * vec.z;

	return out;
}

/*	FUNCTION:		YMatrix3 :: GetInverse
	ARGUMENTS:		none
	RETURN:			inv
	DESCRIPTION:	Get inverse.  M * M(-1) = I
*/
const YMatrix3
YMatrix3::GetInverse() const
{
	YMatrix3 inv;
	inv.m[0] = (m[4] * m[8] - m[7] * m[5]);
	inv.m[1] = -(m[1] * m[8] - m[7] * m[2]);
	inv.m[2] = (m[1] * m[5] - m[4] * m[2]);
	inv.m[3] = -(m[3] * m[8] - m[6] * m[5]);
	inv.m[4] = (m[0] * m[8] - m[6] * m[2]);
	inv.m[5] = -(m[0] * m[5] - m[3] * m[2]);
	inv.m[6] = (m[3] * m[7] - m[6] * m[4]);
	inv.m[7] = -(m[0] * m[7] - m[6] * m[1]);
	inv.m[8] = (m[0] * m[4] - m[3] * m[1]);

	float det = m[0] * inv.m[0] + m[3] * inv.m[1] + m[6] * inv.m[2];
	if (!YIsEqual(det, 0.0f))
	{
		for (int i = 0; i < 9; i++)
			inv.m[i] = inv.m[i] / det;
	}
	else
		memcpy(inv.m, kIdentityMatrix3x3, sizeof(YMatrix3));

	return inv;
}

/*	FUNCTION:		YMatrix3 :: GetTranspose
	ARGUMENTS:		none
	RETURN:			Transponse matrix
	DESCRIPTION:	Get transpose matrix
*/
const YMatrix3
YMatrix3::GetTranspose() const
{
	YMatrix3 t;
	t.m[0] = m[0];		t.m[1] = m[3];		t.m[2] = m[6];
	t.m[3] = m[1];		t.m[4] = m[4];		t.m[5] = m[7];
	t.m[6] = m[2];		t.m[7] = m[5];		t.m[8] = m[8];
	return t;
}

};	//	namespace ymath
