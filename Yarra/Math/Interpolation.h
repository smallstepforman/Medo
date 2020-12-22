/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2020, ZenYes Pty Ltd
	DESCRIPTION:	Interpolation methods
*/

#ifndef _YARRA_INTERPOLATION_H_
#define _YARRA_INTERPOLATION_H_

namespace ymath
{

/* Linear interpolation (lerp) */
const float		YInterpolationLinear(const float y0, const float y1, const float t);
const YVector3	YInterpolationLinear(const YVector3 &p0, const YVector3 &p1, const float t);

/* Cosine interpolation */
const float		YInterpolationCosine(const float y0, const float y1, const float t);
const YVector3	YInterpolationCosine(const YVector3 &p0, const YVector3 &p1, const float t);

/* Smooth step interpolation */
const float		YInterpolationSmoothStep(const float y0, const float y1, const float t);
const YVector3	YInterpolationSmoothStep(const YVector3 &p0, const YVector3 &p1, const float t);

/* Acceleration (slow start) */
const float		YInterpolationAcceleration(const float y0, const float y1, const float t);
const YVector3	YInterpolationAcceleration(const YVector3 &p0, const YVector3 &p1, const float t);

/* Deceleration (smooth stop) */
const float		YInterpolationDeceleration(const float y0, const float y1, const float t);
const YVector3	YInterpolationDeceleration(const YVector3 &p0, const YVector3 &p1, const float t);

/* Cubic Hermite spline (specify tangents at end points) */
const float		YInterpolationCubicHermiteSpline(const float y0, const float m0,
	const float y1, const float m1,
	const float t);
const YVector3	YInterpolationCubicHermiteSpline(const YVector3 &p0, const YVector3 &m0,
	const YVector3 &p1, const YVector3 &m1,
	const float t);

/*	Bezier interpolation */
const YVector3	YInterpolationBezier(const std::vector<YVector3> &points, const float t);

};	//	namespace ymath

#endif	//#ifndef __YARRA_INTERPOLATION_H__
