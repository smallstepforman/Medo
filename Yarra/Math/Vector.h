/*	PROJECT:		Yarra Engine
	AUTHORS:		Zenja Solaja, Melbourne Australia
	COPYRIGHT:		Yarra Engine	2008-2018, ZenYes Pty Ltd
	DESCRIPTION:	2, 3 and 4 element tuples
*/

#ifndef _YARRA_VECTOR_H_
#define _YARRA_VECTOR_H_

namespace yplatform {extern void	Debug(const char *format, ...);};

namespace ymath
{

/***************************
	YVector2
	Typically a 2D / mouse down point
****************************/
class YVector2
{
public:
	union
	{
		struct
		{
			float x;
			float y;
		};
		float v[2];
	};
	inline const float operator [](const int index) { return v[index]; }

	YVector2() {}
	YVector2(const float a, const float b) { x = a; y = b; }
	YVector2(const YVector2 &aVector) { x = aVector.x; y = aVector.y; }
	inline const YVector2 & operator = (const YVector2 & aVector) { x = aVector.x;	y = aVector.y; return *this; }
	inline void Set(const float a, const float b) { x = a; y = b; }

	const bool operator == (const YVector2 &aVector) const { return (x == aVector.x && y == aVector.y); }
	const bool operator != (const YVector2 &aVector) const { return !(*this == aVector); }

	inline const YVector2 operator + (const YVector2& aVector) const { return YVector2(x + aVector.x, y + aVector.y); }
	inline const YVector2 operator + () const { return YVector2(*this); }
	inline const YVector2 & operator += (const YVector2 &aVector) { x += aVector.x; y += aVector.y; return *this; }
	inline const YVector2 & Add(const float a, const float b) { x += a; y += b; return *this; }
	inline const YVector2 operator - (const YVector2 &aVector) const { return YVector2(x - aVector.x, y - aVector.y); }
	inline const YVector2 operator - () const { return YVector2(-x, -y); }
	inline const YVector2 &operator -= (const YVector2& aVector) { x -= aVector.x; y -= aVector.y; return *this; }

	inline const YVector2 operator * (const YVector2 &aVector) const { return YVector2(x * aVector.x, y * aVector.y); }
	inline const YVector2 & operator *= (const YVector2 &aVector) { x *= aVector.x; y *= aVector.y; return *this; }
	inline const YVector2 operator * (const float d) const { return YVector2(x * d, y * d); }
	inline const YVector2 & operator *= (const float d) { x *= d; y *= d; return *this; }
	inline const YVector2 operator / (const YVector2 &aVector) const { return YVector2(x / aVector.x, y / aVector.y); }
	inline const YVector2 &operator /= (const YVector2 &aVector) { x /= aVector.x; y /= aVector.y; return *this; }
	inline const YVector2 operator / (const float d) const { const float _d = 1.0f / d;	return YVector2(x * _d, y * _d); }
	inline const YVector2 & operator /= (const float d) { const float _d = 1.0f / d; x *= _d; y *= _d; return *this; }

	const float Length() const;
	inline void Normalise()
	{
		float len = Length();
		if (len == 0.0f)
			len = 1.0f;
		*this *= (1.0f / len);
	}
	inline void PrintToStream() const { yplatform::Debug("{%0.3f, %0.3f}\n", x, y); }
};

/***************************
	YVector3
	Typically a 3D position or vector
****************************/
class alignas(16) YVector3
{
public:
	union
	{
		struct alignas(16)
		{
			float x;
			float y;
			float z;
			float w;
		};

		alignas(16) float v[4];
	};

	inline const float operator [](const int index) { return v[index]; }

	YVector3() {}
	YVector3(const float a, const float b, const float c) { x = a; y = b; z = c; }
	YVector3(const YVector3 &aVector) { x = aVector.x; y = aVector.y; z = aVector.z; }
	inline const YVector3 & operator = (const YVector3 &aVector) { x = aVector.x; y = aVector.y; z = aVector.z; return *this; }
	inline void Set(const float a, const float b, const float c) { x = a; y = b; z = c; }

