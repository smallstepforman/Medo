/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Mathematical definitions and functions
*/

#ifndef _YARRA_MATH_H_
#define _YARRA_MATH_H_

#include <cmath>

#ifndef _YARRA_VECTOR_H_
#include "Yarra/Math/Vector.h"
#endif

namespace ymath
{

/**********************************
	Common math definitions
***********************************/
#define Y_PI			3.14159265358979323846f
#define Y_TWO_PI		(2.0f*Y_PI)
#define Y_PI_DIV_2		(Y_PI/2.0f)
#define Y_PI_DIV_180	(Y_PI/180.0f)
#define Y_RADIAN		(180.0f/Y_PI)
#define Y_DEG2RAD(a)	((a)*Y_PI/180.0f)
#define Y_RAD2DEG(a)	((a)*Y_RADIAN)

/**********************************
	The following trigonometric functions use degrees instead of radians.
	Benchmarks have shown no 'measurable' performance impact, and they
	significantly enhance readability eg. sind(90) vs sinf(Y_PI_DIV_2)
***********************************/
#define sind(a)		(sinf((a) * Y_PI_DIV_180))
#define cosd(a)		(cosf((a) * Y_PI_DIV_180))
#define tand(a)		(tanf((a) * Y_PI_DIV_180))

/**********************************
	Common functions
***********************************/

//	Float comparison
inline bool YIsEqual(const float x, const float y, const float epsilon = 1e-5f) { return (x + epsilon >= y) && (x - epsilon <= y); }

//	Check for NaN
inline bool YisNaN(float x) { return x != x; }

//	Fast modulus (a%b)
inline long Ymod(const long numerator, const long divisor) { return (numerator)&((divisor)-1); }

//	Calculate distance between 2 points
inline float YCalculateDistance(const YVector3 &a, const YVector3 &b)
{
	YVector3 diff = b - a;
	return diff.Length();
}

//	Clamp n to lie within the range [min, max]
inline float YClamp(const float n, const float min, const float max)
{
	if (n < min)	return min;
	if (n > max)	return max;
	return n;
}

//	Calculate distance squared between 2 points
inline float YCalculateDistanceSquared(const YVector3 &a, const YVector3 &b)
{
	float x = a.x - b.x;
	float y = a.y - b.y;
	float z = a.z - b.z;
	return (x*x + y*y + z*z);
}

//	Calculate distance between point and line(p1-p0)
float YCalculateDistancePointLine(const YVector3 &point, const YVector3 &p0, const YVector3 &p1);

//	Calculate distance between point and line segment (p1-p0)
float YCalculateDistancePointLineSegment(const YVector3 &point, const YVector3 &p0, const YVector3 &p1);

//	Modify point so that it is 'distance' away from origin in direction of point
void YClipPointOnLine(const YVector3 *origin, YVector3 *point, const float distance);

//	Calculate intersection of line with plane
bool YIntersectLinePlane(const YVector3 &start_pos, const YVector3 &direction, const YVector3 &plane_normal,
	const float plane_dist_origin, YVector3 &intersection);

//	Inverse square root 1/sqrt(x)
float YInverseSquareRoot(float x);

//	Clamp angle between 0 and 359
inline float YAngleClamp(float angle) { while (angle < 0) angle += 360.0f; while (angle >= 360.0f) angle -= 360.0f; return angle; }

//	Delta between 2 angles (with wraparound)
inline float YAngleDelta(float a, float b) { return fabs(YAngleClamp(a + 180 - b) - 180); }

/*	Angle comparison (with wrap around), assuming coordinate system where 0=north, 90=east, 180=south, 270=west
		Return 0 if angle_a == angle_b
		Return -1 if angle_b is to the left of angle_a
		Return 1 if angle_b is to the RIGHT of angle_a
*/
inline int YAngleCompare(float angle_a, float angle_b) { return (angle_a == angle_b ? 0 : (YAngleClamp(angle_a + 360 - angle_b) < 180 ? -1 : 1)); }

//	Determine if CCW winding
bool YDetermineWinding(const YVector3 &a, const YVector3 &b, const YVector3 &c);

//	Calculate number of active bits.
//	From MIT HAKMEM (works for 32-bit numbers only)
inline int YBitCount(unsigned int n)
{
	unsigned int tmp;
	tmp = n - ((n >> 1) & 033333333333) - ((n >> 2) & 011111111111);
	return ((tmp + (tmp >> 3)) & 030707070707) % 63;
}

/******************************
	Real Time Collision Detection
	Book by Christer Ericson
*******************************/
float YClosestPtSegmentSegmentSquared(const YVector3 &p1, const YVector3 &q1, const YVector3 &p2, const YVector3 &q2,
	float &s, float &t, YVector3 &c1, YVector3 &c2);

};	//	namespace ymath

#endif	//#ifndef _YARRA_MATH_H_

