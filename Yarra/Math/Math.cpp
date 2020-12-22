/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	Various mathematical functions
*/

#include "Yarra/Math/Math.h" 

namespace ymath
{

/*	FUNCTION:		YClipPointOnLine
	ARGUMENTS:		origin		line start
					point		line end
					distance	how far from start to clip
	RETURN:			n/a
	DESCRIPTION:	Calculate a new point on line (between origin and point)
					The point will be at specified distance from origin.
*/
void YClipPointOnLine(const YVector3 *origin, YVector3 *point, const float distance)
{
	YVector3 segment = *point - *origin;
	segment.Normalise();
	segment *= distance;
	*point = *origin + segment;
}

/*	FUNCTION:		YCalculateDistancePointSegment
	ARGUMENTS:		point
					p0, p1 - line
	RETURN:			distance between point and line
	DESCRIPTION:	Calculate and return distance between point and line
					Borrowed from (http://geometryalgorithms.com/Archive/algorithm_0102/algorithm_0102.htm)
*/
float YCalculateDistancePointLine(const YVector3 &point, const YVector3 &p0, const YVector3 &p1)
{
	YVector3 v = p1 - p0;
	YVector3 w = point - p0;

	float c1 = w.DotProduct(v);
	float c2 = v.DotProduct(v);
	float b = c1 / c2;
	YVector3 Pb = p0 + v*b;

	return YCalculateDistance(point, Pb);
}

/*	FUNCTION:		YCalculateDistancePointSegment
	ARGUMENTS:		point
					p0, p1 - segment
	RETURN:			distance between point and line segment
	DESCRIPTION:	Calculate and return distance between point and line segment (or 2 edge points)
					Borrowed from (http://geometryalgorithms.com/Archive/algorithm_0102/algorithm_0102.htm)
*/
float YCalculateDistancePointLineSegment(const YVector3 &point, const YVector3 &p0, const YVector3 &p1)
{
	YVector3 v = p1 - p0;
	YVector3 w = point - p0;

	float c1 = w.DotProduct(v);
	if (c1 <= 0)
		return YCalculateDistance(point, p0);

	float c2 = v.DotProduct(v);
	if (c2 <= c1)
		return YCalculateDistance(point, p1);

	float b = c1 / c2;
	YVector3 Pb = p0 + v*b;
	return YCalculateDistance(point, Pb);
}

/*	FUNCTION:		YIntersectLinePlane
	ARGUMENTS:		start_pos
					direction		- must be normalised
					plane_normal
					plane_dist_origin
					intersection
	RETURN:			true if intersects
	DESCRIPTION:	Calculate intersection point of line and plane
*/
bool YIntersectLinePlane(const YVector3 &start_pos, const YVector3 &direction, const YVector3 &plane_normal,
	const float plane_dist_origin, YVector3 &intersection)
{
	float a = start_pos.DotProduct(plane_normal) + plane_dist_origin;
	float b = direction.DotProduct(plane_normal);

	// check if parallel
	if (YIsEqual(b, 0.0f))
	{
		// check if line already on plane
		if (YIsEqual(a, 0.0f))
		{
			intersection = start_pos;
			return true;
		}
		else
			return false;
	}

	float distance = -a / b;
	intersection = start_pos + direction*distance;
	return true;
}

/*	FUNCTION:		YInverseSquareRoot
	ARGUMENTS:		x
	RETURN:			1/sqrt(x)
	DESCRIPTION:	Chris Lomont implementation of Fast Inverse Square Root using Newtons approximation
					Borrowed from (http://www.codemaestro.com/reviews/9)
*/
float YInverseSquareRoot(float x)
{
	float xhalf = 0.5f*x;
	int i = *(int*)&x;			// get bits for floating value
	i = 0x5f375a86 - (i >> 1);		// gives initial guess y0
	x = *(float*)&i;			// convert bits back to float
	x = x*(1.5f - xhalf*x*x);		// Newton step, repeating increases accuracy
	return x;
}

//	Determine if CCW winding
bool YDetermineWinding(const YVector3 &a, const YVector3 &b, const YVector3 &c)
{
	YVector3 center = (a + b + c) / 3.0f;
	YVector3 u = b - a;
	YVector3 v = c - a;

	YVector3 c1 = u.CrossProduct(v);

	if (c1.DotProduct(center) >= 0)
		return true;
	else
		return false;
}

/***********************************
	Real Time Collision Detection
	Book by Christer Ericson
************************************/

/*	FUNCTION:		YClosestPtSegmentSegmentSquared
	ARGUMENTS:		See description
	RETURN:			SquaredDistance
	DESCRIPTION:	Computes closest points C1 and C2 of S1(s)=P1+s*(Q1-P1) and
					S2(t)=P2+t*(Q2-P2), returning s and t. Function result is squared
					distance between between S1(s) and S2(t)
*/
float YClosestPtSegmentSegmentSquared(const YVector3 &p1, const YVector3 &q1, const YVector3 &p2, const YVector3 &q2,
	float &s, float &t, YVector3 &c1, YVector3 &c2)
{
	const float EPSILON = 1e-5f;
	YVector3 d1 = q1 - p1;	// Direction vector of segment S1
	YVector3 d2 = q2 - p2;	// Direction vector of segment S2	
	YVector3 r = p1 - p2;
	float a = d1.DotProduct(d1);	// Squared length of segment S1, always nonnegative
	float e = d2.DotProduct(d2);	// Squared length of segment S2, always nonnegative
	float f = d2.DotProduct(r);

	// Check if either or both segments degenerate into points
	if (a <= EPSILON && e <= EPSILON)
	{
		// Both segments degenerate into points
		s = t = 0.0f;
		c1 = p1;
		c2 = p2;
		YVector3 temp = c1 - c2;
		return temp.DotProduct(c1 - c2);
	}
	if (a <= EPSILON)
	{
		// First segment degenerates into a point
		s = 0.0f;
		t = f / e; // s = 0 => t = (b*s + f) / e = f / e
		t = YClamp(t, 0.0f, 1.0f);
	}
	else
	{
		float c = d1.DotProduct(r);
		if (e <= EPSILON)
		{
			// Second segment degenerates into a point
			t = 0.0f;
			s = YClamp(-c / a, 0.0f, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
		}
		else
		{
			// The general nondegenerate case starts here
			float b = d1.DotProduct(d2);
			float denom = a*e - b*b; // Always nonnegative

			// If segments not parallel, compute closest point on L1 to L2, and
			// clamp to segment S1. Else pick arbitrary s (here 0)
			if (denom != 0.0f)
				s = YClamp((b*f - c*e) / denom, 0.0f, 1.0f);
			else
				s = 0.0f;

			// Compute point on L2 closest to S1(s) using
			// t = Dot((P1+D1*s)-P2,D2) / Dot(D2,D2) = (b*s + f) / e
			t = (b*s + f) / e;

			// If t in [0,1] done. Else clamp t, recompute s for the new value
			// of t using s = Dot((P2+D2*t)-P1,D1) / Dot(D1,D1)= (t*b - c) / a
			// and clamp s to [0, 1]
			if (t < 0.0f)
			{
				t = 0.0f;
				s = YClamp(-c / a, 0.0f, 1.0f);
			}
			else if (t > 1.0f)
			{
				t = 1.0f;
				s = YClamp((b - c) / a, 0.0f, 1.0f);
			}
		}
	}

	c1 = p1 + d1 * s;
	c2 = p2 + d2 * t;
	YVector3 temp = c1 - c2;
	return temp.DotProduct(c1 - c2);
}

};	//	namespace ymath