	const bool operator == (const YVector3 &aVector) const { return (x == aVector.x && y == aVector.y && z == aVector.z); }
	const bool operator != (const YVector3 &aVector) const { return !(*this == aVector); }

	inline const YVector3 operator + (const YVector3 &aVector) const { return YVector3(x + aVector.x, y + aVector.y, z + aVector.z); }
	inline const YVector3 operator + () const { return YVector3(*this); }
	inline const YVector3 & operator += (const YVector3 &aVector) { x += aVector.x; y += aVector.y; z += aVector.z; return *this; }
	inline const YVector3 & Add(const float a, const float b, const float c) { x += a; y += b; z += c; return *this; }
	inline const YVector3 operator - (const YVector3 &aVector) const { return YVector3(x - aVector.x, y - aVector.y, z - aVector.z); }
	inline const YVector3 operator - () const { return YVector3(-x, -y, -z); }
	inline const YVector3 & operator -= (const YVector3 &aVector) { x -= aVector.x; y -= aVector.y; z -= aVector.z; return *this; }

	inline const YVector3 operator * (const YVector3 &aVector) const { return YVector3(x * aVector.x, y * aVector.y, z * aVector.z); }
	inline const YVector3 & operator *= (const YVector3 &aVector) { x *= aVector.x; y *= aVector.y; z *= aVector.z; return *this; }
	inline const YVector3 operator * (const float d) const { return YVector3(x * d, y * d, z * d); }
	inline const YVector3 & operator *= (const float d) { x *= d; y *= d; z *= d; return *this; }
	inline const YVector3 operator / (const YVector3 &aVector) const { return YVector3(x / aVector.x, y / aVector.y, z / aVector.z); }
	inline const YVector3 & operator /= (const YVector3 &aVector) { x /= aVector.x; y /= aVector.y; z /= aVector.z; return *this; }
	inline const YVector3 operator / (const float d) const { const float _d = 1.0f / d; return YVector3(x * _d, y * _d, z * _d); }
	inline const YVector3& operator /= (const float d) { const float _d = 1.0f / d; x *= _d; y *= _d; z *= _d; return *this; }

	//	Dot Product
	inline const float DotProduct(const YVector3& aVector) const
	{
		return x*aVector.x + y*aVector.y + z*aVector.z;
	}
	//	Cross Product
	inline const YVector3 CrossProduct(const YVector3& aVector) const
	{
		return YVector3(
			y * aVector.z - aVector.y * z,
			z * aVector.x - aVector.z * x,
			x * aVector.y - aVector.x * y);
	}

	//	Set length (Normalise if 1)
	const float Length() const;

	inline const float LengthSquared() const
	{
		return (x*x + y*y + z*z);
	}
	inline void Normalise()
	{
		float len = Length();
		if (len == 0.0f)
			len = 1.0f;
		*this *= (1.0f / len);
	}
	inline void Clamp(float magnitude)
	{
		if (Length() > magnitude)
		{
			Normalise();
			x *= magnitude;
			y *= magnitude;
			z *= magnitude;
		}
	}

	//
	//	More Advanced Functions

	//	The angle between two vectors in degrees
	const float GetAngle(const YVector3& aNormal) const;

	//	Reflect in Normal Vector
	const YVector3 GetReflection(const YVector3& aPlaneNormal) const;

	//	Get rotated vector around normal
	const YVector3 GetRotatedVector(const float degrees, const YVector3& aNormal) const;

	//	Rotate around X axis
	void RotateAroundX(const float degrees, const YVector3& center = YVector3(0, 0, 0));

	//	Rotate around Y axis
	void RotateAroundY(const float degrees, const YVector3& center = YVector3(0, 0, 0));

	//	Rotate around Z axis
	void RotateAroundZ(const float degrees, const YVector3& center = YVector3(0, 0, 0));

	//	debug data dump
	inline void PrintToStream(const bool new_line = true) const { yplatform::Debug("{%0.3f, %0.3f, %0.3f}%c", x, y, z, new_line ? '\n' : ' '); }
};

/***************************
	YVector4
	Typically a colour
****************************/
class alignas(16) YVector4
{
public:

