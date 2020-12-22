/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd 
	DESCRIPTION:	Matrix 4x4 support
*/

#ifndef _YARRA_MATRIX4_H_
#define _YARRA_MATRIX4_H_

#ifndef _YARRA_VECTOR_H_
#include "Vector.h"
#endif

namespace ymath
{

class YQuaternion;

/***************************************
	YMatrix4 4x4
****************************************/
class alignas(16) YMatrix4
{
public:
	alignas(16) float	m[16];

	YMatrix4() {}
	YMatrix4(const float m0, const float m1, const float m2, const float m3,
		const float m4, const float m5, const float m6, const float m7,
		const float m8, const float m9, const float m10, const float m11,
		const float m12, const float m13, const float m14, const float m15);
	YMatrix4(const YQuaternion &quat);

	//	array index, eg. YMatrix4 a;  x = a[12];
	inline const float operator [](const int index) { return m[index]; }

	//	Copy operator
	const YMatrix4 & operator =(const YMatrix4 &aMatrix);
	//	Equality operator
	bool operator ==(const YMatrix4 &aMatrix) const;
	//	Inequality operator
	inline bool operator !=(const YMatrix4 &aMatrix) const { return !(*this == aMatrix); }
	//	Multiply by another matrix
	const YMatrix4 operator *(const YMatrix4 &aMatrix) const;
	//	Multiply by another matrix
	YMatrix4 & operator *=(const YMatrix4 &aMatrix);

	void		LoadIdentity();
	void		PrintToStream() const;

	inline void	Translate(const YVector3 &t)
	{
		m[12] += t.x;	m[13] += t.y;	m[14] += t.z;
	}
	void		RotateEuler(const YVector3 &euler);
	void		Rotate(const YQuaternion &q);
	inline void	Scale(const YVector3 &s)
	{
		m[0] *= s.x;	m[5] *= s.y;	m[10] *= s.z;
	}
//	Getters
	const YVector3		GetTranslation() const;
	const YQuaternion	GetQuaternionRotation() const;
	const YVector3 		GetEulerRotation() const;
	const YVector3		GetScale() const;

	//	Projection matrix
	void	CreateProjectionPerspective(float FOV, float aspect, float zNear, float zFar);
	void	CreateProjectionOrthographic(float left, float right, float bottom, float top, float zNear, float zFar);

	//	Transform functions
	const YVector3	Transform(const YVector3 &vec3) const;
	const YVector4	Transform(const YVector4 &vec4) const;
	const YVector3	TransformDirection(const YVector3 &dir) const;

	//	Miscellaneous functions
	void			LookAt(const YVector3 &direction, const YVector3 &position, const YVector3 &up = YVector3(0, 0, 1));
	const YMatrix4	GetTranspose() const;
	const YMatrix4	GetInverse() const;
};

/***************************************
	General YMatrix4 functions
****************************************/

//	Matrix multiplication
void YMatrixMultiply4f(float * product, const float * a, const float * b);
inline void YMatrixMultiply4(YMatrix4 *res, const YMatrix4 *a, const YMatrix4 *b) { YMatrixMultiply4f(res->m, a->m, b->m); }
inline void YMatrixMultiply4(YMatrix4 &res, const YMatrix4 &a, const YMatrix4 &b) { YMatrixMultiply4f(res.m, a.m, b.m); }
inline void YMatrixMultiply4(YMatrix4 &res, const YMatrix4 *a, const YMatrix4 *b) { YMatrixMultiply4f(res.m, a->m, b->m); }
inline void YMatrixMultiply4(YMatrix4 &res, const YMatrix4 &a, const YMatrix4 *b) { YMatrixMultiply4f(res.m, a.m, b->m); }
inline void YMatrixMultiply4(YMatrix4 &res, const YMatrix4 *a, const YMatrix4 &b) { YMatrixMultiply4f(res.m, a->m, b.m); }

//	Invert a matrix
const bool YInvertMatrix4(const float * m, float * out);

};	//	namespace ymath

#endif	//#ifndef __YARRA_MATRIX4_H__
