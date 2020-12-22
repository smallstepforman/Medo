/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2020, ZenYes Pty Ltd
	DESCRIPTION:	Interpolation methods
					Visualisations available at http://codeplea.com/simple-interpolation
*/

#include <vector>

#include "Yarra/Platform.h"
#include "Math.h"
#include "Interpolation.h"

namespace ymath
{

/************************************
	Linear interpolation (lerp)
	Transition from one value to another at a constant rate
*************************************/

/*	FUNCTION:		YInterpolationLinear
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Linear interpolation (lerp)
*/
const float YInterpolationLinear(const float y0, const float y1, const float t)
{
	return y0 + t * (y1 - y0);
}

/*	FUNCTION:		YInterpolationLinear
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Linear interpolation (lerp)
*/
const YVector3 YInterpolationLinear(const YVector3 &p0, const YVector3 &p1, const float t)
{
	return p0 + (p1 - p0)*t;
}

/************************************
	Cosine interpolation
	Smooth acceleration and deceleration,
	based on cosine half-period
*************************************/

/*	FUNCTION:		YInterpolationCosine
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Cosine interpolation
*/
const float YInterpolationCosine(const float y0, const float y1, const float t)
{
	const float ct = -0.5f*cosd(180 * t) + 0.5f;
	return y0 + ct * (y1 - y0);
}

/*	FUNCTION:		YInterpolationCosine
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Cosine interpolation
*/
const YVector3 YInterpolationCosine(const YVector3 &p0, const YVector3 &p1, const float t)
{
	const float ct = -0.5f*cosd(180 * t) + 0.5f;
	return p0 + (p1 - p0)*ct;
}

/************************************
	Smooth step interpolation
*************************************/

/*	FUNCTION:		YInterpolationSmoothStep
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Smooth step interpolation
*/
const float YInterpolationSmoothStep(const float y0, const float y1, const float t)
{
	const float st = t * t*(3 - 2 * t);
	return y0 + st * (y1 - y0);
}

/*	FUNCTION:		YInterpolationSmoothStep
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Smooth step interpolation
*/
const YVector3 YInterpolationSmoothStep(const YVector3 &p0, const YVector3 &p1, const float t)
{
	const float st = t * t*(3 - 2 * t);
	return p0 + (p1 - p0)*st;
}

/************************************
	Acceleration (slow start)
*************************************/

/*	FUNCTION:		YInterpolationAcceleration
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Acceleration (slow start)
*/
const float YInterpolationAcceleration(const float y0, const float y1, const float t)
{
	return y0 + t * t*(y1 - y0);
}

/*	FUNCTION:		YInterpolationAcceleration
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Acceleration (slow start)
*/
const YVector3 YInterpolationAcceleration(const YVector3 &p0, const YVector3 &p1, const float t)
{
	return p0 + (p1 - p0)*t*t;
}

/************************************
	Deceleration (smooth stop)
*************************************/

/*	FUNCTION:		YInterpolationDeceleration
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Deceleration (smooth stop)
*/
const float YInterpolationDeceleration(const float y0, const float y1, const float t)
{
	const float dt = 1 - (1 - t)*(1 - t);
	return y0 + dt * (y1 - y0);
}

/*	FUNCTION:		YInterpolationDeceleration
	ARGUMENTS:		y0, y1
					t
	RETURN:			interpolated value
	DESCRIPTION:	Deceleration (smooth stop)
*/
const YVector3 YInterpolationDeceleration(const YVector3 &p0, const YVector3 &p1, const float t)
{
	const float dt = 1 - (1 - t)*(1 - t);
	return p0 + (p1 - p0)*dt;
}

/************************************
	Cubic Hermite spline (specify tangents at end points)
*************************************/

/*	FUNCTION:		YInterpolationCubicHermiteSpline
	ARGUMENTS:		y0, y1		end points
					m0, m1		tangent at end points
					t
	RETURN:			interpolated value
	DESCRIPTION:	Cubic Hermite spline (specify tangents at end points)
*/
const float YInterpolationCubicHermiteSpline(const float y0, const float m0,
	const float y1, const float m1, const float t)
{
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float h00 = 2 * t3 - 3 * t2 + 1;
	const float h10 = t3 - 2 * t2 + t;
	const float h01 = -2 * t3 + 3 * t2;
	const float h11 = t3 - t2;
	return y0 * h00 + m0 * h10 + y1 * h01 + m1 * h11;
}

/*	FUNCTION:		YInterpolationCubicHermiteSpline
	ARGUMENTS:		y0, y1		end points
					m0, m1		tangent at end points
	RETURN:			interpolated value
	DESCRIPTION:	Cubic Hermite spline (specify tangents at end points)
*/
const YVector3 YInterpolationCubicHermiteSpline(const YVector3 &p0, const YVector3 &m0,
	const YVector3 &p1, const YVector3 &m1, const float t)
{
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float h00 = 2 * t3 - 3 * t2 + 1;
	const float h10 = t3 - 2 * t2 + t;
	const float h01 = -2 * t3 + 3 * t2;
	const float h11 = t3 - t2;
	return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

/************************************
	Bezier curve
 *************************************/

 /*	FUNCTION:		YInterpolationBezier
	 ARGUMENTS:		points
					 t
	 RETURN:			interpolated value
	 DESCRIPTION:	Position along Bezier curve
  */
const YVector3 YInterpolationBezier(const std::vector<YVector3> &points, const float t)
{
	size_t i = points.size() - 1;
	std::vector<YVector3> p = std::move(points);
	while (i > 0)
	{
		for (size_t j = 0; j < i; j++)
			p[j] += (p[j + 1] - p[j])*t;
		i--;
	}
	return p[0];
}

};	//	namespace ymath