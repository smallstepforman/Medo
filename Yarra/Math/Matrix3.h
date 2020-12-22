/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Matrix 3x3
*/

#ifndef _YARRA_MATRIX3_H_
#define _YARRA_MATRIX3_H_

#ifndef _YARRA_MATRIX4_H_
#include "Math/Matrix4.h"
#endif

namespace ymath
{

/***************************************
	YMatrix4 3x3
****************************************/
class YMatrix3
{
public:
	float	m[9];

	YMatrix3() {}
	YMatrix3(const YMatrix4 &m4);

	//	array indexing
	inline const float operator [](const int index) { return m[index]; }
	//	Copy operator
	const YMatrix3 & operator =(const YMatrix3 &aMatrix);
	//	Multiply by another matrix
	const YMatrix3 operator *(const YMatrix3 &aMatrix) const;
	//	Multiply by another matrix
	YMatrix3 & operator *=(const YMatrix3 &aMatrix);

	const YVector3		Transform(const YVector3 &vec) const;
	const YMatrix3		GetInverse() const;
	const YMatrix3		GetTranspose() const;
	void				PrintToStream() const;
};

void YMatrixMultiply3(float * dest, const float * a, const float * b);

};	//	namespace ymath

#endif	//#ifndef __YARRA_MATRIX3_H__
