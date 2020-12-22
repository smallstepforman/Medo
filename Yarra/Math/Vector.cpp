/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	2, 3 and 4 element tuple
*/

#include <cmath>
#include <cstring>	// needed for memcpy

#include "Platform.h"
#include "Vector.h"
#include "Math.h"

namespace ymath
{

/***************************************
	YVector2
****************************************/

/*	FUNCTION:		YVector2 :: Length
	ARGUMENTS:		none
	RETURN:			length of vector
	DESCRIPTION:	Return length of vector
*/
const float YVector2::Length() const
{
	return sqrtf(x*x + y*y);
}

/***************************************
	YVector3
****************************************/

/*	FUNCTION:		YVector3 :: Length
	ARGUMENTS:		none
	RETURN:			length of vector
	DESCRIPTION:	Return length of vector
*/
const float YVector3::Length() const
{
	return sqrtf(x*x + y*y + z*z);
}

/*	FUNCTION:		YVector3 :: GetAngle
	ARGUMENTS:		aNormal
	RETURN:			angle between two vectors in degrees
	DESCRIPTION:	Return angle between two vectors in degrees
*/
const float YVector3::GetAngle(const YVector3& aNormal) const
{
	return acosf((*this).DotProduct(aNormal)) * Y_RADIAN;
}

/*	FUNCTION:		YVector3 :: GetReflection
	ARGUMENTS:		aPlaneNormal
	RETURN:
	DESCRIPTION:	Reflect in Normal Vector
*/
const YVector3 YVector3::GetReflection(const YVector3& aPlaneNormal) const
{
	return (*this - aPlaneNormal * 2.0 * ((*this).DotProduct(aPlaneNormal))) * (*this).Length();
}

/*	FUNCTION:		YVector3 :: GetRotatedVector
	ARGUMENTS:		dAngle
					aNormal
	RETURN:
	DESCRIPTION:	Rotate vector around normal
*/
const YVector3 YVector3::GetRotatedVector(const float dAngle, const YVector3& aNormal) const
{
	const float dCos = cosd(dAngle);
	const float dSin = sind(dAngle);
	return YVector3(*this * dCos +
		((aNormal * *this) * (1.0f - dCos)) * aNormal +
		((*this).CrossProduct(aNormal)) * dSin);
}

/*	FUNCTION:		YVector3 :: RotateAroundX
	ARGUMENTS:		degrees
					center
	RETURN:
	DESCRIPTION:	Rotate around X axis
*/
void YVector3::RotateAroundX(const float degrees, const YVector3& center)
{
	float cs = cosd(degrees);
	float sn = sind(degrees);
	z -= center.z;
	y -= center.y;
	Set(x, (y*cs - z*sn), (y*sn + z*cs));
	z += center.z;
	y += center.y;
}

/*	FUNCTION:		YVector3 :: RotateAroundY
	ARGUMENTS:		degrees
					center
	RETURN:
	DESCRIPTION:	Rotate around Y axis
*/
void YVector3::RotateAroundY(const float degrees, const YVector3& center)
{
	float cs = cosd(degrees);
	float sn = sind(degrees);
	x -= center.x;
	z -= center.z;
	Set((x*cs - z*sn), y, (x*sn + z*cs));
	x += center.x;
	z += center.z;
}

/*	FUNCTION:		YVector3 :: RotateAroundZ
	ARGUMENTS:		degrees
					center
	RETURN:
	DESCRIPTION:	Rotate around Z axis
*/
void YVector3::RotateAroundZ(const float degrees, const YVector3& center)
{
	float cs = cosd(degrees);
	float sn = sind(degrees);
	x -= center.x;
	y -= center.y;
	Set((x*cs - y*sn), (x*sn + y*cs), z);
	x += center.x;
	y += center.y;
}

/***************************************
	YVector4
****************************************/

/*	FUNCTION:		YVector4 :: YVector4
	ARGUMENTS:		hex_colour
	RETURN:			n/a
	DESCRIPTION:	Constructor - colour is 4 hex digits
*/
YVector4::YVector4(const unsigned int hex_colour)
{
	r = ((hex_colour >> 24) & 0xff) / 255.0f;
	g = ((hex_colour >> 16) & 0xff) / 255.0f;
	b = ((hex_colour >> 8) & 0xff) / 255.0f;
	a = (hex_colour & 0xff) / 255.0f;
}

};	//	namespace ymath