	union
	{
		struct alignas(16)
		{
			float r;
			float g;
			float b;
			float a;
		};
		struct alignas(16)
		{
			float x;
			float y;
			float z;
			float w;
		};
		alignas(16) float v[4];
	};

	inline const float operator [](const int index) { return v[index]; }

	YVector4() {}
	YVector4(const unsigned int hex_colour);
	YVector4(float a1, float a2, float a3, float a4) { r = a1; g = a2; b = a3; a = a4; }
	YVector4(const YVector4 &aVector) { r = aVector.r; g = aVector.g; b = aVector.b; a = aVector.a; }
	YVector4(const YVector3 &aVector) { x = aVector.x; y = aVector.y; z = aVector.z; w = 1.0f; }
	inline void Set(float a1, float a2, float a3, float a4) { r = a1; g = a2; b = a3; a = a4; }
	inline const YVector4 & operator = (const YVector4 &aVector) { r = aVector.r; g = aVector.g; b = aVector.b; a = aVector.a; return *this; }
	inline const YVector4 & operator = (const YVector3 &aVector) { x = aVector.x; y = aVector.y; z = aVector.z; return *this; }

	const bool operator == (const YVector4 &aVector) const { return (r == aVector.r && g == aVector.g && b == aVector.b && a == aVector.a); }
	const bool operator != (const YVector4 &aVector) const { return !(*this == aVector); }

	inline const YVector4 operator + (const YVector4 &aVector) const { return YVector4(r + aVector.r, g + aVector.g, b + aVector.b, a + aVector.a); }
	inline const YVector4 operator + () const { return YVector4(*this); }
	inline const YVector4 & operator += (const YVector4 &aVector) { r += aVector.r; g += aVector.g; b += aVector.b; a += aVector.a; return *this; }
	inline const YVector4 operator - (const YVector4 &aVector) const { return YVector4(r - aVector.r, g - aVector.g, b - aVector.b, a - aVector.a); }
	inline const YVector4 operator - () const { return YVector4(-r, -g, -b, -a); }
	inline const YVector4 & operator -= (const YVector4 &aVector) { r -= aVector.r; g -= aVector.g; b -= aVector.b; a -= aVector.a; return *this; }

	inline const YVector4 operator * (const YVector4 &aVector) const { return YVector4(r * aVector.r, g * aVector.g, b * aVector.b, a * aVector.a); }
	inline const YVector4 & operator *= (const YVector4 &aVector) { r *= aVector.r; g *= aVector.g; b *= aVector.b; a *= aVector.a; return *this; }
	inline const YVector4 operator * (const float d) const { return YVector4(r * d, g * d, b * d, a * d); }
	inline const YVector4 & operator *= (const float d) { r *= d; g *= d; b *= d; a *= d; return *this; }
	inline const YVector4 operator / (const YVector4 &aVector) const { return YVector4(r / aVector.r, g / aVector.g, b / aVector.b, a / aVector.a); }
	inline const YVector4 & operator /= (const YVector4 &aVector) { r /= aVector.r; g /= aVector.g; b /= aVector.b; a /= aVector.a; return *this; }
	inline const YVector4 operator / (const float d) const { const float _d = 1.0f / d; return YVector4(r * _d, g * _d, b * _d, a * _d); }
	inline const YVector4 & operator /= (const float d) { const float _d = 1.0f / d; r *= _d; g *= _d; b *= _d; a *= _d; return *this; }

	inline const float DotProduct(const YVector4& aVector) const { return x*aVector.x + y*aVector.y + z*aVector.z + w*aVector.w; }
	inline const YVector3 GetYVector3() const { return YVector3(x, y, z); }
	inline void PrintToStream() const { yplatform::Debug("{%0.3f, %0.3f, %0.3f, %0.3f}\n", r, g, b, a); }
};

};	//	namespace ymath

#endif	//#ifndef _YARRA_VECTOR_H_

